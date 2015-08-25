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

#include "gqueue.h"
#include "rt-d3s.h"

#define RESP_LOG_BUF_SIZE 64

#define DDS_SNAP_LAG_SAMPLES 3
#define DDS_ACC_BITS 43
#define DDS_ACC_MASK ((1ULL << (DDS_ACC_BITS) ) - 1)




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

struct dds_loop_stats {
    int sent_packets;
    int sent_fixups;
    int sent_tunes;
};


struct dds_tune_entry {
    int sample;
    int tai;
    int tune;
};

#define SLAVE_WAIT_FIXUP 0
#define SLAVE_RUN 1
#define SLAVE_APPLY_FIXUP 2
#define SLAVE_WAIT_CONFIG 3

#define TUNE_QUEUE_ENTRIES 8
#define MAX_SLAVE_DELAY 8

static uint32_t _tune_queue_buf[ TUNE_QUEUE_ENTRIES * sizeof(struct dds_tune_entry) / 4];

struct dds_loop_state {
    int master; // non-zero: master mode
    int locked; // lock detect
    int integ; 
    int kp, ki;
    int enabled;
    int lock_counter;
    int lock_samples;
    int lock_id;
    int delock_samples;
    int lock_threshold;
    int last_tune;
    int64_t fixup_phase;
    int fixup_count;
    int64_t base_tune;
    int fixup_tai;
    int fixup_tune_valid;
    int sample_count;
    int samples_per_second;
    int stream_id;
    int sampling_divider;
    int slave_delay;
    int slave_state;
    int tune_delay[MAX_SLAVE_DELAY];
    int tune_delay_pos;
    int current_sample_idx;
    
    struct dds_loop_stats stats;
    struct generic_queue tune_queue;
};

#define RF_COUNTER_WAIT_LOCK 0
#define RF_COUNTER_MASTER_WAIT_SNAPSHOT 1
#define RF_COUNTER_MASTER_WAIT_NEXT_SAMPLE 2
#define RF_COUNTER_GOT_FIXUP 4


struct rf_counter_state {
    int state;
    uint32_t period;    
    struct dds_loop_state *dds;
};

struct resp_log_state rsplog;
struct dds_loop_state dds_loop;
struct rf_counter_state rf_cnt;

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
        //pp_printf("Tx blk %d rem %d\n", state->block_index, state->remaining_samples);
        
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


void init() 
{
    ad9516_init();

    /* Setup for 352 MHz RF reference: divider = 8 (44 MHz internal/DDS frequency) */

    ad9510_init();


    d3s_reset();

	/* Set up the phase detector to work at ~15 MHz (44 MHz / 3) */
    adf4002_configure(2,2,4);

    resp_log_init(&rsplog);

    /* all SVEC GPIOs->outputs */
    gpio_set(24);
    gpio_set(25);
    gpio_set(26);

    /* Set RF counter safe load phase (chosen experimentally) */
    dp_writel( DDS_RF_RST_PHASE_LO_W(220) | DDS_RF_RST_PHASE_HI_W(30), DDS_REG_RF_RST_PHASE );

    pp_printf("RT_D3S firmware initialized.\n");
}


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
		pp_printf("[master] RF lock acquired\n");
                state->locked = 1;
                state->lock_id++;
                state->lock_counter = 0;
            }
        }
    } else { // locked==1
        if(  abs(err) > state->lock_threshold)
        {
            state->lock_counter++;
            if(state->lock_counter == state->delock_samples)
            {
		pp_printf("[master] RF lock lost\n");
                state->locked = 0;
                state->lock_counter = 0;
            }

        }
    }
        
        
    return y;
}


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
    msg->lock_id = state->lock_id;

    return msg;
}

