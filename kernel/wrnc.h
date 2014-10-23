/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v2
 */

#ifndef __WRNC_H__
#define __WRNC_H__

#include "hw/mqueue.h"

#define WRNC_MAX_CPU 8
#define WRNC_MAX_HMQ_SLOT MAX_MQUEUE_SLOTS
#define WRNC_MAX_CARRIER 20
#define WRNC_MAX_CPU_MINORS (WRNC_MAX_CPU * WRNC_MAX_CARRIER)
#define WRNC_MAX_HMQ_MINORS (WRNC_MAX_HMQ_SLOT * WRNC_MAX_CARRIER)
#define WRNC_MAX_MINORS (WRNC_MAX_CARRIER + WRNC_MAX_CPU_MINORS + WRNC_MAX_HMQ_MINORS)
#define WRNC_MAX_PAYLOAD_SIZE 128

#define to_wrnc_cpu(_dev) (container_of(_dev, struct wrnc_cpu, dev))
#define to_wrnc_dev(_dev) (container_of(_dev, struct wrnc_dev, dev))
#define to_fmc_dev(_wrnc) (container_of(_wrnc->dev.parent, struct fmc_device, dev))
#define to_wrnc_hmq(_dev) (container_of(_dev, struct wrnc_hmq, dev))

#define WRNC_FLAG_HMQ_DIR (1 << 0) /**< 1 CPU input, 0 CPU output */
#define WRNC_FLAG_HMQ_SHR (1 << 1) /**< 1 shared, means that more than
				      1 CPU is using it */

enum wrnc_dev_type {
	WRNC_DEV,
	WRNC_CPU,
	WRNC_HMQ,
};

struct wrnc_msg {
	uint32_t datalen;
	uint32_t data[WRNC_MAX_PAYLOAD_SIZE];
};

struct wrnc_msg_element {
	struct list_head list;
	struct wrnc_msg *msg;
};

struct wrnc_hmq {
	struct device dev;
	int index; /**< instance number */
	unsigned long flags; /**< describe the status of the hmq slot from
			      the driver point of view */

	uint32_t status; /**< describe the status of the mhq slot from the
			  cpu point of view */
	uint32_t base_sr; /**< base address of the slot register */
	struct list_head list_msg; /**< list of messages */
	struct spinlock lock;
	struct mutex mtx;
	wait_queue_head_t q_msg; /**< wait queue for synchronous messages */
};


/**
 * It describes a single instance of a CPU of the WRNC
 */
struct wrnc_cpu {
	struct device dev; /**< device representing a single CPU */
	int index; /**< instance number */

	struct wrnc_hmq *hmq[WRNC_MAX_HMQ_SLOT]; /**< list of HMQ slots used by
						    this CPU */
};

struct wrnc_dev {
	unsigned int app_id; /**< Application ID of the FPGA bitstream */
	struct device dev;

	unsigned int n_cpu; /**< number of CPU in the FPGA bitstream */
	struct wrnc_cpu cpu[WRNC_MAX_CPU];

	unsigned int n_hmq; /**< number of Slots in the HMQ */
	struct wrnc_hmq hmq[WRNC_MAX_HMQ_SLOT];
	uint32_t base_core; /**< base address of the WRNC component */
	uint32_t base_csr; /**< base address of the Shared Control Register */
	uint32_t base_hmq; /**< base address of the HMQ */
	uint32_t base_gcr; /**< base address of the Global Control Register
			      for the HMQ */
};


enum ual_ioctl_commands {
        WRNC_MSG_SYNC, /**< send a synchronous message */
};

#define WRNC_IOCTL_MAGIC 's'
#define WRNC_IOCTL_MSG_SYNC _IOWR(WRNC_IOCTL_MAGIC, WRNC_MSG_SYNC, struct wrnc_msg)


#endif
