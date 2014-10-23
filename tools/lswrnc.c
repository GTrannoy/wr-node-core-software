/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */

#include <stdint.h>
#include <stdio.h>
#include <libwrnc.h>

int main(int argc, char *argv[])
{
	const char (*list)[WRNC_NAME_LEN];
	uint32_t count;
	int i;

	wrnc_init();

	count = wrnc_count();
	list = wrnc_list();
	for (i = 0; i < count; ++i) {
		fprintf(stdout, "%s\n" , list[i]);
	}

	wrnc_exit();

	return 0;
}
