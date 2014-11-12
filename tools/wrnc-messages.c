/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
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


int main(int argc, char *argv[])
{
	int slot_index = 0, err;
	unsigned int n = 0, i = 0, j;
	uint32_t dev_id = 0;
	char c, str[128];
	struct wrnc_dev *wrnc;
	struct wrnc_msg *wmsg;

	atexit(wrnc_exit);

	while ((c = getopt (argc, argv, "hi:D:n:")) != -1) {
		switch (c) {
		default:
			help();
			break;
		case 'i':
			sscanf(optarg, "%d", &slot_index);
			break;
		case 'D':
			sscanf(optarg, "0x%x", &dev_id);
			break;
		case 'n':
			sscanf(optarg, "%d", &n);
			break;
		}
	}

	if (!dev_id) {
		fprintf(stderr, "Invalid wrnc device\n");
		exit(1);
	}

	wrnc_init();

	wrnc = wrnc_open_by_fmc(dev_id);
	if (!wrnc) {
		fprintf(stderr, "Cannot open WRNC: %s\n", wrnc_strerror(errno));
		exit(1);
	}

	/* Get messages */
	while(n == 0 || i < n) {
		errno = 0;
		wmsg = wrnc_slot_receive(wrnc, slot_index);
		if (!wmsg) {
			if (errno) {
				fprintf(stderr,
					"Cannot receive from slot %d: %s\n",
					slot_index, strerror(errno));
				goto out;
			}
			continue;
		}
		i++;
		if (wmsg->data[0] == 0xdeadbeef) {
			for (j = 0; j < 128 - 1 && j < wmsg->datalen; ++j)
				str[j] = wmsg->data[j + 1];
			str[j - 1] = '\0';
			fprintf(stdout, "SLOT-%d : %s\n", slot_index, str);
		}
		free(wmsg);
	}

out:
	wrnc_close(wrnc);

	exit(0);
}
