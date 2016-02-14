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

void rf_counter_update(struct dds_slave_state *state);

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
        state->rf_cnt_state = RF_CNT_SLAVE_WAIT_FIXUP;
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
            state->fixups_applied = 0;
            state->rf_cnt_period = msg->phase_fixup.rf_cnt_period;
            state->vco_gain = msg->phase_fixup.vco_gain;


            /* set DDS center frequency */
            dp_writel(state->base_freq >> 32, DDS_REG_FREQ_HI);
            dp_writel(state->base_freq & 0xffffffff, DDS_REG_FREQ_LO);
            dp_writel(state->vco_gain, DDS_REG_GAIN);
            dp_writel(state->rf_cnt_period-1, DDS_REG_RF_CNT_PERIOD);

            dp_writel(DDS_CR_RF_CNT_ENABLE | DDS_CR_SAMP_EN | DDS_CR_SAMP_DIV_W(state->sampling_divider - 1), DDS_REG_CR);

    	    gqueue_init(&state->tune_queue, TUNE_QUEUE_ENTRIES, sizeof(struct tune_queue_entry), _tune_queue_buf);

    	    dbg_printf("Got config: sps %d base_freq 0x%08x%08x rfPeriod %d vco_gain %d lock_id %d\n",
    		state->samples_per_second,
    		(uint32_t) (state->base_freq >> 32),
    		(uint32_t) (state->base_freq & 0xffffffff),
    		state->rf_cnt_period,
    		state->vco_gain,
    		state->lock_id);


            state->slave_state = SLAVE_RUN;
            state->rf_cnt_state = RF_CNT_SLAVE_WAIT_FIXUP;

            state->fixup_phase = msg->phase_fixup.fixup_phase;
    	    state->fixup_time.seconds =  msg->phase_fixup.fixup_tai;
        	state->fixup_count++;

            state->rf_cnt_snap_timestamp.ticks = msg->phase_fixup.rf_cnt_snap_cycles;
            state->rf_cnt_snap_count = msg->phase_fixup.rf_cnt_snap_count;

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

    if(state->slave_state != SLAVE_RUN)
        return;

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

    acc = phase_per_sample * (uint64_t) n;
    acc &= DDS_ACC_MASK;

    return acc;
}

void dds_slave_update(struct dds_slave_state *state)
{
    uint32_t pd_data;

    if(!state->enabled)
        return;

    if(!wr_is_timing_ok())
    {
        state->slave_state = SLAVE_WAIT_CONFIG;
        return;
    }

    do_rx( state );

    if (state->slave_state != SLAVE_RUN )
        return;

    rf_counter_update (state);

	if(!dds_poll_next_sample(&pd_data))
        return;

    // produce timestamp of the current tune sample
	int sample_idx = dp_readl(DDS_REG_SAMPLE_IDX);
	uint32_t tai = lr_readl(WRN_CPU_LR_REG_TAI_SEC);

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

    while( !gqueue_empty ( &state->tune_queue ) )
    {
        struct tune_queue_entry *ent = gqueue_front ( &state->tune_queue );

		if(ent->tai == tai && ent->sample_id == sample_idx)
		{
            if(sample_idx == state->delay_samples)
            {
                state->phase_correction = calculate_phase_correction( state, ent->tune, state->delay_samples );

                uint64_t fixup_phase_next = state->fixup_phase;
                //fixup_phase_next += (int64_t) ent->tune * state->vco_gain;
                //fixup_phase_next += (int64_t) (state->base_freq); // + (int64_t)ent->tune * state->vco_gain);
                //fixup_phase_next += (int64_t) ent->tune * state->vco_gain;
                //fixup_phase_next += (uint64_t) ent->tune * state->vco_gain;

                dp_writel(fixup_phase_next >> 32, DDS_REG_ACC_LOAD_HI);
        		dp_writel(fixup_phase_next & 0xffffffff, DDS_REG_ACC_LOAD_LO);

            //    dp_writel( state->phase_correction & 0xffffffff, DDS_REG_PHASE_LO );
            //    dp_writel( ((state->phase_correction >> 32) & 0x7ff) | DDS_PHASE_HI_UPDATE, DDS_REG_PHASE_HI );

                dp_writel((ent->tune & 0xffffff)| DDS_TUNE_VAL_LOAD_ACC, DDS_REG_TUNE_VAL);

                state->fixups_applied++;
                pp_printf("apply\n");
            } else {
                dp_writel((ent->tune & 0xffffff), DDS_REG_TUNE_VAL);
            }


            state->last_tune = ent->tune;

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
	    state->delay_samples = 4;
        memset(&state->stats, 0, sizeof(struct dds_slave_stats));
}

void dds_slave_start(struct dds_slave_state *state)
{
    state->enabled = 1;

    state->slave_state = SLAVE_WAIT_CONFIG;
    state->rf_cnt_state = RF_CNT_SLAVE_WAIT_FIXUP;

    gqueue_init(&state->tune_queue, TUNE_QUEUE_ENTRIES, sizeof(struct tune_queue_entry), _tune_queue_buf);
}

void dds_slave_stop(struct dds_slave_state *state)
{
    state->enabled = 0;
    dp_writel( 0, DDS_REG_CR );
}

void rf_counter_update(struct dds_slave_state *state)
{
    // we need the RF to be stable in order to sync the counters
    if(state->slave_state != SLAVE_RUN)
        return;

    switch(state->rf_cnt_state)
    {
        case RF_CNT_SLAVE_WAIT_FIXUP:
	    {
            if ( !state->fixups_applied )
                return;


    		uint64_t tune = ( state->base_freq + ( state->last_tune * state->vco_gain ) ) ;
		    uint32_t delay_clks = ( state->delay_samples * state->sampling_divider * 125);
		    uint32_t correction = (uint64_t)delay_clks * tune / (1ULL<<43) + 2;
            uint32_t trig_cycles = delay_clks + state->rf_cnt_snap_timestamp.ticks;

            int32_t rf_cycles = state->rf_cnt_snap_count + correction;
		    if(rf_cycles < 0)
		          rf_cycles += state->rf_cnt_period;
		    if(rf_cycles >= state->rf_cnt_period)
		          rf_cycles -= state->rf_cnt_period;


	        dp_writel(rf_cycles, DDS_REG_RF_CNT_SYNC_VALUE );
	        dp_writel(trig_cycles | DDS_RF_CNT_TRIGGER_ARM_LOAD, DDS_REG_RF_CNT_TRIGGER );

            pp_printf("CounterCorr %d %d\n", rf_cycles, trig_cycles );

            state->rf_cnt_state = RF_CNT_SLAVE_RESYNC;
            break;
	    }

	    case RF_CNT_SLAVE_RESYNC:
        {
	        uint32_t cr = dp_readl(DDS_REG_RF_CNT_TRIGGER);

    	    if(cr & DDS_RF_CNT_TRIGGER_DONE)
    	    {
                    smem_rf_ok = 1;
    			    state->rf_cnt_state = RF_CNT_SLAVE_READY;
    	    }
            break;
	    }

        case RF_CNT_SLAVE_READY:
        {
            break;
        }

        default:
            break;

    }
}
