/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <libmockturtle.h>
#include <getopt.h>


static void help()
{
	fprintf(stderr, "\n");
	fprintf(stderr, "mockturtle-loader -D 0x<hex-number> -i <number> -f <path> [options]\n\n");
	fprintf(stderr, "It loads (or dumps) an application to a white-rabbit node-core internal CPU\n\n");
	fprintf(stderr, "-D   device identificator\n");
	fprintf(stderr, "-i   CPU index\n");
	fprintf(stderr, "-f   path to the binary to load. If the option '-d' is set, then\n");
	fprintf(stderr, "     this is where the program will store the current CPU application\n");
	fprintf(stderr, "-d   dump current application\n");
	fprintf(stderr, "-h   show this help\n");
	fprintf(stderr, "\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	int cpu_index = 0, err, dump = 0;
	uint32_t dev_id = 0, rst;
	char *file = NULL, c;
	struct trtl_dev *trtl;

	atexit(trtl_exit);

	while ((c = getopt (argc, argv, "hi:D:f:d")) != -1) {
		switch (c) {
		case 'h':
		case '?':
			help();
			break;
		case 'i':
			sscanf(optarg, "%d", &cpu_index);
			break;
		case 'f':
			file = optarg;
			break;
		case 'D':
			sscanf(optarg, "0x%x", &dev_id);
			break;
		case 'd':
			dump = 1;
			break;
		}
	}

	if (!file) {
		fprintf(stderr, "Missing binary file to load\n");
		exit(1);
	}

	if (!dev_id) {
		fprintf(stderr, "Invalid Mock Turtle device\n");
		exit(1);
	}

	err = trtl_init();
	if (err) {
		fprintf(stderr, "Cannot init Mock Turtle lib: %s\n",
			trtl_strerror(errno));
		exit(1);
	}


	trtl = trtl_open_by_fmc(dev_id);
	if (!trtl) {
		fprintf(stderr, "Cannot open Mock Turtle device: %s\n", trtl_strerror(errno));
		exit(1);
	}

	err = trtl_cpu_reset_get(trtl, &rst);
	if (err) {
		fprintf(stderr, "Cannot get current reset line status: %s\n",
			trtl_strerror(errno));
		goto out;
	}
	err = trtl_cpu_reset_set(trtl, (rst & ~(1 << cpu_index)));
	if (err) {
		fprintf(stderr, "Cannot put CPU %d in reset state: %s\n",
			cpu_index, trtl_strerror(errno));
		goto out;
	}
	if (dump) {
		/* Read the CPU application memory content to a given file */
		err = trtl_cpu_dump_application_file(trtl, cpu_index, file);
		if (err) {
			fprintf(stderr,
				"Cannot load application to CPU %d: %s\n",
				cpu_index,
				trtl_strerror(errno));
		}
	} else {
		/* Write the content of a given file to the CPU application memory */
		err = trtl_cpu_load_application_file(trtl, cpu_index, file);
		if (err) {
			fprintf(stderr,
				"Cannot load application to CPU %d: %s\n",
				cpu_index,
				trtl_strerror(errno));
		}
	}
	err = trtl_cpu_reset_set(trtl, rst);
out:
	trtl_close(trtl);

	exit(err);
}
