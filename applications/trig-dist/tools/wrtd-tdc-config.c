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
#include <libwrtd.h>

static int wrtd_cmd_state(struct wrtd_node *wrtd, int input,
			    int argc, char *argv[]);
static int wrtd_cmd_enable(struct wrtd_node *wrtd, int input,
			   int argc, char *argv[]);
static int wrtd_cmd_disable(struct wrtd_node *wrtd, int input,
			    int argc, char *argv[]);


struct wrtd_commands {
	const char *name;
	const char *desc;
	int (*handler)(struct wrtd_node *wrtd, int input,
		       int argc, char **argv);
};

static struct wrtd_commands cmds[] = {
	{ "state", "shows input state", wrtd_cmd_state },
	{ "enable", "enable the input", wrtd_cmd_enable },
	{ "disable", "disable the input", wrtd_cmd_disable },
	/*{ "assign", "assigns a trigger", wrtd_cmd_assign },
	{ "unassign", "un-assigns the currently assigned trigger", wrtd_cmd_unassign },
	{ "delay", "sets the input delay", wrtd_cmd_set_delay },
	{ "deadtime", "sets the dead time", wrtd_cmd_set_dead_time },
	{ "mode", "sets triggering mode", wrtd_cmd_set_mode },
	{ "arm", "arms the input", wrtd_cmd_arm },
	{ "disarm", "disarms the input", wrtd_cmd_disarm },
	{ "reset", "resets statistics counters", wrtd_cmd_reset_counters },
	{ "swtrig", "sends a software trigger", wrtd_cmd_sw_trigger },
	{ "state", "shows input state", wrtd_cmd_state },*/
	{ NULL }
};

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


void decode_flags(char *buf, uint32_t flags)
{
    int l;
    strcpy(buf,"");

    if( flags & WRTD_ENABLED )
        strcat(buf, "Enabled ");
    if( flags & WRTD_TRIGGER_ASSIGNED )
        strcat(buf, "TrigAssigned ");
    if( flags & WRTD_LAST_VALID )
        strcat(buf, "LastTimestampValid ");
    if( flags & WRTD_ARMED )
        strcat(buf, "Armed ");
    if( flags & WRTD_TRIGGERED )
        strcat(buf, "Triggered ");

    l = strlen(buf);
    if(l)
        buf[l-1] = 0;
}

void decode_mode (char *buf, int mode)
{
    switch(mode)
    {
        case WRTD_TRIGGER_MODE_AUTO:
            strcpy(buf, "Auto");
            break;
        case WRTD_TRIGGER_MODE_SINGLE:
            strcpy(buf, "Single shot");
            break;
        default:
            strcpy(buf,"?");
            break;
    }
}


void decode_log_level(char *buf, uint32_t flags)
{
    strcpy(buf,"");
    if(flags == 0)
        strcpy(buf, "off");
    if (flags & WRTD_LOG_RAW)
        strcat(buf, "Raw ");
    if (flags & WRTD_LOG_SENT)
        strcat(buf, "Sent ");
    if (flags & WRTD_LOG_PROMISC)
        strcat(buf, "Promiscious ");
    if (flags & WRTD_LOG_FILTERED)
        strcat(buf, "Filtered ");
    if (flags & WRTD_LOG_EXECUTED)
        strcat(buf, "Exceuted ");
}

void format_ts( char *buf, struct wr_timestamp ts, int with_seconds )
{
    uint64_t picoseconds = (uint64_t) ts.ticks * 8000 + (uint64_t)ts.bins * 8000ULL / 4096ULL;


    if(with_seconds)
    {
        sprintf (buf, "%llu:%03llu,%03llu,%03llu ns + %3llu ps",
            (long long)(ts.seconds),
            (picoseconds / (1000LL * 1000 * 1000)),
            (picoseconds / (1000LL * 1000) % 1000),
            (picoseconds / (1000LL) % 1000),
            (picoseconds % 1000LL));
    } else {
        sprintf (buf, "%03llu,%03llu,%03llu ns + %3llu ps",
            (picoseconds / (1000LL * 1000 * 1000)),
            (picoseconds / (1000LL * 1000) % 1000),
            (picoseconds / (1000LL) % 1000),
            (picoseconds % 1000LL));
    }
}

void format_id (char *buf, struct wrtd_trig_id id )
{
    sprintf( buf, "%04x:%04x:%08x", id.system, id.source_port,id.trigger);
}

uint64_t ts_to_picos ( struct wr_timestamp ts )
{
    return (uint64_t) ts.seconds * 1000LL * 1000 * 1000 * 1000
            + (uint64_t) ts.ticks * 8000ULL +
            + (uint64_t) ts.bins * 8000LL / 4096LL;
}

void dump_input_state(struct wrtd_input_state *state)
{
	char tmp[1024], tmp2[1024];

	if(!(state->flags & WRTD_ENABLED)) {
		printf("Channel %d: disabled\n", state->input );
		return;
	}

	decode_flags(tmp,state->flags);
	printf("Channel %d state:\n", state->input);
	printf(" - Flags:                 %s\n", tmp);

	decode_mode(tmp,state->mode);
	printf(" - Mode:                  %s\n", tmp);

	format_ts(tmp, state->delay, 0);
	printf(" - Delay:                 %s\n", tmp    );
	printf(" - Tagged pulses:         %-10d\n", state->tagged_pulses );
	printf(" - Sent triggers:         %-10d\n", state->sent_triggers );
	printf(" - Sent packets:          %-10d\n", state->sent_packets );

	format_id(tmp, state->assigned_id);
	printf(" - Assigned ID:           %s\n",
	       state->flags & WRTD_TRIGGER_ASSIGNED ? tmp : "none" );

	if( state-> flags & WRTD_LAST_VALID ) {
		format_ts( tmp, state->last, 1 );
		printf(" - Last input pulse:      %s\n", tmp );
	}

	if(state->sent_triggers > 0) {
		format_ts( tmp, state->last_sent.ts, 1 );
		format_id( tmp2, state->last_sent.id );
		printf(" - Last sent trigger:     %s, ID: %s, SeqNo %d\n",
		       tmp, tmp2, state->last_sent.seq);
	}

	printf(" - Dead time:             %d ns\n",
	       ts_to_picos( state->dead_time ) / 1000 );

	decode_log_level(tmp,state->log_level);
	printf(" - Log level:             %s\n", tmp);

}

static int wrtd_cmd_state(struct wrtd_node *wrtd, int input,
			    int argc, char *argv[])
{
	struct wrtd_input_state state;
	int err;

	err = wrtd_in_state_get(wrtd, input, &state);
	if (err)
		return err;
	dump_input_state(&state);
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

int main(int argc, char *argv[])
{
	struct wrtd_node *wrtd;
	struct wrnc_dev *wrnc;
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
			err = cmds[i].handler(wrtd, chan, argc - optind - 1,
					 argv + optind + 1);
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
