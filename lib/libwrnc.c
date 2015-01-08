/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *         inspired by a draft of Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * License: GPL v3
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>
#include <fcntl.h>
#include <glob.h>

#include "libwrnc-internal.h"

static uint32_t count;
static char wrnc_dev_list[WRNC_MAX_CARRIER + 1][WRNC_NAME_LEN];
static char *wrnc_error_str[] = {
	"Cannot parse data from sysfs attribute",
	"Invalid slot",
	"Operation not yet implemented",
	"The HMQ slot is close",
	NULL,
};


/**
 * It returns a string messages corresponding to a given error code. If
 * it is not a libwrnc error code, it will run strerror(3)
 * @param[in] err error code
 * @return a message error
 */
char *wrnc_strerror(int err)
{
	if (err < EWRNC_INVAL_PARSE || err > __EWRNC_MAX)
		return strerror(err);
	return wrnc_error_str[err - EWRNC_INVAL_PARSE];
}


/**
 * It initializes the WRNC library. It must be called before doing
 * anything else. If you are going to load/unload WRNC devices, then
 * you have to un-load (wrnc_exit()) e reload (wrnc_init()) the library.
 * @return 0 on success, otherwise -1 and errno is appropriately set
 */
int wrnc_init()
{
	glob_t g;
	int err, i;

	err = glob("/dev/wrnc-[A-Fa-f0-9][A-Fa-f0-9][A-Fa-f0-9][A-Fa-f0-9]",
		   GLOB_NOSORT, NULL, &g);
	if (err)
		return -1;

	count = g.gl_pathc;
	for (i = 0; i < count; ++i)
		strncpy(wrnc_dev_list[i], basename(g.gl_pathv[i]),
			WRNC_NAME_LEN);
	for (i = count; i < WRNC_MAX_CARRIER; ++i)
		wrnc_dev_list[i][0] = '\0';

	return 0;
}


/**
 * It releases the resources allocated by wrnc_init(). It must be called when
 * you stop to use this library. Then, you cannot use functions from this
 * library
 */
void wrnc_exit()
{
	int i;

	for (i = 0; i < WRNC_MAX_CARRIER; ++i)
		wrnc_dev_list[i][0] = '\0';
}


/**
 * It returns the number of available WRNCs. This is not calculated on demand.
 * It depends on library initialization.
 * @return the number of WRNC available
 */
uint32_t wrnc_count()
{
	return count;
}


/**
 * It allocates and returns the list of available WRNC devices. The user is
 * in charge to free(3) the allocated memory
 * @return a list of WRNC devices
 */
char (*wrnc_list())[WRNC_NAME_LEN]
{
	char *list;

	list = malloc((WRNC_MAX_CARRIER + 1) * WRNC_NAME_LEN);
	if (!list)
		return NULL;
	memcpy(list, wrnc_dev_list, (WRNC_MAX_CARRIER + 1) * WRNC_NAME_LEN);

	return (char (*)[WRNC_NAME_LEN])list;
}


/**
 * It opens a WRNC device using a string descriptor. The descriptor correspond
 * to the char device name of the white-rabbit node-core.
 * @param[in] device name of the device to open
 * @return the WRNC token, NULL on error and errno is appropriately set
 */
struct wrnc_dev *wrnc_open(const char *device)
{
	struct wrnc_desc *wrnc;
	int i;

	wrnc = malloc(sizeof(struct wrnc_desc));
	if (!wrnc)
		return NULL;

	strncpy(wrnc->name, device, WRNC_NAME_LEN);

	wrnc->fd_dev = -1;
	for (i = 0; i < WRNC_MAX_CPU; ++i)
		wrnc->fd_cpu[i] = -1;
        for (i = 0; i < WRNC_MAX_HMQ_SLOT / 2; ++i)
		wrnc->fd_hmq_in[i] = -1;
        for (i = 0; i < WRNC_MAX_HMQ_SLOT / 2; ++i)
		wrnc->fd_hmq_out[i] = -1;

	return (struct wrnc_dev *)wrnc;
}


