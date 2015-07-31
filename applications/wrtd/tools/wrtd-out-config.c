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
static int wrtd_cmd_enable(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[]);
static int wrtd_cmd_disable(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[]);
static int wrtd_cmd_pulse_width(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[]);
static int wrtd_cmd_show(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[]);
static int wrtd_cmd_trig_assign(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[]);
static int wrtd_cmd_trig_unassign(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[]);
static int wrtd_cmd_trig_enable(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[]);
static int wrtd_cmd_trig_disable(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[]);
static int wrtd_cmd_trig_stats(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[]);
static int wrtd_cmd_trig_delay(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[]);
static int wrtd_cmd_trig_cond_delay(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[]);
static int wrtd_cmd_trig_find(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[]);
static int wrtd_cmd_arm(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[]);
static int wrtd_cmd_disarm(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[]);
static int wrtd_cmd_mode(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[]);
static int wrtd_cmd_dead_time(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[]);
static int wrtd_cmd_reset_counters(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[]);
static int wrtd_cmd_log_level(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[]);
static int wrtd_cmd_software_trigger(struct wrtd_node *wrtd, int output,
				     int argc, char *argv[]);
static int wrtd_cmq_has_trig(struct wrtd_node *wrtd, int output,
			     int argc, char *argv[]);
static struct wrtd_commands cmds[] = {
	{ "state", "", "shows output state",
	  wrtd_cmd_state },
	{ "assign", "<trig-id> [<trig-id>]", "assigns a trigger (see Trigger ID)",
	  wrtd_cmd_trig_assign },
	{ "unassign", "<trig-id>", "un-assigns a given trigger",
	  wrtd_cmd_trig_unassign },
	{ "show", "", "shows assigned triggers",
	  wrtd_cmd_show },
	{ "enable", "", "enables an output",
	  wrtd_cmd_enable },
	{ "disable", "", "disables an output",
	  wrtd_cmd_disable },
	{ "arm", "", "arms an output",
	  wrtd_cmd_arm },
	{ "disarm", "", "disarm an output",
	  wrtd_cmd_disarm },
	{ "mode", "<mode>", "set output mode output (see Trigger Modes)",
	  wrtd_cmd_mode },
	{ "dead_time", "<number>", "set output dead time in pico-seconds",
	  wrtd_cmd_dead_time },
	{ "reset_counters", "", "reset statistic counters",
	  wrtd_cmd_reset_counters },
	{ "log_level", "<level>", "set log level (see Log Levels)",
	  wrtd_cmd_log_level },
	{ "pulse_width", "<number>", "sets the output pulse width in pico-seconds",
	  wrtd_cmd_pulse_width },
	{ "sw_trig", "", "forces a software trigger",
	  wrtd_cmd_software_trigger },
	{ "has_trig", "", "return true it as a trigger assigned",
	  wrtd_cmq_has_trig},
	{ "trig_enable", "<trig-id>", "enables a particular trigger",
	  wrtd_cmd_trig_enable },
	{ "trig_disable", "<trig-id>", "disables a particular trigger",
	  wrtd_cmd_trig_disable },
	{ "trig_stats", "", "shows per-trigger statistics",
	  wrtd_cmd_trig_stats },
	{ "trig_delay", "<trig-id> <number>", "sets the delay in pico-seconds for a particular trigger ",
	  wrtd_cmd_trig_delay },
	{ "trig_cond_delay", "<trig-id> <number>", "sets the delay in pico-seconds for a particular trigger condition",
	  wrtd_cmd_trig_cond_delay },
	{ "trig_find", "<trig-id>", "retrieves a trigger entry based on its ID",
	  wrtd_cmd_trig_find },
	{ NULL }
};

