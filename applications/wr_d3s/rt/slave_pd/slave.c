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
#include "shared_vars.h"

struct tune_queue_entry {
    int sample_id;
    int tai;
    int tune;
};

#define TUNE_QUEUE_ENTRIES 8
#define MAX_SLAVE_DELAY 8

static uint32_t _tune_queue_buf[ TUNE_QUEUE_ENTRIES * sizeof(struct tune_queue_entry) / 4];

int dds_poll_next_sample(uint32_t *pd_data)
{
	uint32_t v;
    v = dp_readl(DDS_REG_PD_DATA);
    if (!( v & DDS_PD_DATA_VALID))
        return 0;

    *pd_data = v;

    dp_writel(0, DDS_REG_PD_DATA);

    return 1;
}

void slave_got_fixup(struct dds_slave_state *state, struct wr_d3s_remote_message *msg)
{
    if(!state->enabled)
        return;

    if( state->stream_id != msg->stream_id)
    	return; // not a stream we're interested in

    if(state->lock_id != msg->lock_id)
    {
		dbg_printf("Master has relocked. Restarting slave.\n");
		state->slave_state = SLAVE_WAIT_CONFIG;
        state->rf_cnt_state = RF_CNT_SLAVE_WAIT_LOCK;
        state->lock_id = msg->lock_id;

	    gqueue_init(&state->tune_queue, TUNE_QUEUE_ENTRIES, sizeof(struct tune_queue_entry), _tune_queue_buf);
    }

    switch (state->slave_state)
    {
        case SLAVE_WAIT_CONFIG:
        {
            state->base_freq = msg->phase_fixup.base_freq;
    	    state->sampling_divider = msg->sampling_divider;
//    	state->slave_delay = 2; // 1 sample -> 100 us (with sampling divider = 100)
    	    state->samples_per_second = 1000000 / state->sampling_divider;
    	    state->fixup_count = 0;
            state->rf_cnt_period = msg->phase_fixup.rf_cnt_period;
            state->vco_gain = msg->phase_fixup.vco_gain;


            /* set DDS center frequency */
            dp_writel(state->base_freq >> 32, DDS_REG_FREQ_HI);
            dp_writel(state->base_freq & 0xffffffff, DDS_REG_FREQ_LO);
            dp_writel(state->vco_gain, DDS_REG_GAIN);
            dp_writel(state->rf_cnt_period, DDS_REG_RF_CNT_PERIOD);

            dp_writel(DDS_CR_SAMP_EN | DDS_CR_SAMP_DIV_W(state->sampling_divider - 1), DDS_REG_CR);

    	    gqueue_init(&state->tune_queue, TUNE_QUEUE_ENTRIES, sizeof(struct tune_queue_entry), _tune_queue_buf);

    	    dbg_printf("Got config: sps %d base_freq 0x%08x%08x rfPeriod %d vco_gain %d lock_id %d\n",
    		state->samples_per_second,
    		(uint32_t) (state->base_freq >> 32),
    		(uint32_t) (state->base_freq & 0xffffffff),
    		state->rf_cnt_period,
    		state->vco_gain,
    		state->lock_id);


            state->slave_state = SLAVE_APPLY_FIXUP;
            state->rf_cnt_state = RF_CNT_SLAVE_WAIT_LOCK;

            state->fixup_phase = msg->phase_fixup.fixup_phase;
    	    state->fixup_time.seconds =  msg->phase_fixup.fixup_tai;
        	state->fixup_count++;

            break;
        }
        default:
        {
            state->fixup_phase = msg->phase_fixup.fixup_phase;
        	state->fixup_time.seconds =  msg->phase_fixup.fixup_tai;
        	state->fixup_count++;

        	state->rf_cnt_snap_timestamp.ticks = msg->phase_fixup.rf_cnt_snap_cycles;
            state->rf_cnt_snap_count = msg->phase_fixup.rf_cnt_snap_count;

            state->fixup_count++;
            break;
        }
    }
}

