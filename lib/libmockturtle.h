/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *         inspired by a draft of Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * License: GPL v3
 */

#ifndef __LIB_TRTL_H__
#define __LIB_TRTL_H__
/** @file libmockturtle.h */

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <endian.h>
#include <stdint.h>
#include <stdio.h>
#include <poll.h>
#include "mockturtle-common.h"
#include "mockturtle.h"

extern const unsigned int trtl_default_timeout_ms;

struct trtl_dev;

#define TRTL_SYSFS_PATH_LEN 128

/**
 * Debug descriptor. It is not obfuscated because it is meant for debugging
 * purpose. This way, it leaves the user all the freedom to read/poll the
 * debug channel as (s)he wants.
 */
struct trtl_dbg {
	struct trtl_dev *trtl; /**< token of the device */
	unsigned int cpu_index; /**< CPU where read debug messages */
	int fd; /**< file descriptor of the debug interface */
};


/**
 * HMQ slot descriptor
 */
struct trtl_hmq {
	struct trtl_dev *trtl; /**< where this slot belong to */
	char syspath[TRTL_SYSFS_PATH_LEN];
	unsigned int index; /* index of the slot. Note that we have
			       different kind of slot and each kind start
			       counting from 0*/
	unsigned long flags; /**< flags associated to the slot */
	int fd; /**< file descriptor */
};

#define TRTL_FMC_OFFSET 2 /* FIXME this is an hack because fmc-bus does not allow
			   us a dynamic allocation of fake fmc devices */

#define TRTL_NAME_LEN 16
#define TRTL_PATH_LEN 32

#define TRTL_SYSFS_READ_LEN 32
#define TRTL_DEVICE_PATH_LEN 64


#define TRTL_HMQ_INCOMING	(1 << 0)
#define TRTL_HMQ_OUTCOMING	0x0
#define TRTL_HMQ_EXCLUSIVE	(1 << 1)
#define TRTL_HMQ_SHARED		0x0


/**
 * Error codes for white-rabbit node-core applications
 */
enum trtl_error_number {
	ETRTL_INVAL_PARSE = 83630, /**< cannot parse data from sysfs */
	ETRTL_INVAL_SLOT, /**< invalid slot */
	ETRTL_NO_IMPLEMENTATION, /**< a prototype is not implemented */
	ETRTL_HMQ_CLOSE, /**< The HMQ is closed */
	ETRTL_INVALID_MESSAGE, /**< Invalid message */
	ETRTL_HMQ_READ, /**< Error while reading messages */
	__ETRTL_MAX,
};


/**
 * TLV structure used to embed structures within a message
 */
struct trtl_structure_tlv {
	uint32_t index; /**< structure index (type) */
	void *structure; /**< pointer to the structure */
	size_t size; /**< structure size in byte */
};

/**
 * @file libmockturtle.c
 */
/**
 * @defgroup dev Device
 * Set of functions to perform basic openartion on the device
 * @{
 */
extern char *trtl_strerror(int err);
extern int trtl_init();
extern void trtl_exit();
extern uint32_t trtl_count();
extern char **trtl_list();
extern void trtl_list_free(char **list);

extern struct trtl_dev *trtl_open(const char *device);
extern struct trtl_dev *trtl_open_by_fmc(uint32_t device_id);
extern struct trtl_dev *trtl_open_by_lun(unsigned int lun);
extern void trtl_close(struct trtl_dev *trtl);
extern char *trtl_name_get(struct trtl_dev *trtl);
extern int trtl_app_id_get(struct trtl_dev *trtl, uint32_t *app_id);
/**@}*/

/**
 * @defgroup cpu CPU
 * Set of function that allow you to manage the FPGA cores
 * @{
 */
extern int trtl_cpu_load_application_raw(struct trtl_dev *trtl,
					 unsigned int index,
					 void *code, size_t length,
					 unsigned int offset);
extern int trtl_cpu_load_application_file(struct trtl_dev *trtl,
					  unsigned int index,
					  char *path);
extern int trtl_cpu_dump_application_raw(struct trtl_dev *trtl,
					 unsigned int index,
					 void *code, size_t length,
					 unsigned int offset);
extern int trtl_cpu_dump_application_file(struct trtl_dev *trtl,
					  unsigned int index,
					  char *path);

extern int trtl_cpu_count(struct trtl_dev *trtl, uint32_t *n_cpu);
extern int trtl_cpu_reset_set(struct trtl_dev *trtl, uint32_t mask);
extern int trtl_cpu_reset_get(struct trtl_dev *trtl, uint32_t *mask);
extern int trtl_cpu_run_set(struct trtl_dev *trtl, uint32_t mask);
extern int trtl_cpu_run_get(struct trtl_dev *trtl, uint32_t *mask);
extern int trtl_cpu_enable(struct trtl_dev *trtl, unsigned int index);
extern int trtl_cpu_disable(struct trtl_dev *trtl, unsigned int index);
extern int trtl_cpu_is_enable(struct trtl_dev *trtl, unsigned int index,
			      unsigned int *enable);
extern int trtl_cpu_start(struct trtl_dev *trtl, unsigned int index);
extern int trtl_cpu_stop(struct trtl_dev *trtl, unsigned int index);
extern int trtl_cpu_is_running(struct trtl_dev *trtl, unsigned int index,
			       unsigned int *run);
