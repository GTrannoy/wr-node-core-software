/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <poll.h>
#include <inttypes.h>
#include <libwrnc.h>
#include <libwrtd.h>

static void help()
{
	fprintf(stderr, "wrtd-logging -D 0x<hex-number>\n");
	fprintf(stderr, "It shows logging information coming from Real-Time applications\n");
	fprintf(stderr, "-D device id\n");
	fprintf(stderr, "-n number of messages to read (0 means infinite)\n");
	fprintf(stderr, "-s show current logging level for all channels\n");
	exit(1);
}

static int print_message(struct wrnc_hmq *hmq)
{
	struct wrtd_log_entry log;
	int count;

	count = wrtd_log_read(hmq, &log, 1, 0);
	if (count <= 0)
		return -1;

	fprintf(stdout, "Event Type  %s\n", wrtd_strlogging(log.type));
	if (log.type == WRTD_LOG_PROMISC || log.channel < 0)
		fprintf(stdout, "Channel     --\n");
	else
		fprintf(stdout, "Channel     %d\n", log.channel);
	fprintf(stdout, "Miss reason %x\n", log.miss_reason);
	fprintf(stdout, "Seq         %d\n", log.seq);
	if (log.type == WRTD_LOG_RAW)
		fprintf(stdout, "Identifier  ----:----:----\n");
	else
		fprintf(stdout, "Identifier  %04x:%04x:%04x\n",
			log.id.system, log.id.source_port, log.id.trigger);
	fprintf(stdout, "Timestamp   %"PRIu64"s  %"PRIu32"tick %"PRIu32"frac\n",
		log.ts.seconds, log.ts.ticks, log.ts.frac);
	fprintf(stdout, "----\n");

	return 0;
}

static void show_logging_level(struct wrtd_node *dev, enum wrtd_core core)
{
	uint32_t log_level;
	char log_level_str[128];
	int i, err, max = core ? FD_NUM_CHANNELS : TDC_NUM_CHANNELS;

	fprintf(stdout, "%s log levels\n", core ? "Output" : "Input");
	for (i = 0; i < max; i++) {
		if (core)
			err = wrtd_out_log_level_get(dev, i, &log_level);
		else
			err = wrtd_in_log_level_get(dev, i, &log_level);
		if (err) {
			fprintf(stdout, "\tchannel %d: --- ERROR ---\n", i);
		} else {
			wrtd_strlogging_full(log_level_str, log_level);
			fprintf(stdout, "\tchannel %d: %s\n", i, log_level_str);
		}
	}
}


#define N_LOG 2
int main(int argc, char *argv[])
{
	struct wrnc_hmq *log[N_LOG];
	struct pollfd p[N_LOG];  /* each node has 2 logging channels (in, out) */
	int n = 0, i = 0, ret, k, err, chan = -1, show_log = 0;
	struct wrtd_node *wrtd;
	uint32_t dev_id = 0;
	char c;

	while ((c = getopt (argc, argv, "hD:n:s")) != -1) {
		switch (c) {
		default:
			help();
			break;
		case 'D':
			sscanf(optarg, "0x%x", &dev_id);
			break;
		case 'n':
			sscanf(optarg, "0x%x", &n);
			break;
		case 's':
			show_log = 1;
			break;
		}
	}

	if (dev_id == 0)
		help();

	atexit(wrtd_exit);
	err = wrtd_init();
	if (err) {
		fprintf(stderr, "Cannot init White Rabbit Node Core lib: %s\n",
			wrnc_strerror(errno));
		exit(1);
	}

	wrtd = wrtd_open_by_fmc(dev_id);
	if (!wrtd) {
		fprintf(stderr, "Cannot open WRNC: %s\n", wrtd_strerror(errno));
		exit(1);
	}

	if (show_log) {
		show_logging_level(wrtd, WRTD_CORE_IN);
		show_logging_level(wrtd, WRTD_CORE_OUT);
		exit(0);
	}

	/* Open logging interfaces */
	log[0] = wrtd_in_log_open(wrtd, chan);
	if (!log[0]) {
		fprintf(stderr, "Cannot open input logging HMQ: %s\n",
				wrtd_strerror(errno));
		goto out_in;
	}
	p[0].fd = log[0]->fd;
	p[0].events = POLLIN;

	log[1] = wrtd_out_log_open(wrtd, chan);
	if (!log[1]) {
		fprintf(stderr, "Cannot open output logging HMQ: %s\n",
				wrtd_strerror(errno));
		goto out_out;
	}
	p[1].fd = log[1]->fd;
	p[1].events = POLLIN;

	/* Print messages till the end */
	while (i < n || n == 0) {
		/* Polling debug messages */
		ret = poll(p, N_LOG, 10000);
		switch (ret) {
		default:
			/* Dump from the slot */
			for (k = 0; k < N_LOG; ++k) {
				if (!(p[k].revents & POLLIN))
					continue;
				ret = print_message(log[k]);
				if (!ret)
					continue;
				i++;
			}
			break;
		case 0:
			/* timeout */
			break;
		case -1:
			/* error */
			fprintf(stderr, "Cannot poll the HMQ: %s\n",
				wrtd_strerror(errno));
		        goto out_dump;
			break;
		}
	}

out_dump:
	wrtd_log_close(log[1]);
out_out:
	wrtd_log_close(log[0]);
out_in:
	wrtd_close(wrtd);
	exit(0);
}
