/*
 * This work is part of the White Rabbit Node Core project.
 *
 * Copyright (C) 2013-2014 CERN (www.cern.ch)
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * Released according to the GNU GPL, version 2 or any later version.
 */

/*.
 * White Rabbit Node Core
 *
 * rt-common.c: common RT CPU functions
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "rt-mqueue.h"
#include "rt-common.h"

static int debug_slot;

void rt_set_debug_slot(int slot)
{
    debug_slot = slot;
}

int puts(const char *p)
{
    int i;
    volatile uint32_t *buf = mq_map_out_buffer(0, debug_slot);
    
    mq_claim(0, debug_slot);
    
    buf[0] = 0xdeadbeef;
    for(i=0;i<127;i++,p++)
      {
  	   if(*p)
	       buf[i+1] = *p;
	     else
	       break;
      }

    mq_send(0, debug_slot, i + 1);
    return i;
}

