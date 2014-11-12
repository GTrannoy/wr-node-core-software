/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */

#include <stdint.h>
#include <stdio.h>
#include <getopt.h>
#include <libwrnc.h>

static void help()
{
	fprintf(stderr, "\n");
	fprintf(stderr, "lswrnc [options]\n\n");
	fprintf(stderr, "It shows the current wrnc on the system\n\n");
	fprintf(stderr, "-v   show more information about a wrnc\n");
	fprintf(stderr, "-h   show this help\n");
	fprintf(stderr, "\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	const char (*list)[WRNC_NAME_LEN];
	unsigned int appid = 0, cpucount = 0;
	struct wrnc_dev *wrnc;
	uint32_t count;
	char c;
	int i, verbose = 0;

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

	wrnc_init();

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
