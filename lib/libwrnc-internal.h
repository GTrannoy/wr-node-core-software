/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */

#ifndef __LIBWRNC_INTERNAL_H__
#define __LIBWRNC_INTERNAL_H__
#include "libwrnc.h"

/**
 * Internal descriptor for a WRNC device
 */
struct wrnc_desc {
	char name[WRNC_NAME_LEN]; /**< Name of the device */
	int fd_dev; /**< File Descriptor of the device */
	int fd_cpu[WRNC_MAX_CPU];  /**< File Descriptor of the CPUs */
};

#endif
