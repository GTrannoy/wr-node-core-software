/*
 * This work is part of the White Rabbit Node Core project.
 *
 * Copyright (C) 2013-2014 CERN (www.cern.ch)
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * Released according to the GNU GPL, version 2 or any later version.
 */


/* 
 * LHC Instability Trigger Distribution (LIST) Firmware 
 *
 * hash.h: Trigger Output hash table
 */

#ifndef __LIST_HASH_H
#define __LIST_HASH_H

#include "rt.h"
#include "wrtd-common.h"

/* The hash is a fast search structure O(1) for matching incoming trigger messages against their IDs
   with triggering rules associated with each output of the node. It consists of FD_NUM_CHANNEL buckets. 
   Each bucket may contain a single ID entry or a single-linked list in case of a collision. */

/* Rule defining the behaviour of a trigger output upon reception of a trigger message
   with matching ID */
struct lrt_output_rule {
    /* Delay to add to the timestamp enclosed within the trigger message */
    uint32_t delay_cycles;
    uint16_t delay_frac;
    /* State of the rule (empty, disabled, conditional action, condition, etc.) */
    uint16_t state;
    /* Pointer to conditional action. Used for rules that define triggering conditions. */
    struct lrt_output_rule *cond_ptr;
    /* Worst-case latency (in 8ns ticks)*/
    uint32_t latency_worst;
    /* Average latency accumulator and number of samples */
    uint32_t latency_avg_sum;
    uint32_t latency_avg_nsamples;
    /* Number of times the rule has successfully produced a pulse */
    int hits;
    /* Number of times the rule has missed a pulse (for any reason) */
    int misses;
};

/* Bucket of the hash table */
struct lrt_hash_entry {
    /* Trigger ID to match against */
    struct wrtd_trig_id id;			
    /* Rules for each trigger output */		
    struct lrt_output_rule *ocfg [FD_NUM_CHANNELS];	
    /* Linked list pointer in case of hash collision */
    struct lrt_hash_entry *next;			
};

extern struct lrt_hash_entry* htab [FD_HASH_ENTRIES];

void hash_init ();
struct lrt_hash_entry *hash_add (struct wrtd_trig_id *id, int output, struct lrt_output_rule *rule);
int hash_remove (struct lrt_hash_entry *ent, int output);
int hash_free_count ();
struct lrt_hash_entry *hash_get_entry (int bucket, int pos);

/* Hash function, returing the hash table index corresponding to a given trigger ID */
static inline int hash_func( struct wrtd_trig_id *id )
{
    int h = 0;
    h += id->system * 10291;
    h += id->source_port * 10017;
    h += id->trigger * 3111;
    return h & (FD_HASH_ENTRIES - 1); // hash table size must be a power of 2
}

/* Searches for hash entry matching given trigger id and returns a non-NULL pointer if found.
   If (pos) is not null, the index in the hash table entry is stored there */
static inline struct lrt_hash_entry *hash_search( struct wrtd_trig_id *id, int *pos )
{
    int p = hash_func( id );

    struct lrt_hash_entry *ent = htab [p];
    if(pos)
        *pos = p;
    
    while (ent)
    {
        if( ent->id.system == id->system &&
            ent->id.source_port == id->source_port &&
            ent->id.trigger == id->trigger)
                return ent;
    
        ent = ent->next;
    }

    return NULL;
};

#endif