static void dump_output_state(struct wrtd_output_state *state)
{
	char tmp[1024], tmp2[1024];

	if(! (state->flags & WRTD_ENABLED))
		printf("Channel %d: disabled\n", state->output);

	decode_flags(tmp, state->flags);
	printf("Output channel %d state:\n", state->output);
	printf(" - Flags:                         %s\n", tmp);
	decode_mode(tmp, state->mode);
	printf(" - Mode:                          %s\n", tmp);
	format_ts(tmp, state->pulse_width, 0);
	printf(" - Pulse width:                   %s\n", tmp);
	format_ts(tmp, state->dead_time, 1);
	printf(" - Dead time:                     %s\n", tmp);

	printf(" - Executed pulses:               %-10d\n",
		       state->executed_pulses);
	printf(" - Missed pulses (latency):       %-10d\n",
		       state->missed_pulses_late);
	printf(" - Missed pulses (dead time):     %-10d\n",
		       state->missed_pulses_deadtime);
	printf(" - Missed pulses (overflow):      %-10d\n",
		       state->missed_pulses_overflow);
	printf(" - Missed pulses (no WR timing):  %-10d\n",
		       state->missed_pulses_no_timing);

	format_ts(tmp, state->last_executed.ts, 1);
	format_id(tmp2, state->last_executed.id);
	printf(" - Last executed trigger:         %s, ID: %s, SeqNo %d\n",
		       tmp, tmp2, state->last_executed.seq);

	format_ts(tmp, state->last_enqueued.ts, 1);
	format_id(tmp2, state->last_enqueued.id);
	printf(" - Last enqueued trigger:         %s, ID: %s, SeqNo %d\n",
		       tmp, tmp2, state->last_enqueued.seq);

	format_ts(tmp, state->last_received.ts, 1);
	format_id(tmp2, state->last_received.id);
	printf(" - Last received trigger:         %s, ID: %s, SeqNo %d\n",
		       tmp, tmp2, state->last_received.seq);

	format_ts(tmp, state->last_lost.ts, 1);
	format_id(tmp2, state->last_lost.id);
	printf(" - Last missed/lost trigger:      %s, ID: %s, SeqNo %d\n",
		       tmp, tmp2, state->last_lost.seq);
	wrtd_strlogging_full(tmp, state->log_level);
	printf(" - Log level:             %s\n", tmp);

	printf("Global Output information:\n");
	printf(" - Total RX messages:             %-10d\n", state->received_messages);
	printf(" - Total loopback messages:       %-10d\n", state->received_loopback);
}

static int wrtd_cmq_has_trig(struct wrtd_node *wrtd, int output,
			     int argc, char *argv[])
{
	struct wrtd_trig_id id_t;
	unsigned int ass;
	int err;

	if (argc == 1) {
		err = parse_trigger_id(argv[0], &id_t);
		if(err)
			return err;
		err = wrtd_out_has_trigger(wrtd, output, &id_t, &ass);
	} else {
		err = wrtd_out_has_trigger(wrtd, output, NULL, &ass);
	}

	if(err)
		return err;

	if (ass)
		printf("Channel %d has %s trigger assigned\n", output,
		       argc == 1 ? argv[0] : "a");
	else
		printf("Channel %d has no trigger assigned\n", output);

	return 0;
}


static int trig_enable(struct wrtd_node *wrtd, int output,
			   int argc, char *argv[], int enable)
{
	int err;
	struct wrtd_output_trigger_state trig;
	struct wrtd_trig_id tid;

	if (argc != 1 || argv[0] == NULL) {
		fprintf(stderr,
			"Missing arguments: trig_%s <trig-id>\n", enable ? "enable" : "disable");
		return -1;
	}
	err = parse_trigger_id(argv[0], &tid);
	if (err)
		return err;

	/* Get a trigger */
	err = wrtd_out_trig_state_get_by_id(wrtd, output, &tid, &trig);
	if (err)
		return err;

	return wrtd_out_trig_enable(wrtd, &trig.handle, enable);

}

static int wrtd_cmd_trig_enable(struct wrtd_node *wrtd, int output,
			   int argc, char *argv[])
{
	return trig_enable(wrtd, output, argc, argv, 1);
}

static int wrtd_cmd_trig_disable(struct wrtd_node *wrtd, int output,
			    int argc, char *argv[])
{
	return trig_enable(wrtd, output, argc, argv, 0);
}

static int wrtd_cmd_enable(struct wrtd_node *wrtd, int output,
			   int argc, char *argv[])
{
	return wrtd_out_enable(wrtd, output, 1);
}

static int wrtd_cmd_arm(struct wrtd_node *wrtd, int output,
			   int argc, char *argv[])
{
	return wrtd_out_arm(wrtd, output, 1);
}

static int wrtd_cmd_disarm(struct wrtd_node *wrtd, int output,
			   int argc, char *argv[])
{
	return wrtd_out_arm(wrtd, output, 0);
}

