/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libmockturtle.h>
#include <getopt.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <libgen.h>


#define MAX_DEV 4
#define MAX_SLOT 32
#define MAX_CPU 8


struct trtl_thread_desc {
	struct trtl_dev *trtl;
	uint32_t dev_id;
	int cpu_index[MAX_CPU];
	int n_cpu;
	int slot_index[MAX_SLOT];
	int n_slot;
	int share;
};

static unsigned int  cnt, n, cnt_dbg, N;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static int timestamp = 0;

static void help()
{
	fprintf(stderr, "\n");
	fprintf(stderr, "mockturtle-messages -D 0x<hex-number> -i <number> [options]\n\n");
	fprintf(stderr, "It dumps all messages from a given set of white-rabbit node-core slots\n\n");
	fprintf(stderr, "-D   device identificator in hexadecimal format\n");
	fprintf(stderr, "-i   slot index\n");
	fprintf(stderr, "-n   number of total messages to read. The default is 0 (infinite)\n");
	fprintf(stderr, "-t   print message timestamp\n");
	fprintf(stderr, "-d <CPU index>  show debug messages for given CPU\n");
	fprintf(stderr, "-N   number of total debug messages\n");
	fprintf(stderr, "-Q   show debug messages for all found WRN CPUs\n");
	fprintf(stderr, "-s   share all HMQ with other users\n");
	fprintf(stderr, "-h   show this help\n");
	fprintf(stderr, "\n");
	fprintf(stderr,
		"You can dump from several devices slots, so the arguments '-D' and '-i' may appear several times. The argument '-i' refers to the previous device id declared\n\n");
	fprintf(stderr,
		"e.g. Dumping messagges from slots 2 and 3 of devices 0x0402 and 0x0382\n\n");
	fprintf(stderr,
		"        mockturtle-messages -D 0x0382 -i 2 -i 3 -D 0x0402 -i 2 -i 3\n\n");
	exit(1);
}


/**
 * It retreives a message from a given slots and it prints its content
 * @param[in] trtl device to use
 * @param[in] hmq slot to read
 */
static int dump_message(struct trtl_dev *trtl, struct trtl_hmq *hmq)
{
	struct trtl_proto_header h;
	struct trtl_msg *wmsg;
	time_t tm;
	char stime[64];
	struct tm *gm;
	int i;

	if (timestamp) {
		tm = time(NULL);
		gm = gmtime(&tm);
		strftime(stime, 64,"%T", gm);
		fprintf(stdout, "[%s] ", stime);
	}
	fprintf(stdout, "%s :", basename(hmq->syspath));
	wmsg = trtl_hmq_receive(hmq);
	if (!wmsg) {
		fprintf(stdout, " error : %s\n", trtl_strerror(errno));
		return -1;
	}

	trtl_message_unpack(wmsg, &h, NULL);

	fprintf(stdout, "\n    ---- header ----\n");
	fprintf(stdout, "    app_id 0x%x | msg_id %d | slot_io 0x%x | seq %d\n",
		  h.rt_app_id, h.msg_id, h.slot_io, h.seq);
	fprintf(stdout, "    len %d | flags 0x%x | trans 0x%x | time %d\n",
		  h.len, h.flags, h.trans, h.time);

	if (h.len) {
		uint32_t payload[h.len];

		trtl_message_unpack(wmsg, &h, payload);
		fprintf(stdout, "    ---- payload -----");
		for (i = 0; i < h.len; ++i) {
			if (i % 4 == 0)
				fprintf(stdout, "\n    %04d :", i);
			fprintf(stdout, " 0x%08x", payload[i]);
		}
		fprintf(stdout, "\n");
	}

	free(wmsg);

	return 0;
}

int print_debug(struct trtl_dbg *dbg)
{
	int n;
	char c[256];

	n = trtl_debug_message_get(dbg, c, 256);
	if (n < 0)
		return -1;
	if (strlen(c) > 0)
		return fprintf(stderr, "%s-cpu-%02d: %s\n",
			       trtl_name_get(dbg->trtl), dbg->cpu_index, c);
	return 0;
}