static void slave_got_tune_update(struct dds_slave_state *state, struct wr_d3s_remote_message *msg)
{
    if(!state->enabled)
        return;

    switch ( state->slave_state )
    {
        case SLAVE_RUN:
	    case SLAVE_APPLY_FIXUP:
	    {
            if( state->stream_id != msg->stream_id)
        	   return; // not a stream we're interested in

    	    struct tune_queue_entry *ent = gqueue_push( &state->tune_queue );
	        if(!ent)
	        {
                state->stats.queue_drops ++;
                return;
	        }

	        ent->sample_id = msg->tune_update.sample_id;
	        ent->tai = msg->tune_update.tai;
	        ent->tune  = msg->tune_update.tune;

		    ent->sample_id += state->delay_samples;

		    if(ent->sample_id >= state->samples_per_second)
		    {
		        ent->sample_id -= state->samples_per_second;
		        ent->tai++;
		    }
            break;
	    }
        default:
	       break;
    }
}


static void do_rx(struct dds_slave_state *state)
{
    if (rmq_poll( WR_D3S_REMOTE_IN_STREAM )) {
        struct wr_d3s_remote_message *msg = mq_map_in_buffer (1, WR_D3S_REMOTE_IN_STREAM) - sizeof(struct rmq_message_addr);

        switch(msg->type)
        {
            case D3S_MSG_PHASE_FIXUP:
            	    slave_got_fixup(state, msg);
            	    break;
            case D3S_MSG_TUNE_UPDATE:
            	    slave_got_tune_update(state, msg);
                    break;
        }

        mq_discard (1, WR_D3S_REMOTE_IN_STREAM);
    }
}

static int64_t calculate_phase_correction(struct dds_slave_state *state, int32_t last_tune, int n)
{
    uint64_t acc = 0;
    uint64_t samples_per_tune = 125 * state->sampling_divider;
    uint64_t phase_per_sample = samples_per_tune * ( state->base_freq + ((int64_t)last_tune * (int64_t)state->vco_gain ) );

    acc = phase_per_sample * (uint64_t) (n + 1);
    //acc += state->fixup_phase;
    acc &= DDS_ACC_MASK;

    return acc;
}

int n = 1;

static inline void set_phase_correction( int64_t corr )
{
    n++;
    if(n == 10000)
    {
	dbg_printf("SetCorrection %d\n", (uint32_t) (corr >> 32));
	n = 0;
    }
}

int misses = 0;

