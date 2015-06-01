/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */


#ifndef __LIBWRTD_INTERNAL__H__
#define __LIBWRTD_INTERNAL__H__

#include <stdlib.h>
#include <errno.h>
#include <libwrtd.h>

/* FIXME
 * Statically defined but we must find a dynamic way to determinate
 * these offsets
 */
#define WRTD_TDC_DEV_ID_OFFSET 0
#define WRTD_FD_DEV_ID_OFFSET 1

/**
 * Description of a White-Rabbit Trigger-Distribution device
 */
struct wrtd_desc {
	struct wrnc_dev *wrnc; /**< WRNC device associated */
	uint32_t dev_id; /**< fmc device id */
	uint32_t app_id; /**< Application id */
	uint32_t n_cpu; /**< Number of CPUs */
};

/**
 * @file libwrtd-interal.c
 */
void unbag_ts(uint32_t *buf, int offset, struct wr_timestamp *ts);
int wrtd_validate_acknowledge(struct wrnc_msg *msg);
int wrtd_log_read(struct wrnc_hmq *hmq_log, struct wrtd_log_entry *log,
		  int count);
int wrtd_trig_id_cmp(struct wrtd_trig_id *id1, struct wrtd_trig_id *id2);
extern int wrtd_trivial_request(struct wrtd_node *dev,
				struct wrnc_msg *request_msg,
				enum wrtd_core core);
extern int wrtd_send_and_receive_sync(struct wrtd_desc *wrtd,
				      struct wrnc_msg *msg,
				      enum wrtd_core core);
#endif