/**
 * It opens a WRNC device using its FMC device_id. The white-rabbit node-core
 * driver is based upon the FMC bus infrastructure, so all wrnc devices are
 * identified with their fmc-bus-id.
 * @param[in] device_id FMC device id of the device to use
 * @return the WRNC token, NULL on error and errno is appropriately set
 */
struct wrnc_dev *wrnc_open_by_fmc(uint32_t device_id)
{
	char name[12];

	snprintf(name, 12, "wrnc-%04x", device_id);
	return wrnc_open(name);
}


/**
 * It opens a WRNC device using its Logical Unit Number. The Logical Unit Number
 * is an instance number of a particular hardware. The LUN to use is the carrier
 * one, and not the mezzanine one (if any)
 * @param[in] lun Logical Unit Number of the device to use
 * @return the WRNC token, NULL on error and errno is appropriately set
 */
struct wrnc_dev *wrnc_open_by_lun(unsigned int lun)
{
	char name[12];

	snprintf(name, 12, "wrnc.%d", lun);
	return wrnc_open(name);
}


/**
 * It closes a WRNC device opened with one of the following function:
 * wrnc_open(), wrcn_open_by_lun(), wrnc_open_by_fmc()
 * @param[in] wrnc device token
 */
void wrnc_close(struct wrnc_dev *wrnc)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;
	int i;

	if (wdesc->fd_dev)
		close(wdesc->fd_dev);

	for (i = 0; i < WRNC_MAX_CPU; ++i)
		if (wdesc->fd_cpu[i] > 0)
			close(wdesc->fd_cpu[i]);

	for (i = 0; i < WRNC_MAX_HMQ_SLOT / 2; ++i)
		wrnc_hmq_close(wrnc, i, 1);
	for (i = 0; i < WRNC_MAX_HMQ_SLOT / 2; ++i)
		wrnc_hmq_close(wrnc, i, 0);

	free(wdesc);
}

/**
 * Generic function that reads from a sysfs attribute
 */
static int wrnc_sysfs_read(char *path, void *buf, size_t len)
{
	int fd, i;

	fd = open(path, O_RDONLY);
	if (!fd)
		return -1;
	i = read(fd, buf, len);
	close(fd);

	return i;
}


/**
 * Generic function that writes to a sysfs attribute
 */
static int wrnc_sysfs_write(char *path, void *buf, size_t len)
{
	int fd, i;

	fd = open(path, O_WRONLY);
	if (!fd)
		return -1;
	i = write(fd, buf, len);
	close(fd);

	return i;
}

/**
 * Generic function that parse a string from a sysfs attribute
 */
static int wrnc_sysfs_scanf(char *path, const char *fmt, ...)
{
	char buf[WRNC_SYSFS_READ_LEN];
	va_list args;
	int ret;

	ret = wrnc_sysfs_read(path, buf, WRNC_SYSFS_READ_LEN);
	if (ret < 0)
		return ret;

	va_start(args, fmt);
	ret = vsscanf(buf, fmt, args);
	va_end(args);
	if (ret < 0)
		return ret;
	if (ret != 1) {
		errno = EWRNC_INVAL_PARSE;
		return -1;
	}

	return 0;
}


/**
 * Generic function that build a string to be written in a sysfs attribute
 */
static int wrnc_sysfs_printf(char *path, const char *fmt, ...)
{
	char buf[WRNC_SYSFS_READ_LEN];
	va_list args;
	int ret;

	va_start(args, fmt);
	vsnprintf(buf, WRNC_SYSFS_READ_LEN, fmt, args);
	va_end(args);

        ret = wrnc_sysfs_write(path, buf, WRNC_SYSFS_READ_LEN);
	if (ret == WRNC_SYSFS_READ_LEN)
		return 0;
	return -1;
}


/**
 * It returns the number of available WRNC CPUs on the FPGA bitstream
 * @param[in] wrnc device token
 * @param[out] n_cpu the number of available CPUs
 * @return 0 on success; -1 on error and errno is set appropriately
 */