/**@}*/

/**
 * @defgroup hmq Host Message Queue
 * Functions to manage the HMQ slots: configuration and transmission
 * @{
 */
extern struct trtl_hmq *trtl_hmq_open(struct trtl_dev *trtl,
				      unsigned int index,
				      unsigned long flags);
extern void trtl_hmq_close(struct trtl_hmq *hmq);
extern int trtl_hmq_share_set(struct trtl_dev *trtl, unsigned int dir,
			      unsigned int index, unsigned int status);
extern int trtl_hmq_share_get(struct trtl_dev *trtl, unsigned int dir,
			      unsigned int index, unsigned int *status);
extern int trtl_hmq_receive_n(struct trtl_hmq *hmq,
			      struct trtl_msg *msg, unsigned int n);
extern struct trtl_msg *trtl_hmq_receive(struct trtl_hmq *hmq);
extern int trtl_hmq_send(struct trtl_hmq *hmq, struct trtl_msg *msg);
extern int trtl_hmq_send_and_receive_sync(struct trtl_hmq *hmq,
					   unsigned int index_out,
					   struct trtl_msg *msg,
					   unsigned int timeout_ms);
extern int trtl_hmq_buffer_size_set(struct trtl_hmq *hmq, uint32_t size);
extern int trtl_hmq_buffer_size_get(struct trtl_hmq *hmq, uint32_t *size);
extern int trtl_hmq_count_max_hw_get(struct trtl_hmq *hmq, uint32_t *max);
extern int trtl_hmq_width_get(struct trtl_hmq *hmq, uint32_t *width);
extern int trtl_hmq_msg_max_get(struct trtl_hmq *hmq, uint32_t *max);
/* FIXME to be tested */
extern int trtl_hmq_filter_add(struct trtl_hmq *hmq,
			       struct trtl_msg_filter *filter);
/* FIXME to be tested */
extern int trtl_hmq_filter_clean(struct trtl_hmq *hmq);
extern int trtl_bind(struct trtl_dev *trtl, struct trtl_msg_filter *flt,
		     unsigned int length);
/**@}*/

/**
 * @defgroup smem Shared Memory
 * Functions to access the shared memory from the host
 * @{
 */
extern int trtl_smem_read(struct trtl_dev *trtl, uint32_t addr, uint32_t *data,
			  size_t count, enum trtl_smem_modifier mod);
extern int trtl_smem_write(struct trtl_dev *trtl, uint32_t addr, uint32_t *data,
			   size_t count, enum trtl_smem_modifier mod);
/**@}*/

/**
 * @defgroup dbg Debug
 * Functions to access the debug serial stream
 * @{
 */
extern struct trtl_dbg *trtl_debug_open(struct trtl_dev *trtl,
					unsigned int index);
extern void trtl_debug_close(struct trtl_dbg *dbg);
extern int trtl_debug_message_get(struct trtl_dbg *dbg,
				  char *buf, size_t count);
/**@}*/


/**
 * @defgroup proto Protocol management
 * Set of utilities to properly handle the protocol
 * @{
 */
extern void trtl_message_header_set(struct trtl_msg *msg,
				    struct trtl_proto_header *hdr);
extern void trtl_message_header_get(struct trtl_msg *msg,
				    struct trtl_proto_header *hdr);
extern void trtl_message_pack(struct trtl_msg *msg,
			      struct trtl_proto_header *hdr,
			      void *payload);
extern void trtl_message_unpack(struct trtl_msg *msg,
				struct trtl_proto_header *hdr,
				void *payload);
extern void trtl_message_structure_push(struct trtl_msg *msg,
					struct trtl_proto_header *hdr,
					struct trtl_structure_tlv *tlv);
extern void trtl_message_structure_pop(struct trtl_msg *msg,
				       struct trtl_proto_header *hdr,
				       struct trtl_structure_tlv *tlv);
/**@}*/

/**
 * @defgroup rtmsg Real Time service messages
 * Message builders for RT service messages
 * @{
 */
extern int trtl_rt_version_get(struct trtl_dev *trtl,
			       struct trtl_rt_version *version,
			       unsigned int hmq_in, unsigned int hmq_out);
extern int trtl_rt_ping(struct trtl_dev *trtl,
			unsigned int hmq_in, unsigned int hmq_out);
extern int trtl_rt_variable_set(struct trtl_dev *trtl,
				struct trtl_proto_header *hdr,
				uint32_t *var, unsigned int n_var);
extern int trtl_rt_variable_get(struct trtl_dev *trtl,
				struct trtl_proto_header *hdr,
				uint32_t *var, unsigned int n_var);
extern int trtl_rt_structure_set(struct trtl_dev *trtl,
				 struct trtl_proto_header *hdr,
				 struct trtl_structure_tlv *tlv,
				 unsigned int n_tlv);
extern int trtl_rt_structure_get(struct trtl_dev *trtl,
				 struct trtl_proto_header *hdr,
				 struct trtl_structure_tlv *tlv,
				 unsigned int n_tlv);
/**@}*/
#ifdef __cplusplus
};
#endif

#endif
