/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <libwrnc.h>

static void help()
{
	fprintf(stderr, "\n");
	fprintf(stderr, "wrnc-cpu-restart  -D 0x<hex-number> -i <number> [options]\n\n");
	fprintf(stderr, "It restarts a set of given CPUs by stopping, resetting and then start again\n\n");
	fprintf(stderr, "-D   WRNC device identificator in hexadecimal format\n");
	fprintf(stderr, "-i   cpu index\n");
	fprintf(stderr, "-h   show this help\n");
	fprintf(stderr, "\n");
	fprintf(stderr,
		"You can restart several CPUs, so the arguments '-D' and '-i' may appear several times. The argument '-i' refers to the previous device id declared\n\n");
	fprintf(stderr,
		"e.g. Restart CPUs 0 and 1 of devices 0x0402 and 0x0382\n\n");
	fprintf(stderr,
		"        wrnc-cpu-restart -D 0x0382 -i 0 -i 1 -D 0x0402 -i 0 -i 1\n\n");
	exit(1);
}


#define MAX_DEV 4
#define MAX_CPU 8
int main(int argc, char *argv[])
{
	unsigned int i = 0, j, si = 0, di = 0;
	unsigned int index[MAX_DEV][MAX_CPU];
	uint32_t dev_id[MAX_DEV];
	struct wrnc_dev *wrnc[MAX_DEV];
	char c;
	int err = 0;

	atexit(wrnc_exit);

	while ((c = getopt (argc, argv, "h:i:D:")) != -1) {
		switch (c) {
		default:
			help();
			break;
		case 'i':
		/* Save cpu index for each device id */
			if (si >= MAX_CPU && di > 0)
				break;
			sscanf(optarg, "%d", &index[di - 1][si]);
			si++;
			break;
		case 'D':
		/* Save device ids to use */
			if (di >= MAX_DEV)
				break;
			sscanf(optarg, "0x%x", &dev_id[di]);
			di++;
			si = 0;
			break;
		}
	}

	wrnc_init();
	/* Open all devices */
	for (i = 0; i < di; i++) {
		wrnc[i] = wrnc_open_by_fmc(dev_id[i]);
		if (!wrnc[i]) {
			fprintf(stderr, "Cannot open WRNC: %s\n", wrnc_strerror(errno));
			exit(1);
		}
	}

	/* Restart given CPUs */
	for (i = 0; i < di; i++) {
		for (j = 0; j < si; j++) {
			err = 0;
			err |= wrnc_cpu_stop(wrnc[i], index[i][j]);
			err |= wrnc_cpu_disable(wrnc[i], index[i][j]);
			err |= wrnc_cpu_enable(wrnc[i], index[i][j]);
			err |= wrnc_cpu_start(wrnc[i], index[i][j]);
			if (err) {
				fprintf(stderr,
					"Failed to restart CPU %d. Last error: %s\n",
					index[i][j], wrnc_strerror(errno));
			}
		}
	}

	/* Close all devices */
	for (i = 0; i < di; i++)
		wrnc_close(wrnc[i]);

        exit(0);
}
