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
	fprintf(stderr, "-h   show this help\n");
	fprintf(stderr, "\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	const char (*list)[WRNC_NAME_LEN];
	uint32_t count;
	char c;
	int i;

	atexit(wrnc_exit);

	while ((c = getopt (argc, argv, "h:")) != -1) {
		switch (c) {
		default:
			help();
			break;
		}
	}

	wrnc_init();

	count = wrnc_count();
	list = wrnc_list();
	for (i = 0; i < count; ++i) {
		fprintf(stdout, "%s\n" , list[i]);
	}
	free(list);

        exit(0);
}
