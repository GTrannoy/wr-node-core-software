/*
 * This work is part of the White Rabbit Node Core project.
 *
 * Copyright (C) 2013-2015 CERN (www.cern.ch)
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * Released according to the GNU GPL, version 2 or any later version.
 */


/*.
 * WR Distributed DDS Realtime Firmware.
 *
 * rt-d3s.c: Main RT firnmware for the Distributed DDS system.
 */

#include <string.h>

#include "wr-d3s-common.h"
#include "wrtd-serializers.h"

#include "rt-d3s.h"

/* Forces a software reset of the DDS core. May be only run
   AFTER the AD9516 has been programmed and locked */
void d3s_reset()
{

/* First, reset the DDS DAC serdes PLL */
    dp_writel(DDS_RSTR_PLL_RST, DDS_REG_RSTR);
    delay(1000);
    dp_writel(0, DDS_REG_RSTR);
    delay(1000);
    while(! gpior_get(DDS_GPIOR_SERDES_PLL_LOCKED) );

    dp_writel(DDS_RSTR_SW_RST, DDS_REG_RSTR); // trigger a software reset
    delay(1000);
    dp_writel(0, DDS_REG_RSTR);
    delay(1000);

    pp_printf("Reset done\n");

    delay(1000000);

    /* set DDS center frequency to 44 MHz (use scripts/calc_tune.py) */
    dp_writel(0x2d0, DDS_REG_FREQ_HI);
    dp_writel(0xe5604189, DDS_REG_FREQ_LO);
    /* Tuning gain = 1 */
    dp_writel(1<<12, DDS_REG_GAIN);

    pp_printf("FreqHi %x\n", dp_readl(DDS_REG_FREQ_HI));

    pp_printf("FreqLo %x\n", dp_readl(DDS_REG_FREQ_LO));
}

void d3s_set_tune(uint64_t tune)
{

}



#define RESP_LOG_BUF_SIZE 64

struct resp_log_state
{
    int seq;
    int remaining_samples;
    int block_index;
    int block_samples;
    int block_offset;
    int req_id;
    int active;
    uint32_t buf[RESP_LOG_BUF_SIZE];
};

struct resp_log_state rsplog;

void resp_log_init(struct resp_log_state *state)
{
    state->active = 0;
}

void resp_log_update(struct resp_log_state *state, int phase, int y)
{
    if(!state->active)
        return;

    state->buf[state->block_offset++] = phase;
    state->buf[state->block_offset++] = y;

    state->remaining_samples--;

    if(state->block_offset == state->block_samples || !state->remaining_samples)
    {
        struct wrnc_msg buf = hmq_msg_claim_out (WR_D3S_OUT_CONTROL, RESP_LOG_BUF_SIZE + 3);
        pp_printf("Tx blk %d rem %d\n", state->block_index, state->remaining_samples);
        
        int id = WR_D3S_REP_LOG_PAYLOAD;
        int i;

        wrnc_msg_header (&buf, &id, &state->seq);
        wrnc_msg_int32 (&buf, &state->block_offset);
        wrnc_msg_int32 (&buf, &state->block_index);
        
        for(i = 0; i < state->block_offset; i++)
            wrnc_msg_int32(&buf, &state->buf[i]);
    
        hmq_msg_send (&buf);

        state->block_offset = 0;
        state->block_index++;
    }

    if(state->remaining_samples == 0)
    {
        state->active = 0;
    }
}

void resp_log_start( struct resp_log_state *state, int seq, int n_samples )
{
    state->block_index = 0;
    state->seq = seq;
    state->block_offset = 0;
    state->block_samples = RESP_LOG_BUF_SIZE;
    state->remaining_samples = n_samples;
    state->active = 1;
}

void init() 
{
	ad9516_init();

	/* Setup for 352 MHz RF reference: divider = 8 (44 MHz internal/DDS frequency) */

	ad9510_init();


	d3s_reset();

	/* Set up the phase detector to work at ~15 MHz (44 MHz / 3) */
    adf4002_configure(2,2,4);

    resp_log_init(&rsplog);

	pp_printf("RT_D3S firmware initialized.\n");

}

struct dds_loop_state {
    int master; // non-zero: master mode
    int locked; // lock detect
    int integ; 
    int kp, ki;
    int enabled;
    int lock_counter;
    int lock_samples;
    int delock_samples;
    int lock_threshold;
};


int dds_pi_update(struct dds_loop_state *state, int phase_error)
{
    int err = -(phase_error - 32768);

    state->integ += err;

    int y = (state->kp * err + state->ki * state->integ) >> 10;

    resp_log_update(&rsplog, phase_error, y);

    if( !state->locked )
    {
        if(  abs(phase_error) <= state->lock_threshold)
        {
            state->lock_counter++;
            if(state->lock_counter == state->lock_samples)
            {
                state->locked = 1;
                state->lock_counter = 0;
            }
        }
    } else { // locked==1
        if(  abs(phase_error) > state->lock_threshold)
        {
            state->lock_counter++;
            if(state->lock_counter == state->delock_samples)
            {
                state->locked = 0;
                state->lock_counter = 0;
            }

        }
    }
        
        
    return y;
}

