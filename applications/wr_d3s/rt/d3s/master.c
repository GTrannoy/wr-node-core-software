/*
 * This work is part of the White Rabbit Node Core project.
 *
 * Copyright (C) 2013-2016 CERN (www.cern.ch)
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * Released according to the GNU GPL, version 2 or any later version.
 */


/*.
 * WR Distributed DDS Realtime Firmware.
 *
 * master.c - master side PLL
 */

#include <string.h>

#include "wr-d3s-common.h"
#include "wrtd-serializers.h"
#include "rt-d3s.h"

#include "master.h"


/*

PLL response logging code

*/

// initializes the PLL response log
static void resp_log_init(struct resp_log_state *state)
{
    state->active = 0;
}

// adds a new phase-control value entry to the response log, send it to the host if full
void resp_log_update(struct resp_log_state *state, int phase, int y)
{
    if(!state->active)
        return;

    state->buf[state->block_offset++] = phase;
    state->buf[state->block_offset++] = y;

    state->remaining_samples--;

    // we've got a full block? make a message
    if(state->block_offset == state->block_samples || !state->remaining_samples)
    {
        struct wrnc_msg buf = hmq_msg_claim_out (WR_D3S_OUT_CONTROL, RESP_LOG_BUF_SIZE + 3);

        uint32_t id = WR_D3S_REP_LOG_PAYLOAD;
        int i;

        wrnc_msg_header (&buf, &id, &state->seq);
        wrnc_msg_int32 (&buf, (int*)&state->block_offset);
        wrnc_msg_int32 (&buf, (int*)&state->block_index);

        for(i = 0; i < state->block_offset; i++)
            wrnc_msg_int32(&buf, (int*)&state->buf[i]);

        hmq_msg_send (&buf);

        state->block_offset = 0;
        state->block_index++;
    }

    if(state->remaining_samples == 0)
    {
        state->active = 0;
    }
}

// starts response logging
void resp_log_start( struct resp_log_state *state, int seq, int n_samples )
{
    state->block_index = 0;
    state->seq = seq;
    state->block_offset = 0;
    state->block_samples = RESP_LOG_BUF_SIZE;
    state->remaining_samples = n_samples;
    state->active = 1;
}

/*

Message handling

*/

static inline volatile struct wr_d3s_remote_message* prepare_message(struct dds_master_state *state, int type )
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

static inline void send_phase_fixup( struct dds_master_state *state )
{
    volatile struct wr_d3s_remote_message *msg = prepare_message ( state, D3S_MSG_PHASE_FIXUP );

    msg->phase_fixup.base_freq = state->base_freq;
    msg->phase_fixup.vco_gain = state->vco_gain;

    msg->phase_fixup.fixup_tai = state->fixup_time.seconds;
    msg->phase_fixup.fixup_phase = state->fixup_phase;

    msg->phase_fixup.rf_cnt_period = state->rf_cnt_period;
    msg->phase_fixup.rf_cnt_snap_cycles = state->rf_cnt_snap_timestamp.ticks;
    msg->phase_fixup.rf_cnt_snap_count  = state->rf_cnt_snap_count;

    dbg_printf("SendFixup [%d]!\n", state->stats.sent_updates);


    mq_send(1, WR_D3S_REMOTE_OUT_STREAM, 32);

    state->stats.sent_fixups++;
}

static inline void send_tune_update( struct dds_master_state *state, struct wr_timestamp *ts )
{
    volatile struct wr_d3s_remote_message *msg = prepare_message ( state, D3S_MSG_TUNE_UPDATE );

    msg->tune_update.tai = ts->seconds;
    msg->tune_update.tune = state->current_tune;
    msg->tune_update.sample_id = state->current_sample_idx;

    mq_send(1, WR_D3S_REMOTE_OUT_STREAM, 32);

    state->stats.sent_updates++;
}


/*

Master PLL servo

*/

// PI PLL Servo
static int pi_update(struct dds_master_state *state, int phase_error)
{
    int err = -(phase_error - 32768);

    state->integ += err;

    // do the PI control
    int y = (state->kp * err + state->ki * state->integ) >> 14;

    // log the PLL response
    resp_log_update(&state->rsp_log, phase_error, y);

    // Lock detection logic
    if( !state->locked )
    {
        if(  abs(err) <= state->lock_threshold)
        {
            state->lock_counter++;
            if(state->lock_counter == state->lock_samples)
            {
        		dbg_printf("[master] RF lock acquired\n");
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
		        dbg_printf("[master] RF lock lost\n");
                state->locked = 0;
                state->lock_counter = 0;
            }

        }
    }


    return y;
}

