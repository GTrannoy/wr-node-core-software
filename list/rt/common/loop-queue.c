#include "rt.h"
#include "list-common.h"

#define LOOP_QUEUE_SIZE 16

SMEM int head, tail, count;
SMEM struct list_trigger_entry buf[16];