static int wrtd_cmd_disable(struct wrtd_node *wrtd, int output,
			    int argc, char *argv[])
{
	return wrtd_out_enable(wrtd, output, 0);
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

static int wrtd_cmd_trig_delay(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[])
{
	struct wrtd_output_trigger_state trig;
	struct wrtd_trig_id tid;
	uint64_t dtime = 0;
	int err;

	if (argc != 2 || argv[0] == NULL || argv[1] == NULL) {
		fprintf(stderr,
			"Missing arguments: trig_delay <trig-id> <delay>\n");
		return -1;
	}

	err = parse_trigger_id(argv[0], &tid);
	if (err)
		return err;

	/* Get a trigger */
	err = wrtd_out_trig_state_get_by_id(wrtd, output, &tid, &trig);
	if (err)
		return err;

	parse_delay(argv[1], &dtime);

	return wrtd_out_trig_delay_set(wrtd, &trig.handle, dtime);
}

static int wrtd_cmd_trig_cond_delay(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[])
{
	struct wrtd_output_trigger_state trig;
	struct wrtd_trig_id tid;
	uint64_t dtime = 0;
	int err;

	if (argc != 2 || argv[0] == NULL || argv[1] == NULL) {
		fprintf(stderr,
			"Missing arguments: trig_cond_delay <trig-id> <delay>\n");
		return -1;
	}

	err = parse_trigger_id(argv[0], &tid);
	if (err)
		return err;

	/* Get a trigger */
	err = wrtd_out_trig_state_get_by_id(wrtd, output, &tid, &trig);
	if (err)
		return err;

	parse_delay(argv[1], &dtime);

	return 0;
	//return wrtd_out_trig_delay_set(wrtd, &trig.handle, dtime);
}

static int wrtd_cmd_mode(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[])
{
	enum wrtd_trigger_mode mode;

	if (argc != 1 || argv[0] == NULL) {
		fprintf(stderr,
			"Missing arguments: mode <auto/single>\n");
		return -1;
	}

	parse_mode(argv[0], &mode);

	return wrtd_out_trigger_mode_set(wrtd, output, mode);
}

static int wrtd_cmd_reset_counters(struct wrtd_node *wrtd, int output,
			   int argc, char *argv[])
{
	return wrtd_out_counters_reset(wrtd, output);
}


static int wrtd_cmd_pulse_width(struct wrtd_node *wrtd, int output,
			  int argc, char *argv[])
{
	uint64_t dtime = 0;

	if (argc != 1 || argv[0] == NULL) {
		fprintf(stderr,
			"Missing arguments: pulse_width <pulse width>\n");
		return -1;
	}

	/* Get a trigger */
	parse_delay(argv[0], &dtime);

	return wrtd_out_pulse_width_set(wrtd, output, dtime);
}

static int wrtd_cmd_dead_time(struct wrtd_node *wrtd, int output,
			  int argc, char *argv[])
{
	uint64_t dtime = 0;

	if (argc != 1 || argv[0] == NULL) {
		fprintf(stderr,
			"Missing arguments: dead_time <dead time>\n");
		return -1;
	}

	/* Get the dead time */
	parse_delay(argv[0], &dtime);

	return wrtd_out_dead_time_set(wrtd, output, dtime);
}

static int wrtd_cmd_log_level(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[])
{
	int log_level;

	if (argc < 1) {
		fprintf(stderr,
			"Missing arguments: log_level <all off executed missed filtered promisc>\n");
		return -1;
	}

	parse_log_level(argv, argc, &log_level);

	return wrtd_out_log_level_set(wrtd, output, log_level);
}

static int wrtd_cmd_software_trigger(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[])
{
	return -1;
}

static int wrtd_cmd_trig_assign(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[])
{
	struct wrtd_trigger_handle h;
	struct wrtd_trig_id id_t, id_cond;
	int err, cond = 0;

	if ((argc != 1 && argc != 2) || argv[0] == NULL) {
		fprintf(stderr, "Missing arguments: assign <trigger ID> [condition ID]\n");
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

static int wrtd_cmd_trig_unassign(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[])
{
	struct wrtd_output_trigger_state trig;
	struct wrtd_trig_id tid;
	int err;

	if (argc != 1 || argv[0] == NULL) {
		fprintf(stderr, "Missing arguments: unassign <trigger ID>\n");
		return -1;
	}

	err = parse_trigger_id(argv[0], &tid);
	if (err)
		return err;

	/* Get a trigger */
	err = wrtd_out_trig_state_get_by_id(wrtd, output, &tid, &trig);
	if (err)
		return err;

	return wrtd_out_trig_unassign(wrtd, &trig.handle);
}


/**
 * It prints trigger statistics
 */
static void wrtd_trigger_stats_print(struct wrtd_output_trigger_state *state,
				     int index, int latency_stats)
{
	char ts[1024], id[1024];

	format_ts(ts, state->delay_trig, 0);
	format_id(id, state->trigger);
	if (index >= 0)
		printf(" %-3d: ID: %s, delay: %s, enabled: %d\n",
		       index, id, ts, state->enabled );
	else
		printf(" ---: ID: %s, delay: %s, enabled: %d\n",
		       id, ts, state->enabled );

	if(state->is_conditional) {
		format_ts(ts, state->delay_cond, 0);
		format_id(id, state->condition);
		printf("     (condition ID: %s, delay: %s)\n", id, ts);
	}
	if(latency_stats)
	{
		printf("  - executed pulses:      %d\n",
		       state->executed_pulses);
		printf("  - missed pulses:        %d\n",
		       state->missed_pulses);
		printf("  - latency (worst case): %d us\n",
		       state->latency_worst_us);
		printf("  - latency (average):    %d us\n",
		       state->latency_average_us);
	}
}
static int wrtd_show_triggers(struct wrtd_node *wrtd, int output,
			    int argc, char *argv[], int latency_stats)
{
	struct wrtd_output_trigger_state trigs[256];
	int ret, i;

	ret = wrtd_out_trig_get_all(wrtd, output, trigs, 256);
	if (ret < 0)
		return -1;

	printf("Output %d: %d trigger(s) assigned\n", output, ret);
	for (i = 0; i < ret; i++)
		wrtd_trigger_stats_print(&trigs[i], i, latency_stats);

	return 0;
}


static int wrtd_cmd_show(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[])
{
	return wrtd_show_triggers (wrtd, output, argc, argv, 0);
}

static int wrtd_cmd_trig_stats(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[])
{
	return wrtd_show_triggers (wrtd, output, argc, argv, 1);
}

static int wrtd_cmd_trig_find(struct wrtd_node *wrtd, int output,
				  int argc, char *argv[])
{
	struct wrtd_output_trigger_state state;
	struct wrtd_trig_id id;
	int ret;

	if (argc != 1 || argv[0] == NULL) {
		fprintf(stderr, "Missing arguments: trig_find <trigger id>\n");
		return -1;
	}

	ret = parse_trigger_id(argv[0], &id);
	if (ret)
		return -1;

	ret = wrtd_out_trig_state_get_by_id(wrtd, output, &id, &state);
	if (ret)
		return -1;

	wrtd_trigger_stats_print(&state, -1, 1);

	return 0;
}

static void help()
{
	fprintf(stderr, "\n\n");
	fprintf(stderr, "wrtd-out-config -D 0x<hex-number> -C <string> -c <number> [cmd-options]\n\n");
	fprintf(stderr, "It configures an output channel on a White Rabbit Trigger-Distribution node\n\n");
	fprintf(stderr, "-D\tdevice id\n");
	fprintf(stderr, "-C\tcommand name (see Available commands)\n");
	fprintf(stderr, "-c\tchannel to configure [0, %d]\n",
		FD_NUM_CHANNELS - 1);
	fprintf(stderr, "\n\n");
	help_commands(cmds);
	fprintf(stderr, "\n\n");
	help_trig_id();
	/* Comple*/
	fprintf(stderr, "In order to ease the development of this tool sometimes we refer to a trigger with its index within the real time application. You can get the current trigger index from the 'trig_stats' command\n");
	fprintf(stderr, "\n\n");
	help_trig_mode();
	fprintf(stderr, "\n\n");
	help_log_level();
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
	err = wrtd_init();
	if (err) {
		fprintf(stderr, "Cannot init White Rabbit Node Core lib: %s\n",
			wrnc_strerror(errno));
		exit(1);
	}

	wrtd = wrtd_open_by_fmc(dev_id);
	if (!wrtd) {
		fprintf(stderr, "Cannot open WRNC: %s\n", wrtd_strerror(errno));
		exit(1);
	}

	for (i = 0; cmds[i].handler; i++) {
 		if(!strcmp(cmds[i].name, cmd)) {
			err = cmds[i].handler(wrtd, chan, argc - optind,
					      argv + optind);
		 	break;
 		}
	}

	if(!cmds[i].handler)
	{
		fprintf(stderr,"Unrecognized command: '%s'\n", cmd);
		exit(1);
	}

	if (err)
		fprintf(stderr, "Error while executing command '%s': %s\n",
			cmd, wrtd_strerror(errno));

	wrtd_close(wrtd);

	exit(0);
}
