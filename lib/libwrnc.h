/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *         inspired by a draft of Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * License: GPL v3
 */

#ifndef __LIB_WRNC_H__
#define __LIB_WRNC_H__

#include <stdint.h>
#include <stdio.h>
#include <wrnc-user.h>

struct wrnc_dev;

#define WRNC_NAME_LEN 12
#define WRNC_MAX_CARRIERS 20
#define WRNC_MAX_CPU 8
#define WRNC_MAX_HMQ_SLOT 16

#define WRNC_SYSFS_PATH_LEN 128
#define WRNC_SYSFS_READ_LEN 32
#define WRNC_DEVICE_PATH_LEN 64

enum wrnc_error_number {
	EWRNC_INVAL_PARSE,
	__EWRNC_MAX,
};

enum wrnc_msg_filter_operation_type {
	WRNC_MSG_FILTER_OR,
	WRNC_MSG_FILTER_AND,
	WRNC_MSG_FILTER_NOT,
	WRNC_MSG_FILTER_EQ,
};

/**
 * It describe a filter to apply to messages
 */
struct wrnc_msg_filter {
	enum wrnc_msg_filter_operation_type operation; /**< kind of operation to perform */
	unsigned int word_offset; /**< offset of the word to check */
	uint32_t mask; /**< mask to apply before the operation */
	uint32_t value; /**< second operand of the operation */
};


extern char *wrnc_strerror(int err);

/**
 * It initializes the WRNC library
 */
extern int wrnc_init();
/**
 * It releases the resources used by this library
 */
extern void wrnc_exit();

/**
 * It returns the number of available WRNC
 * @return the number of WRNC available, 0 is also an error check errno to be sure
 */
extern uint32_t wrnc_count();

/**
 * It returns the list of available WRNC devices
 * @return a list of WRNC devices
 */
extern char (*wrnc_list())[WRNC_NAME_LEN];

/**
 * It opens a wrnc device using string descriptor
 * @param[in] device description of the device to open
 */
extern struct wrnc_dev *wrnc_open(const char *device);

/**
 * It opens a wrnc device using its FMC device_id
 * @param[in] device_id FMC device id of the device to use
 */
static struct wrnc_dev *wrnc_open_by_fmc(uint32_t device_id)
{
	char name[12];

	snprintf(name, 12, "wrnc-%04x", device_id);
	return wrnc_open(name);
}

/**
 * It opens a wrnc device using its Logical Unit Number
 * @param[in] lun Logical Unit Number of the device to use
 */
static struct wrnc_dev *wrnc_open_by_lun(unsigned int lun)
{
	char name[12];
	uint32_t dev_id;

	snprintf(name, 12, "wrnc.%d", lun);
	/*TODO convert to FMC */
	return wrnc_open_by_fmc(dev_id);
}

/**
 * It closes an opened wrnc device
 * @param[in] wrnc device to close
 */
extern void wrnc_close(struct wrnc_dev *wrnc);

/**
 * It returns the application identifier of the FPGA bitstream in use
 * @param[in] wrnc device that we are insterested in
 * @param[out] app_id application identifier
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
extern int wrnc_app_id_get(struct wrnc_dev *wrnc, uint32_t *app_id);

/**
 * It loads a wrnc CPU firmware from a given buffer
 * @param[in] wrnc device to program
 * @param[in] index CPU index to program
 * @param[in] code buffer to write in the CPU program memory
 * @param[in] length buffer length
 * @return the number of written byte, on error -1 and errno is
 *         set appropriately
 */
extern int wrnc_cpu_load_application_raw(struct wrnc_dev *wrnc,
						 unsigned int index,
						 void *code, size_t length,
						 unsigned int offset);
/**
 * It loads a wrnc CPU firmware from a given file
 * @param[in] wrnc device to program
 * @param[in] index CPU index to program
 * @param[in] path path to the file to write into CPU memory
 */
extern int wrnc_cpu_load_application_file(struct wrnc_dev *wrnc,
					  unsigned int index,
					  char *path);

/**
 * It dumps a wrnc CPU firmware into a given buffer
 * @param[in] wrnc device to program
 * @param[in] index CPU index to program
 * @param[in] code buffer to write in the CPU program memory
 * @param[in] length buffer length
 * @return the number of written byte, on error -1 and errno is
 *         set appropriately
 */
extern int wrnc_cpu_dump_application_raw(struct wrnc_dev *wrnc,
						 unsigned int index,
						 void *code, size_t length,
						 unsigned int offset);
/**
 * It dumps a wrnc CPU firmware into a given file
 * @param[in] wrnc device to program
 * @param[in] index CPU index to program
 * @param[in] path path to the file to write
 */
extern int wrnc_cpu_dump_application_file(struct wrnc_dev *wrnc,
					  unsigned int index,
					  char *path);

/**
 * It returns the number of available CPU on the FPGA bitstream
 * @param[in] wrnc device to use
 * @param[out] n_cpu the number of available CPUs
 * @return 0 on success; -1 on error and errno is set appropriately
 */
