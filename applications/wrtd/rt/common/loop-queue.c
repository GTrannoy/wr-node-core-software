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
 * loop-queue.c: Shared Memory-based Loopback queue.
 */

#include "rt.h"
#include "wrtd-common.h"

#define LOOP_QUEUE_SIZE 16

static SMEM int head, tail, count;
static SMEM struct wrtd_trigger_entry buf[16];

void loop_queue_init()
{
	head = tail = count = 0;
}

void loop_queue_push(struct wrtd_trig_id *id, uint32_t seq, struct wr_timestamp *ts)
{
	if(count == LOOP_QUEUE_SIZE)
		return;

	buf[head].id = *id;
	buf[head].seq = seq;
	buf[head].ts = *ts;

	smem_atomic_add(&head, 1);

	if (head == LOOP_QUEUE_SIZE)
		head = 0;

	smem_atomic_add(&count, 1);
}

struct wrtd_trigger_entry *loop_queue_pop()
{
	if(!count)
		return NULL;

	struct wrtd_trigger_entry *rv = &buf[tail];

	smem_atomic_add(&tail, 1);

	if(tail == LOOP_QUEUE_SIZE)
		tail = 0;
	
	smem_atomic_sub(&count, 1);

	return rv;
}
