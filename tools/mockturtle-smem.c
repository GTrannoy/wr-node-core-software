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
#include <time.h>

#define MAX_DEV 4
#define MAX_SLOT 32

static void help()
{
	fprintf(stderr, "\n");
	fprintf(stderr, "wrnc-smem -D 0x<hex-number> -a 0x<number> [options] [value]\n\n");
	fprintf(stderr, "This program reads/write to/from the white-rabbit node-core shared memory. If you privide data, it will write these data into the shared memory. Then, it dumps the content after the write operation (if any). If you do not provide any data, it will simply read the current content of the shared memory\n\n");
	fprintf(stderr, "-D   WRNC device identificator in hexadecimal format\n");
	fprintf(stderr, "-a   address where start operation\n");
	fprintf(stderr, "-n   number of word (32bit) to read/write. The default is 1\n");
	fprintf(stderr, "-m   write operation mode. The default is 0\n");
	fprintf(stderr, "     raw operations\n");
	fprintf(stderr, "       0 direct mode\n");
	fprintf(stderr, "     atomic operations\n");
	fprintf(stderr, "       1 on write the given value will be ORed with the memory content\n");
	fprintf(stderr, "       2 on write the given value will be ANDed with the complement of the memory content\n");
	fprintf(stderr, "       3 on write the given value will be XORed with the memory content\n");
	fprintf(stderr, "       4 on write the given value will be ADDed to the memory content\n");
	fprintf(stderr, "-h   show this help\n");
	fprintf(stderr, "\n");
	exit(1);
}

uint64_t hex_to_int(char **argv, int index)
{
	uint64_t val;
	char *end;

	val = strtol(argv[index], &end, 16);
	if (end && *end) {
		fprintf(stderr, "\"%s\" is not an hex number\n", argv[index]);
		exit(1);
	}

	return val;
}

int main(int argc, char *argv[])
{
	unsigned int i, n = 1, mod = 0;
	uint32_t addr, *val, dev_id;
	int err, do_write;
	struct wrnc_dev *wrnc;
	char c;

	atexit(wrnc_exit);

	while ((c = getopt (argc, argv, "hD:a:n:m:v")) != -1) {
		switch (c) {
		default:
			help();
			break;
		case 'a':
			sscanf(optarg, "0x%x", &addr);
			break;
		case 'D':
			sscanf(optarg, "0x%x", &dev_id);
			break;
		case 'n':
			sscanf(optarg, "%d", &n);
			break;
		case 'm':
		        sscanf(optarg, "%d", &mod);
			if (mod > WRNC_SMEM_ADD) {
				fprintf(stderr, "Invalid operation mode\n");
				help();
				exit(1);
			}
			break;
		}
	}
	do_write = (optind != argc);
	printf("write? %d - %d %d\n", do_write, optind, argc);

	err = wrnc_init();
	if (err) {
		fprintf(stderr, "Cannot init White Rabbit Node Core lib: %s\n",
			wrnc_strerror(errno));
		exit(1);
	}

	wrnc = wrnc_open_by_fmc(dev_id);
	if (!wrnc) {
		fprintf(stderr, "Cannot open WRNC: %s\n", wrnc_strerror(errno));
		exit(1);
	}

	val = malloc(sizeof(uint32_t) * n);
	if (!val) {
		fprintf(stderr, "Cannot allocate memory\n");
		exit(1);
	}

	if (do_write) {
		for (i = 0; optind + i < argc && i < n; i++) {
			val[i] = hex_to_int(argv, optind + i);
		}

		err = wrnc_smem_write(wrnc, addr, val, n, mod);
	} else {
		err = wrnc_smem_read(wrnc, addr, val, n, mod);
				printf("%s:%d\n", __func__, __LINE__);
	}

	if (err) {
		fprintf(stderr, "Cannot do IO on shared memory: %s\n",
			wrnc_strerror(errno));
		fprintf(stderr, "Attention maybe the memory was partially written\n");
	        exit(1);
	}

	/* Show the current status of the shared memory */
	for (i = 0; i< n; i++)
		fprintf(stdout, "(0x%x) = 0x%x\n", addr + (i * 4), val[i]);

	free(val);
	exit(0);
}