void dds_slave_update(struct dds_slave_state *state)
{
    uint32_t pd_data;

    if(!wr_is_timing_ok())
    {
        misses = 0;
	return;
    }

    if(!state->enabled)
        return;

    do_rx( state );

    if (state->slave_state != SLAVE_APPLY_FIXUP && state->slave_state != SLAVE_RUN )
        return;

//    rf_counter_update (&state);

	if(!dds_poll_next_sample(&pd_data))
        return;


    // produce timestamp of the current tune sample
	int sample_idx = dp_readl(DDS_REG_SAMPLE_IDX);
	uint32_t tai = lr_readl(WRN_CPU_LR_REG_TAI_SEC);


/*    int next =state->current_sample_idx+1;
    if(next == state->samples_per_second)
        next = 0;

    if (next != sample_idx)
        misses++;*/

	state->current_sample_idx = sample_idx;



	sample_idx++;

    if(sample_idx >= state->samples_per_second)
    {
        sample_idx -= state->samples_per_second;
        tai++;
    }

    struct tune_queue_entry *ent = gqueue_front ( &state->tune_queue );

    if(!ent) // queue empty
        return;

    if( ent->tai > tai || (ent->tai == tai && ent->sample_id > sample_idx ) ) // we've received a timestamp from the FUTURE? Call dr Brown...
        return;

        if(sample_idx == state->delay_samples + 100)
        {
            //state->phase_correction = calculate_phase_correction( state, ent->tune, state->delay_samples  );
            pp_printf("miss %d\n", misses);
        //    pp_printf("PC %d\n", (uint32_t) (state->phase_correction >> 32));
            //dp_writel( state->phase_correction & 0xffffffff, DDS_REG_PHASE_LO );
            //dp_writel( ((state->phase_correction >> 32) & 0x7ff) | DDS_PHASE_HI_UPDATE, DDS_REG_PHASE_HI );
            //dp_writel( ((state->phase_correction >> 32) & 0x7ff) | DDS_PHASE_HI_UPDATE, DDS_REG_PHASE_HI );
        }

    while( !gqueue_empty ( &state->tune_queue ) )
    {
        struct tune_queue_entry *ent = gqueue_front ( &state->tune_queue );


		if(ent->tai == tai && ent->sample_id == sample_idx)
		{
		    if(state->slave_state == SLAVE_APPLY_FIXUP)
		    {
                if(ent->sample_id == state->delay_samples)
                {
                    state->phase_correction = calculate_phase_correction( state, ent->tune, state->delay_samples - 1 );




            	    dp_writel(state->fixup_phase >> 32, DDS_REG_ACC_LOAD_HI);
        			dp_writel(state->fixup_phase & 0xffffffff, DDS_REG_ACC_LOAD_LO);

                    //dp_writel( 0, DDS_REG_PHASE_LO );
                    //dp_writel( 0 | DDS_PHASE_HI_UPDATE, DDS_REG_PHASE_HI );

                    dp_writel( state->phase_correction & 0xffffffff, DDS_REG_PHASE_LO );
                    dp_writel( ((state->phase_correction >> 32) & 0x7ff) | DDS_PHASE_HI_UPDATE, DDS_REG_PHASE_HI );

            	    //dp_writel(state->phase_correction >> 32, DDS_REG_ACC_LOAD_HI);
        			//dp_writel(state->phase_correction & 0xffffffff, DDS_REG_ACC_LOAD_LO);

        			dp_writel((ent->tune & 0xffffff)| DDS_TUNE_VAL_LOAD_ACC, DDS_REG_TUNE_VAL);
    	            state->slave_state = SLAVE_RUN;

                    volatile uint32_t dummy = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
                    pp_printf("apply-fixup %d\n", lr_readl(WRN_CPU_LR_REG_TAI_CYCLES));
                } else {
                    dp_writel((ent->tune & 0xffffff), DDS_REG_TUNE_VAL);

                }

		    }
		    else
		    {

                if(sample_idx == state->delay_samples + 100)
                {
                    //state->phase_correction = calculate_phase_correction( state, ent->tune, state->delay_samples  );
                    pp_printf("miss %d\n", misses);
                //    pp_printf("PC %d\n", (uint32_t) (state->phase_correction >> 32));
                    //dp_writel( state->phase_correction & 0xffffffff, DDS_REG_PHASE_LO );
                    //dp_writel( ((state->phase_correction >> 32) & 0x7ff) | DDS_PHASE_HI_UPDATE, DDS_REG_PHASE_HI );
                    //dp_writel( ((state->phase_correction >> 32) & 0x7ff) | DDS_PHASE_HI_UPDATE, DDS_REG_PHASE_HI );
                }
//                {

//                }

        //        state->phase_correction = calculate_phase_correction( state, ent->tune );
        //        set_phase_correction ( state->phase_correction );
		        dp_writel((ent->tune & 0xffffff), DDS_REG_TUNE_VAL);
		    }
            gqueue_pop ( &state->tune_queue );
            return ;
        }

		gqueue_pop ( &state->tune_queue );
    }
}

void dds_slave_init(struct dds_slave_state *state)
{
        state->enabled = 0;
        state->lock_id = -1;
        state->stream_id = 123;
	    state->delay_samples = 3;
        memset(&state->stats, 0, sizeof(struct dds_slave_stats));
}

void dds_slave_start(struct dds_slave_state *state)
{
    state->enabled = 1;
}

#if 0
/*
void trc_start()
{
    ctai = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
    ccyc = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
}

void trc_show(char *what)
{
    	pp_printf("%s tai %d cyc %d \n", what, ctai, ccyc);
}*/


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
    state->kp = 100 << 4;
    state->ki = 1;
    state->locked = 0;
    state->lock_samples = 10000;
    state->delock_samples = 1000;
    state->lock_threshold = 10000;
    state->lock_counter = 0;
    state->sampling_divider = 100;
    state->lock_id = 0;

    dp_writel(0, DDS_REG_CR);

    if(!state->enabled)
        return;

    if(state->master)
    {
        /* set DDS center frequency */
        dp_writel(DDS_CR_SAMP_DIV_W(state->sampling_divider - 1), DDS_REG_CR);

        dp_writel(state->base_tune >> 32, DDS_REG_FREQ_HI);
        dp_writel(state->base_tune & 0xffffffff, DDS_REG_FREQ_LO);
	delay(100);

        dp_writel(DDS_CR_SAMP_EN | DDS_CR_SAMP_DIV_W(state->sampling_divider - 1), DDS_REG_CR);

    } else {

	state->slave_state = SLAVE_WAIT_CONFIG;
    }

    /* Tuning gain = 1 */
    dp_writel(DDS_TUNE_GAIN, DDS_REG_GAIN);
}