int wrnc_cpu_count(struct wrnc_dev *wrnc, uint32_t *n_cpu)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;
	char path[WRNC_SYSFS_PATH_LEN];

	snprintf(path, WRNC_SYSFS_PATH_LEN, "/sys/class/wr-node-core/%s/n_cpu",
		 wdesc->name);

	return wrnc_sysfs_scanf(path, "%x", n_cpu);
}


/**
 * It returns the application identifier of the FPGA bitstream in use
 * @param[in] wrnc device token
 * @param[out] app_id application identifier
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int wrnc_app_id_get(struct wrnc_dev *wrnc, uint32_t *app_id)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;
	char path[WRNC_SYSFS_PATH_LEN];

	snprintf(path, WRNC_SYSFS_PATH_LEN,
		 "/sys/class/wr-node-core/%s/application_id",
		 wdesc->name);

	return wrnc_sysfs_scanf(path, "%x", app_id);
}


/**
 * It returns the current status of the WRNC CPUs' reset line
 * @param[in] wrnc device token
 * @param[out] mask bit mask of the reset-lines
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int wrnc_cpu_reset_get(struct wrnc_dev *wrnc, uint32_t *mask)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;
	char path[WRNC_SYSFS_PATH_LEN];

	snprintf(path, WRNC_SYSFS_PATH_LEN,
		 "/sys/class/wr-node-core/%s/reset_mask",
		 wdesc->name);

	return wrnc_sysfs_scanf(path, "%x", mask);
}


/**
 * Assert or de-assert the reset line of the WRNC CPUs
 * @param[in] wrnc device to use
 * @param[in] mask bit mask of the reset-lines
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int wrnc_cpu_reset_set(struct wrnc_dev *wrnc, uint32_t mask)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;
	char path[WRNC_SYSFS_PATH_LEN];

	snprintf(path, WRNC_SYSFS_PATH_LEN,
		 "/sys/class/wr-node-core/%s/reset_mask",
		 wdesc->name);

	return wrnc_sysfs_printf(path, "%x", mask);
}


/**
 * It returns the current status of the WRNC CPUs' enable line
 * @param[in] wrnc device token
 * @param[out] mask bit mask of the enable-lines
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int wrnc_cpu_run_get(struct wrnc_dev *wrnc, uint32_t *mask)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;
	char path[WRNC_SYSFS_PATH_LEN];

	snprintf(path, WRNC_SYSFS_PATH_LEN,
		 "/sys/class/wr-node-core/%s/enable_mask",
		 wdesc->name);

	return wrnc_sysfs_scanf(path, "%x", mask);
}


/**
 * Assert or de-assert the enable (a.k.a. running) line of the WRNC CPUs
 * @param[in] wrnc device token
 * @param[in] mask bit mask of the enable-lines
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int wrnc_cpu_run_set(struct wrnc_dev *wrnc, uint32_t mask)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;
	char path[WRNC_SYSFS_PATH_LEN];

	snprintf(path, WRNC_SYSFS_PATH_LEN,
		 "/sys/class/wr-node-core/%s/enable_mask",
		 wdesc->name);

	return wrnc_sysfs_printf(path, "%x", mask);
}


/**
 * It loads a wrnc CPU firmware from a given buffer
 * @param[in] wrnc device token
 * @param[in] index CPU index
 * @param[in] code buffer containing the CPU firmware binary code
 * @param[in] length code length
 * @return the number of written byte, on error -1 and errno is
 *         set appropriately
 */
int wrnc_cpu_load_application_raw(struct wrnc_dev *wrnc,
				  unsigned int index,
				  void *code, size_t length,
				  unsigned int offset)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;
	char path[WRNC_DEVICE_PATH_LEN];
	int fd, i = 0;

	snprintf(path, WRNC_DEVICE_PATH_LEN, "/dev/%s-cpu-%02d",
		 wdesc->name, index);
	fd = open(path, O_WRONLY);
	if (!fd)
		return -1;
	lseek(fd, offset, SEEK_SET);

	do {
		i += write(fd, code, length);
	} while (i < length);

	close(fd);
	return i;
}


