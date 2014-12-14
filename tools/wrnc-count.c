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
	uint32_t count;

	wrnc_init();

	count = wrnc_count();
	printf("%d\n", count);

	wrnc_exit();

	return 0;
}
