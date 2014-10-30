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
#include <libwrtd-internal.h>

struct wrtd_desc {
	struct wrnc_dev *wrnc;

	uint32_t app_id;
	uint32_t n_cpu;
};

/**
 * @file libwrtd-interal.c
 */
void unbag_ts(uint32_t *buf, int offset, struct wr_timestamp *ts);
struct wr_timestamp picos_to_ts(uint64_t p);
int wrtd_validate_acknowledge(struct wrnc_msg *msg);

#endif
