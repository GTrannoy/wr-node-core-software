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

#include <libmockturtle.h>
#include <libwrtd.h>
#include <wrtd-common.h>
#include <wrtd-internal.h>

void help_commands(struct wrtd_commands *cmds)
{
	int i;

	fprintf(stderr, "Available commands:\n");
	for(i = 0; cmds[i].handler; i++) {
		fprintf(stderr, "  %s %s\n\t%s\n\n",
			cmds[i].name, cmds[i].parm, cmds[i].desc);
	}
}

void help_log_level()
{
	fprintf(stderr, "Log Levels\n");
	fprintf(stderr, "You can set more than one log level. Here the list of valid log level strings:\n\n");
	fprintf(stderr, "\toff, Raw, Sent, Promiscious, Executed, Missed.\n\n");
	fprintf(stderr, "For details about their meaning refer, for example, to the library documentation.\n\n");
}

void help_trig_mode()
{
	fprintf(stderr, "Trigger Modes\n");
	fprintf(stderr, "You can active only one trigger mode at time. Following the list of valid trigger mode strings:\n\n");
	fprintf(stderr, "\tauto, single\n\n");
	fprintf(stderr, "For details about their meaning refer, for example, to the library documentation.\n\n");
}

void help_trig_id()
{
	fprintf(stderr, "Trigger ID\n");
	fprintf(stderr, "The trigger Id is made of 3 number separated by a colon\n\n");
	fprintf(stderr, "\t<number>:<number>:<number>\n\n");
	fprintf(stderr, "Looking at them from their semantic point of view:\n\n");
	fprintf(stderr, "\t<system>:<port>:<trigger>\n\n");
	fprintf(stderr, "For details about their meaning refer, for example, to the library documentation.\n\n");
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
    if( flags & WRTD_NO_WR )
        strcat(buf, "NoWRTiming ");

    l = strlen(buf);
    if(l)
        buf[l-1] = 0;
}


void decode_mode(char *buf, int mode)
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

void format_ts(char *buf, struct wr_timestamp ts, int with_seconds)
{
    uint64_t picoseconds = (uint64_t) ts.ticks * 8000 + (uint64_t)ts.frac * 8000ULL / 4096ULL;


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

void format_ago(char *buf, struct wr_timestamp ts, struct wr_timestamp current)
{
    uint64_t delta = current.seconds - ts.seconds;
    char when[16];
    if (delta < 0)
    {
	sprintf(when, "future");
	delta = -delta;
    } else {
    	sprintf(when, "past");
    }

    
    if(delta < 60)
	sprintf(buf, "%lu seconds in the %s", delta, when);
    else if (delta < 3600)
	sprintf(buf, "%lu minutes in the %s", delta/60, when);
    else if (delta < 3600*24)
	sprintf(buf, "%lu hours in the %s", delta/3600, when);
    else
	sprintf(buf, "%lu days in the %s", delta/(24*3600), when);
}

void format_id(char *buf, struct wrtd_trig_id id)
{
    sprintf( buf, "%04x:%04x:%08x", id.system, id.source_port,id.trigger);
}

uint64_t ts_to_picos(struct wr_timestamp ts)
{
    return (uint64_t) ts.seconds * 1000LL * 1000 * 1000 * 1000
            + (uint64_t) ts.ticks * 8000ULL +
            + (uint64_t) ts.frac * 8000LL / 4096LL;
}

int parse_delay(char *dly, uint64_t *delay_ps)
{
    int l = strlen(dly);
    char last;
    uint64_t mult;
    double d;

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

int parse_mode (char *mode_str, enum wrtd_trigger_mode *mode)
{
    if(!strcmp(mode_str, "auto"))
        *mode = WRTD_TRIGGER_MODE_AUTO;
    else if(!strcmp(mode_str, "single"))
        *mode = WRTD_TRIGGER_MODE_SINGLE;
    else
        return -1;

    return 0;
}

int parse_trigger_id(const char *str, struct wrtd_trig_id *id)
{
    return (sscanf(str,"%x:%x:%x", &id->system, &id->source_port, &id->trigger) == 3 ? 0 : -1);
}

int parse_log_level (char *list[], int count, int *log_level)
{
	uint32_t l = 0, tmp;

    while(count--)
    {
        if(!list[0])
            return -1;

	tmp = wrtd_strlogging_to_level(list[0]);
	if (tmp == WRTD_LOG_ALL || tmp == WRTD_LOG_NOTHING) {
		l = tmp;
		break;
	}
	l |= tmp;
        if(!strcmp(list[0], "all")) {
            l = WRTD_LOG_ALL;
            break;
        }
        list++;
    }

    *log_level = l;
    return 0;
}
