/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v2
 */

#ifndef __WRNC_H__
#define __WRNC_H__

#include "hw/mqueue.h"
#include "wrnc-user.h"

#define WRNC_MAX_CPU 8
#define WRNC_MAX_HMQ_SLOT (MAX_MQUEUE_SLOTS * 2)
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
/**
 * Available type of devices
 */
enum wrnc_dev_type {
	WRNC_DEV, /**< the whole WRNC component */
	WRNC_CPU, /**< CPU core of the WRNC*/
	WRNC_HMQ, /**< HMQ slot ot the WRNC */
};

/**
 * Message structure for the driver
 */
struct wrnc_msg_element {
	struct list_head list; /**< to keep it in our local queue */
	struct wrnc_msg *msg; /**< the real message */
};

/**
 * It describe the status of a HMQ slot
 */
struct wrnc_hmq {
	struct device dev;
	int index; /**< instance number */
	unsigned long flags; /**< describe the status of the HMQ slot from
			      the driver point of view */

	uint32_t status; /**< describe the status of the HMQ slot from the
			  cpu point of view */
	uint32_t base_sr; /**< base address of the slot register */
	struct list_head list_msg; /**< list of messages to/from th HMQ */
	unsigned int count; /**< number of messages in the list */
	struct spinlock lock; /**< to protect list read/write */
	struct mutex mtx; /**< to protect operations on the HMQ */
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

/**
 * It describes the generic instance of a WRNC
 */
struct wrnc_dev {
	unsigned int app_id; /**< Application ID of the FPGA bitstream */
	struct device dev;

	unsigned int n_cpu; /**< number of CPU in the FPGA bitstream */
	struct wrnc_cpu cpu[WRNC_MAX_CPU]; /**< CPU instances */

	unsigned int n_hmq_in; /**< number of input slots in the HMQ */
	unsigned int n_hmq_out; /**< number of output slots in the HMQ */
	struct wrnc_hmq hmq_in[MAX_MQUEUE_SLOTS]; /**< HMQ input instances */
	struct wrnc_hmq hmq_out[MAX_MQUEUE_SLOTS]; /**< HMQ output instances */
	uint32_t base_core; /**< base address of the WRNC component */
	uint32_t base_csr; /**< base address of the Shared Control Register */
	uint32_t base_hmq; /**< base address of the HMQ */
	uint32_t base_gcr; /**< base address of the Global Control Register
			      for the HMQ */
	uint32_t irq_mask; /**< IRQ mask in use */
};

#endif
