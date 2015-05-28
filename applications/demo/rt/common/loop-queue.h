/*
 * This work is part of the White Rabbit Node Core project.
 *
 * Copyright (C) 2013-2014 CERN (www.cern.ch)
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * Released according to the GNU GPL, version 2 or any later version.
 */


/*.
 * WR Trigger Distribution (WRTD) Firmware.
 *
 * loop-queue.h: Shared Memory-based Loopback queue.
 */

#ifndef __LOOP_QUEUE_H
#define __LOOP_QUEUE_H

#include "rt.h"
#include "wrtd-common.h"

#define LOOP_QUEUE_SIZE 16

void loop_queue_init();
void loop_queue_push(struct wrtd_trig_id *id, uint32_t seq, struct wr_timestamp *ts);
struct wrtd_trigger_entry *loop_queue_pop();

#endif


