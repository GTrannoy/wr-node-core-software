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





/* Pulse FIFO for a single Fine Delay output */
struct generic_queue
{
    void *buf;
    int head, tail, count, size, entry_size;
};

void gqueue_init(struct generic_queue *p, int n, int entry_size, void *buf)
{
    p->head = 0;
    p->tail = 0;
    p->count = 0;
    p->size = n;
    p->entry_size = entry_size;
    p->buf = buf;
}

/* Requests a new entry in a pulse queue. Returns pointer to the ne
   entry or NULL if the queue is full. */
void *gqueue_push(struct generic_queue *p)
{
    if (p->count == p->size)
        return NULL;

    void *ent = p->buf + p->head * p->entry_size;

    p->count++;
    p->head++;

    if (p->head == p->size)
        p->head = 0;

    return ent;
}

/* Returns non-0 if pulse queue p contains any pulses. */
int gqueue_empty(struct generic_queue *p)
{
    return (p->count == 0);
}

/* Returns the oldest entry in the pulse queue (or NULL if empty). */
void* gqueue_front(struct generic_queue *p)
{
    if (!p->count)
       return NULL;
    return p->buf + p->tail * p->entry_size;
}

/* Returns the newest entry in the pulse queue (or NULL if empty). */
void* gqueue_back(struct generic_queue *p)
{
    if (!p->count)
       return NULL;
    return &p->buf + p->head * p->entry_size;
}

/* Releases the oldest entry from the pulse queue. */
void gqueue_pop(struct generic_queue *p)
{
    p->tail++;

    if(p->tail == p->size)
        p->tail = 0;
    p->count--;
}



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

    delay(1000000);
    
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

struct dds_loop_stats {
    int sent_packets;
    int sent_fixups;
    int sent_tunes;
};


struct dds_tune_entry {
    int target_sample;
    int target_tai;
    int tune;
};

#define TUNE_QUEUE_ENTRIES 8

static uint32_t _tune_queue_buf[ TUNE_QUEUE_ENTRIES * sizeof(struct dds_tune_entry) / 4];


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
    int64_t fixup_tune;
    int64_t base_tune;
    int fixup_tune_valid;
    int sample_count;
    int stream_id;
    int sampling_divider;
    struct dds_loop_stats stats;
    struct generic_queue tune_queue;
};