void *debug_thread(void *arg)
{
	struct trtl_thread_desc *th_data = arg;
	struct pollfd p[MAX_SLOT];
	struct trtl_dbg *wdbg[MAX_CPU];
	struct trtl_dev *trtl = th_data->trtl;
	int ret, i;

	if (!th_data->n_cpu)
		return NULL;

	/* Open the device */
	if (!trtl)
		trtl = trtl_open_by_fmc(th_data->dev_id);

	if (!trtl) {
		fprintf(stderr, "Cannot open TRTL: %s\n", trtl_strerror(errno));
		pthread_exit(NULL);
	}

	/* If there, open all debug channels */
	for (i = 0; i < th_data->n_cpu; i++) {
		wdbg[i] = trtl_debug_open(trtl, th_data->cpu_index[i]);
		if (!wdbg[i]) {
			fprintf(stderr, "Cannot open Mock Turtle debug channel: %s\n",
				trtl_strerror(errno));
			fprintf(stderr,
				"NOTE: the debug interface is on debugfs, be sure that is mounted\n");
			goto out;
		}
		p[i].fd = wdbg[i]->fd;
		p[i].events = POLLIN | POLLERR;
	}

	/* Start dumping messages */
	while (N == 0 || N > cnt_dbg) {
		/* Polling debug messages */
		ret = poll(p, th_data->n_cpu, 10);
		switch (ret) {
		default:
			/* Dump from the slot */
			for (i = 0; i < th_data->n_cpu; ++i) {
				if (!(p[i].revents & POLLIN))
					continue;
				ret = print_debug(wdbg[i]);
				if (ret < 0)
					goto out;
				pthread_mutex_lock(&mtx);
				cnt_dbg++;
				pthread_mutex_unlock(&mtx);
			}
			break;
		case 0:
			/* timeout */
			break;
		case -1:
			/* error */
			goto out;
			break;
		}
	}

out:
	/* Close all debug channels */
	while (--i >= 0)
	  trtl_debug_close(wdbg[i]);
	trtl_close(trtl);

	return NULL;
}

/**
 * pthread for each device. It dumps messages from slots
 * @param[in] arg a pointer to the device index
 */
void *message_thread(void *arg)
{
	struct trtl_thread_desc *th_data = arg;
	struct pollfd p[MAX_SLOT];
	struct trtl_dev *trtl = th_data->trtl;
	struct trtl_hmq *hmq[th_data->n_slot];
	int ret, err, i;

	if (!th_data->n_slot)
		return NULL;

	/* Open the device */
	if (!trtl)
		trtl = trtl_open_by_fmc(th_data->dev_id);

	if (!trtl) {
		fprintf(stderr, "Cannot open Mock Turtle device: %s\n", trtl_strerror(errno));
		pthread_exit(NULL);
	}

	/* Build the polling structures */
	for (i = 0; i < th_data->n_slot; ++i) {
		trtl_hmq_share_set(trtl, 0 , th_data->slot_index[i], 1);

		hmq[i] = trtl_hmq_open(trtl, th_data->slot_index[i],
				    TRTL_HMQ_OUTCOMING);
		if (!hmq[i]) {
			fprintf(stderr, "Cannot open HMQ: %s\n",
				trtl_strerror(errno));
			goto out;
		}
		p[i].fd = hmq[i]->fd;
		p[i].events = POLLIN | POLLERR;
	}

	/* Start dumping messages */
	while (n == 0 || n > cnt) {
		/* Polling slots */
		ret = poll(p, th_data->n_slot, 10);
		switch (ret) {
		default:
			/* Dump from the slot */
			for (i = 0; i < th_data->n_slot; ++i) {
				if (!(p[i].revents & POLLIN))
					continue;

				err = dump_message(trtl, hmq[i]);
				if (err)
					continue;
				pthread_mutex_lock(&mtx);
				cnt++;
				pthread_mutex_unlock(&mtx);
			}
			break;
		case 0:
			/* timeout */
			break;
		case -1:
			/* error */
			goto out;
		}
	}

out:
	/* Close all message slots */
	while (--i >= 0)
		trtl_hmq_close(hmq[i]);
	trtl_close(trtl);
	return NULL;
}


