/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <libwrtd.h>

void help()
{
	fprintf(stderr, "wrtd-ping [options]\n");
	fprintf(stderr, "  -h             print this help\n");
	fprintf(stderr, "  -D 0x<dev_id>  device id to ping\n");
	fprintf(stderr, "  -n <num>       number of ping to perform\n");
	fprintf(stderr, "  -p <num>       ping period in micro-seconds\n");
	fprintf(stderr, "  -t             show device base time\n");
}

int main(int argc, char *argv[])
{
	struct wrtd_node *wrtd;
	uint32_t dev_id = 0, n = 1;
	uint64_t period = 0;
	struct wr_timestamp tsi, tso;
	int err, time = 0;
	char c;

	while ((c = getopt (argc, argv, "hD:n:p:t")) != -1) {
		switch (c) {
		case 'h':
		case '?':
			help();
			exit(1);
			break;
		case 'D':
			sscanf(optarg, "0x%x", &dev_id);
			break;
		case 'n':
			sscanf(optarg, "%d", &n);
			break;
		case 'p':
			sscanf(optarg, "%"SCNu64, &period);
			break;
		case 't':
			time = 1;
			break;
		}
	}

	if (dev_id == 0) {
		help();
		exit(1);
	}

	atexit(wrtd_exit);
	err = wrtd_init();
	if (err) {
		fprintf(stderr,
			"Cannot init White Rabbit Trigger Distribution lib: %s\n",
			wrtd_strerror(errno));
		exit(1);
	}

	wrtd = wrtd_open_by_fmc(dev_id);
	if (!wrtd) {
		fprintf(stderr, "Cannot open WRNC: %s\n", wrtd_strerror(errno));
		exit(1);
	}

	while (n--) {
		/* Get base time here to reduce the delay between the
		   two requests */
		if (time) {
			wrtd_in_base_time(wrtd, &tsi);
			wrtd_in_base_time(wrtd, &tso);
		}

		/* Check input */
		err = wrtd_in_ping(wrtd);
		if (err) {
			fprintf(stderr, "Cannot ping input source: %s\n",
				wrtd_strerror(errno));
		} else {
			fprintf(stdout, "input  : it is running!\n");
			if (time)
				fprintf(stdout,
					"\tbase time\ts:%"PRIu64" t:%d f:%d\n",
					tsi.seconds, tsi.ticks, tsi.frac);
		}
		/* check output */
		err = wrtd_out_ping(wrtd);
		if (err) {
			fprintf(stderr, "Cannot ping output source: %s\n",
				wrtd_strerror(errno));
		} else {
			fprintf(stdout, "output : it is running!\n");
			if (time)
				fprintf(stdout,
					"\tbase time\ts:%"PRIu64" t:%d f:%d\n",
					tso.seconds, tso.ticks, tso.frac);
		}
		fprintf(stdout, "\n");
		usleep(period);
	}

	wrtd_close(wrtd);
	exit(0);
}