/**
 * It dumps a WRNC CPU firmware into a given buffer
 * @param[in] wrnc device token
 * @param[in] index CPU index
 * @param[out] code buffer containing the CPU firmware binary code
 * @param[in] length code length
 * @return the number of written byte, on error -1 and errno is
 *         set appropriately
 */
int wrnc_cpu_dump_application_raw(struct wrnc_dev *wrnc,
				  unsigned int index,
				  void *code, size_t length,
				  unsigned int offset)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;
	char path[WRNC_DEVICE_PATH_LEN];
	int fd, i = 0, c = 100;

	snprintf(path, WRNC_DEVICE_PATH_LEN, "/dev/%s-cpu-%02d",
		 wdesc->name, index);
	fd = open(path, O_RDONLY);
	if (!fd)
		return -1;

	lseek(fd, offset, SEEK_SET);

	while (i < length && --c)
		i += read(fd, code + i, length - i);
	if (!c)
		fprintf(stderr, "Cannot read all the CPU firmware\n");

	close(fd);
	return i;
}


/**
 * It loads a WRNC CPU firmware from a given file
 * @param[in] wrnc device token
 * @param[in] index CPU index
 * @param[in] path path to the firmware file
 * @return 0 on success, on error -1 and errno is set appropriately
 */
int wrnc_cpu_load_application_file(struct wrnc_dev *wrnc,
				   unsigned int index,
				   char *path)
{
	int i, len;
	void *code;
	FILE *f;

	f = fopen(path, "rb");
	if (!f)
		return -1;
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	rewind(f);
	if (!len)
		return 0;

	code = malloc(len);
	if (!code)
		return -1;
	/* Get the code from file */
	i = fread(code, 1, len, f);
	fclose(f);
	if (!i || i != len) {
		/*TODO: maybe optimize me with a loop */
		free(code);
		return -1;
	}

	i = wrnc_cpu_load_application_raw(wrnc, index, code, len, 0);
	if (i != len)
		return -1;

	free(code);

	return 0;
}


/**
 * It dumps a WRNC CPU firmware into a given file
 * @param[in] wrnc device token
 * @param[in] index CPU index
 * @param[in] path path to the firmware file
 * @return 0 on success, on error -1 and errno is set appropriately
 */
int wrnc_cpu_dump_application_file(struct wrnc_dev *wrnc,
				   unsigned int index,
				   char *path)
{
	int i = 0;
	uint8_t code[4096];
	FILE *f;

	f = fopen(path, "wb");
	if (!f)
		return -1;

        do {
		/* Read from driver and write into file */
		i += wrnc_cpu_dump_application_raw(wrnc, index, code, 4096, i);
		if (i != 0)
			fwrite(code, 1, i, f);
	} while(i % 4096 == 0);

	fclose(f);

	return i;
}

/**
 * It opens an HMQ slot
 * @param[in] wdesc device token
 * @param[in] index HMQ index
 * @param[in] flags HMQ flags
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrnc_hmq_open(struct wrnc_dev *wrnc, unsigned int index,
		  unsigned long flags)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;
	char path[64];
	int *fd, dir = flags & WRNC_HMQ_INCOMING;

	if (index >= WRNC_MAX_HMQ_SLOT / 2) {
		errno = EWRNC_INVAL_SLOT;
		return -1;
	}

	fd = dir ? wdesc->fd_hmq_in : wdesc->fd_hmq_out;

	/* Do not open if it is already opened */
	if (fd[index] < 0) {
		snprintf(path, 64, "/dev/%s-hmq-%c-%02d",
			 wdesc->name, (dir ? 'i' : 'o'), index);
		fd[index] = open(path, (dir ? O_WRONLY : O_RDONLY));
		if (fd[index] < 0)
			return -1;
	}

	return 0;
}


/**
 * It closes a HMQ slot
 * @param[in] wdesc device token
 * @param[in] index HMQ index
 * @param[in] flags HMQ flags
 * @return 0 on success, -1 on error and errno is set appropriately
 */
