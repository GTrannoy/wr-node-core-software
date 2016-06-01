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
 * rt-message.h: Message assembling helper functions
 */

#ifndef __RT_MESSAGE_H
#define __RT_MESSAGE_H

#include <mockturtle-common.h>

#ifdef WRNODE_RT

enum trtl_msg_direction {
	WRNC_MSG_DIR_SEND = 0,
	WRNC_MSG_DIR_RECEIVE = 1
};

/**
 * White-Rabbit Node-Core message descriptor
 */
struct trtl_msg {
	uint32_t datalen; /**< payload length*/
	volatile uint32_t *data; /**< payload */
	uint32_t max_size; /**< maximum message size for chosen slot */
	uint32_t offset; /**< serialization/deserialization offset */
	enum trtl_msg_direction direction; /**< serialization direction (used by trtl_msg_x functions) */
	int error; /** serialization error status */
	int slot; /** concerned slot */
};


/**
 * It claims an output slot. This means that you get exclusive access to
 * the slot.
 */
static inline struct trtl_msg rt_mq_claim_out(struct trtl_proto_header *h)
{
	struct trtl_msg b;
	int remote = !!(h->flags & TRTL_PROTO_FLAG_REMOTE);
	int slot = h->slot_io & 0xF;

	mq_claim(remote, slot);

	b.data = mq_map_out_buffer(remote, slot);
	b.direction = WRNC_MSG_DIR_SEND;
	b.max_size = h->len;
	b.offset = 0;
	b.datalen = 0;
	b.slot = slot;

	return b;
}

/**
 * Obsolete. Use rt_mq_claim_out
 */
static inline struct trtl_msg hmq_msg_claim_out(int slot, int max_size)
{
	struct trtl_proto_header h = {
		.slot_io = (slot & 0xF),
		.len = max_size,
	};

	return rt_mq_claim_out(&h);
}

/**
 * It claims an input slot. This mean that you get exclusive access to
 * the slot
 */
static inline struct trtl_msg rt_mq_claim_in(struct trtl_proto_header *h)
{
	struct trtl_msg b;
	int slot = (h->slot_io >> 4) & 0xF;

	b.data = mq_map_in_buffer(0, slot);
	b.direction = WRNC_MSG_DIR_RECEIVE;
	b.max_size = h->len;
	b.datalen = h->len;
	b.offset = 0;
	b.slot = slot;

	return b;
}

/**
 * Obsolete. Use rt_mq_claim_in
 */
static inline struct trtl_msg hmq_msg_claim_in(int slot, int max_size)
{
	struct trtl_proto_header h = {
		.slot_io = (slot & 0xF) << 4,
		.len = max_size,
	};

	return rt_mq_claim_in(&h);
}


/**
 * It enqueue the message in the slot, and it will be sent as soon as possible
 */
static inline void hmq_msg_send(struct trtl_msg *buf)
{
	mq_send(0, buf->slot, buf->datalen);
}

#endif

#endif