static inline void dds_send_phase_fixup( struct dds_loop_state *state, int64_t fixup_phase, struct wr_timestamp *ts )
{
    volatile struct wr_d3s_remote_message *msg = dds_prepare_message ( state, D3S_MSG_PHASE_FIXUP );

    msg->phase_fixup.tai = ts->seconds;
    msg->phase_fixup.fixup_value = fixup_phase;
    msg->phase_fixup.base_tune = state->base_tune;

//    pp_printf("tx fixup-phase %x %x\n", (uint32_t)( fixup_phase  >> 32 ), (uint32_t)(fixup_phase & 0xffffffffULL));

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

static inline int dds_poll_next_sample(uint32_t *pd_data)
{
	uint32_t v;
        v = dp_readl(DDS_REG_PD_DATA);
        if (!( v & DDS_PD_DATA_VALID))
            return 0;
        
        *pd_data = v;

        dp_writel(0, DDS_REG_PD_DATA);

        return 1;
}


static uint64_t dds_get_phase_snapshot(struct dds_loop_state *state)
{
    uint32_t snap_lo = dp_readl ( DDS_REG_ACC_SNAP_LO );
    uint32_t snap_hi = dp_readl ( DDS_REG_ACC_SNAP_HI );

    int64_t acc_snap = snap_lo;
    acc_snap |= ((int64_t) snap_hi ) << 32;

    acc_snap += (int64_t)DDS_SNAP_LAG_SAMPLES * (state->base_tune + state->last_tune) ;
    acc_snap &= DDS_ACC_MASK;
    
    return acc_snap;
}

static inline void dds_master_update(struct dds_loop_state *state)
{
	uint32_t pd_data;
	
	if(!dds_poll_next_sample(&pd_data))
    	    return;
	
        // produce timestamp of the current tune sample
        state->current_sample_idx = dp_readl(DDS_REG_SAMPLE_IDX);
        struct wr_timestamp ts;

        ts.seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
        ts.ticks = state->current_sample_idx * state->sampling_divider * 125;
        
        /* clear valid flag */
        int y = dds_pi_update(state, pd_data & 0xffff);

        dp_writel(y, DDS_REG_TUNE_VAL);

	state->last_tune = y;

        if(!state->locked || !wr_is_timing_ok())
            return;

        if(state->current_sample_idx == 0)
        {
	    uint64_t acc_snap = dds_get_phase_snapshot(state);
            dds_send_phase_fixup(state, acc_snap, &ts );
        }
 
        dds_send_tune_update(state, state->current_sample_idx, y, &ts);    
}

int pf_cnt = 0;

void dds_slave_got_fixup(struct dds_loop_state *state, struct wr_d3s_remote_message *msg)
{
    
    if( state->stream_id != msg->stream_id)
    	return; // not a stream we're interested in

    if(state->lock_id != msg->lock_id)
    {
	pp_printf("Master has relocked. Restarting loop.\n");
	state->lock_id = msg->lock_id;
	state->slave_state = SLAVE_WAIT_CONFIG;
    }

    if ( state->slave_state == SLAVE_WAIT_CONFIG )
    {
	state->base_tune = msg->phase_fixup.base_tune;
	state->sampling_divider = msg->sampling_divider;
	state->slave_delay = 2; // 1 sample -> 100 us (with sampling divider = 100)
	state->samples_per_second = 1000000 / state->sampling_divider;
	state->fixup_count = 0;
	
	
        uint32_t tune_hi = (state->base_tune >> 32) & 0xffffffff;
	uint32_t tune_lo = (state->base_tune >> 0) & 0xffffffff;

//        pp_printf("BaseHI=0x%x\n", tune_hi);
//        pp_printf("BaseO=0x%x\n", tune_lo);
//        pp_printf("Delay %d samples, div %d, Fsamp %d\n\n", state->slave_delay, state->sampling_divider, state->samples_per_second);

        /* set DDS center frequency */
        dp_writel(state->base_tune >> 32, DDS_REG_FREQ_HI);
        dp_writel(state->base_tune & 0xffffffff, DDS_REG_FREQ_LO);

        dp_writel(DDS_CR_SAMP_EN | DDS_CR_SAMP_DIV_W(state->sampling_divider - 1), DDS_REG_CR);

	gqueue_init(&state->tune_queue, TUNE_QUEUE_ENTRIES, sizeof(struct dds_tune_entry), _tune_queue_buf);

	state->slave_state = SLAVE_WAIT_FIXUP;
	state->tune_delay_pos = 0;
	pf_cnt = 0;
    } else {
	state->fixup_phase = msg->phase_fixup.fixup_value;
	state->fixup_tai =  msg->phase_fixup.tai;
	state->fixup_count++;
	if(state->fixup_count == 3)
	    state->slave_state = SLAVE_APPLY_FIXUP;
    }
}

void dds_slave_got_tune_update(struct dds_loop_state *state, struct wr_d3s_remote_message *msg)
{
    switch ( state->slave_state )
    {
	case SLAVE_RUN:
	case SLAVE_APPLY_FIXUP:
	{	
	    if( state->stream_id != msg->stream_id)
        	return; // not a stream we're interested in

    	    struct dds_tune_entry *ent = gqueue_push( &state->tune_queue );
	    if(!ent)
		return; // queue full

	    ent->sample = msg->tune_update.sample_id;
	    ent->tai = msg->tune_update.tai;
	    ent->tune  = msg->tune_update.tune;

		ent->sample += state->slave_delay;

		if(ent->sample >= state->samples_per_second)
		{
		    ent->sample -= state->samples_per_second;
		    ent->tai++;
//		    pp_printf("Overflow %d %d\n", ent->sample, sample_idx);
		}

	}
    	default:
	    break;
    }
}

void do_rx(struct dds_loop_state *state)
{
    if (rmq_poll( WR_D3S_REMOTE_IN_STREAM )) {
        struct wr_d3s_remote_message *msg = mq_map_in_buffer (1, WR_D3S_REMOTE_IN_STREAM) - sizeof(struct rmq_message_addr);

        switch(msg->type) 
        {
            case D3S_MSG_PHASE_FIXUP:
            	    dds_slave_got_fixup(state, msg);
            	    break;
            case D3S_MSG_TUNE_UPDATE:
            	    dds_slave_got_tune_update(state, msg);
                    break;
        }

        mq_discard (1, WR_D3S_REMOTE_IN_STREAM);
    }
}

int64_t calculate_phase_correction(struct dds_loop_state *state)
{
    int i;
//    pp_printf("Sidx-q %d\n",   dp_readl(DDS_REG_SAMPLE_IDX));

    uint64_t acc = 0;
    uint64_t samples_per_tune = 125 * state->sampling_divider;
    uint64_t phase_per_sample = samples_per_tune * ( state->base_tune + (state->last_tune * 4096 ) ) ; // fixme: gain 

    acc = phase_per_sample * (uint64_t) (state->current_sample_idx + 1);
    acc += state->fixup_phase;

    acc &=  DDS_ACC_MASK;

//    pp_printf("acc = %x %x\n", (int)(acc>>32), (int)acc);
//    pp_printf("Sidx-p %d\n",   dp_readl(DDS_REG_SAMPLE_IDX));
    return acc;
}

void dds_slave_update(struct dds_loop_state *state)
{
    uint32_t pd_data;

    switch ( state->slave_state )
    {
	case SLAVE_APPLY_FIXUP:	
	case SLAVE_RUN:
	{
	
	    if(!dds_poll_next_sample(&pd_data))
    		return;
	
        // produce timestamp of the current tune sample
	    int sample_idx = dp_readl(DDS_REG_SAMPLE_IDX);
	    uint32_t tai = lr_readl(WRN_CPU_LR_REG_TAI_SEC);

	    state->current_sample_idx = sample_idx;
	    
	    if(pf_cnt++ < 6)
	    {
		//pp_printf("SID %d\n", sample_idx );
	    }

	    sample_idx++;

	    if(sample_idx >= state->samples_per_second)
	    {
		sample_idx -= state->samples_per_second;
		tai++;
	    }

	    if( gqueue_empty ( &state->tune_queue ) )
		return;
		
	    struct dds_tune_entry *ent = gqueue_front ( &state->tune_queue );
	    
	    if( ent->tai > tai || (ent->tai == tai && ent->sample > sample_idx ) )
		return;


	    
	    while( !gqueue_empty ( &state->tune_queue ) )
	    {
		struct dds_tune_entry *ent = gqueue_front ( &state->tune_queue );

		
		if(ent->tai == tai && ent->sample == sample_idx)
		{

		    state->last_tune = ent->tune;
		    
		    if(state->slave_state == SLAVE_APPLY_FIXUP) 
		    {
			uint64_t fixup_corrected = calculate_phase_correction( state );

			dp_writel(fixup_corrected >> 32, DDS_REG_ACC_LOAD_HI);
			dp_writel(fixup_corrected >> 0, DDS_REG_ACC_LOAD_LO);
			dp_writel(ent->tune | DDS_TUNE_VAL_LOAD_ACC, DDS_REG_TUNE_VAL);
			
//			int cs = dp_readl(DDS_REG_SAMPLE_IDX);
//			pp_printf("Sidx-p %d %d ltune %d\n", state->current_sample_idx, cs, state->last_tune  );
			state->slave_state = SLAVE_RUN;
		    }
		    else
		    {
		    
		    dp_writel(ent->tune, DDS_REG_TUNE_VAL);
		    
		    }
		}

		gqueue_pop ( &state->tune_queue );
	    }
	break;
	}
	
	default:
	    break;
	
    }
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
    state->kp = 3000;
    state->ki = 3;
    state->locked = 0;
    state->lock_samples = 20000;
    state->delock_samples = 1000;
    state->lock_threshold = 1000;
    state->sampling_divider = 100;
    state->lock_id = 0;

    dp_writel(0, DDS_REG_CR);

    if(!state->enabled)
        return;

    if(state->master)
    {
        /* set DDS center frequency */
        dp_writel(state->base_tune >> 32, DDS_REG_FREQ_HI);
        dp_writel(state->base_tune & 0xffffffff, DDS_REG_FREQ_LO);

        dp_writel(DDS_CR_SAMP_EN | DDS_CR_SAMP_DIV_W(state->sampling_divider - 1), DDS_REG_CR);
	delay(100);
        
    } else {

	state->slave_state = SLAVE_WAIT_CONFIG;
    }

    /* Tuning gain = 1 */
    dp_writel(1<<12, DDS_REG_GAIN);
}

void setup_test_output(uint32_t tune_hi, uint32_t tune_lo)
{
    uint32_t pd_data;

    dp_writel(tune_hi, DDS_REG_FREQ_HI);
    dp_writel(tune_lo, DDS_REG_FREQ_LO);

    dp_writel(DDS_CR_SAMP_EN | DDS_CR_SAMP_DIV_W(100 - 1), DDS_REG_CR );

    dp_writel(0, DDS_REG_ACC_LOAD_HI);
    dp_writel(0, DDS_REG_ACC_LOAD_LO);
    
    int sample_idx;
    
    do {
	while(!dds_poll_next_sample(&pd_data));
	
	sample_idx = dp_readl(DDS_REG_SAMPLE_IDX);
    } while ( sample_idx != 0);
    
    
    dp_writel( DDS_TUNE_VAL_LOAD_ACC, DDS_REG_TUNE_VAL);
    
    
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

    dds_loop.enabled = (mode != D3S_STREAM_OFF ) ? 1 : 0;
    dds_loop.master = (mode == D3S_STREAM_MASTER ) ? 1 : 0;

    pp_printf("StreamConfig: mode %d\n", mode );

    dds_loop_start(&dds_loop);
//    rf_counter_init(&rf_cnt, &dds_loop);

    ctl_ack (seq);
}

static inline void ctl_d3s_test_signal (uint32_t seq, struct wrnc_msg *ibuf)
{
    uint64_t base_tune;

    wrnc_msg_int64(ibuf, &base_tune);

    uint32_t tune_hi = (base_tune >> 32) & 0xffffffff;
    uint32_t tune_lo = (base_tune >> 0) & 0xffffffff;

    setup_test_output (tune_hi, tune_lo);

    dds_loop.enabled = 0;

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
    default:
          break;
    }

    /* Drop the message once handled */
    mq_discard(0, WR_D3S_IN_CONTROL);
}




