/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *         inspired by a draft of Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * License: GPL v3
 */

#ifndef __LIB_WRNC_H__
#define __LIB_WRNC_H__
/** @file libwrnc.h */

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <endian.h>
#include <stdint.h>
#include <stdio.h>
#include <poll.h>
#include "wrnc-common.h"
#include "wrnc-user.h"

extern const unsigned int wrnc_default_timeout_ms;

struct wrnc_dev;

#define WRNC_SYSFS_PATH_LEN 128

/**
 * Debug descriptor. It is not obfuscated because it is meant for debugging
 * purpose. This way, it leaves the user all the freedom to read/poll the
 * debug channel as (s)he wants.
 */
struct wrnc_dbg {
	struct wrnc_dev *wrnc; /**< token of the device */
	unsigned int cpu_index; /**< CPU where read debug messages */
	int fd; /**< file descriptor of the debug interface */
};


/**
 * HMQ slot descriptor
 */
struct wrnc_hmq {
	struct wrnc_dev *wrnc; /**< where this slot belong to */
	char syspath[WRNC_SYSFS_PATH_LEN];
	unsigned int index; /* index of the slot. Note that we have
			       different kind of slot and each kind start
			       counting from 0*/
	unsigned long flags; /**< flags associated to the slot */
	int fd; /**< file descriptor */
};

#define WRNC_FMC_OFFSET 2 /* FIXME this is an hack because fmc-bus does not allow
			   us a dynamic allocation of fake fmc devices */

#define WRNC_NAME_LEN 16
#define WRNC_PATH_LEN 32

#define WRNC_SYSFS_READ_LEN 32
#define WRNC_DEVICE_PATH_LEN 64


#define WRNC_HMQ_INCOMING	(1 << 0)
#define WRNC_HMQ_OUTCOMING	0x0
#define WRNC_HMQ_EXCLUSIVE	(1 << 1)
#define WRNC_HMQ_SHARED		0x0


/**
 * Error codes for white-rabbit node-core applications
 */
enum wrnc_error_number {
	EWRNC_INVAL_PARSE = 83630, /**< cannot parse data from sysfs */
	EWRNC_INVAL_SLOT, /**< invalid slot */
	EWRNC_NO_IMPLEMENTATION, /**< a prototype is not implemented */
	EWRNC_HMQ_CLOSE, /**< The HMQ is closed */
	EWRNC_INVALID_MESSAGE, /**< Invalid message */
	__EWRNC_MAX,
};


/**
 * @file libwrnc.c
 */
/**
 * @defgroup dev Device
 * Set of functions to perform basic openartion on the device
 * @{
 */
extern char *wrnc_strerror(int err);
extern int wrnc_init();
extern void wrnc_exit();
extern uint32_t wrnc_count();
extern char **wrnc_list();
extern void wrnc_list_free(char **list);

extern struct wrnc_dev *wrnc_open(const char *device);
extern struct wrnc_dev *wrnc_open_by_fmc(uint32_t device_id);
extern struct wrnc_dev *wrnc_open_by_lun(unsigned int lun);
extern void wrnc_close(struct wrnc_dev *wrnc);
extern char *wrnc_name_get(struct wrnc_dev *wrnc);
extern int wrnc_app_id_get(struct wrnc_dev *wrnc, uint32_t *app_id);
/**@}*/

/**
 * @defgroup cpu CPU
 * Set of function that allow you to manage the FPGA cores
 * @{
 */
extern int wrnc_cpu_load_application_raw(struct wrnc_dev *wrnc,
					 unsigned int index,
					 void *code, size_t length,
					 unsigned int offset);
extern int wrnc_cpu_load_application_file(struct wrnc_dev *wrnc,
					  unsigned int index,
					  char *path);
extern int wrnc_cpu_dump_application_raw(struct wrnc_dev *wrnc,
					 unsigned int index,
					 void *code, size_t length,
					 unsigned int offset);
extern int wrnc_cpu_dump_application_file(struct wrnc_dev *wrnc,
					  unsigned int index,
					  char *path);

extern int wrnc_cpu_count(struct wrnc_dev *wrnc, uint32_t *n_cpu);
extern int wrnc_cpu_reset_set(struct wrnc_dev *wrnc, uint32_t mask);
extern int wrnc_cpu_reset_get(struct wrnc_dev *wrnc, uint32_t *mask);
extern int wrnc_cpu_run_set(struct wrnc_dev *wrnc, uint32_t mask);
extern int wrnc_cpu_run_get(struct wrnc_dev *wrnc, uint32_t *mask);
extern int wrnc_cpu_enable(struct wrnc_dev *wrnc, unsigned int index);
extern int wrnc_cpu_disable(struct wrnc_dev *wrnc, unsigned int index);
extern int wrnc_cpu_start(struct wrnc_dev *wrnc, unsigned int index);
extern int wrnc_cpu_stop(struct wrnc_dev *wrnc, unsigned int index);
/**@}*/

