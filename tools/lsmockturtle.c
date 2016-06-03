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
#include <libmockturtle.h>

static void help()
{
	fprintf(stderr, "\n");
	fprintf(stderr, "lsmockturtle [options]\n\n");
	fprintf(stderr, "It shows the current Mock Turtle available on the system.\n\n");
	fprintf(stderr, "-v   show more information\n");
	fprintf(stderr, "-h   show this help\n");
	fprintf(stderr, "\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	char **list;
	unsigned int appid = 0, cpucount = 0;
	struct trtl_dev *trtl;
	char c;
	int i, verbose = 0, err;

	atexit(trtl_exit);

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

	err = trtl_init();
	if (err) {
		fprintf(stderr, "Cannot init White Rabbit Node Core lib: %s\n",
			trtl_strerror(errno));
		exit(1);
	}

	list = trtl_list();
	if (!list)
		goto out;
	for (i = 0; list[i]; ++i) {
		fprintf(stdout, "%s\n" , list[i]);
		trtl = trtl_open(list[i]);
		if (!trtl) {
			fprintf(stderr, "Cannot open device: %s\n",
				trtl_strerror(errno));
			continue;
		}
		if (verbose == 1) {
			trtl_app_id_get(trtl, &appid);
			trtl_cpu_count(trtl, &cpucount);
			fprintf(stdout, "    Application ID: 0x%08x\n", appid);
			fprintf(stdout, "    Number of CPU: %d\n", cpucount);
		}
		trtl_close(trtl);
	}
	trtl_list_free(list);
out:
	exit(0);
}