void rf_counter_init(struct rf_counter_state *state, struct dds_loop_state *loop);

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

    pp_printf("StreamConfig: mode %d enabled %d\n", mode, dds_loop.enabled );

    dds_loop_start(&dds_loop);
    rf_counter_init(&rf_cnt, &dds_loop);

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
    state->period = 44 * 1000 * 1000; // roughly 100 ms @ RF = 352 MHz, make configurable
    state->dds = loop;
    state->fixup_ready = 0;

    dp_writel(state->period - 1, DDS_REG_RF_CNT_PERIOD);

    uint32_t cr = dp_readl(DDS_REG_CR);

    dp_writel(cr | DDS_CR_RF_CNT_ENABLE, DDS_REG_CR);

}


    uint32_t c_start, c_complete;

void rf_counter_update(struct rf_counter_state *state)
{
    if(!state->dds->enabled)
    {
	state->state=  RF_COUNTER_WAIT_LOCK;
	return;
    }


    switch(state->state)
    {
	case RF_COUNTER_WAIT_LOCK:
	    if(state->dds->master)
	    {
		if(state->dds->locked)
		    state->state = RF_COUNTER_MASTER_WAIT_SNAPSHOT;
	    }
	    else if(state->dds->slave_state == SLAVE_RUN )
	    { // slave mode
		    state->state = RF_COUNTER_SLAVE_WAIT_FIXUP;


	    }
		break;

	case RF_COUNTER_MASTER_WAIT_SNAPSHOT:
	    if(state->dds->current_sample_idx == 0)
	    {
		state->snap_rf_cycles = dp_readl(DDS_REG_RF_CNT_RF_SNAPSHOT);
		state->snap_tai_cycles = dp_readl(DDS_REG_RF_CNT_CYCLES_SNAPSHOT);
		state->snap_tai_seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);

		state->state = RF_COUNTER_MASTER_WAIT_SNAPSHOT;
	    }
	    break;

	case RF_COUNTER_SLAVE_WAIT_FIXUP:
	    if ( state->fixup_ready )
	    {

		state->fixup_ready = 0;
		state->state = RF_COUNTER_SLAVE_RESYNC;

    		uint64_t tune = ( state->dds->base_tune + (state->dds->last_tune * DDS_TUNE_GAIN ) ) ; // fixme: gain
		uint32_t delay_clks = (state->dds->slave_delay * state->dds->sampling_divider * 125);
		uint32_t correction = (uint64_t)delay_clks  * tune / (1ULL<<43) + 2;


		uint32_t trig_cycles = delay_clks + state->snap_tai_cycles;



	        dp_writel(rf_cnt.period-1, DDS_REG_RF_CNT_PERIOD);

		uint32_t cr = dp_readl(DDS_REG_CR);
		dp_writel (cr | DDS_CR_RF_CNT_ENABLE,DDS_REG_CR);



		int32_t rf_cycles = state->snap_rf_cycles + correction;
		if(rf_cycles < 0)
		    rf_cycles += rf_cnt.period;
		if(rf_cycles >= rf_cnt.period)
		    rf_cycles -= rf_cnt.period;



	        dp_writel(rf_cycles, DDS_REG_RF_CNT_SYNC_VALUE );
	        dp_writel(trig_cycles | DDS_RF_CNT_TRIGGER_ARM_LOAD, DDS_REG_RF_CNT_TRIGGER );


	    }
	    break;

	case RF_COUNTER_SLAVE_RESYNC:
	{
	    uint32_t cr = dp_readl(DDS_REG_RF_CNT_TRIGGER);

	    if(cr & DDS_RF_CNT_TRIGGER_DONE)
	    {
                smem_rf_ok = 1;
			    state->state = RF_COUNTER_SLAVE_READY;
	    }


	    break;
	}


	case RF_COUNTER_SLAVE_READY:
	    if(state->dds->current_sample_idx == 0)
	    {
		state->snap_rf_cycles = dp_readl(DDS_REG_RF_CNT_RF_SNAPSHOT);
	    }

	    break;


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
    return 1;
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
return 1;
    return (wr_state == WR_LINK_SYNCED);
}



void main_loop()
{
    smem_rf_ok = 0;
    wr_state = WR_LINK_OFFLINE;
    wr_enable_lock(0);

    for(;;)
    {
        do_rx(&dds_loop);
        dds_loop_update (&dds_loop);
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

#endif
