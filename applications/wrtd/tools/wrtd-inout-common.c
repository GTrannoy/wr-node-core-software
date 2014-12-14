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

#include <libwrnc.h>
#include <libwrtd.h>
#include <wrtd-common.h>
#include <wrtd-internal.h>

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

int parse_trigger_id(const char *str, struct wrtd_trig_id *id)
{
    return (sscanf(str,"%i:%i:%i", &id->system, &id->source_port, &id->trigger) == 3 ? 0 : -1);
}