// reads the next phase error sample from the ADC (if there's a fresh sample returns 1, otherwise 0)
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

// reads the DDS accumulator value (a.k.a 'phase snapshot')
// at the beginning of the current sampling period
static uint64_t get_phase_snapshot(struct dds_master_state *state)
{
    uint32_t snap_lo = dp_readl ( DDS_REG_ACC_SNAP_LO );
    uint32_t snap_hi = dp_readl ( DDS_REG_ACC_SNAP_HI );

    int64_t acc_snap = snap_lo;
    acc_snap |= ((int64_t) snap_hi ) << 32;

// the snapshot is taken few clock cycles after the sampling trigger, we need to compensate for this
    acc_snap += (int64_t)DDS_SNAP_LAG_SAMPLES * (state->base_freq + state->current_tune) ;
    acc_snap &= DDS_ACC_MASK;

    return acc_snap;
}

// main master loop
void dds_master_update(struct dds_master_state *state)
{
	uint32_t pd_data;

    if( !state->enabled )
        return;

    if (!wr_is_timing_ok())
        return;

    if(!dds_poll_next_sample(&pd_data))
    	return;

    // produce timestamp of the current tune sample
    state->current_sample_idx = dp_readl(DDS_REG_SAMPLE_IDX);

    struct wr_timestamp ts;
    ts.seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
    ts.ticks = state->current_sample_idx * state->sampling_divider * 125;

    /* do the actual PLL's job */
    int y = pi_update(state, pd_data & 0xffff);

    // and drive the oscillator
    dp_writel(y & 0xffffff, DDS_REG_TUNE_VAL);

	state->current_tune = y;//& 0xffffff;

    if(!state->locked)
        return;

    // for every sample aligned with the PPS, send a 'fixup' message
    // containing the setup (i.e. base frequency & counter period/snapshot).
    if(state->current_sample_idx == 0)
    {
        // take snapshot of the RF phase
        state->fixup_time  = ts;
        state->fixup_phase = get_phase_snapshot(state);
        state->fixup_ready = 1;

        // and the RF counter
        state->rf_cnt_snap_count = dp_readl(DDS_REG_RF_CNT_RF_SNAPSHOT);
		state->rf_cnt_snap_timestamp.ticks = dp_readl(DDS_REG_RF_CNT_CYCLES_SNAPSHOT);
        state->rf_cnt_snap_timestamp.seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);

        send_phase_fixup(state);
    }
    // for every sample, send the current tune value
    send_tune_update(state, &ts);
}


void dds_master_init (struct dds_master_state *state)
{
    state->enabled = 0;
    state->kp = 200000;//100 << 4;
    state->ki = 1024; //1
    state->lock_samples = 10000;
    state->delock_samples = 1000;
    state->lock_threshold = 2000;
    state->sampling_divider = 100;
    state->vco_gain = 1024;
    state->stream_id = 123;
    state->lock_id = 0;

    state->rf_cnt_period = 44 * 1000 * 1000; // 44 MHz by default
    state->base_freq=0x2d0e5604189ULL; // 352 MHz by default

}

void dds_master_start(struct dds_master_state *state)
{
    state->enabled = 1;
    state->integ = 0;
    state->locked = 0;
    state->lock_counter = 0;
    state->fixup_ready = 0;

    resp_log_init(&state->rsp_log);

    dp_writel(0, DDS_REG_CR);

    /* set DDS center frequency */
    dp_writel(DDS_CR_SAMP_DIV_W(state->sampling_divider - 1), DDS_REG_CR);
    dp_writel(state->base_freq >> 32, DDS_REG_FREQ_HI);
    dp_writel(state->base_freq & 0xffffffff, DDS_REG_FREQ_LO);
    dp_writel(state->vco_gain, DDS_REG_GAIN);
    dp_writel(state->rf_cnt_period - 1, DDS_REG_RF_CNT_PERIOD);
    delay(100);

    /* Start sampling */
    dp_writel(DDS_CR_RF_CNT_ENABLE | DDS_CR_SAMP_EN | DDS_CR_SAMP_DIV_W(state->sampling_divider - 1), DDS_REG_CR);
}


void dds_master_stop(struct dds_master_state *state)
{
    dp_writel(0, DDS_REG_CR);
    state->enabled = 0;
}
