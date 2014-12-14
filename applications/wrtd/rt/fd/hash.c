/*
 * This work is part of the White Rabbit Node Core project.
 *
 * Copyright (C) 2013-2014 CERN (www.cern.ch)
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * Released according to the GNU GPL, version 2 or any later version.
 */


/*.
 * LHC Instability Trigger Distribution (LIST) Firmware.
 *
 * hash.c: Trigger Output hash table implementation
 */

#include <string.h>

#include "hash.h"

/* constant sized block memory pool, providing constant time allocation and freeing */
struct blockpool {
    int blk_size;
    int blk_count;
    uint16_t *fq;
    int fq_head, fq_tail, fq_count;
    void *pool;
};

static uint32_t hash_pool_mem [ FD_HASH_ENTRIES * sizeof(struct lrt_hash_entry) / 4 ];
static uint16_t hash_pool_queue [ FD_HASH_ENTRIES +1 ];
static struct blockpool hash_blockpool;

struct lrt_hash_entry* htab [ FD_HASH_ENTRIES ];

void blockpool_init( struct blockpool *bp, int blk_size, int blk_count, void *data, void *queue )
{
    int i;

    bp->pool = data;
    bp->fq = queue;
    bp->blk_count = blk_count;
    bp->blk_size = blk_size;
    bp->fq_head = blk_count;
    bp->fq_tail = 0;
    bp->fq_count = blk_count;

    for(i=0;i<bp->blk_count;i++)
	bp->fq[i] = i;
}

void *blockpool_alloc( struct  blockpool *bp )
{
    if(bp->fq_head == bp->fq_tail)
    {
	return NULL;
    }

    void *blk = bp->pool + bp->blk_size * (int)bp->fq[bp->fq_tail];
    if(bp->fq_tail == bp->blk_count)
	bp->fq_tail = 0;
    else
	bp->fq_tail++;

    bp->fq_count--;

    

    return blk;
}

void blockpool_free( struct blockpool *bp, void *ptr )
{
    int blk_id = (ptr - bp->pool) / bp->blk_size;
    bp->fq[bp->fq_head] = blk_id;

    if(bp->fq_head == bp->blk_count)
	bp->fq_head = 0;
    else
	bp->fq_head++;
    bp->fq_count++;
}


void hash_init()
{
    blockpool_init(&hash_blockpool, sizeof(struct lrt_hash_entry), FD_HASH_ENTRIES, hash_pool_mem, hash_pool_queue);
    memset(&htab, 0, sizeof(htab) );
}

struct lrt_hash_entry *hash_alloc( int pos )
{
    struct lrt_hash_entry *prev = NULL, *current = htab[pos];
    
    while(current)
    {
	prev = current;
	current = current->next;
    }
	
    current = blockpool_alloc(&hash_blockpool);

    if(!prev)
        htab[pos] = current;
    else
       	prev->next = current;

    current->next = NULL;
    return current;
}

struct lrt_hash_entry *hash_add ( struct wrtd_trig_id *id, int output, struct lrt_output_rule *rule )
{
    int pos;
    struct lrt_hash_entry *ent = hash_search( id, &pos );

    if(!ent)
	   ent = hash_alloc( pos );

    if(!ent)
       return NULL;

    rule->worst_latency = 0;

    ent->id = *id;
    ent->ocfg[output] = *rule;
    return ent;
}

int hash_remove ( struct lrt_hash_entry *ent, int output )
{
    int pos, i;
    ent->ocfg[output].state = HASH_ENT_EMPTY;

    for(i = 0; i < FD_NUM_CHANNELS; i++)
        if(ent->ocfg[i].state != HASH_ENT_EMPTY) // the same ID is assigned to another output
            return 0;

    if (ent == htab[pos])
        htab[pos] = ent->next;
    else {
        struct lrt_hash_entry *tmp = htab[pos]->next;
        htab[pos]->next = ent->next;
        ent->next = tmp;
    }

    blockpool_free(&hash_blockpool, ent);
    return 0;
}

struct lrt_hash_entry *hash_get_entry (int bucket, int pos)
{
    int i;
    if(bucket < 0 || bucket >= FD_HASH_ENTRIES)
	return NULL;

    struct lrt_hash_entry *l = htab[bucket];

    for(i = 0; l != NULL && i < pos; i++, l=l->next)
	if(!l)
	    return NULL;

    return l;
}

int hash_free_count()
{
    return hash_blockpool.fq_count;
}
