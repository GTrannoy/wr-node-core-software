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

/* a helper macro to declare a blockpool of given data type and reserve the memory
   for storage */
#define DECLARE_BLOCK_POOL(name, entry_type, entries_count )                            \
    static uint32_t name##_pool_mem [ (entries_count) * (sizeof (entry_type) + 3) / 4 ];     \
    static uint16_t name##_pool_queue [ (entries_count ) + 1];                               \
    static struct blockpool name##_blockpool;

/* Hash table entry block pool */
DECLARE_BLOCK_POOL ( hash, struct lrt_hash_entry, FD_HASH_ENTRIES )
/* Separate pool for output rule allocation (to save memory) */
DECLARE_BLOCK_POOL ( rules, struct lrt_output_rule, FD_HASH_ENTRIES )
/* The hash table itself */
struct lrt_hash_entry* htab [ FD_HASH_ENTRIES ];

/* Initializes an empty blockpool structure */
void blockpool_init (struct blockpool *bp, int blk_size, int blk_count, void *data, void *queue)
{
    int i;

    bp->pool = data;
    bp->fq = queue;
    bp->blk_count = blk_count;
    bp->blk_size = blk_size;
    bp->fq_head = blk_count;
    bp->fq_tail = 0;
    bp->fq_count = blk_count;

    /* Fill in the free queue with all available blocks */
    for(i=0;i<bp->blk_count;i++)
	   bp->fq[i] = i;
}

/* Returns a new block from the blockpool bp (or null if none available) */
void *blockpool_alloc(struct blockpool *bp)
{
    if (bp->fq_head == bp->fq_tail)
       return NULL;
    
    void *blk = bp->pool + bp->blk_size * (int)bp->fq [bp->fq_tail];
    
    if(bp->fq_tail == bp->blk_count)
	   bp->fq_tail = 0;
    else
	   bp->fq_tail++;

    bp->fq_count--;

    return blk;
}

/* Releases a block back into the blockpool */
void blockpool_free( struct blockpool *bp, void *ptr )
{
    int blk_id = (ptr - bp->pool) / bp->blk_size;
    bp->fq [bp->fq_head] = blk_id;

    if(bp->fq_head == bp->blk_count)
	    bp->fq_head = 0;
    else
	    bp->fq_head++;
        bp->fq_count++;
}

/* Initializes the hash table & blockpools */
void hash_init()
{
    blockpool_init (&hash_blockpool, sizeof(struct lrt_hash_entry), FD_HASH_ENTRIES, hash_pool_mem, hash_pool_queue);
    blockpool_init (&rules_blockpool, sizeof(struct lrt_output_rule), FD_HASH_ENTRIES, rules_pool_mem, rules_pool_queue);

    memset (&htab, 0, sizeof (htab));
}

/* Creates an empty entry in the hash bucket (pos) */
struct lrt_hash_entry *hash_alloc( int pos )
{
    struct lrt_hash_entry *prev = NULL, *current = htab[pos];

    /* Is there already something @ pos? Iterate to the end
       of the linked list if so */     
    while (current)
    {
    	prev = current;
    	current = current->next;
    }

    /* Allocate memory for new entry */
    current = blockpool_alloc(&hash_blockpool);
    memset(current->ocfg, 0, FD_NUM_CHANNELS * sizeof (struct lrt_output_rule *));

    /* And link it to the bucket/list */
    if(!prev)
        htab[pos] = current;
    else
       	prev->next = current;

    current->next = NULL;
    return current;
}

/* Adds a new output rule for given trigger ID and output pair or overwrites an existing one. */
struct lrt_hash_entry *hash_add ( struct wrtd_trig_id *id, int output, struct lrt_output_rule *rule )
{
    int pos;
    struct lrt_hash_entry *ent = hash_search( id, &pos );

    if(!ent)
	   ent = hash_alloc( pos );

    if(!ent)
       return NULL;

    rule->latency_worst = 0;
    rule->latency_avg_sum = 0 ;
    rule->latency_avg_nsamples = 0 ;
    rule->hits = 0;
    rule->misses = 0;

    ent->id = *id;
    
    struct lrt_output_rule *orule = ent->ocfg[output];
    if(orule == NULL) /* no memory allocated yet */
    {
        orule = blockpool_alloc(&rules_blockpool);
        if(!orule)
            return NULL; // should never happen as hashes are of same size
    }
    
    memcpy(orule, rule, sizeof(struct lrt_output_rule));
    ent->ocfg[output] = orule;

    return ent;
}

/* Removes an output rule from the hash. */
int hash_remove ( struct lrt_hash_entry *ent, int output )
{
	int bucket, i;
	struct lrt_hash_entry *e;

	/* release the rule */
	blockpool_free(&rules_blockpool, ent->ocfg[output]);
	ent->ocfg[output] = NULL;

	for(i = 0; i < FD_NUM_CHANNELS; i++)
		if(ent->ocfg[i]) /* Same ID is assigned to another output? */
			return 0;

	/* Identify the bucket where the entity is supposed to be stored */
	bucket = hash_func(&ent->id);

	/* Removing the given entity from the hash table */
	e = htab[bucket];
	if (e == ent)
		goto out;
	for (; e != NULL; e = e->next) {
		if (e->next == ent)
			goto out;
	}

	/* If we reach this point something is wrong in the htab */
	pp_printf("%s: entity is not in the hash table\n");
	return -1;

out:
	blockpool_free(&hash_blockpool, ent);
	e->next = ent->next;
	return 0;
}

/* Returns a pos-th hash entry in given hash bucket. */
struct lrt_hash_entry *hash_get_entry (int bucket, int pos)
{
    int i;
    
    if (bucket < 0 || bucket >= FD_HASH_ENTRIES)
	   return NULL;

    struct lrt_hash_entry *l = htab[bucket];

    for (i = 0; l != NULL && i < pos; i++, l = l->next)
	   if (!l)
	        return NULL;

    return l;
}


/* It counts the number of triggers assigned to a given channel */
int hash_count_rules(int ch)
{
	struct lrt_hash_entry *l, *ln;
	int count, i, k;

	for (i = 0; i < FD_HASH_ENTRIES; i++) {
		l = htab[i];
		for (k = 0; ln; ln = ln->next) {
			if (ln->ocfg[ch])
				count++;
		}
	}

	return count;
}

/* Returns the number of free hash entries */
int hash_free_count()
{
    return hash_blockpool.fq_count ;
}