/**
 * @defgroup hmq Host Message Queue
 * Functions to manage the HMQ slots: configuration and transmission
 * @{
 */
extern struct wrnc_hmq *wrnc_hmq_open(struct wrnc_dev *wrnc,
				      unsigned int index,
				      unsigned long flags);
extern void wrnc_hmq_close(struct wrnc_hmq *hmq);
extern int wrnc_hmq_share_set(struct wrnc_dev *wrnc, unsigned int dir,
			      unsigned int index, unsigned int status);
extern int wrnc_hmq_share_get(struct wrnc_dev *wrnc, unsigned int dir,
			      unsigned int index, unsigned int *status);
extern struct wrnc_msg *wrnc_hmq_receive(struct wrnc_hmq *hmq);
extern int wrnc_hmq_send(struct wrnc_hmq *hmq, struct wrnc_msg *msg);
extern int wrnc_hmq_send_and_receive_sync(struct wrnc_hmq *hmq,
					   unsigned int index_out,
					   struct wrnc_msg *msg,
					   unsigned int timeout_ms);

/* FIXME to be tested */
extern int wrnc_hmq_filter_add(struct wrnc_hmq *hmq,
			       struct wrnc_msg_filter *filter);
/* FIXME to be tested */
extern int wrnc_hmq_filter_clean(struct wrnc_hmq *hmq);
extern int wrnc_bind(struct wrnc_dev *wrnc, struct wrnc_msg_filter *flt,
		     unsigned int length);
/**@}*/

/**
 * @defgroup smem Shared Memory
 * Functions to access the shared memory from the host
 * @{
 */
extern int wrnc_smem_read(struct wrnc_dev *wrnc, uint32_t addr, uint32_t *data,
			  size_t count, enum wrnc_smem_modifier mod);
extern int wrnc_smem_write(struct wrnc_dev *wrnc, uint32_t addr, uint32_t *data,
			   size_t count, enum wrnc_smem_modifier mod);
/**@}*/

/**
 * @defgroup dbg Debug
 * Functions to access the debug serial stream
 * @{
 */
extern struct wrnc_dbg *wrnc_debug_open(struct wrnc_dev *wrnc,
					unsigned int index);
extern void wrnc_debug_close(struct wrnc_dbg *dbg);
extern int wrnc_debug_message_get(struct wrnc_dbg *dbg,
				  char *buf, size_t count);
/**@}*/


/**
 * @defgroup rtmsg Real Time service messages
 * Message builders for RT service messages
 * @{
 */
extern int wrnc_rt_version_get(struct wrnc_dev *wrnc,
			       struct wrnc_rt_version *version,
			       unsigned int hmq_in, unsigned int hmq_out);
extern int wrnc_rt_ping(struct wrnc_dev *wrnc,
			unsigned int hmq_in, unsigned int hmq_out);
/**@}*/

/**
 * It embeds the header into the message
 * @param[in] msg the wrnc message
 * @param[in] hdr the header you want to embed into the message
 */
static inline void wrnc_message_header_set(struct wrnc_msg *msg,
					   struct wrnc_proto_header *hdr)
{
	uint32_t *hdr32 = (uint32_t *) hdr;

	msg->data[0] = htobe32((hdr32[0] & 0xFFFF0000) |
			       ((hdr32[0] >> 8) & 0x00FF) |
			       ((hdr32[0] << 8) & 0xFF00));
	msg->data[1] = hdr32[1];
	msg->data[2] = htobe32(hdr32[2]);
	msg->data[3] = hdr32[3];
	msg->datalen = sizeof(struct wrnc_proto_header) / 4;
	msg->max_size = msg->datalen;
}

/**
 * It retrieves the header from the message
 * @param[in] msg the wrnc message
 * @param[out] hdr the header from the message
 */
static inline void wrnc_message_header_get(struct wrnc_msg *msg,
					   struct wrnc_proto_header *hdr)
{
	uint32_t *hdr32 = (uint32_t *) hdr;

	hdr32[0] = be32toh((msg->data[0] & 0x0000FFFF) |
			   ((msg->data[0] >> 8) & 0x00FF0000) |
			   ((msg->data[0] << 8) & 0xFF000000));
	hdr32[1] = msg->data[1];
	hdr32[2] = be32toh(msg->data[2]);
	hdr32[3] = msg->data[3];
}


