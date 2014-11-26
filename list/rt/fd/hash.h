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
#include "list-common.h"

struct lrt_output_rule {
    uint32_t delay_cycles;
    uint16_t delay_frac;
    uint16_t state;
    struct lrt_output_rule *cond_ptr;
    int worst_latency;
};


struct lrt_hash_entry {
    struct list_id id;					
    struct lrt_output_rule ocfg [FD_NUM_CHANNELS];	
    struct lrt_hash_entry *next;			
};

extern struct lrt_hash_entry* htab [ FD_HASH_ENTRIES ];

void hash_init();
struct lrt_hash_entry *hash_add ( struct list_id *id, int output, struct lrt_output_rule *rule );
int hash_remove ( struct lrt_hash_entry *ent, int output );
int hash_free_count();
struct lrt_hash_entry *hash_get_entry (int bucket, int pos);

static inline int hash_func( struct list_id *id )
{
    int h = 0;
    h += id->system * 10291;
    h += id->source_port * 10017;
    h += id->trigger * 3111;
    return h & (FD_HASH_ENTRIES - 1); // hash table size must be a power of 2
}

static inline struct lrt_hash_entry *hash_search( struct list_id *id, int *pos )
{
    int p = hash_func( id );

    struct lrt_hash_entry *ent = htab[ p ];
    if(pos)
        *pos = p;
    
    
    while (ent)
    {
        if(ent->id.system == id->system &&
       ent->id.source_port == id->source_port &&
           ent->id.trigger == id->trigger)
            return ent;
    
    ent = ent->next;
    }
    return NULL;
};

#endif

