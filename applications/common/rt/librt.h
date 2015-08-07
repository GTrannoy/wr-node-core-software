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

extern int rt_mq_action_register(uint32_t id, action_t action);
extern int rt_mq_action_dispatch(unsigned int in_slot, unsigned int is_remote);


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
extern void rt_variable_export(struct rt_variable *variable,
			       unsigned int count);
extern int rt_variable_setter(struct wrnc_proto_header *hin, void *pin,
			    struct wrnc_proto_header *hout, void *pout);
extern int rt_variable_getter(struct wrnc_proto_header *hin, void *pin,
			    struct wrnc_proto_header *hout, void *pout);
extern void rt_structure_export(struct rt_structure *structures,
				unsigned int count);
extern int rt_structure_setter(struct wrnc_proto_header *hin, void *pin,
			       struct wrnc_proto_header *hout, void *pout);
extern int rt_structure_getter(struct wrnc_proto_header *hin, void *pin,
			       struct wrnc_proto_header *hout, void *pout);
extern int rt_recv_ping(struct wrnc_proto_header *hin, void *pin,
			struct wrnc_proto_header *hout, void *pout);
extern int rt_version_getter(struct wrnc_proto_header *hin, void *pin,
			     struct wrnc_proto_header *hout, void *pout);

#endif /* __LIBRT_H__ */
