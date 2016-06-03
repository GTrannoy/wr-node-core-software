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
#include <libgen.h>
#include <libwrtd.h>

void help()
{
	fprintf(stderr, "wrtd-ping [options]\n");
	fprintf(stderr, "  -h             print this help\n");
	fprintf(stderr, "  -D 0x<dev_id>  device id to ping\n");
	fprintf(stderr, "  -n <num>       number of ping to perform\n");
	fprintf(stderr, "  -p <num>       ping period in micro-seconds\n");
	fprintf(stderr, "  -t             show device base time\n");
	fprintf(stderr, "  -v             show device version\n");
}

static void print_time(struct wr_timestamp *t)
{
	fprintf(stdout,	"\tbase time\ts:%"PRIu64" t:%d f:%d\n",
		t->seconds, t->ticks, t->frac);
}

static void print_version(struct trtl_rt_version *v)
{
	fprintf(stdout, "\tRealTime Application Version:");
	fprintf(stdout, "\tfpga_id\t\t%x\n",
		v->fpga_id);
	fprintf(stdout, "\trt_id\t\t%x\n",
		v->rt_id);
	fprintf(stdout, "\trt_version\t\t%x\n",
		v->rt_version);
	fprintf(stdout, "\tgit_version\t\t%x\n",
		v->git_version);
}


int main(int argc, char *argv[])
{
	struct wrtd_node *wrtd;
	uint32_t dev_id = 0, n = 1;
	struct trtl_rt_version vi, vo;
	uint64_t period = 0;
	struct wr_timestamp tsi, tso;
	int err, time = 0, version = 0;
	char c;

	while ((c = getopt (argc, argv, "hD:n:p:tv")) != -1) {
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
		case 'v':
			version = 1;
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

	if (!wrtd_version_is_valid(wrtd)) {
		fprintf(stderr, "Cannot run %s: %s\n",
			basename(argv[0]), wrtd_strerror(errno));
		goto out;
	}

	while (n--) {
		/* Get base time here to reduce the delay between the
		   two requests */
		if (time) {
			wrtd_in_base_time(wrtd, &tsi);
			wrtd_out_base_time(wrtd, &tso);
		}
		if (version) {
			wrtd_in_version(wrtd, &vi);
			wrtd_out_version(wrtd, &vo);
		}

		/* Check input */
		err = wrtd_in_ping(wrtd);
		if (err) {
			fprintf(stderr, "Cannot ping input source: %s\n",
				wrtd_strerror(errno));
			goto skip_input;
		}

		fprintf(stdout, "input  : it is running!\n");
		if (time)
			print_time(&tsi);
		if (version)
			print_version(&vi);
	skip_input:
		/* check output */
		err = wrtd_out_ping(wrtd);
		if (err) {
			fprintf(stderr, "Cannot ping output source: %s\n",
				wrtd_strerror(errno));
			goto skip_output;
		}

		fprintf(stdout, "output : it is running!\n");
		if (time)
		        print_time(&tso);
		if (version)
			print_version(&vo);

	skip_output:
		fprintf(stdout, "\n");
		usleep(period);
	}

out:
	wrtd_close(wrtd);
	exit(0);
}
