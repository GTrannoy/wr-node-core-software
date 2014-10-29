/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */

#include "libwrtd.h"

struct wrtd_desc {
	struct wrnc_dev *wrnc;

	uint32_t app_id;
	uint32_t n_cpu;
};
