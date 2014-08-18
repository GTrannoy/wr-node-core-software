#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "wrn-lib.h"
#include "list-lib.h"


void decode_flags(char *buf, uint32_t flags)
{
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

    int l = strlen(buf);
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

void dump_input_state ( struct list_input_state *state )
{
    char tmp[1024], tmp2[1024];

    decode_flags(tmp,state->flags);
    printf("Channel %d state:\n", state->input );
    printf(" - Flags:                 %s\n", tmp);
    decode_mode(tmp,state->mode);
    printf(" - Mode:                  %s\n", tmp    );
    format_ts ( tmp, state->delay, 0 );
    printf(" - Delay:                 %s\n", tmp    );
    printf(" - Tagged pulses:         %-10d\n", state->tagged_pulses );
    printf(" - Sent triggers:         %-10d\n", state->sent_triggers );
    printf(" - Sent packets:          %-10d\n", state->sent_packets );
    format_id(tmp, state->assigned_id);
    printf(" - Assigned ID:           %s\n", state->flags & LIST_TRIGGER_ASSIGNED ? tmp : "none" );

    if( state-> flags & LIST_LAST_VALID )
    {
        format_ts( tmp, state->last, 1 );
        printf(" - Last input pulse:      %s\n", tmp );
    }

    if(state->sent_triggers > 0)
    {
        format_ts( tmp, state->last_sent.ts, 1 );
        format_id( tmp2, state->last_sent.id );
        printf(" - Last sent trigger:     %s, ID: %s, SeqNo %d\n", tmp, tmp2, state->last_sent.seq);
        
    }

    printf(" - Dead time:             %d ns\n", ts_to_picos( state->dead_time ) / 1000 );
    decode_log_level(tmp,state->log_level);
    printf(" - Log level:             %s\n", tmp);

}

void dump_log( struct list_log_entry *log, int n)
{
    int i;
    for(i=0;i<n;i++)
    {
        char tmp[1024];
        struct list_log_entry *e = &log[i];
        if(e->type == LIST_LOG_RAW)
            printf("Raw   ");
        printf("Ch: %d ", e->channel);
        format_ts(tmp, e->ts, 1);
        printf("Ts: %s\n", tmp);
    }
}

main()
{
    struct list_node *n = list_open_node_by_lun ( 0 );
    struct list_id trig = { 10, 20, 30 };
    struct list_id trig2 = { 10, 20, 31 };
    
    fprintf(stderr,"SDT\n");
    list_in_set_dead_time(n, 1, 90 * 1000 * 1000);
fprintf(stderr,"STO\n");
    list_in_set_timebase_offset (n, 1, 216000ULL );
    int i;

    for( i = 1; i <= 5; i++)
    {
        list_in_enable(n, i, 0);
        list_in_set_delay(n, i, 1 * 1000 * 1000); // 1us  delay
        list_in_enable(n, i, 1);
        trig.trigger = i + 123;
        list_in_assign_trigger(n, i, &trig);
        list_in_set_log_level(n, i, LIST_LOG_RAW);
        list_in_set_trigger_mode(n, i, LIST_TRIGGER_MODE_AUTO);
        list_in_arm(n, i, 1);
    }




    for(;;)
    {
        struct list_input_state state;
        struct list_log_entry logbuf[128];

        //list_in_get_state(n, 1, &state);
        //dump_input_state(&state);

        int n_ent = list_in_read_log(n, logbuf, LIST_LOG_RAW | LIST_LOG_SENT, 0xff, 12);
    
        dump_log(logbuf, n_ent);
          //sleep(1);
    }

    return 0;
}
