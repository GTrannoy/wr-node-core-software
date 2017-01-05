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
	fprintf(stderr, "wrnc-loader -D 0x<hex-number> -i <number> -f <path> [options]\n\n");
	fprintf(stderr, "It loads (or dumps) an application to a white-rabbit node-core internal CPU\n\n");
	fprintf(stderr, "-D   WRNC device identificator\n");
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
	uint32_t dev_id = 0;
	char *file = NULL, c;
	struct wrnc_dev *wrnc;

	atexit(wrnc_exit);

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
		fprintf(stderr, "Invalid wrnc device\n");
		exit(1);
	}

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

	if (dump) {
		/* Read the CPU application memory content to a given file */
		err = wrnc_cpu_dump_application_file(wrnc, cpu_index, file);
		if (err) {
			fprintf(stderr,
				"Cannot load application to CPU %d: %s\n",
				cpu_index,
				wrnc_strerror(errno));
		}
	} else {
		/* Write the content of a given file to the CPU application memory */
		err = wrnc_cpu_load_application_file(wrnc, cpu_index, file);
		if (err) {
			fprintf(stderr,
				"Cannot load application to CPU %d: %s\n",
				cpu_index,
				wrnc_strerror(errno));
		}
	}

	wrnc_close(wrnc);

	exit(0);
}