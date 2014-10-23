/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */

#include "libwrnc.h"

struct wrnc_desc {
	char name[WRNC_NAME_LEN];
	int fd_dev;
	int fd_cpu[WRNC_MAX_CPU];
	int fd_hmq_in[WRNC_MAX_HMQ_SLOT];
  	int fd_hmq_out[WRNC_MAX_HMQ_SLOT];
};
