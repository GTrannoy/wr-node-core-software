/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *         inspired by a draft of Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * License: GPL v3
 */

#ifndef __LIB_WRNC_H__
#define __LIB_WRNC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <poll.h>
#include <wrnc-user.h>

struct wrnc_dev;

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

#define WRNC_SYSFS_PATH_LEN 128
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
	__EWRNC_MAX,
};


/**
 * @file libwrnc.c
 */
extern char *wrnc_strerror(int err);
extern int wrnc_init();
extern void wrnc_exit();
extern uint32_t wrnc_count();
extern char (*wrnc_list())[WRNC_NAME_LEN];

extern struct wrnc_dev *wrnc_open(const char *device);
extern struct wrnc_dev *wrnc_open_by_fmc(uint32_t device_id);
extern struct wrnc_dev *wrnc_open_by_lun(unsigned int lun);
extern void wrnc_close(struct wrnc_dev *wrnc);
extern char *wrnc_name_get(struct wrnc_dev *wrnc);
extern int wrnc_app_id_get(struct wrnc_dev *wrnc, uint32_t *app_id);

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

extern struct wrnc_hmq *wrnc_hmq_open(struct wrnc_dev *wrnc,
				      unsigned int index,
				      unsigned long flags);
extern void wrnc_hmq_close(struct wrnc_hmq *hmq);
extern struct wrnc_msg *wrnc_hmq_receive(struct wrnc_hmq *hmq);
extern int wrnc_hmq_send(struct wrnc_hmq *hmq, struct wrnc_msg *msg);
extern int wrnc_hmq_send_and_receive_sync(struct wrnc_hmq *hmq,
					   unsigned int index_out,
					   struct wrnc_msg *msg,
					   unsigned int timeout_ms);
extern int wrnc_smem_read(struct wrnc_dev *wrnc, uint32_t addr, uint32_t *data,
			  size_t count, enum wrnc_smem_modifier mod);
extern int wrnc_smem_write(struct wrnc_dev *wrnc, uint32_t addr, uint32_t *data,
			   size_t count, enum wrnc_smem_modifier mod);
extern int wrnc_bind(struct wrnc_dev *wrnc, struct wrnc_msg_filter *flt,
		     unsigned int length);

extern struct wrnc_dbg *wrnc_debug_open(struct wrnc_dev *wrnc,
					unsigned int index);
extern void wrnc_debug_close(struct wrnc_dbg *dbg);
extern int wrnc_debug_message_get(struct wrnc_dbg *dbg,
				  char *buf, size_t count);

#ifdef __cplusplus
};
#endif

#endif
