/*
 * This work is part of the White Rabbit Node Core project.
 *
 * Copyright (C) 2013-2014 CERN (www.cern.ch)
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * Released according to the GNU GPL, version 2 or any later version.
 */

#ifndef __WR_D3S_COMMON_H
#define __WR_D3S_COMMON_H

#include <stdint.h>

#ifdef WRNODE_RT
#include "rt-mqueue.h"
#endif

/* WR Node CPU Core indices */
#define WR_D3S_CPU_MAIN 0			/* Core 0 controls the D3S mezzanine */

#define WR_D3S_IN_CONTROL	0
#define WR_D3S_OUT_CONTROL 0

#define WR_D3S_REMOTE_OUT_STREAM 0
#define WR_D3S_REMOTE_IN_STREAM 0

#define WR_D3S_CMD_START_RESPONSE_LOGGING		0x1
#define WR_D3S_REP_LOG_PAYLOAD    0x2
#define WR_D3S_REP_ACK_ID    0x3
#define WR_D3S_CMD_PING    0x4
#define WR_D3S_CMD_STREAM_CONFIG 0x5
#define WR_D3S_CMD_TEST_SIGNAL 0x6
#define WR_D3S_REP_NACK    0x7
#define WR_D3S_CMD_SET_GAINS 0x8
#define WR_D3S_CMD_SET_AD9516 0xa

#define WR_D3S_ERR_INCORRECT_MODE 0x1

#define D3S_STREAM_MASTER 0x1
#define D3S_STREAM_SLAVE 0x2
#define D3S_STREAM_OFF 0x3

#define D3S_MSG_PHASE_FIXUP 0x1
#define D3S_MSG_TUNE_UPDATE 0x2
#define D3S_MSG_EVENT 0x3
#define D3S_MSG_COUNTER_FIXUP 0x4

struct wr_timestamp {
	uint64_t seconds;
	uint32_t ticks;
	uint32_t frac;
};

#ifdef WRNODE_RT
struct wr_d3s_remote_message {
    struct rmq_message_addr hdr;
    /* Message type (see D3S_MSG_ macros). Types are:
	PHASE_FIXUP: initial 'fixup' phase value sent at the beginning of every second
	TUNE_UPDATE: momentary frequency value, sent at every sample */
    int type;
    /* ID of the RF stream */
    int stream_id;
    /* Sampling clock divider value (FSamp = 1 MHz / sampling_divider) */
    int sampling_divider;
    /* Master lock ID. If increased, the master has-relocked and the slave has to re-lock as well */
    int lock_id;

    /* Software TX timestamp */
    uint32_t transmit_seconds;
    uint32_t transmit_cycles;

    union {
        struct {
	    /* TAI second the fixup has been generated for */
            uint32_t fixup_tai;
	    /* Phase value (1 << DDS_ACC_BITS = 2*Pi) */
            int64_t fixup_phase;
	    /* Base frequency (DDS step) */
            int64_t base_freq;
	    /* TAI cycles at which the RF counter snapshot was taken */
            uint32_t rf_cnt_snap_cycles;
	    /* RF counter value at the snapshot time above */
            uint32_t rf_cnt_snap_count;
	    /* Period of the RF counter */
	    uint32_t rf_cnt_period;
	    /* Gain of the master VCO (multiplicand of the tune) */
            int32_t vco_gain;
        } phase_fixup;
        struct {
	    /* TAI seconds at which the sample was taken) */
            uint32_t tai;
	    /* Tune value (DDS-specific) */
            int32_t tune;
	    /* Index of the sample within the current TAI second (0...sampling_divider-1) */
            uint32_t sample_id;
        } tune_update;
    };
    uint32_t pad;
};
#endif


struct wr_d3s_state {
    int locked;
    int mode;

};

#ifdef WRNODE_RT
static inline void ts_add(struct wr_timestamp *a, const struct wr_timestamp *b)
{
    a->frac += b->frac;

    if(a->frac >= 4096)
    {
    	a->frac -= 4096;
    	a->ticks ++;
    }

    a->ticks += b->ticks;

    if(a->ticks >= 125000000)
    {
    	a->ticks -= 125000000;
    	a->seconds++;
    }

    a->seconds += b->seconds;
}

static inline void ts_sub(struct wr_timestamp *a, const struct wr_timestamp *b)
{
    a->frac -= b->frac;

    if(a->frac < 0)
    {
    	a->frac += 4096;
    	a->ticks --;
    }

    a->ticks -= b->ticks;

    if(a->ticks < 0)
    {
    	a->ticks += 125000000;
    	a->seconds--;
    }

    a->seconds -= b->seconds;

    if(a->seconds == -1)
    {
      a->seconds = 0;
      a->ticks -= 125000000;
    }
}
#endif

#endif
