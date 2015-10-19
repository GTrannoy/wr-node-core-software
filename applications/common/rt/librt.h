/*
 * Copyright (C) 2015 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *
 * Released according to the GNU GPL, version 3
 */

#ifndef __LIBRT_H__
#define __LIBRT_H__

#include <stdint.h>
#include "wrnc-common.h"
#include "hw/wrn_cpu_lr.h"
#include "rt-common.h"
#include "rt-mqueue.h"
#include "rt-message.h"
#include "pp-printf.h"

#define RT_VARIABLE_FLAG_REG	(1 << 0)

#define RT_MQ_FLAGS_REMOTE (1 << 0)
#define RT_MQ_FLAGS_OUTPUT (1 << 1)
#define RT_MQ_FLAGS_LOCAL (0)
#define RT_MQ_FLAGS_INPUT (0)

#define RT_VERSION_MAJ(_v) ((_v >> 16) & 0xFFFF)
#define RT_VERSION_MIN(_v) (_v & 0xFFFF)
#define RT_VERSION(_a, _b) (((_a & 0xFFFF) << 16) | (_b & 0xFFFF))

#define ARRAY_SIZE(_a) (sizeof(_a) / sizeof(_a[0]))

#ifdef LIBRT_DEBUG
static inline void rt_print_data(uint32_t *d, unsigned int count)
{
	int i;

	for (i = 0; i < count; i++) {
		pp_printf("%s: data[%d] = 0x%x\n", __func__, i , d[i]);
		delay(1000);
	}
}
static inline void rt_print_header(struct wrnc_proto_header *h)
{
	pp_printf("header ----\n");
	delay(1000);
	pp_printf("    app_id 0x%x | msg_id %d | slot_io 0x%x | seq %d\n",
		  h->rt_app_id, h->msg_id, h->slot_io, h->seq);
	delay(1000);
	pp_printf("    len %d | flags 0x%x | trans 0x%x | time %d\n",
		  h->len, h->flags, h->trans, h->time);
	delay(1000);
}
#endif

extern uint32_t msg_seq;

#define RT_VARIABLE_FLAG_WO (1 << 0)
/**
 * Description of a RealTime variable that we want to export to user-space
 */
struct rt_variable {
	uint32_t addr; /**< variable location (RAM, SMEM, CPU, peripherals)*/
	uint32_t mask; /**< variable mask without offset applied */
	uint8_t offset; /**< variable offset within the word */
	uint32_t flags; /**< variable options */
};

typedef int (action_t)(struct wrnc_proto_header *hin, void *pin,
		       struct wrnc_proto_header *hout, void *pout);

/**
 * Description of a RealTime structure that we want to export to user-space
 */
struct rt_structure {
	void *struct_ptr; /**< structure location */
	uint32_t len; /**< data structure lenght */
	/* Maybe other option later in time */
};

struct rt_mq {
	uint8_t index;
	unsigned long flags;
};

/**
 * Real-Time Application Descriptor
 */
struct rt_application {
	const char name[16];
	const struct wrnc_rt_version version;
	struct rt_mq *mq; /**< list of used MQ */
	uint8_t n_mq; /**< number of available MQ */

	struct rt_structure *structures; /**< exported structures */
	unsigned int n_structures; /**< number or exported structures */

	struct rt_variable *variables; /**< exported variables */
	unsigned int n_variables; /**< number or exported variables */

	action_t **actions;
	unsigned int n_actions;
};

extern void rt_init(struct rt_application *app);
extern int rt_mq_register(struct rt_mq *mq, unsigned int n);
extern int rt_mq_action_register(uint32_t id, action_t action);
extern int rt_mq_action_dispatch(unsigned int mq_in);


/**
 * This is a helper that send back a simple ACK message. Keep it static inline
 * to avoid a function call. We gain performance, we loose memory
 */
static inline void rt_send_ack(struct wrnc_proto_header *hin, void *pin,
			       struct wrnc_proto_header *hout, void *pout)
{
	hout->msg_id = RT_ACTION_SEND_ACK;
	hout->len = 0;
}

/**
 * This is a helper that send back a simple NACK message. Keep it static inline
 * to avoid a function call. We gain performance, we loose memory
 */
static inline void rt_send_nack(struct wrnc_proto_header *hin, void *pin,
				struct wrnc_proto_header *hout, void *pout)
{
	hout->msg_id = RT_ACTION_SEND_NACK;
	hout->len = 0;
}


extern void rt_get_time(uint32_t *seconds, uint32_t *cycles);
extern void rt_action_export(action_t **actions, unsigned int count);
extern int rt_variable_setter(struct wrnc_proto_header *hin, void *pin,
			    struct wrnc_proto_header *hout, void *pout);
extern int rt_variable_getter(struct wrnc_proto_header *hin, void *pin,
			    struct wrnc_proto_header *hout, void *pout);
extern int rt_structure_setter(struct wrnc_proto_header *hin, void *pin,
			       struct wrnc_proto_header *hout, void *pout);
extern int rt_structure_getter(struct wrnc_proto_header *hin, void *pin,
			       struct wrnc_proto_header *hout, void *pout);
extern int rt_recv_ping(struct wrnc_proto_header *hin, void *pin,
			struct wrnc_proto_header *hout, void *pout);
extern int rt_version_getter(struct wrnc_proto_header *hin, void *pin,
			     struct wrnc_proto_header *hout, void *pout);


/**
 * It send the message associate to the given header
 * @param[in] msg message to send
 */
static inline void rt_mq_msg_send(struct wrnc_msg *msg)
{
	struct wrnc_proto_header *hdr;

	hdr = rt_proto_header_get((void *)msg->data);

	/* When we are not using sync messages, we use the global
	   sequence number */
	if (!(hdr->flags & WRNC_PROTO_FLAG_SYNC))
		hdr->seq = msg_seq++;

	mq_send(!!(hdr->flags & WRNC_PROTO_FLAG_REMOTE),
		(hdr->slot_io & 0xF),
		hdr->len + (sizeof(struct wrnc_proto_header) / 4));
}

#endif /* __LIBRT_H__ */