struct dds_loop_state dds_loop;

void dds_loop_update(struct dds_loop_state *state)
{
    if(!state->enabled)
        return;

    if(state->master)
    {
        uint32_t pd_data;

        pd_data = dp_readl(DDS_REG_PD_DATA);
        if (!( pd_data & DDS_PD_DATA_VALID))
            return;

        /* clear valid flag */
        dp_writel(0, DDS_REG_PD_DATA);

        int y = dds_pi_update(state, pd_data & 0xffff);

        dp_writel(y, DDS_REG_TUNE_VAL);

        
    } else {

    }
}

void dds_loop_start(struct dds_loop_state *state)
{
    state->integ = 0;
    state->enabled = 1;
    state->kp = 10000;
    state->ki = 10;
    state->master = 1;
    state->locked = 0;
    state->lock_samples = 6000;
    state->delock_samples = 6000;
    state->lock_threshold = 1000;
}

/* Sends an acknowledgement reply */
static inline void ctl_ack( uint32_t seq )
{
    struct wrnc_msg buf = hmq_msg_claim_out (WR_D3S_OUT_CONTROL, 16);
    uint32_t id_ack = WR_D3S_REP_ACK_ID;

    wrnc_msg_header (&buf, &id_ack, &seq);
    hmq_msg_send (&buf);
}

static inline void ctl_d3s_start_response_logging (uint32_t seq, struct wrnc_msg *ibuf)
{
    int n_samples;

    wrnc_msg_int32 (ibuf, &n_samples);
    ctl_ack(seq);
    resp_log_start(&rsplog, seq, n_samples);
}


static inline void ctl_d3s_ping (uint32_t seq, struct wrnc_msg *ibuf)
{
    pp_printf("ping[%d]\n", seq);
    ctl_ack(seq);
}


/* Receives command messages and call matching command handlers */
static inline void do_control()
{
    uint32_t cmd, seq;
    uint32_t p = mq_poll();

    /* HMQ control slot empty? */
    if(! ( p & ( 1<< WR_D3S_IN_CONTROL )))
        return;

    struct wrnc_msg ibuf = hmq_msg_claim_in (WR_D3S_IN_CONTROL, 16);

    wrnc_msg_header(&ibuf, &cmd, &seq);

#define _CMD(id, func)          \
    case id:                    \
    {                           \
        func(seq, &ibuf);       \
        break;                  \
    }

    switch(cmd)
    {
    _CMD(WR_D3S_CMD_START_RESPONSE_LOGGING,                    ctl_d3s_start_response_logging)
    _CMD(WR_D3S_CMD_PING,                    ctl_d3s_ping)
    default:
          break;
    }

    /* Drop the message once handled */
    mq_discard(0, WR_D3S_IN_CONTROL);
}

#define RF_COUNTER_WAIT_PLL_LOCK 0
#define RF_COUNTER_WAIT_TRIGGER 1
#define RF_COUNTER_WAIT_SYNC 2
#define RF_COUNTER_SYNCED 3

struct rf_counter_state {
    int state;

};

struct rf_counter_state rf_cnt;

void rf_counter_update(struct rf_counter_state *state)
{
    uint32_t trig_csr = dp_readl(DDS_REG_TRIG_IN_CSR);
    
    if(trig_csr & DDS_TRIG_IN_CSR_DONE)
    {
        uint32_t trig_snap;
        trig_snap = dp_readl( DDS_REG_TRIG_IN_SNAPSHOT );
        dp_writel(DDS_TRIG_IN_CSR_ARM, DDS_REG_TRIG_IN_CSR);

        pp_printf("Trigger Snapshot : %d\n", trig_snap);

    }

}

void main_loop()
{
    
    /* enable sampling, set divider */
    dp_writel(DDS_CR_SAMP_EN | DDS_CR_SAMP_DIV_W(100), DDS_REG_CR);
    dp_writel(DDS_TRIG_IN_CSR_ARM, DDS_REG_TRIG_IN_CSR);
    dp_writel(44000000, DDS_REG_RF_CNT_PERIOD); // roughly 1s


    dds_loop_start(&dds_loop);

    for(;;)
    {
        dds_loop_update (&dds_loop);
        rf_counter_update (&rf_cnt);
        
        do_control();
    }

}   


int main()
{
	init();

	main_loop();



#if 0
        dp_writel(1000000, DDS_REG_FREQ_MEAS_GATE);

    

	for(;;)
	{
	    int locked = dp_readl(DDS_REG_GPIOR) & DDS_GPIOR_SERDES_PLL_LOCKED ? 1 : 0;
	    int ref_freq = dp_readl(DDS_REG_FREQ_MEAS_VCXO_REF);
	    pp_printf("l: %d  f:%d [%x]\n", locked,ref_freq,ad95xx_read_reg(0,0x1f));

    	    delay_us(500000);
    	    delay_us(500000);
	    adf4002_configure(3,3,4);
	}

#endif

	
	return 0;
}