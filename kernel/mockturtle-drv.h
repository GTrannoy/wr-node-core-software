/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v2
 */

#ifndef __TRTL_H__
#define __TRTL_H__

#include <linux/circ_buf.h>
#include "hw/mockturtle_queue.h"
#include "mockturtle.h"

#define TRTL_MAX_CPU_MINORS (TRTL_MAX_CPU * TRTL_MAX_CARRIER)
#define TRTL_MAX_HMQ_MINORS (TRTL_MAX_HMQ_SLOT * TRTL_MAX_CARRIER)
#define TRTL_MAX_MINORS (TRTL_MAX_CARRIER + TRTL_MAX_CPU_MINORS + TRTL_MAX_HMQ_MINORS)

#define TRTL_SMEM_MAX_SIZE 65536

#define to_trtl_cpu(_dev) (container_of(_dev, struct trtl_cpu, dev))
#define to_trtl_dev(_dev) (container_of(_dev, struct trtl_dev, dev))
#define to_fmc_dev(_trtl) (container_of(_trtl->dev.parent, struct fmc_device, dev))
#define to_trtl_hmq(_dev) (container_of(_dev, struct trtl_hmq, dev))

#define TRTL_FLAG_HMQ_DIR (1 << 0) /**< 1 CPU input, 0 CPU output */
#define TRTL_FLAG_HMQ_SHR (1 << 1) /**< 1 shared, means that more than
				      1 CPU is using it */

#define TRTL_FLAG_HMQ_SHR_USR (1 << 2) /**< Shared by users */
#define TRTL_FLAG_HMQ_SYNC_WAIT (1 << 3) /**< wait sync answer */
#define TRTL_FLAG_HMQ_SYNC_READY (1 << 4) /**< sync answer is ready */


static inline uint32_t trtl_get_sequence(struct trtl_msg *msg)
{
	return msg->data[1];
}

struct trtl_msg_filter_element {
	struct trtl_msg_filter filter;
	struct list_head list;
};


/**
 * Available type of devices
 */
enum trtl_dev_type {
	TRTL_DEV, /**< the whole WRNC component */
	TRTL_CPU, /**< CPU core of the WRNC*/
	TRTL_HMQ, /**< HMQ slot ot the WRNC */
};

/**
 * Message structure for the driver
 */
struct trtl_msg_element {
	struct list_head list; /**< to keep it in our local queue */
	struct trtl_msg *msg; /**< the real message */
};


/**
 * Circular buffer implementation for MockTurtle
 */
struct mturtle_hmq_buffer {
	void *mem;
	unsigned int size; /**< buffer size */
	unsigned int ptr_w; /**< circular buffer head */
	unsigned int ptr_r; /**< circular buffer tail - used only when the
			       buffer is used to transferm from host to mturtle.
			       Otherwise use the user->ptr_r */
	unsigned int max_msg_size; /**< maximum message size storable in the
				      buffer. This is a temporary field; once
				      we move to the mturtle protocol we will
				      not need such field FIXME */
};


/**
 * Collection of HMQ statistics
 */
struct trtl_hmq_stats {
	unsigned int count; /**< number of messages passed throught the HMQ */
};

/**
 * It describe the status of a HMQ slot
 */
struct trtl_hmq {
	struct device dev;
	int index; /**< instance number */
	unsigned long flags; /**< describe the status of the HMQ slot from
			      the driver point of view */

	uint32_t status; /**< describe the status of the HMQ slot from the
			  cpu point of view */
	uint32_t base_sr; /**< base address of the slot register */
	struct list_head list_msg_input; /**< list of messages to
					    input slot */
	unsigned int n_input; /**< number of messages in the list */
	struct spinlock lock; /**< to protect list read/write */
	struct mutex mtx; /**< to protect operations on the HMQ */
	struct mutex mtx_sync; /**< to protect sync messages on the HMQ */
	wait_queue_head_t q_msg; /**< wait queue for synchronous messages */

	struct list_head list_usr; /**< list of consumer of the output slot  */
	unsigned int n_user; /**< number of users in the list */


	unsigned int waiting_seq; /**< sequence number to wait */
	struct trtl_msg sync_answer; /**< synchronous answer message */

	unsigned int max_width; /**< maximum words number per single buffer */
	unsigned int max_depth; /**< maximum buffer queue length (HW) */

	struct mturtle_hmq_buffer buf; /**< Circular buffer */

	struct trtl_hmq_stats stats;
};

/**
 * It describes the consumer of the output slot
 */
struct trtl_hmq_user {
	struct list_head list; /**< to keep it in our local queue */
	struct trtl_hmq *hmq; /**< reference to opened HMQ */
	struct spinlock lock; /**< to protect list read/write */

	struct list_head list_filters; /**< list of filters to apply */
	unsigned int n_filters; /**< number of filters */
	struct spinlock lock_filter; /**< to protect filter list read/write */

	unsigned int ptr_r; /**< read pointer for the message circular buffer */
};


/**
 * It describes a single instance of a CPU of the WRNC
 */
struct trtl_cpu {
	int index; /**< instance number */

	struct device dev; /**< device representing a single CPU */
	struct dentry *dbg_msg; /**< debug messages interface */
	struct circ_buf cbuf; /**< debug circular buffer */
	struct spinlock lock;
	struct trtl_hmq *hmq[TRTL_MAX_HMQ_SLOT]; /**< list of HMQ slots used by
						    this CPU */
};

/**
 * It describes the generic instance of a WRNC
 */
struct trtl_dev {
	unsigned int app_id; /**< Application ID of the FPGA bitstream */
	struct device dev;

	unsigned int n_cpu; /**< number of CPU in the FPGA bitstream */
	struct trtl_cpu cpu[TRTL_MAX_CPU]; /**< CPU instances */

	unsigned int n_hmq_in; /**< number of input slots in the HMQ */
	unsigned int n_hmq_out; /**< number of output slots in the HMQ */
	struct trtl_hmq hmq_in[MAX_MQUEUE_SLOTS]; /**< HMQ input instances */
	struct trtl_hmq hmq_out[MAX_MQUEUE_SLOTS]; /**< HMQ output instances */
	uint32_t base_core; /**< base address of the WRNC component */
	uint32_t base_csr; /**< base address of the Shared Control Register */
	uint32_t base_hmq; /**< base address of the HMQ */
	uint32_t base_gcr; /**< base address of the Global Control Register
			      for the HMQ */
	uint32_t base_smem; /**< base address of the Shared Memory */
	uint32_t irq_mask; /**< IRQ mask in use */

	enum trtl_smem_modifier mod; /**< smem operation modifier */

	struct dentry *dbg_dir; /**< root debug directory */

	uint32_t message_sequence; /**< message sequence number */
};

/* Global data */
extern struct device *minors[TRTL_MAX_MINORS];
/* CPU data */
extern const struct file_operations trtl_cpu_dbg_fops;
extern const struct file_operations trtl_cpu_fops;
extern const struct attribute_group *trtl_cpu_groups[];
extern void trtl_cpu_enable_set(struct trtl_dev *trtl, uint8_t mask);
extern void trtl_cpu_reset_set(struct trtl_dev *trtl, uint8_t mask);
extern int dbg_max_msg;
extern irqreturn_t trtl_irq_handler_debug(int irq_core_base, void *arg);
/* HMQ */
extern int hmq_default_buf_size;
extern int hmq_shared;
extern const struct attribute_group *trtl_hmq_groups[];
extern const struct file_operations trtl_hmq_fops;
extern irqreturn_t trtl_irq_handler(int irq_core_base, void *arg);
#endif
