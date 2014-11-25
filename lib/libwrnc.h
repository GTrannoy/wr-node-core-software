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

#define WRNC_NAME_LEN 12

#define WRNC_SYSFS_PATH_LEN 128
#define WRNC_SYSFS_READ_LEN 32
#define WRNC_DEVICE_PATH_LEN 64


/**
 * Error codes for white-rabbit node-core applications
 */
enum wrnc_error_number {
	EWRNC_INVAL_PARSE = 83630, /**< cannot parse data from sysfs */
	EWRNC_INVAL_SLOT, /**< invalid slot */
	EWRNC_NO_IMPLEMENTATION, /**< a prototype is not implemented */
	__EWRNC_MAX,
};

enum wrnc_msg_filter_operation_type {
	WRNC_MSG_FILTER_OR,
	WRNC_MSG_FILTER_AND,
	WRNC_MSG_FILTER_NOT,
	WRNC_MSG_FILTER_EQ,
};

/**
 * It describe a filter to apply to messages
 */
struct wrnc_msg_filter {
	enum wrnc_msg_filter_operation_type operation; /**< kind of operation to perform */
	unsigned int word_offset; /**< offset of the word to check */
	uint32_t mask; /**< mask to apply before the operation */
	uint32_t value; /**< second operand of the operation */
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

extern struct wrnc_msg *wrnc_slot_receive(struct wrnc_dev *wrnc,
					  unsigned int index);
extern int wrnc_slot_send(struct wrnc_dev *wrnc, unsigned int index,
			  struct wrnc_msg *msg);
extern int wrnc_slot_send_and_receive_sync(struct wrnc_dev *wrnc,
					   unsigned int index_in,
					   unsigned int index_out,
					   struct wrnc_msg *msg,
					   unsigned int timeout_ms);
extern int wrnc_slot_poll(struct wrnc_dev *wrnc, struct pollfd *p, nfds_t nfds,
			  int timeout);
extern int wrnc_slot_fd_get(struct wrnc_dev *wrnc, unsigned int is_input,
			    unsigned int index);
extern int wrnc_smem_read(struct wrnc_dev *wrnc, uint32_t addr, uint32_t *data,
			  size_t count, enum wrnc_smem_modifier mod);
extern int wrnc_smem_write(struct wrnc_dev *wrnc, uint32_t addr, uint32_t *data,
			   size_t count, enum wrnc_smem_modifier mod);
extern int wrnc_bind(struct wrnc_dev *wrnc, struct wrnc_msg_filter *flt,
		     unsigned int length);

#ifdef __cplusplus
};
#endif

#endif
