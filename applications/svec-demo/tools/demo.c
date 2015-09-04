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
		"demo -D 0x<hex-number> -L 0x<hex-number> -l 0x<hex-number> -c <char> -a <char> -s\n");
	fprintf(stderr, "-D device id\n");
	fprintf(stderr, "-l value to write into the LED register\n");
	fprintf(stderr, "-L value to write into the LEMO register\n");
	fprintf(stderr, "-d value to write into the LEMO direction register\n");
	fprintf(stderr, "-s it reports the content of LED and LEMO registers\n");
	fprintf(stderr, "-c set led color (g: green, r: red, o: orange)\n");
	fprintf(stderr, "-a set autodemo status (r: run, s: stop)\n");
	fprintf(stderr, "-v show version\n");
	fprintf(stderr, "-t send random value to the structure and read them back\n");
	fprintf(stderr, "\n");
	exit(1);
}

static void demo_print_status(struct demo_status *status)
{
	fprintf(stdout, "Status:\n");
	fprintf(stdout, "\tled\t0x%x\n", status->led);
	fprintf(stdout, "\tlemo\t0x%x\n", status->lemo);
	fprintf(stdout, "\t\tdirection\t0x%x\n", status->lemo_dir);
	fprintf(stdout, "\tautodemo\t%s\n", status->autodemo ? "run" : "stop");
}

static void demo_print_version(struct wrnc_rt_version *version)
{
	fprintf(stdout, "Version:\n");
	fprintf(stdout, "\tFPGA: 0x%x\n", version->fpga_id);
	fprintf(stdout, "\tRT: 0x%x\n", version->rt_id);
	fprintf(stdout, "\tRT Version: 0x%x\n", version->rt_version);
	fprintf(stdout, "\tGit Version: 0x%x\n", version->git_version);
}

static void demo_print_structure(struct demo_structure *test)
{
	int i;

	fprintf(stdout, "\tfield1: 0x%x\n", test->field1);
	fprintf(stdout, "\tfield2: 0x%x\n", test->field2);
	for (i = 0; i < DEMO_STRUCT_MAX_ARRAY; i++)
		fprintf(stdout, "\tarray[%d]: 0x%x\n", i, test->array[i]);
}

int main(int argc, char *argv[])
{
	struct demo_node *demo;
	struct demo_status status;
	uint32_t dev_id = 0;
	int led = -1, lemo = -1, lemo_dir = -1, i;
	char c, c_color = 0, autodemo = 0;
	int err = 0, show_status = 0, show_version = 0, structure = 0;
	enum demo_color color = DEMO_RED;
	struct wrnc_rt_version version;
	struct demo_structure test, test_rb;

	while ((c = getopt (argc, argv, "hD:l:L:d:c:sa:vt")) != -1) {
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
		case 'a':
			sscanf(optarg, "%c", &autodemo);
			break;
		case 'v':
			show_version = 1;
			break;
		case 't':
			structure = 1;
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


	/* Set autodemo status */
	if (autodemo != 0)
		demo_run_autodemo(demo, autodemo == 'r' ? 1 : 0);

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

	if (show_version) {
		err = demo_version(demo, &version);
		if (err)
			fprintf(stderr, "Cannot get version: %s\n",
				demo_strerror(errno));
		else
			demo_print_version(&version);
	}

	if (structure) {
		/* Generate random numbers (cannot use getrandom(2) because
		   of old system)*/
		uint32_t seq = 0;
		test.field1 = seq++;
		test.field2 = seq++;
		for (i = 0; i < DEMO_STRUCT_MAX_ARRAY; i++)
			test.array[i] = seq++;

		fprintf(stdout, "Generated structure:\n");
		demo_print_structure(&test);

		err = demo_test_struct_set(demo, &test);
		if (err) {
			fprintf(stderr, "Cannot set structure: %s\n",
				demo_strerror(errno));
		} else {
			err = demo_test_struct_get(demo, &test_rb);
			if (err) {
				fprintf(stderr, "Cannot get structure: %s\n",
					demo_strerror(errno));
			} else {
				if (memcmp(&test, &test_rb, sizeof(struct demo_structure))) {
					fprintf(stderr, "Got wrong structure: %s\n",
						demo_strerror(errno));
					demo_print_structure(&test_rb);
				} else {
					fprintf(stderr,
						"Structure correctly read back\n");
				}
			}
		}
	}

	demo_close(demo);

	exit(0);
}
