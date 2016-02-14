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

#include "master.h"
#include "slave.h"

static int current_mode = D3S_STREAM_SLAVE;

// state of our DDS loop
static struct dds_master_state master_loop;
static struct dds_slave_state slave_loop;

/* Forces a software reset of the DDS core. May be only run
   AFTER the AD9516 has been programmed and locked */
static void reset_core()
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

    delay(1000000);
}

void init()
{
    ad9516_init();

    /* Setup for 352 MHz RF reference: divider = 8 (44 MHz internal/DDS frequency) */
    ad9510_init();

    reset_core();

	/* Set up the phase detector to work at ~15 MHz (44 MHz / 3) */
    adf4002_configure(2,2,0);

    /* all SVEC GPIOs->outputs */
    gpio_set(24);
    gpio_set(25);
    gpio_set(26);

    /* Set RF counter safe load phase (chosen experimentally) */
    dp_writel( DDS_RF_RST_PHASE_LO_W(220) | DDS_RF_RST_PHASE_HI_W(30), DDS_REG_RF_RST_PHASE );

    wr_init();

    dbg_printf("RT_D3S firmware initialized.");
}

static void setup_test_output(uint32_t tune_hi, uint32_t tune_lo)
{
    uint32_t pd_data;

    dp_writel(tune_hi, DDS_REG_FREQ_HI);
    dp_writel(tune_lo, DDS_REG_FREQ_LO);

    dp_writel(DDS_CR_SAMP_EN | DDS_CR_SAMP_DIV_W(100 - 1), DDS_REG_CR );

    dp_writel(0, DDS_REG_ACC_LOAD_HI);
    dp_writel(0, DDS_REG_ACC_LOAD_LO);

    int sample_idx;

// ensure we're starting at a known TAI time to keep test signals
// generated on multiple devices synchronized as long as they are locked
// to WR
    do {
	       while(!dds_poll_next_sample(&pd_data));

	          sample_idx = dp_readl(DDS_REG_SAMPLE_IDX);
    } while ( sample_idx != 0);


    dp_writel( DDS_TUNE_VAL_LOAD_ACC, DDS_REG_TUNE_VAL);


}

/*

Host control

*/

/* Sends an acknowledgement reply */
static inline void ctl_ack( uint32_t seq )
{
    struct wrnc_msg buf = hmq_msg_claim_out (WR_D3S_OUT_CONTROL, 16);
    uint32_t id_ack = WR_D3S_REP_ACK_ID;

    wrnc_msg_header (&buf, &id_ack, &seq);
    hmq_msg_send (&buf);
}

/* Sends an error reply */
static inline void ctl_err( uint32_t seq, int err )
{
    struct wrnc_msg buf = hmq_msg_claim_out (WR_D3S_OUT_CONTROL, 16);
    uint32_t id_ack = WR_D3S_REP_NACK;

    wrnc_msg_header (&buf, &id_ack, &seq);
    wrnc_msg_int32 ( &buf, &err );
    hmq_msg_send (&buf);
}

static inline void ctl_d3s_start_response_logging (uint32_t seq, struct wrnc_msg *ibuf)
{
    int n_samples;

    if(current_mode != D3S_STREAM_MASTER)
        ctl_err ( seq, -1 );

    wrnc_msg_int32 (ibuf, &n_samples);
    ctl_ack(seq);
    resp_log_start(&master_loop.rsp_log, seq, n_samples);
}

static inline void ctl_d3s_stream_config (uint32_t seq, struct wrnc_msg *ibuf)
{
    int stream_id;
    int64_t base_freq;

    wrnc_msg_int32(ibuf, &current_mode);
    wrnc_msg_int32(ibuf, &stream_id);
    wrnc_msg_int64(ibuf, &base_freq);


    dbg_printf("StreamConfig: mode %d\n", current_mode);

    if( current_mode == D3S_STREAM_MASTER )
    {
        master_loop.base_freq = base_freq;
        master_loop.stream_id = stream_id;
        dds_master_start(&master_loop);
    }
    else if ( current_mode == D3S_STREAM_SLAVE )
    {
        slave_loop.stream_id = stream_id;
        dds_slave_start(&slave_loop);
    }
    else
    {
        dds_slave_stop( &slave_loop );
        dds_master_stop ( &master_loop );
    }

    ctl_ack (seq);
}

static inline void ctl_d3s_test_signal (uint32_t seq, struct wrnc_msg *ibuf)
{
    uint64_t base_tune;

    wrnc_msg_int64(ibuf, &base_tune);

    uint32_t tune_hi = (base_tune >> 32) & 0xffffffff;
    uint32_t tune_lo = (base_tune >> 0) & 0xffffffff;

    setup_test_output (tune_hi, tune_lo);

// disable the loop so it doesn't mess up the test signal
    dds_master_stop( &master_loop );

    ctl_ack (seq);
}

static inline void ctl_d3s_set_gains (uint32_t seq, struct wrnc_msg *ibuf)
{
    wrnc_msg_int32(ibuf, &master_loop.kp);
    wrnc_msg_int32(ibuf, &master_loop.ki);
    wrnc_msg_int32(ibuf, &master_loop.vco_gain);

    dp_writel(master_loop.vco_gain, DDS_REG_GAIN);

    ctl_ack (seq);
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
    _CMD(WR_D3S_CMD_START_RESPONSE_LOGGING,  ctl_d3s_start_response_logging)
    _CMD(WR_D3S_CMD_TEST_SIGNAL,             ctl_d3s_test_signal)
    _CMD(WR_D3S_CMD_STREAM_CONFIG,           ctl_d3s_stream_config)
    _CMD(WR_D3S_CMD_PING,                    ctl_d3s_ping)
    _CMD(WR_D3S_CMD_SET_GAINS,               ctl_d3s_set_gains)
    default:
          break;
    }

    /* Drop the message once handled */
    mq_discard(0, WR_D3S_IN_CONTROL);
}

void main_loop()
{
    dds_master_init ( &master_loop );
    dds_slave_init ( &slave_loop );

    for(;;)
    {
        dds_master_update (&master_loop);
        dds_slave_update ( &slave_loop);
        wr_update_link();
        do_control();
    }
}


int main()
{
	init();
    main_loop();

	return 0;
}
