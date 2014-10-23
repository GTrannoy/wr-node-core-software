/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *         inspired by a draft of Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * License: GPL v3
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <glob.h>

#include "libwrnc.h"
#include "libwrnc-internal.h"

static uint32_t count;
static char wrnc_dev_list[WRNC_MAX_CARRIERS + 1][WRNC_NAME_LEN];
static char *wrnc_error_str[] = {
	"Cannot parse data from sysfs attribute",
	NULL,
};

char *wrnc_strerror(int err)
{
	if (err < EWRNC_INVAL_PARSE || err > __EWRNC_MAX)
		return strerror(err);
	return wrnc_error_str[err - EWRNC_INVAL_PARSE];
}

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
	for (i = count; i < WRNC_MAX_CARRIERS; ++i)
		wrnc_dev_list[i][0] = '\0';

	return 0;
}

void wrnc_exit()
{
	int i;

	for (i = 0; i < WRNC_MAX_CARRIERS; ++i)
		wrnc_dev_list[i][0] = '\0';
}

uint32_t wrnc_count()
{
	return count;
}

char (*wrnc_list())[WRNC_NAME_LEN]
{
	char *list;

	list = malloc((WRNC_MAX_CARRIERS + 1) * WRNC_NAME_LEN);
	if (!list)
		return NULL;
	memcpy(list, wrnc_dev_list, (WRNC_MAX_CARRIERS + 1) * WRNC_NAME_LEN);

	return (char (*)[WRNC_NAME_LEN])list;
}

struct wrnc_dev *wrnc_open(const char *device)
{
	struct wrnc_desc *wrnc;

	wrnc = malloc(sizeof(struct wrnc_desc));
	if (!wrnc)
		return NULL;

	strncpy(wrnc->name, device, WRNC_NAME_LEN);

	return (struct wrnc_dev *)wrnc;
}

void wrnc_close(struct wrnc_dev *wrnc)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;
	int i;

	if (wdesc->fd_dev)
		close(wdesc->fd_dev);

	for (i = 0; i < WRNC_MAX_CPU; ++i)
		if (wdesc->fd_cpu[i])
			close(wdesc->fd_cpu[i]);

	for (i = 0; i < WRNC_MAX_HMQ_SLOT; ++i)
		if (wdesc->fd_hmq_in[i])
			close(wdesc->fd_hmq_in[i]);

	for (i = 0; i < WRNC_MAX_HMQ_SLOT; ++i)
		if (wdesc->fd_hmq_in[i])
			close(wdesc->fd_hmq_in[i]);
	free(wdesc);
}

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

static int wrnc_sysfs_printf(char *path, const char *fmt, ...)
{
	char buf[WRNC_SYSFS_READ_LEN];
	va_list args;

	va_start(args, fmt);
	vsnprintf(buf, WRNC_SYSFS_READ_LEN, fmt, args);
	va_end(args);

        return wrnc_sysfs_write(path, buf, WRNC_SYSFS_READ_LEN);
}


int wrnc_cpu_count(struct wrnc_dev *wrnc, uint32_t *n_cpu)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;
	char path[WRNC_SYSFS_PATH_LEN];

	snprintf(path, WRNC_SYSFS_PATH_LEN, "/sys/class/%s/n_cpu",
		 wdesc->name);

	return wrnc_sysfs_scanf(path, "%x", n_cpu);
}


int wrnc_app_id_get(struct wrnc_dev *wrnc, uint32_t *app_id)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;
	char path[WRNC_SYSFS_PATH_LEN];

	snprintf(path, WRNC_SYSFS_PATH_LEN, "/sys/class/%s/application_id",
		 wdesc->name);

	return wrnc_sysfs_scanf(path, "%x", app_id);
}


int wrnc_cpu_reset_get(struct wrnc_dev *wrnc, uint32_t *mask)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;
	char path[WRNC_SYSFS_PATH_LEN];

	snprintf(path, WRNC_SYSFS_PATH_LEN,  "/sys/class/%s/reset",
		 wdesc->name);

	return wrnc_sysfs_scanf(path, "%x", mask);
}

int wrnc_cpu_reset_set(struct wrnc_dev *wrnc, uint32_t mask)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;
	char path[WRNC_SYSFS_PATH_LEN];

	snprintf(path, WRNC_SYSFS_PATH_LEN,  "/sys/class/%s/reset",
		 wdesc->name);

	return wrnc_sysfs_printf(path, "%x", mask);
}


int wrnc_cpu_run_get(struct wrnc_dev *wrnc, uint32_t *mask)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;
	char path[WRNC_SYSFS_PATH_LEN];

	snprintf(path, WRNC_SYSFS_PATH_LEN,  "/sys/class/%s/enable",
		 wdesc->name);

	return wrnc_sysfs_scanf(path, "%x", mask);
}


int wrnc_cpu_run_set(struct wrnc_dev *wrnc, uint32_t mask)
{
	struct wrnc_desc *wdesc = (struct wrnc_desc *)wrnc;
	char path[WRNC_SYSFS_PATH_LEN];

	snprintf(path, WRNC_SYSFS_PATH_LEN,  "/sys/class/%s/enable",
		 wdesc->name);

	return wrnc_sysfs_printf(path, "%x", mask);
}


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

	i = fread(code, 1, len, f);
	fclose(f);
	if (!i || i != len) {
		free(code);
		return -1;
	}

	i = wrnc_cpu_load_application_raw(wrnc, index, code, len, 0);
	if (i != len)
		return -1;

	free(code);

	return 0;
}

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
		i += wrnc_cpu_dump_application_raw(wrnc, index, code, 4096, i);
		if (i != 0)
			fwrite(code, 1, i, f);
	} while(i % 4096 == 0);

	fclose(f);

	return i;
}
