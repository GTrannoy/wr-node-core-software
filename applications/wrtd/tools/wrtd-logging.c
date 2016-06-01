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
#include <libgen.h>
#include <pthread.h>
#include <libwrtd.h>

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

struct wrtd_log_th {
	struct wrtd_node *wrtd;
	enum wrtd_core core;
	int channel;
	int n_read;
};


static void help()
{
	fprintf(stderr, "wrtd-logging -D 0x<hex-number>\n");
	fprintf(stderr, "It shows logging information coming from Real-Time applications\n");
	fprintf(stderr, "-D device id\n");
	fprintf(stderr, "-n number of messages to read (0 means infinite)\n");
	fprintf(stderr, "-s show current logging level for all channels\n");
	fprintf(stderr, "-o output channel [0, %d]\n", FD_NUM_CHANNELS - 1);
	fprintf(stderr, "-i input channel [0, %d]\n", TDC_NUM_CHANNELS - 1);
	exit(1);
}


static void *logging_thread(void *arg)
{
	struct wrtd_log_th *th_data = arg;
	struct wrnc_hmq *hlog;
	struct wrtd_log_entry log;
	int i, count;

	/* Open logging interfaces */
	switch (th_data->core) {
	case WRTD_CORE_IN:
		fprintf(stdout, "Open INPUT logging interface, channel: %d\n",
			th_data->channel);
		hlog = wrtd_in_log_open(th_data->wrtd, th_data->channel);
		break;
	case WRTD_CORE_OUT:
		fprintf(stdout, "Open OUTPUT logging interface, channel: %d\n",
			th_data->channel);
		hlog = wrtd_out_log_open(th_data->wrtd, th_data->channel);
		break;
	default:
		fprintf(stderr, "Unknow core %d\n", th_data->core);
		return NULL;
	}

	if (!hlog) {
		fprintf(stderr, "Cannot open input logging HMQ: %s\n",
			wrtd_strerror(errno));
	        return NULL;
	}

	while (i < th_data->n_read || th_data->n_read == 0) {
		count = wrtd_log_read(hlog, &log, 1, -1);
		if (count <= 0) {
			fprintf(stderr, "Cannot read message: %s\n",
				wrtd_strerror(errno));
			break;
		}

		fprintf(stdout, "Device      %s\n",
			th_data->core == WRTD_CORE_IN ? "input" : "output");
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

		pthread_mutex_lock(&mtx);
		i++;
		pthread_mutex_unlock(&mtx);
	}

	wrtd_log_close(hlog);
	return NULL;
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
	struct wrtd_log_th th_data[N_LOG];
	pthread_t tid[N_LOG];
	int i = 0, err, show_log = 0;
	uint32_t dev_id = 0;
	char c;

	th_data[0].channel = -1;
	th_data[1].channel = -1;
	while ((c = getopt (argc, argv, "hD:n:so:i:")) != -1) {
		switch (c) {
		default:
			help();
			break;
		case 'D':
			sscanf(optarg, "0x%x", &dev_id);
			break;
		case 'n':
			sscanf(optarg, "0x%x", &th_data[0].n_read);
			th_data[1].n_read = th_data[0].n_read;
			break;
		case 's':
			show_log = 1;
			break;
		case 'o':
			sscanf(optarg, "%d", &th_data[1].channel);
			break;
		case 'i':
			sscanf(optarg, "%d", &th_data[0].channel);
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

	th_data[0].wrtd = wrtd_open_by_fmc(dev_id);
	if (!th_data[0].wrtd) {
		fprintf(stderr, "Cannot open WRNC: %s\n", wrtd_strerror(errno));
		exit(1);
	}
	th_data[1].wrtd = th_data[0].wrtd;

	if (!wrtd_version_is_valid(th_data[0].wrtd)) {
		fprintf(stderr, "Cannot run %s: %s\n",
			basename(argv[0]), wrtd_strerror(errno));
		goto out;
	}

	if (show_log) {
		show_logging_level(th_data[WRTD_CORE_IN].wrtd, WRTD_CORE_IN);
		show_logging_level(th_data[WRTD_CORE_OUT].wrtd, WRTD_CORE_OUT);
		exit(0);
	}

	for (i = 0; i < N_LOG; i++) {
		th_data[i].core = i;
	        err = pthread_create(&tid[i], NULL, logging_thread, (void *)&th_data[i]);
		if (err)
			fprintf(stderr,
				"Cannot create 'logging_thread' instance %d: %s\n",
				i, strerror(errno));
	}


	/* Wait for the threads to finish */
	for (i = 0; i < N_LOG; i++)
		pthread_join(tid[i], NULL);

out:
	wrtd_close(th_data[0].wrtd);
	exit(0);
}
