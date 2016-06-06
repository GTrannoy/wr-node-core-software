/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <libgen.h>
#include <libwrtd.h>

#define WRTD_BOOT_WR_FMC_OFFSET 0x2

enum wrtb_boot_tdc_offset_type {
	OFFSET_ZERO,
	OFFSET_USER,
	OFFSET_WR,
};

char *strtype[] = {
	"zero-offset",
	"user-offset",
	"wr-offset",
};

static void help()
{
	fprintf(stderr, "wrtd-boot [options] -D 0x<hex-number> -t <path> -f <path> [-T <number>]\n");
	fprintf(stderr, "It loads thewhite-rabbit trigger-distribution application into the node-core CPUs\n\n");
	fprintf(stderr, "-D   WRNC device identificator\n");
	fprintf(stderr, "-t   path to TDC real-time application\n");
	fprintf(stderr, "-f   path to Fine-Delay real-time application\n");
	fprintf(stderr, "-o   set base offset\n");
	exit(1);
}

static inline char *stroffset(enum wrtb_boot_tdc_offset_type type) {
	return strtype[type];
}

/**
 * Get a timeoffset form the TDC driver
 */
static int partial_offset_get(unsigned int dev_id,
			      unsigned int channel,
			      int32_t *offset,
			      enum wrtb_boot_tdc_offset_type type)
{
	char path[255];
	FILE *f;
	int ret;
	uint32_t val;

	if (type == OFFSET_WR)
		sprintf(path, "/sys/bus/zio/devices/tdc-1n5c-%04x/%s",
			dev_id - WRTD_BOOT_WR_FMC_OFFSET, stroffset(type));
	else
		sprintf(path, "/sys/bus/zio/devices/tdc-1n5c-%04x/ft-ch%d/%s",
			dev_id - WRTD_BOOT_WR_FMC_OFFSET, channel + 1,
			stroffset(type));

	f = fopen(path, "r");
	if (!f)
		return -1;

	ret = fscanf(f, "%"SCNu32, &val);
	if (ret != 1) {
		fclose(f);
		if (!errno)
		errno = EINVAL;
		return -1;
	}

	fclose(f);

	*offset = (int32_t) val;
	return 0;
}


/**
 * It compute the final time stamp (as it is done on the fmc-tdc driver)
 */
static int offset_get(unsigned int dev_id, unsigned int channel, int32_t *offset)
{
	int32_t tmp;
	int err;

	fprintf(stdout, "Building timestamp offset\n");

	/* Get zero offset */
	err = partial_offset_get(dev_id, channel, &tmp, OFFSET_ZERO);
	if (err)
		return -1;
	*offset = tmp;
	fprintf(stdout, " zero offset : %"PRIi32"\n", tmp);

	/* Get White Rabbit offset */
	err = partial_offset_get(dev_id, channel, &tmp, OFFSET_WR);
	if (err)
		return -1;
	*offset -= tmp;
	fprintf(stdout, "   wr offset : %"PRIi32"\n", tmp);

	/* Get user offset */
	err = partial_offset_get(dev_id, channel, &tmp, OFFSET_USER);
	if (err)
		return -1;
	*offset += tmp;
	fprintf(stdout, " user offset : %"PRIi32"\n", tmp);

	fprintf(stdout, "final offset : %"PRIi32"\n", *offset);

	return 0;
}

int main(int argc, char *argv[])
{
	int err, cerr = 0, i, setoff = 0;
	uint32_t dev_id = 0;
	char *tdc = NULL, *fd =NULL, c;
	struct wrtd_node *wrtd;
	struct trtl_dev *trtl;
	int32_t offset;

	atexit(wrtd_exit);

	while ((c = getopt (argc, argv, "hD:t:f:o")) != -1) {
		switch (c) {
		case 'h':
		case '?':
			help();
			break;
		case 'D':
			sscanf(optarg, "0x%x", &dev_id);
			break;
		case 't':
			tdc = optarg;
			break;
		case 'f':
			fd = optarg;
			break;
		case 'o':
			setoff = 1;
			break;
		}
	}

	if (!fd || !tdc) {
		fprintf(stderr, "Missing binary file to load\n");
		exit(1);
	}

	if (!dev_id) {
		fprintf(stderr, "Invalid trtl device\n");
		exit(1);
	}

	err = wrtd_init();
	if (err) {
		fprintf(stderr, "Cannot init White Rabbit Trigger Distribution lib: %s\n",
			trtl_strerror(errno));
		exit(1);
	}

	wrtd = wrtd_open_by_fmc(dev_id);
	if (!wrtd) {
		fprintf(stderr, "Cannot open WRNC: %s\n", wrtd_strerror(errno));
		exit(1);
	}

	/* Load the application into the WRNC CPUs */
	fprintf(stdout, "Programming on TDC: %s\n", tdc);
	fprintf(stdout, "Programming on  FD: %s\n", fd);
	err =  wrtd_load_application(wrtd, tdc, fd);
	if (err) {
		fprintf(stderr, "Cannot program binary to WRNC: %s\n",
			wrtd_strerror(errno));
		exit(1);
	}

	/* Get the WRNC token */
	trtl = wrtd_get_trtl_dev(wrtd);

	fprintf(stdout, "Reboot applications\n");

	/* Start running application on TDC and FD CPUs */
	err = trtl_cpu_enable(trtl, 0);
	if (err)
		exit(1);
	err = trtl_cpu_enable(trtl, 1);
	if (err)
		exit(1);
	err = trtl_cpu_start(trtl, 0);
	if (err)
		exit(1);
	err = trtl_cpu_start(trtl, 1);
	if (err)
		exit(1);

	if (!wrtd_version_is_valid(wrtd)) {
		fprintf(stderr, "Cannot run %s: %s\n",
			basename(argv[0]), wrtd_strerror(errno));
		goto out;
	}

	if (setoff) {
		/* Inform the input real-time channels about the offset */
		for (i = 0; i < TDC_NUM_CHANNELS; ++i) {
			err = offset_get(dev_id, i, &offset);
			if (err) {
				fprintf(stderr,
					"Channel %d cannot calculate offset: %s\n",
					i, wrtd_strerror(errno));
				cerr++;
				continue;
			}
			err = wrtd_in_timebase_offset_set(wrtd, i, offset);
			if (err) {
				fprintf(stderr,
					"Channel %d cannot set offset: %s\n",
					i, wrtd_strerror(errno));
				cerr++;
				continue;
			}
		}
	}

	if (cerr)
		fprintf(stderr, "White Rabbit Trigger Distribution programmed but with %d problems\n", cerr);
	else
		fprintf(stdout,
			"White Rabbit Trigger Distribution node succesfully programmed\n");

out:
	wrtd_close(wrtd);
	exit(0);
}
