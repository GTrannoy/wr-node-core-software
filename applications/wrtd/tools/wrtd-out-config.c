/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * License: GPL v3
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <libwrnc.h>
#include <libwrtd.h>

#include <wrtd-internal.h>

static int wrtd_cmd_state(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[]);
static int wrtd_cmd_delay(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[]);
static int wrtd_cmd_assign(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[]);
static int wrtd_cmd_unassign(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[]);
static int wrtd_cmd_show(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[]);

static struct wrtd_commands cmds[] = {
	{ "state", "shows output state", wrtd_cmd_state },
	{ "delay", "sets the out put delay", wrtd_cmd_delay },
	{ "assign", "assigns a trigger", wrtd_cmd_assign },
	{ "unassign", "un-assigns a given trigger", wrtd_cmd_unassign },
	{ "show", "shows assigned triggers", wrtd_cmd_show },
	{ NULL }
};

static void dump_output_state(struct wrtd_output_state *state)
{
	char tmp[1024], tmp2[1024];

	if(! (state->flags & WRTD_ENABLED))
        {
		printf("Channel %d: disabled\n", state->output);
    		return;
	}

	decode_flags(tmp, state->flags);
	printf("Output %d state:\n", state->output);
    	printf(" - Flags:                 %s\n", tmp);
	decode_mode(tmp, state->mode);
	printf(" - Mode:                  %s\n", tmp);
	printf(" - Executed pulses:           %-10d\n",
		       state->executed_pulses);
	printf(" - Missed pulses (latency):   %-10d\n",
		       state->missed_pulses_late);
	printf(" - Missed pulses (dead time): %-10d\n",
		       state->missed_pulses_deadtime);
	printf(" - Missed pulses (overflow):  %-10d\n",
		       state->missed_pulses_overflow);
	printf(" - Missed pulses (no WR timing):  %-10d\n",
		       state->missed_pulses_no_timing);

	format_ts(tmp, state->last_executed.ts, 1);
	format_id(tmp2, state->last_executed.id);
	printf(" - Last executed trigger:     %s, ID: %s, SeqNo %d\n",
		       tmp, tmp2, state->last_executed.seq);

	format_ts(tmp, state->last_enqueued.ts, 1);
	format_id(tmp2, state->last_enqueued.id);
	printf(" - Last enqueued trigger:     %s, ID: %s, SeqNo %d\n",
		       tmp, tmp2, state->last_enqueued.seq);

	format_ts(tmp, state->last_received.ts, 1);
	format_id(tmp2, state->last_received.id);
	printf(" - Last received trigger:     %s, ID: %s, SeqNo %d\n",
		       tmp, tmp2, state->last_received.seq);

	format_ts(tmp, state->last_lost.ts, 1);
	format_id(tmp2, state->last_lost.id);
	printf(" - Last missed/lost trigger:  %s, ID: %s, SeqNo %d\n",
		       tmp, tmp2, state->last_lost.seq);

	printf(" - Total RX messages:          %-10d\n", state->rx_packets);
	printf(" - Total loopback messages:    %-10d\n", state->rx_loopback);

}

static int wrtd_cmd_state(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[])
{
	struct wrtd_output_state state;
	int err;

	err = wrtd_out_state_get(wrtd, output, &state);

    	if(err)
		return err;

	dump_output_state(&state);
	return 0;
}

static int wrtd_cmd_delay(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[])
{
	struct wrtd_output_trigger_state trig;
	uint64_t dtime = 0;
	int index, err;

	if (argc != 2 || argv[0] == NULL || argv[1] == NULL) {
		fprintf(stderr,
			"Missing arguments: delay <trig-index> <delay>\n");
		return -1;
	}
	index = atoi(argv[0]);

	/* Get a trigger */
	err = wrtd_out_trig_get_by_index(wrtd, index, output, &trig);
	if (err)
		return err;

	parse_delay(argv[1], &dtime);

	return wrtd_out_trig_delay_set(wrtd, &trig.handle, dtime);
}

