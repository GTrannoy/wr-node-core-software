/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <libwrnc.h>

static void help()
{
	fprintf(stderr, "\n");
	fprintf(stderr, "lswrnc [options]\n\n");
	fprintf(stderr, "It shows the current white-rabbit node-core available on the system.\n\n");
	fprintf(stderr, "-v   show more information about a wrnc\n");
	fprintf(stderr, "-h   show this help\n");
	fprintf(stderr, "\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	char (*list)[WRNC_NAME_LEN];
	unsigned int appid = 0, cpucount = 0;
	struct wrnc_dev *wrnc;
	uint32_t count;
	char c;
	int i, verbose = 0, err;

	atexit(wrnc_exit);

	while ((c = getopt (argc, argv, "h:v")) != -1) {
		switch (c) {
		default:
			help();
			break;
		case 'v':
			verbose++;
			break;
		}
	}

	err = wrnc_init();
	if (err) {
		fprintf(stderr, "Cannot init White Rabbit Node Core lib: %s\n",
			wrnc_strerror(errno));
		exit(1);
	}

	count = wrnc_count();
	list = wrnc_list();
	for (i = 0; i < count; ++i) {
		fprintf(stdout, "%s\n" , list[i]);
		wrnc = wrnc_open(list[i]);
		if (verbose == 1) {
			wrnc_app_id_get(wrnc, &appid);
			wrnc_cpu_count(wrnc, &cpucount);
			fprintf(stdout, "    Application ID: 0x%08x\n", appid);
			fprintf(stdout, "    Number of CPU: %d\n", cpucount);
		}
		wrnc_close(wrnc);
	}
	free(list);

        exit(0);
}
