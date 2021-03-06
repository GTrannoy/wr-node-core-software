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

#include "libmockturtle-internal.h"

const unsigned int trtl_default_timeout_ms = 1000;

static char *trtl_error_str[] = {
	"Cannot parse data from sysfs attribute",
	"Invalid slot",
	"Operation not yet implemented",
	"The HMQ slot is close",
	"Invalid message",
	"Error while reading HMQ messages",
	NULL,
};


/**
 * It returns a string messages corresponding to a given error code. If
 * it is not a libtrtl error code, it will run strerror(3)
 * @param[in] err error code. Typically 'errno' variable
 * @return a message error. No need to free the string.
 */
char *trtl_strerror(int err)
{
	if (err < ETRTL_INVAL_PARSE || err > __ETRTL_MAX)
		return strerror(err);
	return trtl_error_str[err - ETRTL_INVAL_PARSE];
}


/**
 * It initializes the WRNC library. It must be called before doing
 * anything else. If you are going to load/unload WRNC devices, then
 * you have to un-load (trtl_exit()) e reload (trtl_init()) the library.
 * @return 0 on success, otherwise -1 and errno is appropriately set
 */
int trtl_init()
{
	/* As you can see there is nothing to do because everything is
	   dynamical and does not require a special initialization.
	   Instead of removing this function and force the user to use it
	   I choosed to keep it even if it's empty. The reason is simple:
	   I do not know in the future if we need this library initialization,
	   if we will need it and we don't have it than we have to force ALL
	   our users to change all the software stack behind this library
	   which is not trivial and not backward compatible. I prefer to avoid
	   it and force people today to use this function and save a lot of
	   time in the future */
	return 0;
}


/**
 * It releases the resources allocated by trtl_init(). It must be called when
 * you stop to use this library. Then, you cannot use functions from this
 * library anymore.
 */
void trtl_exit()
{
	/* READ trtl_init() */
}


/**
 * It returns the number of available WRNCs. This is not calculated on demand.
 * It depends on library initialization.
 * @return the number of WRNC available
 */
uint32_t trtl_count()
{
	glob_t g;
	uint32_t count;
	int err;

	err = glob("/dev/trtl-[A-Fa-f0-9][A-Fa-f0-9][A-Fa-f0-9][A-Fa-f0-9]",
		   GLOB_NOSORT, NULL, &g);
	if (err != GLOB_NOMATCH)
		return 0;
	err = glob("/dev/mockturtle/trtl-[A-Fa-f0-9][A-Fa-f0-9][A-Fa-f0-9][A-Fa-f0-9]",
		   GLOB_NOSORT | GLOB_APPEND, NULL, &g);
	if (err)
		return 0;
	count = g.gl_pathc;
	globfree(&g);

	return count;
}


/**
 * It allocates and returns the list of available WRNC devices. The user is
 * in charge to free the allocated memory wit trtl_list_free(). The list
 * contains trtl_count() + 1 elements. The last element is a NULL pointer.
 * @return a list of WRNC device's names. NULL on error
 */
char **trtl_list()
{
	char **list = NULL;
	glob_t g;
	int err, i, count;

	err = glob("/dev/trtl-[A-Fa-f0-9][A-Fa-f0-9][A-Fa-f0-9][A-Fa-f0-9]",
		   GLOB_NOSORT, NULL, &g);
	if (err != GLOB_NOMATCH)
		return NULL;
	err = glob("/dev/mockturtle/trtl-[A-Fa-f0-9][A-Fa-f0-9][A-Fa-f0-9][A-Fa-f0-9]",
		   GLOB_NOSORT | GLOB_APPEND, NULL, &g);
	if (err)
		return NULL;
	count = g.gl_pathc;

	list = malloc(sizeof(char *) * (count + 1));
	if (!list)
		return NULL;
	for (i = 0; i < count; ++i)
		list[i] = strdup(basename(g.gl_pathv[i]));
	list[i] = NULL;

	globfree(&g);

	return list;
}


/**
 * It release the list allocated memory
 * @param[in] list device list to release
 */
void trtl_list_free(char **list)
{
	int i;

	for (i = 0; list[i]; i++)
		free(list[i]);
	free(list);
}


/**
 * It opens a WRNC device using a string descriptor. The descriptor correspond
 * to the main char device name of the white-rabbit node-core.
 * @param[in] device name of the device to open
 * @return the WRNC token, NULL on error and errno is appropriately set
 */