/**
 * It packs a message to send to the HMQ. The function uses the header to get
 * the payload size. Rembember that the payload length unit is the 32bit word.
 * Remind also that the WRNC VHDL code, will convert a given 32bit word between
 * little endian and big endian
 * @param[out] msg raw message
 * @param[in] hdr message header
 * @param[in] payload data
 */
static inline void wrnc_message_pack(struct wrnc_msg *msg,
				     struct wrnc_proto_header *hdr,
				     void *payload)
{
	void *data = msg->data;

	wrnc_message_header_set(msg, hdr);
	if (!payload)
		return;
	memcpy(data + sizeof(struct wrnc_proto_header), payload,
	       hdr->len * 4);
	msg->datalen += hdr->len;
	msg->max_size = msg->datalen;
}


/**
 * it unpacks a message coming from the HMQ by separating the header from
 * the payload. You will find the payload size in the header.
 * Rembember that the payload length unit is the 32bit word.
 * Remind also that the WRNC VHDL code, will convert a given 32bit word between
 * little endian and big endian
 * @param[in] msg raw message
 * @param[out] hdr message header
 * @param[out] payload data
 */
static inline void wrnc_message_unpack(struct wrnc_msg *msg,
				       struct wrnc_proto_header *hdr,
				       void *payload)
{
	void *data = msg->data;

	wrnc_message_header_get(msg, hdr);
	if (!payload)
		return;
	memcpy(payload, data + sizeof(struct wrnc_proto_header),
	       hdr->len * 4);
}

struct wrnc_structure_tlv {
	uint32_t index;
	void *structure;
	size_t size;
};

static inline void wrnc_message_structure_push(struct wrnc_msg *msg,
					       struct wrnc_proto_header *hdr,
					       struct wrnc_structure_tlv *tlv)
{
	unsigned int offset = sizeof(struct wrnc_proto_header) / 4 + hdr->len;
	void *data;

	msg->data[offset++] = tlv->index;
	msg->data[offset++] = tlv->size;
	data = &msg->data[offset];

	memcpy(data, tlv->structure, tlv->size);

	hdr->len += 2 + (tlv->size / 4);

	wrnc_message_header_set(msg, hdr);
	msg->datalen = hdr->len + sizeof(struct wrnc_proto_header) / 4;
}

/**
 * A TLV record containing a structure will be take from the message head.
 * The function will update the message lenght in the header by removing
 * the size occupied by the last record.
 * @param[in] msg raw message
 * @param[in] hdr message header
 * @param[out] tlv TLV record containing a structure
 */
static inline void wrnc_message_structure_pop(struct wrnc_msg *msg,
					      struct wrnc_proto_header *hdr,
					      struct wrnc_structure_tlv *tlv)
{
	unsigned int offset = sizeof(struct wrnc_proto_header) / 4;
	void *data;

	wrnc_message_header_get(msg, hdr);

	if (hdr->len < 3)
		return; /* there is nothing to read */

	tlv->index = msg->data[offset++];
	tlv->size = msg->data[offset++];
	if (tlv->size / 4 > hdr->len - 2)
		return; /* TLV greater than what is declared in header */

	data = &msg->data[offset];
	memcpy(tlv->structure, data, tlv->size);
	hdr->len = hdr->len - (tlv->size / 4) - 2; /* -2 because of index
						      and size in TLV */

	/* shift all data - +8 because of index and size which are uint32_t */
	memcpy(data, data + tlv->size + 8, hdr->len * 4);
}

extern int wrnc_rt_variable_set(struct wrnc_dev *wrnc,
				unsigned int hmq_in, unsigned int hmq_out,
				uint32_t *variables,
				unsigned int n_variables,
				unsigned int sync);
extern int wrnc_rt_variable_get(struct wrnc_dev *wrnc,
				unsigned int hmq_in, unsigned int hmq_out,
				uint32_t *variables,
				unsigned int n_variables);
extern int wrnc_rt_structure_set(struct wrnc_dev *wrnc,
				 unsigned int hmq_in, unsigned int hmq_out,
				 struct wrnc_structure_tlv *tlv,
				 unsigned int n_tlv,
				 unsigned int sync);
extern int wrnc_rt_structure_get(struct wrnc_dev *wrnc,
				 unsigned int hmq_in, unsigned int hmq_out,
				 struct wrnc_structure_tlv *tlv,
				 unsigned int n_tlv);
#ifdef __cplusplus
};
#endif

#endif
