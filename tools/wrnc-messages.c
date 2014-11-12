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
	fprintf(stderr, "wrnc-messages -D 0x<hex-number> -i <number> [options]\n\n");
	fprintf(stderr, "It dumps all messages from a given set slots\n\n");
	fprintf(stderr, "-D   WRNC device identificator in hexadecimal format\n");
	fprintf(stderr, "-i   slot index\n");
	fprintf(stderr, "-n   number of total messages to read. The default is 0 (infinite)\n");
	fprintf(stderr, "-h   show this help\n");
	fprintf(stderr, "\n");
	fprintf(stderr,
		"You can dump from several devices slots, so the arguments '-D' and '-i' may");
	fprintf(stderr,
		" appear several times. The argument '-i' refers to the previous device id declared\n\n");
	fprintf(stderr,
		"e.g. Dumping messagges from slots 2 and 3 of devices 0x0400 and 0x0380\n\n");
	fprintf(stderr,
		"        wrnc-messages -D 0x0380 -i 2 -i 3 -D 0x0400 -i 2 -i 3\n\n");
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
	char str[128];
	int j;

	/* Retreive message */
	wmsg = wrnc_slot_receive(wrnc, slot_index);
	if (!wmsg)
		return -1;

	/* Print message */
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

	return 0;
}

#define MAX_DEV 4
#define MAX_SLOT 32
int main(int argc, char *argv[])
{
	unsigned int n = 0, i = 0, j, si = 0, di = 0, cnt = 0;
	unsigned int slot_index[MAX_DEV][MAX_SLOT];
	uint32_t dev_id[MAX_DEV];
	int err;
	char c;
	struct wrnc_dev *wrnc[MAX_DEV];


	atexit(wrnc_exit);

	while ((c = getopt (argc, argv, "hi:D:n:")) != -1) {
		switch (c) {
		default:
			help();
			break;
		case 'i':
		/* Save slot index for each device id */
			if (si >= MAX_SLOT && di > 0)
				break;
			sscanf(optarg, "%d", &slot_index[di - 1][si]);
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
		case 'n':
		/* Number of total messages to dump */
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

	/* Get messages from all devices slots */
	while((n == 0 || cnt < n) && (di > 0 && si > 0)) {
		for (i = 0; i < di; i++) {
			for (j = 0; j < si; j++) {
				err = dump_message(wrnc[i], slot_index[i][j]);
				if (err) {
					fprintf(stderr,
						"Cannot receive from slot %d: %s\n",
						slot_index, strerror(errno));
					goto out;
				}
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
