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
#include <libwrnc.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>


#define MAX_DEV 4
#define MAX_SLOT 32

static unsigned int slot_index[MAX_DEV][MAX_SLOT], idx_valid[MAX_DEV], cnt, n;
static uint32_t dev_id[MAX_DEV];
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static int timestamp = 0;

static void help()
{
	fprintf(stderr, "\n");
	fprintf(stderr, "wrnc-messages -D 0x<hex-number> -i <number> [options]\n\n");
	fprintf(stderr, "It dumps all messages from a given set of white-rabbit node-core slots\n\n");
	fprintf(stderr, "-D   WRNC device identificator in hexadecimal format\n");
	fprintf(stderr, "-i   slot index\n");
	fprintf(stderr, "-n   number of total messages to read. The default is 0 (infinite)\n");
	fprintf(stderr, "-t   print message timestamp\n");
	fprintf(stderr, "-h   show this help\n");
	fprintf(stderr, "\n");
	fprintf(stderr,
		"You can dump from several devices slots, so the arguments '-D' and '-i' may appear several times. The argument '-i' refers to the previous device id declared\n\n");
	fprintf(stderr,
		"e.g. Dumping messagges from slots 2 and 3 of devices 0x0402 and 0x0382\n\n");
	fprintf(stderr,
		"        wrnc-messages -D 0x0382 -i 2 -i 3 -D 0x0402 -i 2 -i 3\n\n");
	exit(1);
}


/**
 * It retreives a message from a given slots and it prints its content
 * @param[in] wrnc device to use
 * @param[in] slot_index index of the slot to read
 */
static int dump_message(struct wrnc_dev *wrnc, unsigned int slot_index)
{
	struct wrnc_msg *wmsg;
        time_t tm;
	char str[128], stime[64];
	struct tm *gm;
	int j;

	if (timestamp) {
		tm = time(NULL);
		gm = gmtime(&tm);
		strftime(stime, 64,"%T", gm);
		fprintf(stdout, "[%s] ", stime);
	}
	fprintf(stdout, "%s-hmq-i-%02d :", wrnc_name_get(wrnc), slot_index);
	wmsg = wrnc_slot_receive(wrnc, slot_index);
	if (!wmsg) {
		fprintf(stdout, " error : %s\n", wrnc_strerror(errno));
		return -1;
	}

	/* Print message */
	switch (wmsg->data[0]) {
        case 0xdeadbeef:
		for (j = 0; j < 128 - 1 && j < wmsg->datalen; ++j)
			str[j] = wmsg->data[j + 1];
		str[j - 1] = '\0';
		fprintf(stdout, " %s\n", str);
		break;
	default:
		fprintf(stdout, " unknown message\n");
		break;
	}

	free(wmsg);

	return 0;
}

/**
 * pthread for each device. It dumps messages from slots
 * @param[in] arg a pointer to the device index
 */
void *dump_thread(void *arg)
{
	unsigned long idx = (unsigned long)arg, i;
	struct pollfd p[MAX_SLOT];
	struct wrnc_dev *wrnc;
	int ret, err;

	/* Open the device */
	wrnc = wrnc_open_by_fmc(dev_id[idx]);
	if (!wrnc) {
		fprintf(stderr, "Cannot open WRNC: %s\n", wrnc_strerror(errno));
	        pthread_exit(NULL);
	}

	/* Build the polling structures */
	for (i = 0; i < idx_valid[idx]; ++i) {
		err = wrnc_hmq_open(wrnc, slot_index[idx][i], 0);
		if (err) {
			fprintf(stderr, "Cannot open HMQ: %s\n",
				wrnc_strerror(errno));
			goto out;
		}
		p[i].fd = slot_index[idx][i];
		p[i].events = POLLIN | POLLERR;
	}

	/* Start dumping messages */
	while (n == 0 || n > cnt) {
		/* Polling slots */
		ret = wrnc_slot_poll(wrnc, p, idx_valid[idx], 10000);
		switch (ret) {
		default:
			/* Dump from the slot */
			for (i = 0; i < idx_valid[idx]; ++i) {
				if (!(p[i].revents & POLLIN))
					continue;
				err = dump_message(wrnc, p[i].fd);
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
			pthread_exit(NULL);
			break;
		}
	}
out:
	for (i = 0; i < idx_valid[idx]; ++i)
		wrnc_hmq_close(wrnc, slot_index[idx][i], 0);
	wrnc_close(wrnc);
	return NULL;
}


int main(int argc, char *argv[])
{
	unsigned int i, di = 0;
	pthread_t tid[MAX_DEV];
	int err;
	char c;

	atexit(wrnc_exit);

	for (i = 0; i < MAX_SLOT; i++)
		idx_valid[i] = 0;

	while ((c = getopt (argc, argv, "hi:D:n:t")) != -1) {
		switch (c) {
		default:
			help();
			break;
		case 'i':
		/* Save slot index for each device id */
			if (di > 0 && idx_valid[di - 1] >= MAX_SLOT)
				break;
			sscanf(optarg, "%d",
			       &slot_index[di - 1][idx_valid[di - 1]]);
			idx_valid[di - 1]++;
			break;
		case 'D':
		/* Save device ids to use */
			if (di >= MAX_DEV)
				break;
			sscanf(optarg, "0x%x", &dev_id[di]);
			di++;
			break;
		case 'n':
		/* Number of total messages to dump */
			sscanf(optarg, "%d", &n);
			break;
		case 't':
			timestamp = 1;
			break;
		}
	}

	wrnc_init();

	/* Run dumping on in parallel from several devices */
	for (i = 0; i < di; i++) {
	        err = pthread_create(&tid[i], NULL, dump_thread, (void *)i);
		if (err)
			fprintf(stderr, "Cannot create 'dump_thread' instance %d: %s\n",
				i, strerror(errno));
	}

	/* Wait for the threads to finish */
	for (i = 0; i < di; i++)
		pthread_join(tid[i], NULL);
	exit(0);
}