struct trtl_dev *trtl_open(const char *device)
{
	struct trtl_desc *trtl;
	char path[TRTL_PATH_LEN + TRTL_NAME_LEN];
	int i, err;
	struct stat sb;

	trtl = malloc(sizeof(struct trtl_desc));
	if (!trtl)
		return NULL;
	strncpy(trtl->name, device, TRTL_NAME_LEN);

	snprintf(path, TRTL_PATH_LEN + TRTL_NAME_LEN, "/dev/%s", trtl->name);
	err = stat(path, &sb);
	if (!err) {
		strncpy(trtl->path, "/dev", TRTL_PATH_LEN);
	} else {
		snprintf(path, TRTL_PATH_LEN + TRTL_NAME_LEN,
			 "/dev/mockturtle/%s", trtl->name);
		err = stat(path, &sb);
		if (err)
			goto out_stat;
		strncpy(trtl->path, "/dev/mockturtle", TRTL_PATH_LEN);
	}

	trtl->fd_dev = -1;
	for (i = 0; i < TRTL_MAX_CPU; ++i)
		trtl->fd_cpu[i] = -1;

	return (struct trtl_dev *)trtl;

out_stat:
	free(trtl);
	return NULL;
}


/**
 * It opens a WRNC device using its FMC device_id. The white-rabbit node-core
 * driver is based upon the FMC bus infrastructure, so all trtl devices are
 * identified with their fmc-bus-id.
 * @param[in] device_id FMC device id of the device to use
 * @return the WRNC token, NULL on error and errno is appropriately set
 */
struct trtl_dev *trtl_open_by_fmc(uint32_t device_id)
{
	char name[12];

	snprintf(name, 12, "trtl-%04x", device_id);
	return trtl_open(name);
}


/**
 * It opens a WRNC device using its Logical Unit Number. The Logical Unit Number
 * is an instance number of a particular hardware. The LUN to use is the carrier
 * one, and not the mezzanine one (if any).
 * The driver is not aware of LUNs but only of FMC-id. So, if this function does
 * not work it means that your installation lacks of symbolic links that
 * convert LUNs to FMC-ids.
 * @param[in] lun Logical Unit Number of the device to use
 * @return the WRNC token, NULL on error and errno is appropriately set
 */
struct trtl_dev *trtl_open_by_lun(unsigned int lun)
{
	char path[16], dev_id_str[4];
	uint32_t dev_id;
	int ret;

	ret = snprintf(path, sizeof(path), "/dev/trtl.%d", lun);
	if (ret < 0 || ret >= sizeof(path)) {
		errno = EINVAL;
		return NULL;
	}
	ret = readlink(path, dev_id_str, sizeof(dev_id_str));
	if (sscanf(dev_id_str, "%4x", &dev_id) != 1) {
		errno = ENODEV;
		return NULL;
	}

	return trtl_open_by_fmc(dev_id);
}


/**
 * It closes a WRNC device opened with one of the following functions:
 * trtl_open(), wrcn_open_by_lun(), trtl_open_by_fmc()
 * @param[in] trtl device token
 */
void trtl_close(struct trtl_dev *trtl)
{
	struct trtl_desc *wdesc = (struct trtl_desc *)trtl;
	int i;

	if (!trtl)
		return;

	if (wdesc->fd_dev >= 0)
		close(wdesc->fd_dev);

	for (i = 0; i < TRTL_MAX_CPU; ++i)
		if (wdesc->fd_cpu[i] >= 0)
			close(wdesc->fd_cpu[i]);

	free(wdesc);
}


/**
 * Generic function that reads from a sysfs attribute
 */
static int trtl_sysfs_read(char *path, void *buf, size_t len)
{
	int fd, i;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;
	i = read(fd, buf, len);
	close(fd);

	return i;
}


/**
 * Generic function that writes to a sysfs attribute
 */
static int trtl_sysfs_write(char *path, void *buf, size_t len)
{
	int fd, i;

	fd = open(path, O_WRONLY);
	if (fd < 0)
		return -1;
	i = write(fd, buf, len);
	close(fd);

	return i;
}

/**
 * Generic function that parse a string from a sysfs attribute
 */
