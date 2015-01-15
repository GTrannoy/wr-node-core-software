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
struct wr_timestamp picos_to_ts(uint64_t p);
int wrtd_validate_acknowledge(struct wrnc_msg *msg);

#endif
