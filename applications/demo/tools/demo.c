/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <libwrnc.h>
#include <libdemo.h>
#include <inttypes.h>


static void help()
{
	fprintf(stderr,
		"demo -D 0x<hex-number> -L 0x<hex-number> -l 0x<hex-number> -c <char> -s\n");
	fprintf(stderr, "-D device id\n");
	fprintf(stderr, "-l value to write into the LED register\n");
	fprintf(stderr, "-L value to write into the LEMO register\n");
	fprintf(stderr, "-d value to write into the LEMO direction register\n");
	fprintf(stderr, "-s it reports the content of LED and LEMO registers\n");
	fprintf(stderr, "-c set led color (g: green, r: red, o: orange)\n");
	fprintf(stderr, "\n");
	exit(1);
}

static void demo_print_status(struct demo_status *status)
{
	fprintf(stdout, "Status:\n");
	fprintf(stdout, "\tled\t0x%x\n", status->led);
	fprintf(stdout, "\tlemo\t0x%x\n", status->lemo);
	fprintf(stdout, "\t\tdirection\t0x%x\n", status->lemo_dir);
}

int main(int argc, char *argv[])
{
	struct demo_node *demo;
	struct demo_status status;
	uint32_t dev_id = 0;
	int led = -1, lemo = -1, lemo_dir = -1;
	char c, c_color;
	int err = 0, show_status = 0;
	enum demo_color color = DEMO_RED;

	while ((c = getopt (argc, argv, "hD:l:L:d:c:s")) != -1) {
		switch (c) {
		case 'h':
		case '?':
			help();
			break;
		case 'D':
			sscanf(optarg, "0x%x", &dev_id);
			break;
		case 'l':
			sscanf(optarg, "0x%x", &led);
			break;
		case 'L':
			sscanf(optarg, "0x%x", &lemo);
			break;
		case 'd':
			sscanf(optarg, "0x%x", &lemo_dir);
			break;
		case 'c':
			sscanf(optarg, "%c", &c_color);
			switch (c_color) {
			case 'g':
				color = DEMO_GREEN;
				break;
			case 'r':
				color = DEMO_RED;
				break;
			case 'o':
				color = DEMO_ORANGE;
				break;
			}
			break;
		case 's':
			show_status = 1;
			break;
		}
	}

	if (dev_id == 0) {
		help();
		exit(1);
	}

	atexit(demo_exit);
	err = demo_init();
	if (err) {
		fprintf(stderr, "Cannot init demo library: %s\n",
			demo_strerror(errno));
		exit(1);
	}

	demo = demo_open_by_fmc(dev_id);
	if (!demo) {
		fprintf(stderr, "Cannot open demo: %s\n", demo_strerror(errno));
		exit(1);
	}

	if (lemo_dir >= 0) {
		/* Set LEMO direction */
		err = demo_lemo_dir_set(demo, lemo_dir);
		if (err)
			fprintf(stderr, "Cannot set LEMO direction: %s\n",
				demo_strerror(errno));
	}

	if (led >= 0) {
		/* Set LED register */
		err = demo_led_set(demo, led, color);
		if (err)
			fprintf(stderr, "Cannot set LED: %s\n", demo_strerror(errno));
	}

	if (lemo >= 0) {
		/* Set LEMO register */
		err = demo_lemo_set(demo, lemo);
		if (err)
			fprintf(stderr, "Cannot set LEMO: %s\n", demo_strerror(errno));
	}

	if (show_status) {
		/* Get the current status */
		err = demo_status_get(demo, &status);
		if (err)
			fprintf(stderr, "Cannot get status: %s\n", demo_strerror(errno));
		else
			demo_print_status(&status);
	}

	demo_close(demo);

	exit(0);
}