static int trtl_sysfs_scanf(char *path, const char *fmt, ...)
{
	char buf[TRTL_SYSFS_READ_LEN];
	va_list args;
	int ret;

	ret = trtl_sysfs_read(path, buf, TRTL_SYSFS_READ_LEN);
	if (ret < 0)
		return ret;

	va_start(args, fmt);
	ret = vsscanf(buf, fmt, args);
	va_end(args);
	if (ret < 0)
		return ret;
	if (ret != 1) {
		errno = ETRTL_INVAL_PARSE;
		return -1;
	}

	return 0;
}


/**
 * Generic function that build a string to be written in a sysfs attribute
 */
static int trtl_sysfs_printf(char *path, const char *fmt, ...)
{
	char buf[TRTL_SYSFS_READ_LEN];
	va_list args;
	int ret;

	va_start(args, fmt);
	vsnprintf(buf, TRTL_SYSFS_READ_LEN, fmt, args);
	va_end(args);

        ret = trtl_sysfs_write(path, buf, TRTL_SYSFS_READ_LEN);
	if (ret == TRTL_SYSFS_READ_LEN)
		return 0;
	return -1;
}


/**
 * It returns the number of available WRNC CPUs on the FPGA bitstream
 * @param[in] trtl device token
 * @param[out] n_cpu the number of available CPUs
 * @return 0 on success; -1 on error and errno is set appropriately
 */
int trtl_cpu_count(struct trtl_dev *trtl, uint32_t *n_cpu)
{
	struct trtl_desc *wdesc = (struct trtl_desc *)trtl;
	char path[TRTL_SYSFS_PATH_LEN];

	snprintf(path, TRTL_SYSFS_PATH_LEN, "/sys/class/mockturtle/%s/n_cpu",
		 wdesc->name);

	return trtl_sysfs_scanf(path, "%x", n_cpu);
}


/**
 * It returns the application identifier of the FPGA bitstream in use
 * @param[in] trtl device token
 * @param[out] app_id application identifier
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int trtl_app_id_get(struct trtl_dev *trtl, uint32_t *app_id)
{
	struct trtl_desc *wdesc = (struct trtl_desc *)trtl;
	char path[TRTL_SYSFS_PATH_LEN];

	snprintf(path, TRTL_SYSFS_PATH_LEN,
		 "/sys/class/mockturtle/%s/application_id",
		 wdesc->name);

	return trtl_sysfs_scanf(path, "%x", app_id);
}


/**
 * It returns the current status of the WRNC CPUs' reset line
 * @param[in] trtl device token
 * @param[out] mask bit mask of the reset-lines
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int trtl_cpu_reset_get(struct trtl_dev *trtl, uint32_t *mask)
{
	struct trtl_desc *wdesc = (struct trtl_desc *)trtl;
	char path[TRTL_SYSFS_PATH_LEN];

	snprintf(path, TRTL_SYSFS_PATH_LEN,
		 "/sys/class/mockturtle/%s/reset_mask",
		 wdesc->name);

	return trtl_sysfs_scanf(path, "%x", mask);
}


/**
 * Assert or de-assert the reset line of the WRNC CPUs
 * @param[in] trtl device to use
 * @param[in] mask bit mask of the reset-lines
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int trtl_cpu_reset_set(struct trtl_dev *trtl, uint32_t mask)
{
	struct trtl_desc *wdesc = (struct trtl_desc *)trtl;
	char path[TRTL_SYSFS_PATH_LEN];

	snprintf(path, TRTL_SYSFS_PATH_LEN,
		 "/sys/class/mockturtle/%s/reset_mask",
		 wdesc->name);

	return trtl_sysfs_printf(path, "%x", mask);
}


/**
 * It returns the current status of the WRNC CPUs' enable line
 * @param[in] trtl device token
 * @param[out] mask bit mask of the enable-lines
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int trtl_cpu_run_get(struct trtl_dev *trtl, uint32_t *mask)
{
	struct trtl_desc *wdesc = (struct trtl_desc *)trtl;
	char path[TRTL_SYSFS_PATH_LEN];

	snprintf(path, TRTL_SYSFS_PATH_LEN,
		 "/sys/class/mockturtle/%s/enable_mask",
		 wdesc->name);

	return trtl_sysfs_scanf(path, "%x", mask);
}


/**
 * Assert or de-assert the enable (a.k.a. running) line of the WRNC CPUs
 * @param[in] trtl device token
 * @param[in] mask bit mask of the enable-lines
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int trtl_cpu_run_set(struct trtl_dev *trtl, uint32_t mask)
{
	struct trtl_desc *wdesc = (struct trtl_desc *)trtl;
	char path[TRTL_SYSFS_PATH_LEN];

	snprintf(path, TRTL_SYSFS_PATH_LEN,
		 "/sys/class/mockturtle/%s/enable_mask",
		 wdesc->name);

	return trtl_sysfs_printf(path, "%x", mask);
}


/**
 * It loads a trtl CPU firmware from a given buffer
 * @param[in] trtl device token
 * @param[in] index CPU index
 * @param[in] code buffer containing the CPU firmware binary code
 * @param[in] length code length
 * @return the number of written byte, on error -1 and errno is
 *         set appropriately
 */
