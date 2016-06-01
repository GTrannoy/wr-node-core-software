/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <libmockturtle.h>

int main(int argc, char *argv[])
{
	uint32_t count;
	int err;

	err = trtl_init();
	if (err) {
		fprintf(stderr, "Cannot init White Rabbit Node Core lib: %s\n",
			trtl_strerror(errno));
		exit(1);
	}

	count = trtl_count();
	printf("%d\n", count);

	trtl_exit();

	return 0;
}
