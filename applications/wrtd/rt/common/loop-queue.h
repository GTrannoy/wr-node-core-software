#ifndef __LOOP_QUEUE_H
#define __LOOP_QUEUE_H

#include "rt.h"
#include "wrtd-common.h"

#define LOOP_QUEUE_SIZE 16

void loop_queue_init();
void loop_queue_push(struct wrtd_trig_id *id, uint32_t seq, struct wr_timestamp *ts);
struct wrtd_trigger_entry *loop_queue_pop();

#endif

