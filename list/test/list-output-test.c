#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "list-lib.h"

void decode_flags(char *buf, uint32_t flags)
{
    int l;

    strcpy(buf,"");

    if( flags & LIST_ENABLED )
        strcat(buf, "Enabled ");
    if( flags & LIST_TRIGGER_ASSIGNED )
        strcat(buf, "TrigAssigned ");
    if( flags & LIST_LAST_VALID )
        strcat(buf, "LastTimestampValid ");
    if( flags & LIST_ARMED )
        strcat(buf, "Armed ");
    if( flags & LIST_TRIGGERED )
        strcat(buf, "Triggered ");

    l = strlen(buf);
    if(l)
        buf[l-1] = 0;
}

void decode_mode (char *buf, int mode)
{
    switch(mode)
    {
        case LIST_TRIGGER_MODE_AUTO:
            strcpy(buf, "Auto");
            break;
        case LIST_TRIGGER_MODE_SINGLE:
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
    if (flags & LIST_LOG_RAW)
        strcat(buf, "Raw ");
    if (flags & LIST_LOG_SENT)
        strcat(buf, "Sent ");
    if (flags & LIST_LOG_PROMISC)
        strcat(buf, "Promiscious ");
    if (flags & LIST_LOG_FILTERED)
        strcat(buf, "Filtered ");
    if (flags & LIST_LOG_EXECUTED)
        strcat(buf, "Exceuted ");
}

void format_ts( char *buf, struct list_timestamp ts, int with_seconds )
{
    uint64_t picoseconds = (uint64_t) ts.cycles * 8000 + (uint64_t)ts.frac * 8000ULL / 4096ULL;


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

void format_id (char *buf, struct list_id id )
{
    sprintf( buf, "%04x:%04x:%08x", id.system, id.source_port,id.trigger);
}

uint64_t ts_to_picos ( struct list_timestamp ts )
{
    return (uint64_t) ts.seconds * 1000LL * 1000 * 1000 * 1000
            + (uint64_t) ts.cycles * 8000ULL +
            + (uint64_t) ts.frac * 8000LL / 4096LL;
}


int parse_trigger_id(const char *str, struct list_id *id)
{
    return (sscanf(str,"%i:%i:%i", &id->system, &id->source_port, &id->trigger) == 3 ? 0 : -1);
}

int parse_delay (char *dly, uint64_t *delay_ps)
{
    double d;
    int l = strlen(dly);
    char last;
    uint64_t mult;

    if(!l)
	return -1;

    last = dly[l-1];
    mult=1;

    switch(last)
    {
	case 'u': mult = 1000ULL * 1000; l--; break;
	case 'm': mult = 1000ULL * 1000 * 1000; l--; break;
	case 'n': mult = 1000ULL; l--; break;
	case 'p': mult = 1; l--; break;
	default: mult = 1; break;
    }

    dly[l] = 0;

    if( sscanf(dly, "%lf", &d) != 1)
	return -1;

    *delay_ps = (uint64_t) (d * (double) mult);

    return 0;
}

void dump_output_state ( struct list_output_state *state )
{
    char tmp[1024], tmp2[1024];

/*    if(! (state->flags & LIST_ENABLED))
    {
        printf("Channel %d: disabled\n", state->input );
        return;
    }*/

//    decode_flags(tmp,state->flags);
    printf("Output %d state:\n", state->output );
//    printf(" - Flags:                 %s\n", tmp);
//    decode_mode(tmp,state->mode);
//    printf(" - Mode:                  %s\n", tmp    );
    printf(" - Executed pulses:           %-10d\n", state->executed_pulses );
    printf(" - Missed pulses (latency):   %-10d\n", state->missed_pulses_late );
    printf(" - Missed pulses (dead time): %-10d\n", state->missed_pulses_deadtime );
    printf(" - Missed pulses (overflow):  %-10d\n", state->missed_pulses_overflow );
    format_ts( tmp, state->last_executed.ts, 1 );
    format_id( tmp2, state->last_executed.id );
    printf(" - Last executed trigger:     %s, ID: %s, SeqNo %d\n", tmp, tmp2, state->last_executed.seq);

    format_ts( tmp, state->last_programmed.ts, 1 );
    format_id( tmp2, state->last_programmed.id );
    printf(" - Last pgm	     trigger:     %s, ID: %s, SeqNo %d\n", tmp, tmp2, state->last_programmed.seq);

    format_ts( tmp, state->last_enqueued.ts, 1 );
    format_id( tmp2, state->last_enqueued.id );
    printf(" - Last enq	     trigger:     %s, ID: %s, SeqNo %d\n", tmp, tmp2, state->last_enqueued.seq);

    printf(" - Total RX packets:          %-10d\n", state->total_rx_packets);

}

struct list_node *dev;

int cmd_state(int output, int argc, char *argv[])
{
    struct list_output_state state;
    int rv = list_out_get_state(dev, output, &state);

    if(rv < 0)
    {
	fprintf(stderr, "list_out_get_state(): %s\n", strerror(-rv));
	return rv;
    }

    dump_output_state(&state);
    return 0;
}

int cmd_assign(int output, int argc, char *argv[])
{
    struct list_trigger_handle h;
    struct list_id id_t, id_cond;
    int cond = 0, rv;

    if(argc < 1)
    {
	fprintf(stderr,"assign: trigger ID expected\n");
	return -1;
    }

    if(argc >= 1 && !strcmp(argv[0],"-h"))
    {
	printf("Command: assign <trigger ID> [condition ID]\n");
	printf("Assigns a trigger ID to the input. If a condition trigger ID is specified, a conditional trigger is assigned.\n");
	return 0;
    }

    if(parse_trigger_id(argv[0], &id_t) < 0)
    {
	fprintf(stderr, "Error parsing trigger ID.\n");
	return -1;
    }

    if(argc > 1)
    {
	cond = 1;
	if(parse_trigger_id (argv[1], &id_cond) < 0)
	{
		fprintf(stderr, "Error parsing condition ID.\n");
		return -1;
	}
    }

    rv = list_out_trig_assign ( dev, &h, output, &id_t, cond ? &id_cond : NULL );

    if(rv < 0)
	fprintf(stderr, "list_out_trig_assign(): %s\n", strerror(-rv));
    return rv;
}


int cmd_show_triggers(int output, int argc, char *argv[])
{
    struct list_output_trigger_state trigs[256];
    char ts[1024], id [1024];
    int rv, i;

    if(argc >= 1 && !strcmp(argv[0],"-h"))
    {
	printf("Command: shows\n");
	printf("Shows the list of triggers assigned to the output.\n");
	return 0;
    }

    rv = list_out_trig_get_all (dev, output, trigs, 256);

    if(rv < 0)
    {
	fprintf(stderr, "list_out_trig_get_all(): %s\n", strerror(-rv));
	return -1;
    }

    if(rv == 0)
    {
	printf("Output %d: no triggers assigned\n", output );
	return 0;
    }

    printf("Output %d: %d trigger(s) assigned\n", output, rv);
    for(i = 0; i < rv ;i++)
    {
        format_ts(ts, trigs[i].delay_trig, 0);
        format_id(id, trigs[i].trigger);
	printf(" %-3d: ID: %s, delay: %s, enabled: %d\n", i, id, ts, trigs[i].enabled );
	if(trigs[i].is_conditional)
	{
            format_ts(ts, trigs[i].delay_cond, 0);
	    format_id(id, trigs[i].condition);
	    printf("     (condition ID: %s, delay: %s)\n", id, ts);
	}
    }
    return rv;
}

int get_trigger_by_index(int idx, int output, struct list_output_trigger_state *st)
{
    struct list_output_trigger_state trigs[256];
    int rv = list_out_trig_get_all (dev, output, trigs, 256);

    if(rv < 0)
    {
	fprintf(stderr, "list_out_trig_get_all(): %s\n", strerror(-rv));
	return -1;
    }

    if(idx < 0 || idx >= rv)
    {
	fprintf(stderr,"unassign: trigger index out of range\n");
	return -1;

    }

    *st = trigs[idx];
    return 0;
}

int cmd_unassign(int output, int argc, char *argv[])
{
    struct list_output_trigger_state st;
    int rv, idx;

    if(argc < 1)
    {
	fprintf(stderr,"unassign: trigger index expected\n");
	return -1;
    }

    if(argc >= 1 && !strcmp(argv[0],"-h"))
    {
	printf("Command: unassign <number>\n");
	printf("Un-assigns the trigger ID with given number in the trigger list.\n");
	return 0;
    }


    idx = atoi(argv[0]);
    rv = get_trigger_by_index(idx, output, &st);
    if(rv < 0)
	return rv;

    rv = list_out_trig_remove(dev, &st.handle);
    if(rv < 0)
	fprintf(stderr, "list_out_trig_remove(): %s\n", strerror(-rv));
    return rv;

}

int cmd_set_delay(int output, int argc, char *argv[])
{
    struct list_output_trigger_state st;
    uint64_t dly;
    int rv, idx;

    if(argc >= 1 && !strcmp(argv[0],"-h"))
    {
	printf("Command: delay <trigger index> <delay value>\n");
	printf("Sets trigger delay (0 s - 1 s). Default unit is 1 picosecond. Fractional values and SI suffixes are accepted (e.g. 1.2u, 10n, etc.)\n");
	return 0;
    }

    if(argc < 2)
    {
	fprintf(stderr,"delay: trigger index and delay value expected\n");
	return -1;
    }

    idx = atoi(argv[0]);
    rv = get_trigger_by_index(idx, output, &st);
    if(rv < 0)
	return rv;

    parse_delay(argv[1], &dly);

    rv = list_out_trig_set_delay ( dev, &st.handle, dly );
    return rv;
}

#if 0

int cmd_arm(int input, int argc, char *argv[])
{
    int rv;

    if(argc >= 1 && !strcmp(argv[0],"-h"))
    {
	printf("Command: arm\n");
	printf("Arms the input, making it ready to send triggers (depending on the selected mode).\n");
	return 0;
    }

    rv = list_in_arm ( dev, input, 1 );
    if(rv < 0)
	fprintf(stderr, "list_in_arm(): %s\n", strerror(-rv));
    return rv;
}

int cmd_disarm(int input, int argc, char *argv[])
{
    int rv;

    if(argc >= 1 && !strcmp(argv[0],"-h"))
    {
        printf("Command: disarm\n");
	printf("Disarms the input. The input will not send triggers until re-armed.\n");
	return 0;
    }

    rv = list_in_arm ( dev, input, 0 );
    if(rv < 0)
	fprintf(stderr, "list_in_arm(): %s\n", strerror(-rv));
    return rv;
}

int cmd_enable(int input, int argc, char *argv[])
{
    int rv;

    if(argc >= 1 && !strcmp(argv[0],"-h"))
    {
	printf("Command: enable\n");
	printf("Enables the TDC input.\n");
	return 0;
    }

    rv = list_in_enable ( dev, input, 1 );
    if(rv < 0)
	fprintf(stderr, "list_in_enable(): %s\n", strerror(-rv));
    return rv;
}

int cmd_disable(int input, int argc, char *argv[])
{
    int rv;

    if(argc >= 1 && !strcmp(argv[0],"-h"))
    {
	printf("Command: disable\n");
	printf("Disables the TDC input.\n");
	return 0;
    }

    rv = list_in_enable ( dev, input, 0 );

    if(rv < 0)
	fprintf(stderr, "list_in_enable(): %s\n", strerror(-rv));
    return rv;
}

int cmd_set_delay(int input, int argc, char *argv[])
{
    uint64_t dly;
    int rv;

    if(argc < 1)
    {
	fprintf(stderr,"delay: delay value expected\n");
	return -1;
    }

    if(argc >= 1 && !strcmp(argv[0],"-h"))
    {
	printf("Command: delay <delay value>\n");
	printf("Sets trigger delay (0 s - 1 s). Default unit is 1 picosecond. Fractional values and SI suffixes are accepted (e.g. 1.2u, 10n, etc.)\n");
	return 0;
    }

    parse_delay(argv[0], &dly);

    rv = list_in_set_delay ( dev, input, dly );
    if(rv < 0)
	fprintf(stderr, "list_in_set_delay(): %s\n", strerror(-rv));
    return rv;
}

int cmd_set_dead_time(int input, int argc, char *argv[])
{
    uint64_t dly;
    int rv;

    if(argc < 1)
    {
	fprintf(stderr,"deadtime: dead time value expected\n");
	return -1;
    }

    if(argc >= 1 && !strcmp(argv[0],"-h"))
    {
	printf("Command: deadtime <dead time value>\n");
	printf("Sets minumum spacing between subsequent input pulses (80 us - 1 s) - a.k.a the dead time.\n");
	printf("Default unit is 1 picosecond. Fractional values and SI suffixes are accepted (e.g. 1.2u, 10n, etc.)\n");
	return 0;
    }

    parse_delay(argv[0], &dly);

    rv = list_in_set_dead_time ( dev, input, dly );
    if(rv < 0)
	fprintf(stderr, "list_in_set_dead_time(): %s\n", strerror(-rv));
    return rv;
}

int cmd_set_mode(int input, int argc, char *argv[])
{
    int mode, rv;

    if(argc < 1)
    {
	fprintf(stderr,"mode: mode expected\n");
	return -1;
    }

    if(argc >= 1 && !strcmp(argv[0],"-h"))
    {
	printf("Command: mode <single|auto>\n");
	printf("Sets triggering mode. Single = process single pulse after arming, auto = process all subsequent pulses.\n");
	return 0;
    }

    if(!strcmp(argv[0],"single"))
	mode = LIST_TRIGGER_MODE_SINGLE;
    else if(!strcmp(argv[0],"auto"))
	mode = LIST_TRIGGER_MODE_AUTO;
    else {
	fprintf(stderr,"Unrecognized mode: %s\n", argv[0]);
	return -1;
    }

    rv = list_in_set_trigger_mode ( dev, input, mode );
    if(rv < 0)
	fprintf(stderr, "list_in_set_trigger_mode(): %s\n", strerror(-rv));
    return rv;
}

int cmd_reset_counters(int input, int argc, char *argv[])
{
    int rv;

    if(argc >= 1 && !strcmp(argv[0],"-h"))
    {
	printf("Command: reset\n");
	printf("Resets statistics counters & 'last' values\n");
	return 0;
    }

    rv = list_in_reset_counters ( dev, input );
    if(rv < 0)
	fprintf(stderr, "list_in_reset_counters(): %s\n", strerror(-rv));
    return rv;
}


int cmd_sw_trigger(int input, int argc, char *argv[])
{
    if(argc < 1)
    {
	fprintf(stderr,"swtrig: trigger ID expected\n");
	return -1;
    }

    if(argc >= 1 && !strcmp(argv[0],"-h"))
    {
	printf("Command: swtrig <trigger ID> [holdoff time]\n");
	printf("Sends a software-forced trigger message with given trigger ID, after certain holdoff time from now (default = 100ms)\n");
	return 0;
    }

    return 0;
}

#endif

struct command {
	const char *name;
	const char *desc;
	int (*handler)(int, int, char **);
};

struct command cmds[] = {
	{ "assign", "assigns a trigger", cmd_assign },
	{ "unassign", "un-assigns a given trigger", cmd_unassign },
	{ "show", "shows assigned triggers", cmd_show_triggers },
	{ "delay", "set trigger delay", cmd_set_delay },
/*	{ "cond_delay", "shows assigned triggers", cmd_set_cond_delay },
	{ "trig_enable", "enables individual triggers", cmd_trig_enable },
	{ "trig_disable", "disables individual triggers", cmd_trig_disable },

	{ "delay", "sets the input delay", cmd_set_delay },
	{ "deadtime", "sets the dead time", cmd_set_dead_time },
	{ "mode", "sets triggering mode", cmd_set_mode },
	{ "enable", "enable the input", cmd_enable },
	{ "disable", "disable the input", cmd_disable },
	{ "arm", "arms the input", cmd_arm },
	{ "disarm", "disarms the input", cmd_disarm },
	{ "reset", "resets statistics counters", cmd_reset_counters },
	{ "swtrig", "sends a software trigger", cmd_sw_trigger },*/
	{ "state", "shows output state", cmd_state },
	{ NULL }
};

int run_command ( int argc, char *argv[] )
{
	int i, output, lun;
 	char *cmd;
 	int optind = 1;

	if(argc < 2)
 	{
 		printf("Usage: %s [-l lun] <output> <command> [command paremeters]\n\n", argv[0]);
 		printf("Available commands are:\n");
 		for(i=0; cmds[i].handler; i++)
 		{
 			printf("  %-10s %s\n", cmds[i].name, cmds[i].desc);
 		}
 		exit(0);
 	}

 	lun = 0;

 	if(!strcmp(argv[1], "-l"))
 	{
 		if(argc < 3)
 		{
 			fprintf(stderr, "Missing LUN.\n");
 			exit(-1);
 		}
 		lun = atoi(argv[2]);
 		optind = 3;
 	}

 	dev = list_open_node_by_lun(lun);

 	if(!dev)
 	{
 		 fprintf(stderr, "Can't open LIST node @ LUN %d (%s)", lun, strerror(errno));
 		 exit(-1);
 	}

 	if(argc <= optind + 1)
 	{
 		fprintf(stderr, "Command & output expected.\n");
 		exit(-1);
 	}

 	output = atoi(argv[optind]);
 	cmd = argv[optind+1];

 	for(i=0; cmds[i].handler; i++)
 		if(!strcmp(cmds[i].name, cmd))
 		{
		    return cmds[i].handler(output, argc-optind-2, argv+optind+2);
 		}

	fprintf(stderr,"Unrecognized command: %s\n", cmd);
	return -1;
}


int main(int argc, char *argv[])
{
    run_command(argc, argv);

    if(!strcmp(argv[argc-1], "sl"))
	sleep(1000);

    return 0;
}
