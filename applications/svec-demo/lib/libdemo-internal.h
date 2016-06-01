/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */


#ifndef __LIBDEMO_INTERNAL__H__
#define __LIBDEMO_INTERNAL__H__

#include <stdlib.h>
#include <libdemo.h>


/**
 * Description of a White-Rabbit Trigger-Distribution device
 */
struct demo_desc {
	struct trtl_dev *trtl; /**< WRNC device associated */
	uint32_t dev_id; /**< fmc device id */
	uint32_t app_id; /**< Application id */
	uint32_t n_cpu; /**< Number of CPUs */
};

#endif
