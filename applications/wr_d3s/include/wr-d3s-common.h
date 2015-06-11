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

/* WR Node CPU Core indices */
#define WR_D3S_CPU_MAIN 0			/* Core 0 controls the D3S mezzanine */

#define WR_D3S_IN_CONTROL	0
#define WR_D3S_OUT_CONTROL 0

#define WR_D3S_CMD_START_RESPONSE_LOGGING		0x1
#define WR_D3S_REP_LOG_PAYLOAD    0x2
#define WR_D3S_REP_ACK_ID    0x3
#define WR_D3S_CMD_PING    0x4


struct wr_timestamp {
	uint64_t seconds;
	uint32_t ticks;
	uint32_t frac;
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