int main(int argc, char *argv[])
{
	struct trtl_thread_desc th_data[MAX_DEV], *last;
	unsigned long i;
	unsigned int di = 0;
	int show_all_debug = 0, share = 0;
	pthread_t tid_msg[MAX_DEV], tid_dbg[MAX_DEV];
	int err;
	char c;

	atexit(trtl_exit);

	memset(th_data, 0, sizeof(struct trtl_thread_desc) * MAX_DEV);

	while ((c = getopt (argc, argv, "Qhi:D:n:N:td:s")) != -1) {
		switch (c) {
		default:
			help();
			break;
		case 's':
			share = 1;
			break;
		case 'i':
		/* Save slot index for each device id */
			if (!last || last->n_slot >= MAX_SLOT)
				break;
			sscanf(optarg, "%d",
			       &last->slot_index[last->n_slot]);
			last->n_slot++;
			break;
		case 'D':
		/* Save device ids to use */
			if (di >= MAX_DEV)
				break;
			last = &th_data[di];
			sscanf(optarg, "0x%x", &last->dev_id);
			di++;
			break;
		case 'n':
		/* Number of total messages to dump */
			sscanf(optarg, "%d", &n);
			break;
		case 'N':
		/* Number of total messages to dump */
			sscanf(optarg, "%d", &N);
			break;
		case 't':
			timestamp = 1;
			break;
		case 'd':
			if (!last || last->n_cpu >= MAX_CPU)
				break;

			sscanf(optarg, "%d", &last->cpu_index[last->n_cpu]);
			last->n_cpu++;
			break;
		case 'Q':
			show_all_debug = 1;
			break;
		}
	}

	if (argc == 1) {
		help();
		exit(1);
	}

	err = trtl_init();
	if (err) {
		fprintf(stderr, "Cannot init White Rabbit Node Core lib: %s\n",
			trtl_strerror(errno));
		exit(1);
	}

	/* Set global sharing option */
	for (i = 0; i < di; ++i)
		th_data[i].share = share;

	if(show_all_debug) {
		char **list = trtl_list();
		unsigned int cpucount = 0;

		for (di = 0; list[di]; ++di) {
			int j;
			struct trtl_dev *trtl = trtl_open(list[di]);

			trtl_cpu_count(trtl, &cpucount);
			printf("ID %s n_cpu %d\n", list[di], cpucount);
			th_data[di].trtl = trtl;
			for(j = 0; j < cpucount; j++)
				th_data[di].cpu_index[j] = j;
			th_data[di].n_cpu = cpucount;
			th_data[di].n_slot = 0;
		}
		trtl_list_free(list);
	}

	/* Run dumping on in parallel from several devices */
	for (i = 0; i < di; i++) {
		err = pthread_create(&tid_msg[i], NULL, message_thread,
				     (void *)(&th_data[i]));
		if (err)
			fprintf(stderr,
				"Cannot create 'dump_thread' instance %ld: %s\n",
				i, strerror(errno));
	}

	/* Run dumping on in parallel from several devices */
	for (i = 0; i < di; i++) {
		err = pthread_create(&tid_dbg[i], NULL, debug_thread,
				     (void *)(&th_data[i]));
		if (err)
			fprintf(stderr,
				"Cannot create 'dump_thread' instance %ld: %s\n",
				i, strerror(errno));
	}


	/* Wait for the threads to finish */
	for (i = 0; i < di; i++)
		pthread_join(tid_msg[i], NULL);
	for (i = 0; i < di; i++)
		pthread_join(tid_dbg[i], NULL);
	exit(0);
}