int trtl_cpu_load_application_raw(struct trtl_dev *trtl,
				  unsigned int index,
				  void *code, size_t length,
				  unsigned int offset)
{
	struct trtl_desc *wdesc = (struct trtl_desc *)trtl;
	char path[TRTL_DEVICE_PATH_LEN];
	int fd, i = 0;

	snprintf(path, TRTL_DEVICE_PATH_LEN, "%s/%s-cpu-%02d",
		 wdesc->path, wdesc->name, index);
	fd = open(path, O_WRONLY);
	if (fd < 0)
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
 * @param[in] trtl device token
 * @param[in] index CPU index
 * @param[out] code buffer containing the CPU firmware binary code
 * @param[in] length code length
 * @return the number of written byte, on error -1 and errno is
 *         set appropriately
 */
int trtl_cpu_dump_application_raw(struct trtl_dev *trtl,
				  unsigned int index,
				  void *code, size_t length,
				  unsigned int offset)
{
	struct trtl_desc *wdesc = (struct trtl_desc *)trtl;
	char path[TRTL_DEVICE_PATH_LEN];
	int fd, i = 0, c = 100;

	snprintf(path, TRTL_DEVICE_PATH_LEN, "%s/%s-cpu-%02d",
		 wdesc->path, wdesc->name, index);
	fd = open(path, O_RDONLY);
	if (fd < 0)
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
 * @param[in] trtl device token
 * @param[in] index CPU index
 * @param[in] path path to the firmware file
 * @return 0 on success, on error -1 and errno is set appropriately
 */
int trtl_cpu_load_application_file(struct trtl_dev *trtl,
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

	i = trtl_cpu_load_application_raw(trtl, index, code, len, 0);
	if (i != len)
		return -1;

	free(code);

	return 0;
}


/**
 * It dumps a WRNC CPU firmware into a given file
 * @param[in] trtl device token
 * @param[in] index CPU index
 * @param[in] path path to the firmware file
 * @return 0 on success, on error -1 and errno is set appropriately
 */
int trtl_cpu_dump_application_file(struct trtl_dev *trtl,
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
		i += trtl_cpu_dump_application_raw(trtl, index, code, 4096, i);
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
 * @return a HMQ token on success, NULL on error and errno is set appropriately
 */
struct trtl_hmq *trtl_hmq_open(struct trtl_dev *trtl, unsigned int index,
			       unsigned long flags)
{
	struct trtl_desc *wdesc = (struct trtl_desc *)trtl;
	struct trtl_hmq *hmq;
	char path[64];
	int fd, dir = flags & TRTL_HMQ_INCOMING;

	if (index >= TRTL_MAX_HMQ_SLOT / 2) {
		errno = ETRTL_INVAL_SLOT;
		return NULL;
	}

	snprintf(path, 64, "%s/%s-hmq-%c-%02d",
		 wdesc->path, wdesc->name, (dir ? 'i' : 'o'), index);
	fd = open(path, (dir ? O_WRONLY : O_RDONLY));
	if (fd < 0)
		return NULL;

	hmq = malloc(sizeof(struct trtl_hmq));
	if (!hmq) {
		close(fd);
		return NULL;
	}

	hmq->trtl = trtl;
	hmq->index = index;
	hmq->flags = flags;
	hmq->fd = fd;
	snprintf(hmq->syspath, 64, "/sys/class/mockturtle/%s/%s-hmq-%c-%02d",
		 wdesc->name, wdesc->name, (dir ? 'i' : 'o'), index);

	return (struct trtl_hmq *)hmq;
}


/**
 * It closes a HMQ slot
 * @param[in] hmq HMQ device descriptor
 * @return 0 on success, -1 on error and errno is set appropriately
 */
void trtl_hmq_close(struct trtl_hmq *hmq)
{
	if (hmq && hmq->fd > 0) {
		close(hmq->fd);
		free(hmq);
	}
}


/**
 * It enables/disables the message share mode. When enable, all users will read
 * the same messages.
 * @param[in] trtl device token
 * @param[in] dir slot direction, 1 CPU input, 0 CPU output
 * @param[in] index slot index
 * @param[in] status status to set: 1 enable, 0 disable
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int trtl_hmq_share_set(struct trtl_dev *trtl, unsigned int dir,
		       unsigned int index, unsigned int status)
{
	struct trtl_desc *wdesc = (struct trtl_desc *)trtl;
	char path[TRTL_SYSFS_PATH_LEN];

	snprintf(path, TRTL_SYSFS_PATH_LEN,
		 "/sys/class/mockturtle/%s/%s-hmq-%c-%02d/shared_by_users",
		 wdesc->name, wdesc->name, (dir ? 'i' : 'o'), index);

	return trtl_sysfs_printf(path, "%d", status);
}


/**
 * It gets the current status of the message share mode
 * @param[in] trtl device token
 * @param[in] dir slot direction, 1 CPU input, 0 CPU output
 * @param[in] index slot index
 * @param[out] status current value
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int trtl_hmq_share_get(struct trtl_dev *trtl, unsigned int dir,
		       unsigned int index, unsigned int *status)
{
	struct trtl_desc *wdesc = (struct trtl_desc *)trtl;
	char path[TRTL_SYSFS_PATH_LEN];

	snprintf(path, TRTL_SYSFS_PATH_LEN,
		 "/sys/class/mockturtle/%s/%s-hmq-%c-%02d/shared_by_users",
		 wdesc->name, wdesc->name, (dir ? 'i' : 'o'), index);

	return trtl_sysfs_scanf(path, "%d", status);
}

/**
 * It sends a synchronous message. The slots are uni-directional, so you must
 * specify where write the message and where the answer is expected.
 * @param[in] hmq HMQ device descriptor on the input slot
 * @param[in] index_out index of the HMQ output slot
 * @param[in,out] msg it contains the message to be sent; the answer will
 *                overwrite the message
 * @param[in] timeout_ms maximum ms to wait for an answer. If you ask for
 *            0ms timeout, the driver will use the default driver timeout.
 * @return On success, a positive number is returned; this number represent
 *         the remaining ms to the timeout. -1 on error and errno is set
 *         appropriately
 */
int trtl_hmq_send_and_receive_sync(struct trtl_hmq *hmq,
				   unsigned int index_out,
				   struct trtl_msg *msg,
				   unsigned int timeout_ms)
{
	struct trtl_msg_sync smsg;
	int err;

	if (!hmq || hmq->fd < 0) {
		errno = ETRTL_HMQ_CLOSE;
		return -1;
	}

	/* Build the message */
	smsg.index_in = hmq->index;
	smsg.index_out = index_out;
	smsg.timeout_ms = timeout_ms;
	smsg.msg = msg;

	/* Send the message */
	err = ioctl(hmq->fd, TRTL_IOCTL_MSG_SYNC, &smsg);
	if (err)
		return -1;

	if (smsg.timeout_ms == 0) {
		errno = ETIME;
		return -1;
	}
	return smsg.timeout_ms;
}


/**
 * It sets the driver buffer size.
 * Note that this does not affect the hardware in any way. The hardware
 * maximum number of messages is fixed and cannot be change
 * @param[in] hmq HMQ device descriptor
 * @param[in] size buffer size in bytes. It must be a power-of-2 value.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int trtl_hmq_buffer_size_set(struct trtl_hmq *hmq, uint32_t size)
{
	char path[TRTL_SYSFS_PATH_LEN];

	snprintf(path, TRTL_SYSFS_PATH_LEN, "%s/buffer_size", hmq->syspath);

	return trtl_sysfs_printf(path, "%d", size);
}


/**
 * It gets the driver buffer size.
 * @param[in] hmq HMQ device descriptor
 * @param[out] size buffer size in bytes
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int trtl_hmq_buffer_size_get(struct trtl_hmq *hmq, uint32_t *size)
{
	char path[TRTL_SYSFS_PATH_LEN];

	snprintf(path, TRTL_SYSFS_PATH_LEN, "%s/buffer_size", hmq->syspath);

	return trtl_sysfs_scanf(path, "%d", size);
}


/**
 * It gets the message size, in other words the buffer width
 * @param[in] hmq HMQ device descriptor
 * @param[out] width maximum message width in bytes
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int trtl_hmq_width_get(struct trtl_hmq *hmq, uint32_t *width)
{
	char path[TRTL_SYSFS_PATH_LEN];
	int err;

	snprintf(path, TRTL_SYSFS_PATH_LEN, "%s/width_max", hmq->syspath);

	err = trtl_sysfs_scanf(path, "%d", width);
	*width = 4 * (*width); /* Convert to byte */

	return err;
}


/**
 * It gets the maximum number of messages that can be saved in the buffer
 * @param[in] hmq HMQ device descriptor
 * @param[out] max maximum number of messages in the buffer
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int trtl_hmq_msg_max_get(struct trtl_hmq *hmq, uint32_t *max)
{
	uint32_t buf_size, msg_size;
	int err;

	err = trtl_hmq_buffer_size_get(hmq, &buf_size);
	if (err)
		return err;
	err = trtl_hmq_width_get(hmq, &msg_size);
	if (err)
		return err;

	*max = buf_size / msg_size;

	return 0;
}


/**
 * It gets the maximum number of messages that can be stored within the
 * mockturtle HMQ (in hardware).
 * @param[in] hmq HMQ device descriptor
 * @param[out] max maximum number of message to set
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int trtl_hmq_count_max_hw_get(struct trtl_hmq *hmq, uint32_t *max)
{
	char path[TRTL_SYSFS_PATH_LEN];

	snprintf(path, TRTL_SYSFS_PATH_LEN, "%s/count_max_hw", hmq->syspath);

	return trtl_sysfs_scanf(path, "%d", max);
}


/**
 * It opens a WRNC device
 * @param[in] wdesc device descriptor
 * @return 0 on success, -1 on error and errno is set appropriately
 */
static int trtl_dev_open(struct trtl_desc *wdesc)
{
	char path[64];


	if (wdesc->fd_dev < 0) {
		snprintf(path, 64, "%s/%s", wdesc->path, wdesc->name);
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
static int trtl_smem_io(struct trtl_desc *wdesc,
			uint32_t addr, uint32_t *data, size_t count,
			enum trtl_smem_modifier mod, int is_input)
{
	struct trtl_smem_io io;
	int err, i;

	err = trtl_dev_open(wdesc);
	if (err)
		return -1;

	io.is_input = is_input;
	io.mod = mod;
	for (i = 0; i < count; i++) {
		io.addr = addr + (i * 4);
		if (!io.is_input)
			io.value = data[i];
		err = ioctl(wdesc->fd_dev, TRTL_IOCTL_SMEM_IO, &io);
		if (err)
			return -1;
		data[i] = io.value;
	}


	return 0;
}

/**
 * It does a direct acces to the shared memory to read a set of cells
 * @param[in] trtl device token
 * @param[in] addr memory address where start the operations
 * @param[out] data values read from in the shared memory. The function will
 *             replace this value with the read back value
 * @param[in] count number of values in data
 * @param[in] mod shared memory operation mode
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int trtl_smem_read(struct trtl_dev *trtl, uint32_t addr, uint32_t *data,
		   size_t count, enum trtl_smem_modifier mod)
{
	struct trtl_desc *wdesc = (struct trtl_desc *)trtl;

	return trtl_smem_io(wdesc, addr, data, count, mod, 1);
}


/**
 * It writes on the shared memory of the WRNC
 * @param[in] trtl device to use
 * @param[in] addr memory address
 * @param[in, out] data values to write in the shared memory. The function will
 *                 replace this value with the read back value
 * @param[in] count number of values in data
 * @param[in] mod shared memory operation mode
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int trtl_smem_write(struct trtl_dev *trtl, uint32_t addr, uint32_t *data,
		    size_t count, enum trtl_smem_modifier mod)
{
	struct trtl_desc *wdesc = (struct trtl_desc *)trtl;

	return trtl_smem_io(wdesc, addr, data, count, mod, 0);
}


/**
 * It binds a slot to manage only messages that comply with the given filter
 * @param[in] trtl device to use
 * @param[in] flt filters to apply
 * @param[in] length number of filters
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int trtl_bind(struct trtl_dev *trtl, struct trtl_msg_filter *flt,
	      unsigned int length)
{
	errno = ETRTL_NO_IMPLEMENTATION;
	return -1;
}


/**
 * It starts to execute code on a given CPU.
 * This function is a wrapper of trtl_cpu_run_set() that allow you to safely
 * start a single CPU.
 * @param[in] trtl device token
 * @param[in] index CPU index
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int trtl_cpu_start(struct trtl_dev *trtl, unsigned int index)
{
	uint32_t tmp;

	trtl_cpu_run_get(trtl, &tmp);
	return trtl_cpu_run_set(trtl, tmp & ~(1 << index));
}


/**
 * It stops the execution of code on a given CPU
 * This function is a wrapper of trtl_cpu_run_set() that allow you to safely
 * stop a single CPU.
 * @param[in] trtl device token
 * @param[in] index CPU index
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int trtl_cpu_stop(struct trtl_dev *trtl, unsigned int index)
{
	uint32_t tmp;

	trtl_cpu_run_get(trtl, &tmp);
	return trtl_cpu_run_set(trtl, tmp | (1 << index));
}


/**
 * It checks if the CPU is running (or not)
 * @param[in] trtl device token
 * @param[in] index CPU index
 * @param[out] run 1 if the CPU is running
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int trtl_cpu_is_running(struct trtl_dev *trtl, unsigned int index,
			unsigned int *run)
{
	uint32_t tmp, err;

	err = trtl_cpu_run_get(trtl, &tmp);
	if (err)
		return -1;

	*run = (tmp & (1 << index) ? 0 : 1);
	return 0;
}


/**
 * It enables a CPU; in other words, it clear the reset line of a CPU.
 * This function is a wrapper of trtl_cpu_reset_set() that allow you to safely
 * enable a single CPU.
 * @param[in] trtl device token
 * @param[in] index CPU index
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int trtl_cpu_enable(struct trtl_dev *trtl, unsigned int index)
{
	uint32_t tmp;

	trtl_cpu_reset_get(trtl, &tmp);
	return trtl_cpu_reset_set(trtl, tmp & ~(1 << index));
}


/**
 * It disables a CPU; in other words, it sets the reset line of a CPU.
 * This function is a wrapper of trtl_cpu_reset_set() that allow you to safely
 * disable a single CPU.
 * @param[in] trtl device token
 * @param[in] index CPU index
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int trtl_cpu_disable(struct trtl_dev *trtl, unsigned int index)
{
	uint32_t tmp;

	trtl_cpu_reset_get(trtl, &tmp);
	return trtl_cpu_reset_set(trtl, tmp | (1 << index));
}


/**
 * It checks if the CPU is enabled (or not)
 * @param[in] trtl device token
 * @param[in] index CPU index
 * @param[out] enable 1 if the CPU is enable
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int trtl_cpu_is_enable(struct trtl_dev *trtl, unsigned int index,
			unsigned int *enable)
{
	uint32_t tmp, err;

	err = trtl_cpu_reset_get(trtl, &tmp);
	if (err)
		return -1;

	*enable = (tmp & (1 << index) ? 0 : 1);
	return 0;
}


/**
 * It gets from the driver a list of messages
 * @param[in] hmq HMQ device descriptor
 * @param[in] msg buffer where store incoming messages
 * @param[in] n maximum number of messages to read
 * @return number of message read, -1 on error and errno is set appropriately
 */
int trtl_hmq_receive_n(struct trtl_hmq *hmq,
		       struct trtl_msg *msg, unsigned int n)
{
	int ret, size;

	if (!hmq || hmq->fd < 0) {
		errno = ETRTL_HMQ_CLOSE;
		return -1;
	}

	/* Get a message from the driver */
	size = sizeof(struct trtl_msg);
	ret = read(hmq->fd, msg, size * n);
	if (ret % sizeof(struct trtl_msg)) {
		errno = ETRTL_HMQ_CLOSE;
		return -1;
	}

	return (ret / size);
}


/**
 * It allocates and returns a message from an output message queue slot.
 * The user of this function is in charge to release the memory.
 * @param[in] hmq HMQ device descriptor
 * @return a WRNC message, NULL on error and errno is set appropriately
 */
struct trtl_msg *trtl_hmq_receive(struct trtl_hmq *hmq)
{
	struct trtl_msg *msg;
	int ret;

	msg = malloc(sizeof(struct trtl_msg));
	if (!msg)
		return NULL;

	ret =  trtl_hmq_receive_n(hmq, msg, 1);
	if (ret < 0) {
		free(msg);
		return NULL;
	}

	return msg;
}


/**
 * It sends a message to an input message queue slot.
 * @param[in] hmq HMQ device descriptor
 * @param[in] msg message to send
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int trtl_hmq_send(struct trtl_hmq *hmq, struct trtl_msg *msg)
{
	int n;

	if (!hmq || hmq->fd < 0) {
		errno = ETRTL_HMQ_CLOSE;
		return -1;
	}

	if (msg->datalen >= TRTL_MAX_PAYLOAD_SIZE) {
		errno = EINVAL;
		return -1;
	}

	/* Get a message from the driver */
	n = write(hmq->fd, msg, sizeof(struct trtl_msg));
	if (n != sizeof(struct trtl_msg))
		return -1;

	return 0;
}


/**
 * It adds a new filter to the given hmq descriptor
 * @param[in] hmq HMQ device descriptor
 * @param[in] filter filter to add
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int trtl_hmq_filter_add(struct trtl_hmq *hmq, struct trtl_msg_filter *filter)
{
	return ioctl(hmq->fd, TRTL_IOCTL_MSG_FILTER_ADD, filter);
}


/**
 * It removes all filters from the given hmq descriptor
 * @param[in] hmq HMQ device descriptor
 * @return 0 on success, -1 otherwise and errno is set appropriately
 */
int trtl_hmq_filter_clean(struct trtl_hmq *hmq)
{
	return ioctl(hmq->fd, TRTL_IOCTL_MSG_FILTER_CLEAN, NULL);
}


/**
 * It returns the device name
 * @param[in] trtl device token
 * @return the string representing the name of the device
 */
char *trtl_name_get(struct trtl_dev *trtl)
{
	struct trtl_desc *wdesc = (struct trtl_desc *)trtl;

	return wdesc->name;
}


/**
 * It opens the debug message stream
 * @param[in] trtl device token
 * @param[in] index CPU index
 * @return a debug token on success, NULL otherwise and errno is set
 *         appropriately
 */
struct trtl_dbg *trtl_debug_open(struct trtl_dev *trtl, unsigned int index)
{
	struct trtl_desc *wdesc = (struct trtl_desc *)trtl;
	struct trtl_dbg *dbg;
	char path[64];

	dbg = malloc(sizeof(struct trtl_dbg));
	if (!dbg)
		return NULL;
	dbg->trtl = trtl;
	dbg->cpu_index = index;
	snprintf(path, 64, "/sys/kernel/debug/%s/%s-cpu-%02d-dbg",
		 wdesc->name, wdesc->name, index);

	dbg->fd = open(path, O_RDONLY);
	if (dbg->fd < 0) {
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
void trtl_debug_close(struct trtl_dbg *dbg)
{
	close(dbg->fd);
	free(dbg);
	dbg = NULL;
}


#define N_RETRY 100
/**
 * It retrieves a message from the debug channel. It fills the buffer with a
 * NULL terminated string.
 * It is a very rare case, but it may happen that you do not receive the entire
 * message in one shot. This may happen when the driver, or the transmission
 * channel, is slower then the consumer (you or this function)
 * @param[in] dbg_tkn debug token
 * @param[out] buf where store incoming message. The message it's always
 *                 properly terminated.
 * @param[in] count maximum number of char to read (terminator included)
 * @return number of byte read on success, -1 otherwise and errno is set
 *         appropriately
 */
int trtl_debug_message_get(struct trtl_dbg *dbg, char *buf, size_t count)
{
	int n = 0, real_count = 0, retry = N_RETRY;

	memset(buf, 0, count);
	do {
		n = read(dbg->fd, buf + real_count, count - real_count);
		if (n < 0)
		        return -1;
		real_count += n;
		/* check if the string from the CPU is shorter */
	        if (buf[real_count - 1] == '\0')
		        break;

		/*
		 * We are expecting a NULL terminated string, but there is
		 * nothing to read now and the string is not terminated. Retry,
		 * maybe we were too slow
		 */
		retry = (n == 0 ? retry - 1 : N_RETRY);
	} while (real_count < count && retry > 0);

	return real_count;
}
#undef N_RETRY
