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
#include <libwrnc.h>
#include <libwrtd.h>

static void help()
{
	fprintf(stderr, "wrtd-boot [options] -D 0x<hex-number> -t <path> -f <path> [-T <number>]\n");
	fprintf(stderr, "It loads thewhite-rabbit trigger-distribution application into the node-core CPUs\n\n");
	fprintf(stderr, "-D   WRNC device identificator\n");
	fprintf(stderr, "-t   path to TDC real-time application\n");
	fprintf(stderr, "-f   path to Fine-Delay real-time application\n");
	fprintf(stderr, "-T   seconds to wait for White-Rabbit synchronization\n");
	exit(1);
}


/**
 * Get the white-rabbit timeoffset form the TDC driver
 */
uint64_t wb_offset_get(unsigned int dev_id, unsigned int channel, uint64_t *offset)
{
	char path[255];
	FILE *f;
	int ret;

	sprintf(path, "/sys/bus/zio/devices/tdc-1n5c-%04x/ft-ch%d/wr-offset",
		dev_id, channel + 1);

	f = fopen(path, "r");
	if (!f)
		return -1;

	ret = fscanf(f, "%"SCNx64, offset);
	if (ret != 1) {
		fclose(f);
		if (!errno)
		errno = EINVAL;
		return -1;
	}

	fclose(f);

	return 0;
}


int main(int argc, char *argv[])
{
	int err, i;
	uint32_t dev_id = 0;
	char *tdc = NULL, *fd =NULL, c;
	struct wrtd_node *wrtd;
	struct wrnc_dev *wrnc;
	unsigned long timeout = 180;
	uint64_t offset;

	atexit(wrtd_exit);

	while ((c = getopt (argc, argv, "hD:t:f:T:")) != -1) {
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
		case 'T':
			sscanf(optarg, "%lu", &timeout);
			break;
		}
	}

	if (!fd || !tdc) {
		fprintf(stderr, "Missing binary file to load\n");
		exit(1);
	}

	if (!dev_id) {
		fprintf(stderr, "Invalid wrnc device\n");
		exit(1);
	}

	err = wrtd_init();
	if (err) {
		fprintf(stderr, "Cannot init White Rabbit Trigger Distribution lib: %s\n",
			wrnc_strerror(errno));
		exit(1);
	}

	wrtd = wrtd_open_by_fmc(dev_id);
	if (!wrtd) {
		fprintf(stderr, "Cannot open WRNC: %s\n", wrtd_strerror(errno));
		exit(1);
	}

	/* Synchronize TDC and FD mezzanines */
	fprintf(stdout, "Synchronizing TDC and FD to White-Rabbit\n");
	err = wrtd_white_rabbit_sync(wrtd, timeout);
	if (err) {
		fprintf(stderr, "Cannot synchronize mezzanines: %s\n",
			wrtd_strerror(errno));
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
	wrnc = wrtd_get_wrnc_dev(wrtd);

	fprintf(stdout, "Reboot applications\n");
	/* Enable TDC and FD CPUs */
	err = wrnc_cpu_enable(wrnc, 0);
	if (err)
		exit(1);
	err = wrnc_cpu_enable(wrnc, 1);
	if (err)
		exit(1);

	/* Start running application on TDC and FD CPUs */
	err = wrnc_cpu_start(wrnc, 0);
	if (err)
		exit(1);
	err = wrnc_cpu_start(wrnc, 1);
	if (err)
		exit(1);

	/* Inform the input real-time channels about the offset */
	for (i = 0; i < TDC_NUM_CHANNELS; ++i) {
		offset = wb_offset_get(dev_id, i, &offset);
		err = wrtd_in_timebase_offset_set(wrtd, i, offset);
		if (err)
			exit(1);
	}

	wrtd_close(wrtd);

	fprintf(stdout,
		"white rabbit trigger distribution node succesfully programmed\n");
	exit(0);
}