void wrnc_hmq_close(struct wrnc_dev *wrnc, unsigned int index,
		   unsigned long flags)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;
	int *fd;

	fd = flags & WRNC_HMQ_INCOMING ? wdesc->fd_hmq_in : wdesc->fd_hmq_out;
	if (fd[index] > 0)
		close(fd[index]);
}


/**
 * It sends a synchronous message. The slots are uni-directional, so you must
 * specify where write the message and where the answer is expected.
 * @param[in] wrnc device token
 * @param[in] index_in index of the HMQ input slot
 * @param[in] index_out index of the HMQ output slot
 * @param[in,out] msg it contains the message to be sent; the answer will
 *                overwrite the message
 * @param[in] timeout_ms maximum ms to wait for an answer
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrnc_slot_send_and_receive_sync(struct wrnc_dev *wrnc,
				    unsigned int index_in,
				    unsigned int index_out,
				    struct wrnc_msg *msg,
				    unsigned int timeout_ms)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;
	struct wrnc_msg_sync smsg;
	int err;

	if (wdesc->fd_hmq_in[index_in] < 0) {
		errno = EWRNC_HMQ_CLOSE;
		return -1;
	}

	/* Build the message */
	smsg.index_in = index_in;
	smsg.index_out = index_out;
	smsg.timeout_ms = timeout_ms;
	memcpy(&smsg.msg, msg, sizeof(struct wrnc_msg));

	/* Send the message */
	err = ioctl(wdesc->fd_hmq_in[index_in], WRNC_IOCTL_MSG_SYNC, &smsg);
	if (err)
		return -1;

	/* Copy the answer */
	memcpy(msg, &smsg.msg, sizeof(struct wrnc_msg));

	msg->error = 0;
	msg->offset = 0;
	msg->direction = WRNC_MSG_DIR_RECEIVE;

	return 0;
}


/**
 * It is a wrapper of poll(2) for a wrnc slot device.
 * @param[in] wrnc device token
 * @param[in] p see poll(2), instead of the real FD use the HMQ slot index.
 *              This, in order to work only with slots' indexes.
 * @param[in] nfds see poll(2).
 * @param[in] timeout see poll(2)
 * @return see poll(2)
 */
int wrnc_slot_poll(struct wrnc_dev *wrnc, struct pollfd *p, nfds_t nfds,
		   int timeout)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;
	struct pollfd lp[nfds];
	int ret, i, index;

	/* Copy requested events and retrieve fd */
	for (i = 0; i < nfds; ++i) {
		lp[i].events = p[i].events;
		index = p[i].fd;
		if (p[i].events & POLLIN) {
			lp[i].fd = wdesc->fd_hmq_out[index];
		}
		if (p[i].events & POLLOUT) {
			lp[i].fd = wdesc->fd_hmq_in[index];
		}
	}



        ret = poll(lp, nfds, timeout);

	/* Copy back the return events */
	for (i = 0; i < nfds; i++)
		p[i].revents = lp[i].revents;


	return ret;
}

/**
 * It opens a WRNC device
 * @param[in] wdesc device descriptor
 * @return 0 on success, -1 on error and errno is set appropriately
 */
static int wrnc_dev_open(struct wrnc_desc *wdesc)
{
	char path[64];


	if (wdesc->fd_dev < 0) {
		snprintf(path, 64, "/dev/%s", wdesc->name);
	        wdesc->fd_dev = open(path, O_RDWR);
		if (wdesc->fd_dev < 0)
			return -1;
	}

	return 0;
}

