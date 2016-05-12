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
#include <libgen.h>
#include <libwrnc.h>
#include <libwrtd.h>
#include <inttypes.h>

#include <wrtd-internal.h>

static int wrtd_cmd_state(struct wrtd_node *wrtd, int input,
			    int argc, char *argv[]);
static int wrtd_cmd_enable(struct wrtd_node *wrtd, int input,
			   int argc, char *argv[]);
static int wrtd_cmd_disable(struct wrtd_node *wrtd, int input,
			    int argc, char *argv[]);
static int wrtd_cmd_set_dead_time(struct wrtd_node *wrtd, int input,
				  int argc, char *argv[]);
static int wrtd_cmd_set_delay(struct wrtd_node *wrtd, int input,
			      int argc, char *argv[]);
static int wrtd_cmd_set_mode(struct wrtd_node *wrtd, int input,
			     int argc, char *argv[]);
static int wrtd_cmd_assign(struct wrtd_node *wrtd, int input,
			   int argc, char *argv[]);
static int wrtd_cmd_unassign(struct wrtd_node *wrtd, int input,
			     int argc, char *argv[]);
static int wrtd_cmd_arm(struct wrtd_node *wrtd, int input,
			int argc, char *argv[]);
static int wrtd_cmd_disarm(struct wrtd_node *wrtd, int input,
			   int argc, char *argv[]);
static int wrtd_cmd_reset(struct wrtd_node *wrtd, int input,
			  int argc, char *argv[]);
static int wrtd_cmd_sw_trigger(struct wrtd_node *wrtd, int input,
			  int argc, char *argv[]);
static int wrtd_cmd_log_level(struct wrtd_node *wrtd, int input,
				  int argc, char *argv[]);

static struct wrtd_commands cmds[] = {
	{ "state", "", "shows input state",
	  wrtd_cmd_state },
	{ "enable", "", "enable the input",
	  wrtd_cmd_enable },
	{ "disable", "", "disable the input",
	  wrtd_cmd_disable },
	{ "deadtime", "<number>", "sets the dead time in pico-seconds",
	  wrtd_cmd_set_dead_time },
	{ "delay", "<number>", "sets the input delay in pico-seconds",
	  wrtd_cmd_set_delay },
	{ "mode", "<mode>", "sets triggering mode (see Trigger Modes)",
	  wrtd_cmd_set_mode },
	{ "assign", "<trig-id>", "assigns a trigger (see Trigger ID)",
	  wrtd_cmd_assign },
	{ "unassign", "", "un-assigns the currently assigned trigger",
	  wrtd_cmd_unassign },
	{ "arm", "", "arms the input",
	  wrtd_cmd_arm },
	{ "disarm", "", "disarms the input",
	  wrtd_cmd_disarm },
	{ "reset", "", "resets statistics counters",
	  wrtd_cmd_reset },
	{ "swtrig", "", "sends a software trigger",
	  wrtd_cmd_sw_trigger },
	{ "log_level", "<level>", "set logging level (see Log Levels)",
	  wrtd_cmd_log_level },
	{ NULL }
};

static void help()
{
	fprintf(stderr, "\n\n");
	fprintf(stderr, "wrtd-in-config -D 0x<hex-number> -C <string> -c <number> [cmd-options]\n\n");
	fprintf(stderr, "It configures an input channel on a White-Rabbit Trigger-Distribution node\n\n");
	fprintf(stderr, "-D\tdevice id\n");
	fprintf(stderr, "-C\tcommand name (see Available commands)\n");
	fprintf(stderr, "-c\tchannel to configure [0, %d]\n",
		TDC_NUM_CHANNELS - 1);
	fprintf(stderr, "\n\n");
	help_commands(cmds);
	fprintf(stderr, "\n\n");
	help_trig_id();
	fprintf(stderr, "\n\n");
	help_trig_mode();
	fprintf(stderr, "\n\n");
	help_log_level();
	exit(1);
}

void dump_input_state(struct wrtd_input_state *state, struct wr_timestamp current)
{
	char tmp[1024], tmp2[1024], tmp3[1024];

	if(!(state->flags & WRTD_ENABLED))
		printf("Channel %d: disabled\n", state->input );

	decode_flags(tmp,state->flags);
	printf("Channel %d state:\n", state->input);
	printf(" - Flags:                 %s\n", tmp);

	decode_mode(tmp,state->mode);
	printf(" - Mode:                  %s\n", tmp);

	format_ts(tmp, state->delay, 0);
	printf(" - Delay:                 %s\n", tmp    );
	printf(" - Tagged pulses:         %-10d\n", state->tagged_pulses );
	printf(" - Sent triggers:         %-10d\n", state->sent_triggers );

	format_id(tmp, state->assigned_id);
	printf(" - Assigned ID:           %s\n",
	       state->flags & WRTD_TRIGGER_ASSIGNED ? tmp : "none" );

	if( state-> flags & WRTD_LAST_VALID ) {
		format_ts( tmp, state->last_tagged_pulse, 1 );
		format_ago( tmp2, state->last_tagged_pulse, current );
		printf(" - Last input pulse:      %s (%s)\n", tmp, tmp2 );
	}

	if(state->sent_triggers > 0) {
		format_ts( tmp, state->last_sent.ts, 1 );
		format_id( tmp2, state->last_sent.id );
		format_ago( tmp3, state->last_sent.ts, current );
		printf(" - Last sent trigger:     %s (%s), ID: %s, SeqNo %d\n",
		       tmp, tmp3, tmp2, state->last_sent.seq);
	}

	printf(" - Dead time:             %" PRIu64 " ns\n",
	       ts_to_picos( state->dead_time ) / 1000 );

	wrtd_strlogging_full(tmp, state->log_level);
	printf(" - Log level:             %s\n", tmp);

	printf("Global Input information:\n");
	printf(" - Sent packets:          %-10d\n", state->sent_packets );
}

