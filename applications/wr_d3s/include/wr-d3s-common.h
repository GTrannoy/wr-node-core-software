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
    int type;
    int stream_id;
    int sampling_divider;
    int lock_id;
    uint32_t transmit_seconds;
    uint32_t transmit_cycles;
  
    union {
        struct {
            uint32_t tai;
            int64_t fixup_value;
            int64_t base_tune;
        } phase_fixup;
        struct {
            uint32_t tai;
            int32_t tune;
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