extern int wrnc_cpu_count(struct wrnc_dev *wrnc, uint32_t *n_cpu);

/**
 * Assert or de-assert the reset line of the WRNC CPUs
 * @param[in] wrnc device to use
 * @param[in] mask bit mask of the line to reset
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
extern int wrnc_cpu_reset_set(struct wrnc_dev *wrnc, uint32_t mask);

/**
 * It returns the current status of the WRNC CPUs' reset line
 * @param[in] wrnc device to use
 * @param[out] mask the current reset line status
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
extern int wrnc_cpu_reset_get(struct wrnc_dev *wrnc, uint32_t *mask);

/**
 * Assert or de-assert the enable (a.k.a. running) line of the WRNC CPUs
 * @param[in] wrnc device to use
 * @param[in] mask bit mask of the line to enable
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
extern int wrnc_cpu_run_set(struct wrnc_dev *wrnc, uint32_t mask);

/**
 * It returns the current status of the WRNC CPUs' enable line
 * @param[in] wrnc device to use
 * @param[out] mask the current running line status
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
extern int wrnc_cpu_run_get(struct wrnc_dev *wrnc, uint32_t *mask);

/**
 * It enables a CPU; in other words, it clear the reset line of a CPU
 * @param[in] wrnc device to use
 * @param[in] index CPU to enable
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
static int wrnc_cpu_enable(struct wrnc_dev *wrnc, unsigned int index)
{
	uint32_t tmp;

	wrnc_cpu_reset_get(wrnc, &tmp);
	return wrnc_cpu_reset_set(wrnc, tmp | (1 << index));
}

/**
 * It disables a CPU; in other words, it sets the reset line of a CPU
 * @param[in] wrnc device to use
 * @param[in] index CPU to enable
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
static int wrnc_cpu_disable(struct wrnc_dev *wrnc, unsigned int index)
{
	uint32_t tmp;

	wrnc_cpu_reset_get(wrnc, &tmp);
	return wrnc_cpu_reset_set(wrnc, tmp & ~(1 << index));
}

/**
 * It starts to execute CPU code
 * @param[in] wrnc device to use
 * @param[in] index cpu to start
 */
static int wrnc_cpu_start(struct wrnc_dev *wrnc, unsigned int index)
{
	uint32_t tmp;

	wrnc_cpu_run_get(wrnc, &tmp);
	return wrnc_cpu_run_set(wrnc, tmp & ~(1 << index));
}

/**
 * It stops to execute CPU code
 * @param[in] wrnc device to use
 * @param[in] index cpu to stop
 */
static int wrnc_cpu_stop(struct wrnc_dev *wrnc, unsigned int index)
{
	uint32_t tmp;

	wrnc_cpu_run_get(wrnc, &tmp);
	return wrnc_cpu_run_set(wrnc, tmp | (1 << index));
}


extern struct wrnc_msg *wrnc_slot_receive(struct wrnc_dev *wrnc,
					  unsigned int index);
extern int wrnc_slot_send(struct wrnc_dev *wrnc, unsigned int index,
			  struct wrnc_msg *msg);

/**
 * It sends a synchronous message. The slots are uni-directional, so you must
 * specify where write the message and where the answer is expected.
 * @param[in] wrnc device to use
 * @param[in] index_in index of the input slot
 * @param[in] index_out index of the output slot
 * @param[in,out] msg it contains the message to be sent; the answer will
 *                overwrite the content
 * @param[in] timeout_ms ms to wait for an answer
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrnc_slot_send_and_receive_sync(struct wrnc_dev *wrnc,
					   unsigned int index_in,
					   unsigned int index_out,
					   struct wrnc_msg *msg,
					   unsigned int timeout_ms);
/**
 * It is a wrapper of poll(2) for a wrnc slot device.
 * @param[in] wrnc device to use
 * @param[in] index slot to poll
 * @param[out] revent returned events by poll
 * @param[in] timeout timeout in ms
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
extern int wrnc_slot_poll(struct wrnc_dev *wrnc, unsigned int index,
			  short *revent, int timeout);

/**
 * It reads from the shared memory of the WRNC
 * @param[in] wrnc device to use
 * @param[in] addr memory address
 * @param[out] data value in the shared memory
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
extern int wrnc_smem_read(struct wrnc_dev *wrnc, uint32_t addr, uint32_t *data);

/**
 * It writes on the shared memory of the WRNC
 * @param[in] wrnc device to use
 * @param[in] addr memory address
 * @param[out] data value to write in the shared memory
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
extern int wrnc_smem_write(struct wrnc_dev *wrnc, uint32_t addr, uint32_t data);

/**
 * It binds a slot to manage only messages that comply with the given filter
 * @param[in] wrnc device to use
 * @param[in] flt filters to apply
 * @param[in] length number of filters
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
extern int wrnc_bind(struct wrnc_dev *wrnc, struct wrnc_msg_filter *flt,
		     unsigned int length);

#endif