/**
 * It execute the ioctl command to read/write an smem address
 * @param[in] wdesc device descriptor
 * @param[in] addr memory address where start the operations
 * @param[in, out] data value in/to the shared memory
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
static int wrnc_smem_io(struct wrnc_desc *wdesc,
			uint32_t addr, uint32_t *data, size_t count,
			enum wrnc_smem_modifier mod, int is_input)
{
	struct wrnc_smem_io io;
	int err, i;

	err = wrnc_dev_open(wdesc);
	if (err)
		return -1;

	io.is_input = is_input;
	io.mod = mod;
	for (i = 0; i < count; i++) {
		io.addr = addr + (i * 4);
		if (!io.is_input)
			io.value = data[i];
		err = ioctl(wdesc->fd_dev, WRNC_IOCTL_SMEM_IO, &io);
		if (err)
			return -1;
		data[i] = io.value;
	}


	return 0;
}

/**
 * It does a direct acces to the shared memory to read a set of cells
 * @param[in] wrnc device token
 * @param[in] addr memory address where start the operations
 * @param[out] data values read from in the shared memory. The function will
 *             replace this value with the read back value
 * @param[in] count number of values in data
 * @param[in] mod shared memory operation mode
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int wrnc_smem_read(struct wrnc_dev *wrnc, uint32_t addr, uint32_t *data,
		   size_t count, enum wrnc_smem_modifier mod)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;

	return wrnc_smem_io(wdesc, addr, data, count, mod, 1);
}


/**
 * It writes on the shared memory of the WRNC
 * @param[in] wrnc device to use
 * @param[in] addr memory address
 * @param[in, out] data values to write in the shared memory. The function will
 *                 replace this value with the read back value
 * @param[in] count number of values in data
 * @param[in] mod shared memory operation mode
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int wrnc_smem_write(struct wrnc_dev *wrnc, uint32_t addr, uint32_t *data,
		    size_t count, enum wrnc_smem_modifier mod)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;

	return wrnc_smem_io(wdesc, addr, data, count, mod, 0);
}


/**
 * It binds a slot to manage only messages that comply with the given filter
 * @param[in] wrnc device to use
 * @param[in] flt filters to apply
 * @param[in] length number of filters
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int wrnc_bind(struct wrnc_dev *wrnc, struct wrnc_msg_filter *flt,
	      unsigned int length)
{
	errno = EWRNC_NO_IMPLEMENTATION;
	return -1;
}


/**
 * It starts to execute code on a given CPU
 * @param[in] wrnc device token
 * @param[in] index CPU index
 */
int wrnc_cpu_start(struct wrnc_dev *wrnc, unsigned int index)
{
	uint32_t tmp;

	wrnc_cpu_run_get(wrnc, &tmp);
	return wrnc_cpu_run_set(wrnc, tmp & ~(1 << index));
}


/**
 * It stops the execution of code on a given CPU
 * @param[in] wrnc device token
 * @param[in] index CPU index
 */
int wrnc_cpu_stop(struct wrnc_dev *wrnc, unsigned int index)
{
	uint32_t tmp;

	wrnc_cpu_run_get(wrnc, &tmp);
	return wrnc_cpu_run_set(wrnc, tmp | (1 << index));
}


/**
 * It enables a CPU; in other words, it clear the reset line of a CPU
 * @param[in] wrnc device token
 * @param[in] index CPU index
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int wrnc_cpu_enable(struct wrnc_dev *wrnc, unsigned int index)
{
	uint32_t tmp;

	wrnc_cpu_reset_get(wrnc, &tmp);
	return wrnc_cpu_reset_set(wrnc, tmp & ~(1 << index));
}

/**
 * It disables a CPU; in other words, it sets the reset line of a CPU
 * @param[in] wrnc device token
 * @param[in] index CPU index
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int wrnc_cpu_disable(struct wrnc_dev *wrnc, unsigned int index)
{
	uint32_t tmp;

	wrnc_cpu_reset_get(wrnc, &tmp);
	return wrnc_cpu_reset_set(wrnc, tmp | (1 << index));
}


/**
 * It allocates and returns a message from an output message queue slot.
 * The user of this function is in charge to release the memory.
 * @param[in] wrnc device to use
 * @param[in] index HMQ slot to read
 * @return a WRNC message, NULL on error and errno is set appropriately
 */
