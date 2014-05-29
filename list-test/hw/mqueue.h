#ifndef __MQUEUE_H
#define __MQUEUE_H

#define BASE_HMQ        0x00000

#define MQUEUE_GCR       ( 0x0)
#define MQUEUE_IN(slot)  ( 0x4000 + (slot) * 0x400)
#define MQUEUE_OUT(slot) ( 0x8000 + (slot) * 0x400)

#define MQUEUE_CMD_CLAIM (1<<24)
#define MQUEUE_CMD_PURGE (1<<25)
#define MQUEUE_CMD_READY (1<<26)
#define MQUEUE_CMD_DISCARD (1<<27)

#define MQUEUE_SLOT_COMMAND 0
#define MQUEUE_SLOT_STATUS 4
#define MQUEUE_SLOT_DATA_START 8

#define MQUEUE_GCR_INCOMING_STATUS_MASK (0x0000ffff)
#define MQUEUE_GCR_SLOT_COUNT 0
#define MQUEUE_GCR_SLOT_STATUS 4
#define MQUEUE_GCR_IRQ_MASK 8
#define MQUEUE_GCR_IRQ_COALESCE 12

#define MQUEUE_SLOT_STATUS_FULL (1<<0)

#endif