static int wrtd_cmd_assign(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[])
{
	struct wrtd_trigger_handle h;
	struct wrtd_trig_id id_t, id_cond;
	int err, cond = 0;

	if ((argc != 1 && argc != 2) || argv[0] == NULL) {
		fprintf(stderr, "Missing arguments\n");
		return -1;
	}

	err = parse_trigger_id(argv[0], &id_t);
   	if(err)
		return err;

	if (argc == 2) {
		cond = 1 ;
		err = parse_trigger_id(argv[1], &id_cond);
   		if(err)
			return err;
	}

	return wrtd_out_trig_assign(wrtd, output, &h, &id_t, cond ? &id_cond : NULL);
}

static int wrtd_cmd_unassign(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[])
{
	struct wrtd_output_trigger_state trig;
	int index, err;

	if (argc != 1 || argv[0] == NULL) {
		fprintf(stderr, "Missing deadtime value\n");
		return -1;
	}
	index = atoi(argv[0]);

	/* Get a trigger */
	err = wrtd_out_trig_get_by_index(wrtd, index, output, &trig);
	if (err)
		return err;

	return wrtd_out_trig_unassign(wrtd, &trig.handle);
}

static int wrtd_cmd_show(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[])
{
	struct wrtd_output_trigger_state trigs[256];
	char ts[1024], id [1024];
	int ret, i;


	ret = wrtd_out_trig_get_all(wrtd, output, trigs, 256);
	if (ret < 0)
		return -1;

	printf("Output %d: %d trigger(s) assigned\n", output, ret);
	for (i = 0; i < ret; i++) {
		format_ts(ts, trigs[i].delay_trig, 0);
		format_id(id, trigs[i].trigger);
		printf(" %-3d: ID: %s, delay: %s, enabled: %d\n", i, id, ts, trigs[i].enabled );
		if(trigs[i].is_conditional) {
			format_ts(ts, trigs[i].delay_cond, 0);
			format_id(id, trigs[i].condition);
			printf("     (condition ID: %s, delay: %s)\n", id, ts);
		}
	}

	return 0;
}

static void help()
{
	int i;

	fprintf(stderr, "wrtd-tdc-config -D 0x<hex-number> -C <string> -c <number> [cmd-options]\n");
	fprintf(stderr, "It configures a channel of a TDC on a white-rabbit trigger distribution node\n");
	fprintf(stderr, "-D device id\n");
	fprintf(stderr, "-C command name\n");
	fprintf(stderr, "-c channel to configure\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Available commands:\n");
	for(i = 0; cmds[i].handler; i++) {
		fprintf(stderr, "  %-10s %s\n", cmds[i].name, cmds[i].desc);
	}
	exit(1);
}

int main(int argc, char *argv[])
{
	struct wrtd_node *wrtd;
	uint32_t dev_id = 0;
	char *cmd, c;
	int err = 0, i, chan = -1;

	while ((c = getopt (argc, argv, "hD:c:C:")) != -1) {
		switch (c) {
		case 'h':
		case '?':
			help();
			break;
		case 'D':
			sscanf(optarg, "0x%x", &dev_id);
			break;
		case 'c':
		        sscanf(optarg, "%d", &chan);
			break;
		case 'C':
		        cmd = optarg;
			break;
		}
	}

	if (dev_id == 0 || !cmd || chan == -1) {
		help();
		exit(1);
	}

	atexit(wrtd_exit);
	wrtd_init();

	wrtd = wrtd_open_by_fmc(dev_id);
	if (!wrtd) {
		fprintf(stderr, "Cannot open WRNC: %s\n", wrtd_strerror(errno));
		exit(1);
	}

	for (i = 0; cmds[i].handler; i++) {
 		if(!strcmp(cmds[i].name, cmd)) {
			err = cmds[i].handler(wrtd, chan, argc - optind,
					      argv + optind);
		   if (err)
			   break;
 		}
	}

	if (err) {
		fprintf(stderr, "Error while executing command '%s': %s\n",
			cmd, wrtd_strerror(errno));
	} else {
		fprintf(stdout, "Command executed!\n");
	}

	wrtd_close(wrtd);

	exit(0);
}