static int wrtd_cmd_state(struct wrtd_node *wrtd, int input,
			  int argc, char *argv[])
{
	struct wrtd_input_state state;
	int err;
	struct wr_timestamp current;

	err = wrtd_in_state_get(wrtd, input, &state);
	if (err)
		return err;

	err = wrtd_in_base_time(wrtd, &current);
    
        if(err)
    	    return err;

	dump_input_state(&state, current);
	return 0;
}
static int wrtd_cmd_enable(struct wrtd_node *wrtd, int input,
			   int argc, char *argv[])
{
	return wrtd_in_enable(wrtd, input, 1);
}
static int wrtd_cmd_disable(struct wrtd_node *wrtd, int input,
			    int argc, char *argv[])
{
	return wrtd_in_enable(wrtd, input, 0);
}

static int wrtd_cmd_set_dead_time(struct wrtd_node *wrtd, int input,
				  int argc, char *argv[])
{
	uint64_t dtime = 0;

	if (argc != 1 || argv[0] == NULL) {
		fprintf(stderr, "Missing deadtime value\n");
		return -1;
	}
	parse_delay(argv[0], &dtime);

	return wrtd_in_dead_time_set(wrtd, input, dtime);
}

static int wrtd_cmd_set_delay(struct wrtd_node *wrtd, int input,
			      int argc, char *argv[])
{
	uint64_t dtime = 0;

	if (argc != 1 || argv[0] == NULL) {
		fprintf(stderr, "Missing deadtime value\n");
		return -1;
	}
	parse_delay(argv[0], &dtime);

	return wrtd_in_delay_set(wrtd, input, dtime);
}

static int wrtd_cmd_set_mode(struct wrtd_node *wrtd, int input,
				  int argc, char *argv[])
{
	enum wrtd_trigger_mode mode;

	if (argc != 1 || argv[0] == NULL) {
		fprintf(stderr, "Missing deadtime value\n");
		return -1;
	}
        if (!strcmp("auto", argv[0])) {
		mode = WRTD_TRIGGER_MODE_AUTO;
	} else if (!strcmp("single", argv[0])) {
		mode = WRTD_TRIGGER_MODE_SINGLE;
	} else {
		fprintf(stderr, "Invalid trigger mode '%s'\n", argv[0]);
		return -1;
	}

	return wrtd_in_trigger_mode_set(wrtd, input, mode);
}

int wrtd_cmd_assign(struct wrtd_node *wrtd, int input,
		    int argc, char *argv[])
{
	struct wrtd_trig_id trig_id;
	int ret;

	if (argc != 1 || argv[0] == NULL) {
		fprintf(stderr, "Missing deadtime value\n");
		return -1;
	}

	ret = parse_trigger_id(argv[0], &trig_id);
	if (ret < 0)
		return -1;

	return wrtd_in_trigger_assign(wrtd, input, &trig_id);
}

int wrtd_cmd_unassign(struct wrtd_node *wrtd, int input,
		      int argc, char *argv[])
{
	return wrtd_in_trigger_unassign(wrtd, input);
}

int wrtd_cmd_arm(struct wrtd_node *wrtd, int input,
		    int argc, char *argv[])
{
	return wrtd_in_arm(wrtd, input, 1);
}

int wrtd_cmd_disarm(struct wrtd_node *wrtd, int input,
		      int argc, char *argv[])
{
	return wrtd_in_arm(wrtd, input, 0);
}

static int wrtd_cmd_reset(struct wrtd_node *wrtd, int input,
			  int argc, char *argv[])
{
	return wrtd_in_counters_reset(wrtd, input);
}

static int wrtd_cmd_sw_trigger(struct wrtd_node *wrtd, int input,
			  int argc, char *argv[])
{
	struct wrtd_trigger_entry ent;
	uint64_t ts;
	int ret;

	if (argc < 1 || argv[0] == NULL) {
		fprintf(stderr, "Missing ID value.\n");
		return -1;
	}

	ret = parse_trigger_id(argv[0], &ent.id);
	if (ret < 0)
		return -1;

	if (argv[1] != NULL) {
		parse_delay(argv[1], &ts);
		wrtd_pico_to_ts(&ts, &ent.ts);
	} else {
		ent.ts.seconds = 0;
		ent.ts.ticks = 100000000000ULL / 8000ULL; /* 100ms */
		ent.ts.frac = 0;
	}

	return wrtd_in_trigger_software(wrtd, &ent);
}

static int wrtd_cmd_log_level(struct wrtd_node *wrtd, int input,
				  int argc, char *argv[])
{
	int log_level;

	if (argc < 1) {
		fprintf(stderr,
			"Missing arguments: log_level <all off executed missed filtered promisc>\n");
		return -1;
	}

	parse_log_level(argv, argc, &log_level);

	return wrtd_in_log_level_set(wrtd, input, log_level);
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
		fprintf(stderr, "Cannot init White Rabbit Trigger Distribution lib: %s\n",
			wrnc_strerror(errno));
		exit(1);
	}

	wrtd = wrtd_open_by_fmc(dev_id);
	if (!wrtd) {
		fprintf(stderr, "Cannot open WRNC: %s\n", wrtd_strerror(errno));
		exit(1);
	}
	if (!wrtd_version_is_valid(wrtd)) {
		fprintf(stderr, "Cannot run %s: %s\n",
			basename(argv[0]), wrtd_strerror(errno));
		goto out;
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
	}

 out:
	wrtd_close(wrtd);
	exit(0);
}
