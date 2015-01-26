/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <libwrnc.h>

int main(int argc, char *argv[])
{
	uint32_t count;
	int err;

	err = wrnc_init();
	if (err) {
		fprintf(stderr, "Cannot init White Rabbit Node Core lib: %s\n",
			wrnc_strerror(errno));
		exit(1);
	}

	count = wrnc_count();
	printf("%d\n", count);

	wrnc_exit();

	return 0;
}