int dds_pi_update(struct dds_loop_state *state, int phase_error)
{
    int err = -(phase_error - 32768);

    state->integ += err;

    int y = (state->kp * err + state->ki * state->integ) >> 10;

    resp_log_update(&rsplog, phase_error, y);

    if( !state->locked )
    {
        if(  abs(err) <= state->lock_threshold)
        {
            state->lock_counter++;
            if(state->lock_counter == state->lock_samples)
            {
                state->locked = 1;
                state->lock_counter = 0;
            }
        }
    } else { // locked==1
        if(  abs(err) > state->lock_threshold)
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

static inline volatile struct wr_d3s_remote_message* dds_prepare_message(struct dds_loop_state *state, int type )
{
    volatile struct wr_d3s_remote_message *msg = mq_map_out_buffer(1, WR_D3S_REMOTE_OUT_STREAM);

    mq_claim(1, WR_D3S_REMOTE_OUT_STREAM);

    msg->hdr.target_ip = 0xffffffff;    /* broadcast */
    msg->hdr.target_port = 0xebd0;      /* port */
    msg->hdr.target_offset = 0x4000;    /* target EB slot */

    /* Embed transmission time for latency measyurement */
    msg->transmit_seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
    msg->transmit_cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
    msg->type = type;
    msg->stream_id = state->stream_id;
    msg->sampling_divider = state->sampling_divider;

    return msg;
}

static inline void dds_send_phase_fixup( struct dds_loop_state *state, int64_t fixup_phase, struct wr_timestamp *ts )
{
    volatile struct wr_d3s_remote_message *msg = dds_prepare_message ( state, D3S_MSG_PHASE_FIXUP );

    msg->phase_fixup.tai = ts->seconds;
    msg->phase_fixup.fixup_value = fixup_phase;

//  pp_printf("fixup-phase %x %x\n", ((uint32_t) fixup_phase ) >> 32, (uint32_t)fixup_phase & 0xffffffff);

    mq_send(1, WR_D3S_REMOTE_OUT_STREAM, 32);

    state->stats.sent_packets++;
    state->stats.sent_fixups++;
}

static inline void dds_send_tune_update( struct dds_loop_state *state, int sample_id, int tune, struct wr_timestamp *ts )
{
    volatile struct wr_d3s_remote_message *msg = dds_prepare_message ( state, D3S_MSG_TUNE_UPDATE );

    msg->tune_update.tai = ts->seconds;
    msg->tune_update.tune = tune;
    msg->tune_update.sample_id = sample_id;

    mq_send(1, WR_D3S_REMOTE_OUT_STREAM, 32);

    state->stats.sent_packets++;
    state->stats.sent_tunes++;
}

static inline void dds_master_update(struct dds_loop_state *state)
{
        uint32_t pd_data;

        pd_data = dp_readl(DDS_REG_PD_DATA);
        if (!( pd_data & DDS_PD_DATA_VALID))
            return;

        // produce timestamp of the current tune sample
        int sample_idx = dp_readl(DDS_REG_SAMPLE_IDX);
        struct wr_timestamp ts;

        ts.seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
        ts.ticks = sample_idx * state->sampling_divider * 125;
        
        /* clear valid flag */
        dp_writel(0, DDS_REG_PD_DATA);

        int y = dds_pi_update(state, pd_data & 0xffff);

        dp_writel(y, DDS_REG_TUNE_VAL);

        if(!state->locked)
            return;

        if(sample_idx == 0)
        {
            uint32_t snap_lo = dp_readl ( DDS_REG_ACC_SNAP_LO );
            uint32_t snap_hi = dp_readl ( DDS_REG_ACC_SNAP_HI );

            pp_printf("Snap: %x %x pkts %d\n", snap_lo, snap_hi, state->stats.sent_packets);

            int64_t acc_snap = snap_lo;
            acc_snap |= ((int64_t) snap_hi ) << 32;

            dds_send_phase_fixup(state, acc_snap, &ts );
        }
 
        dds_send_tune_update(state, sample_idx, y, &ts);
 }



void do_rx()
{
    if (rmq_poll( WR_D3S_REMOTE_IN_STREAM )) {
        struct wr_d3s_remote_message *msg = mq_map_in_buffer (1, WR_D3S_REMOTE_IN_STREAM) - sizeof(struct rmq_message_addr);

        pp_printf("Git Packet!\n");
        switch(msg->type) 
        {
            case D3S_MSG_PHASE_FIXUP:
                pp_printf("Got Phase fixup: stream %d tai %d\n", msg->stream_id, (uint32_t) msg->phase_fixup.tai);
                break;
        }

        mq_discard (1, WR_D3S_REMOTE_IN_STREAM);
    }

}

void dds_slave_update(struct dds_loop_state *state)
{

}

void dds_loop_update(struct dds_loop_state *state)
{
    if(!state->enabled)
        return;


    if(state->master)
        dds_master_update(state);
    else {
        dds_slave_update(state);
    }
}

void dds_loop_start(struct dds_loop_state *state)
{
    state->integ = 0;
    state->kp = 10000;
    state->ki = 10;
    state->locked = 0;
    state->lock_samples = 6000;
    state->delock_samples = 6000;
    state->lock_threshold = 1000;
    state->sampling_divider = 100;

    dp_writel(0, DDS_REG_CR);

    if(!state->enabled)
        return;

    

    if(state->master)
    {

        /* set DDS center frequency */
        dp_writel(state->base_tune >> 32, DDS_REG_FREQ_HI);
        dp_writel(state->base_tune & 0xffffffff, DDS_REG_FREQ_LO);
        
    }

    /* Tuning gain = 1 */
    dp_writel(1<<12, DDS_REG_GAIN);

    dp_writel(DDS_CR_SAMP_EN | DDS_CR_SAMP_DIV_W(state->sampling_divider - 1), DDS_REG_CR);
    delay(100);

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

static inline void ctl_d3s_stream_config (uint32_t seq, struct wrnc_msg *ibuf)
{
    int mode;
   
    wrnc_msg_int32(ibuf, &mode);
    wrnc_msg_int32(ibuf, &dds_loop.stream_id);
    wrnc_msg_int64(ibuf, &dds_loop.base_tune);

    uint32_t tune_hi = (dds_loop.base_tune >> 32) & 0xffffffff;
    uint32_t tune_lo = (dds_loop.base_tune >> 0) & 0xffffffff;

    pp_printf("HI=0x%x\n", tune_hi);
    pp_printf("LO=0x%x\n", tune_lo);

    dds_loop.enabled = (mode != D3S_STREAM_OFF ) ? 1 : 0;
    dds_loop.master = (mode == D3S_STREAM_MASTER ) ? 1 : 0;

    dds_loop_start(&dds_loop);

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
    _CMD(WR_D3S_CMD_STREAM_CONFIG,           ctl_d3s_stream_config)
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

        //pp__printf("Trigger Snapshot : %d en %d master %d locked %d, fxu %d\n", trig_snap, dds_loop.enabled, dds_loop.master, dds_loop.locked, dp_readl(DDS_REG_SAMPLE_IDX) );

    }

}

// WR aux clock disciplining

#define WR_LINK_OFFLINE     1
#define WR_LINK_ONLINE      2
#define WR_LINK_SYNCING     3
#define WR_LINK_SYNCED      4

static int wr_state;

int wr_link_up()
{
    return dp_readl ( DDS_REG_TCR ) & DDS_TCR_WR_LINK;
}

int wr_time_locked()
{
    return dp_readl ( DDS_REG_TCR ) & DDS_TCR_WR_LOCKED;
}

int wr_time_ready()
{
   return dp_readl ( DDS_REG_TCR ) & DDS_TCR_WR_TIME_VALID;
}

int wr_enable_lock( int enable )
{
    if(enable)
        dp_writel ( DDS_TCR_WR_LOCK_EN, DDS_REG_TCR );
    else
        dp_writel ( 0, DDS_REG_TCR);
}

void wr_update_link()
{
    switch(wr_state)
    {
        case WR_LINK_OFFLINE:
            if ( wr_link_up() )
            {
                wr_state = WR_LINK_ONLINE;
                pp_printf("WR link online!\n");

            }
            break;
        
        case WR_LINK_ONLINE:
            if (wr_time_ready())
            {
                wr_state = WR_LINK_SYNCING;
                pp_printf("WR time ok [lock on]!\n");

                wr_enable_lock(1);
            }
            break;

        case WR_LINK_SYNCING:
            if (wr_time_locked())
            {
                pp_printf("WR link locked!\n");
                wr_state = WR_LINK_SYNCED;
            }
            break;

        case WR_LINK_SYNCED:
            break;
    }

    if( wr_state != WR_LINK_OFFLINE && !wr_link_up() )
    {
        wr_state = WR_LINK_OFFLINE;
        wr_enable_lock(0);
    }
}

int wr_is_timing_ok()
{
    return (wr_state == WR_LINK_SYNCED);
}



void main_loop()
{
    wr_state = WR_LINK_OFFLINE;
    wr_enable_lock(0);

    /* enable sampling, set divider */
    dp_writel(DDS_TRIG_IN_CSR_ARM, DDS_REG_TRIG_IN_CSR);
    dp_writel(44000000, DDS_REG_RF_CNT_PERIOD); // roughly 1s


    for(;;)
    {
        do_rx();
        dds_loop_update (&dds_loop);
        rf_counter_update (&rf_cnt);
        wr_update_link();
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