void rf_counter_init(struct rf_counter_state *state, struct dds_loop_state *loop)
{
    state->state = RF_COUNTER_WAIT_LOCK;
    state->period = 44 * 1000 * 100; // roughly 100 ms @ RF = 352 MHz, make configurable
    state->dds = loop;
    
    dp_writel(state->period - 1, DDS_REG_RF_CNT_PERIOD);
}

void rf_counter_update(struct rf_counter_state *state)
{
    switch(state->state)
    {
	case RF_COUNTER_WAIT_LOCK:
	    if(state->dds->master && state->dds->locked)
		state->state = RF_COUNTER_MASTER_WAIT_NEXT_SAMPLE;
	    
		break;
	
	case RF_COUNTER_MASTER_WAIT_NEXT_SAMPLE:
	    if(state->dds->current_sample_idx != 0)
	    {
		state->state = RF_COUNTER_MASTER_WAIT_SNAPSHOT;
	    }
	    break;
	    
	
	case RF_COUNTER_MASTER_WAIT_SNAPSHOT:
	    if(state->dds->current_sample_idx == 0)
	    {
		pp_printf("Snapshot!\n");
		state->state = RF_COUNTER_MASTER_WAIT_NEXT_SAMPLE;
	    }
	    
	    
	    break;
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
        do_rx(&dds_loop);
        dds_loop_update (&dds_loop);
//        rf_counter_update (&rf_cnt);
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