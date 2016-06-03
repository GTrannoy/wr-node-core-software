
/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */

#ifndef __LIBTRTL_INTERNAL_H__
#define __LIBTRTL_INTERNAL_H__
#include "libmockturtle.h"

/**
 * Internal descriptor for a WRNC device
 */
struct trtl_desc {
	char name[TRTL_NAME_LEN]; /**< Name of the device */
	char path[TRTL_PATH_LEN]; /**< path to device */
	int fd_dev; /**< File Descriptor of the device */
	int fd_cpu[TRTL_MAX_CPU];  /**< File Descriptor of the CPUs */

};

#endif