struct wrnc_msg *wrnc_slot_receive(struct wrnc_dev *wrnc, unsigned int index)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;
	struct wrnc_msg *msg;
	int n;

	if (wdesc->fd_hmq_out[index] < 0) {
		errno = EWRNC_HMQ_CLOSE;
		return NULL;
	}

	msg = malloc(sizeof(struct wrnc_msg));
	if (!msg)
		return NULL;

	/* Get a message from the driver */
	n = read(wdesc->fd_hmq_out[index], msg, sizeof(struct wrnc_msg));
	if (n != sizeof(struct wrnc_msg)) {
		free(msg);
		return NULL;
	}

	msg->error = 0;
	msg->offset = 0;
	msg->direction = WRNC_MSG_DIR_RECEIVE;

	return msg;
}


/**
 * It sends a message to an input message queue slot.
 * @param[in] wrnc device to use
 * @param[in] index HMQ slot to write
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int wrnc_slot_send(struct wrnc_dev *wrnc, unsigned int index,
			  struct wrnc_msg *msg)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;
	int n;

	if (wdesc->fd_hmq_in[index] < 0) {
		errno = EWRNC_HMQ_CLOSE;
		return -1;
	}

	msg = malloc(sizeof(struct wrnc_msg));
	if (!msg)
		return -1;

	/* Get a message from the driver */
	n = write(wdesc->fd_hmq_in[index], msg, sizeof(struct wrnc_msg));
	if (n != sizeof(struct wrnc_msg))
		return -1;

	return 0;
}


/**
 * It retreives the file descriptor of a slot
 * @param[in] wrnc device token
 * @param[in] is_input direction of the slot, 1 for input, 0 for output
 * @param[in] index HMQ slot index
 * @return the file descriptor number, -1 if the slot is not yet opened
 */
int wrnc_slot_fd_get(struct wrnc_dev *wrnc, unsigned int is_input,
			    unsigned int index)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;
	int *fd;

	fd = is_input ? wdesc->fd_hmq_in : wdesc->fd_hmq_out;

	return fd[index];
}


/**
 * It returns the name of the device
 * @param[in] wrnc device token
 * @return the string representing the name of the device
 */
char *wrnc_name_get(struct wrnc_dev *wrnc)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;

	return wdesc->name;
}


/**
 * It opens the debug message stream
 * @param[in] wrnc device token
 * @param[in] index CPU index
 * @return a debug token on success, NULL otherwise and errno is set
 *         appropriately
 */
struct wrnc_dbg *wrnc_debug_open(struct wrnc_dev *wrnc, unsigned int index)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;
	struct wrnc_dbg *dbg;
	char path[64];

	dbg = malloc(sizeof(struct wrnc_dbg));
	if (!dbg)
		return NULL;
	dbg->wrnc = wrnc;
	dbg->cpu_index = index;
	snprintf(path, 64, "/sys/kernel/debug/%s/%s-cpu-%02d-dbg",
		 wdesc->name, wdesc->name, index);

	dbg->fd = open(path, O_RDONLY);
	if (!dbg->fd) {
	        free(dbg);
		return NULL;
	}

	return dbg;
}


/**
 * It closes the debug message stream
 * @param[in] dbg_tkn debug token
 * @param[in] index CPU index
 */
void wrnc_debug_close(struct wrnc_dbg *dbg)
{
	close(dbg->fd);
	free(dbg);
	dbg = NULL;
}


/**
 * It retrieve a message from the debug channel. It fills the buffer with a
 * NULL terminated string.
 * @param[in] dbg_tkn debug token
 * @param[out] buf where store incoming message
 * @param[in] count maximum number of char to read (terminator included)
 * @return number of byte read on success, -1 otherwise and errno is set
 *         appropriately
 */
int wrnc_debug_message_get(struct wrnc_dbg *dbg, char *buf, size_t count)
{
	int n = 0, real_count = 0;

	memset(buf, 0, count);
	do {
		n = read(dbg->fd, buf + real_count, count - real_count);
		if (n <= 0)
		        return -1;

		real_count += n;
		/* check if the string from the CPU is shorter */
	        if (buf[real_count - 1] == '\0')
		        break;

	} while (real_count < count);

	/* Put a terminator */
	buf[real_count - 1] = '\0';

	return real_count;
}
