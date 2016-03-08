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
	EWRNC_HMQ_READ, /**< Error while reading messages */
	__EWRNC_MAX,
};


/**
 * TLV structure used to embed structures within a message
 */
struct wrnc_structure_tlv {
	uint32_t index; /**< structure index (type) */
	void *structure; /**< pointer to the structure */
	size_t size; /**< structure size in byte */
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
extern int wrnc_cpu_is_enable(struct wrnc_dev *wrnc, unsigned int index,
			      unsigned int *enable);
extern int wrnc_cpu_start(struct wrnc_dev *wrnc, unsigned int index);
extern int wrnc_cpu_stop(struct wrnc_dev *wrnc, unsigned int index);
extern int wrnc_cpu_is_running(struct wrnc_dev *wrnc, unsigned int index,
			       unsigned int *run);
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
extern int wrnc_hmq_receive_n(struct wrnc_hmq *hmq,
			      struct wrnc_msg *msg, unsigned int n);
extern struct wrnc_msg *wrnc_hmq_receive(struct wrnc_hmq *hmq);
extern int wrnc_hmq_send(struct wrnc_hmq *hmq, struct wrnc_msg *msg);
extern int wrnc_hmq_send_and_receive_sync(struct wrnc_hmq *hmq,
					   unsigned int index_out,
					   struct wrnc_msg *msg,
					   unsigned int timeout_ms);
extern int wrnc_hmq_buffer_size_set(struct wrnc_hmq *hmq, uint32_t size);
extern int wrnc_hmq_buffer_size_get(struct wrnc_hmq *hmq, uint32_t *size);
extern int wrnc_hmq_count_max_hw_get(struct wrnc_hmq *hmq, uint32_t *max);
extern int wrnc_hmq_width_get(struct wrnc_hmq *hmq, uint32_t *width);
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
 * @defgroup proto Protocol management
 * Set of utilities to properly handle the protocol
 * @{
 */
extern void wrnc_message_header_set(struct wrnc_msg *msg,
				    struct wrnc_proto_header *hdr);
extern void wrnc_message_header_get(struct wrnc_msg *msg,
				    struct wrnc_proto_header *hdr);
extern void wrnc_message_pack(struct wrnc_msg *msg,
			      struct wrnc_proto_header *hdr,
			      void *payload);
extern void wrnc_message_unpack(struct wrnc_msg *msg,
				struct wrnc_proto_header *hdr,
				void *payload);
extern void wrnc_message_structure_push(struct wrnc_msg *msg,
					struct wrnc_proto_header *hdr,
					struct wrnc_structure_tlv *tlv);
extern void wrnc_message_structure_pop(struct wrnc_msg *msg,
				       struct wrnc_proto_header *hdr,
				       struct wrnc_structure_tlv *tlv);
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
extern int wrnc_rt_variable_set(struct wrnc_dev *wrnc,
				struct wrnc_proto_header *hdr,
				uint32_t *var, unsigned int n_var);
extern int wrnc_rt_variable_get(struct wrnc_dev *wrnc,
				struct wrnc_proto_header *hdr,
				uint32_t *var, unsigned int n_var);
extern int wrnc_rt_structure_set(struct wrnc_dev *wrnc,
				 struct wrnc_proto_header *hdr,
				 struct wrnc_structure_tlv *tlv,
				 unsigned int n_tlv);
extern int wrnc_rt_structure_get(struct wrnc_dev *wrnc,
				 struct wrnc_proto_header *hdr,
				 struct wrnc_structure_tlv *tlv,
				 unsigned int n_tlv);
/**@}*/
#ifdef __cplusplus
};
#endif

#endif
