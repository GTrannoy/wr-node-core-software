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

static void help()
{
	fprintf(stderr, "\n");
	fprintf(stderr, "wrnc-messages-dump -D 0x<hex-number> -i <number> [options]\n\n");
	fprintf(stderr, "It dumps all messages from a given slot\n\n");
	fprintf(stderr, "-D   WRNC device identificator\n");
	fprintf(stderr, "-i   slot index\n");
	fprintf(stderr, "-n   number of message to read. The default is 0 (infinite)\n");
	fprintf(stderr, "\n");
	exit(1);
}


void dump_message(struct wrnc_dev *wrnc, unsigned int slot_index)
{
	struct wrnc_msg *wmsg;
	char str[128];
	int j;

	errno = 0;
	wmsg = wrnc_slot_receive(wrnc, slot_index);
	if (!wmsg) {
		if (errno) {
			fprintf(stderr,
				"Cannot receive from slot %d: %s\n",
				slot_index, strerror(errno));
		        exit(1);
		}
		return;
	}

	switch (wmsg->data[0]) {
        case 0xdeadbeef:
		for (j = 0; j < 128 - 1 && j < wmsg->datalen; ++j)
			str[j] = wmsg->data[j + 1];
		str[j - 1] = '\0';
		fprintf(stdout, "slot-%d : %s\n", slot_index, str);
		break;
	default:
		fprintf(stdout, "slot-%d : unknown message\n", slot_index);
		break;
	}

	free(wmsg);
}

#define MAX_DEV 4
#define MAX_SLOT 32
int main(int argc, char *argv[])
{
	unsigned int n = 0, i = 0, j, si = 0, di = 0, cnt = 0;
	unsigned int slot_index[MAX_DEV][MAX_SLOT];
	uint32_t dev_id[MAX_DEV];
	char c;
	struct wrnc_dev *wrnc[MAX_DEV];


	atexit(wrnc_exit);

	while ((c = getopt (argc, argv, "hi:D:n:")) != -1) {
		switch (c) {
		default:
			help();
			break;
		case 'i':
			if (si >= MAX_SLOT && di > 0)
				break;
			sscanf(optarg, "%d", &slot_index[di - 1][si]);
			si++;
			break;
		case 'D':
			if (di >= MAX_DEV)
				break;
			sscanf(optarg, "0x%x", &dev_id[di]);
			di++;
			si = 0;
			break;
		case 'n':
			sscanf(optarg, "%d", &n);
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

	/* Get messages */
	while((n == 0 || cnt < n) && (di > 0 && si > 0)) {
		for (i = 0; i < di; i++) {
			for (j = 0; j < si; j++) {
				dump_message(wrnc[i], slot_index[i][j]);
				cnt++;
				if (n > 0 && n < cnt)
					goto out;
			}
		}
	}

out:
	/* Close all devices */
	for (i = 0; i < di; i++)
		wrnc_close(wrnc[i]);

	exit(0);
}
