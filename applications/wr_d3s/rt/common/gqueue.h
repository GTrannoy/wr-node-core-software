/*
 * This work is part of the White Rabbit Node Core project.
 *
 * Copyright (C) 2013-2015 CERN (www.cern.ch)
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * Released according to the GNU GPL, version 2 or any later version.
 */

#ifndef __GQUEUE_H
#define __GQUEUE_H

/* A generic, non-locked FIFO queue */
struct generic_queue
{
    void *buf;
    int head, tail, count, size, entry_size;
};

static inline void gqueue_init(struct generic_queue *p, int n, int entry_size, void *buf)
{
    p->head = 0;
    p->tail = 0;
    p->count = 0;
    p->size = n;
    p->entry_size = entry_size;
    p->buf = buf;
}

/* Requests a new entry in a queue. Returns pointer to the ne
   entry or NULL if the queue is full. */
static inline void *gqueue_push(struct generic_queue *p)
{
    if (p->count == p->size)
        return NULL;

    void *ent = p->buf + p->head * p->entry_size;

    p->count++;
    p->head++;

    if (p->head == p->size)
        p->head = 0;

    return ent;
}

/* Returns non-0 if queue p contains any pulses. */
static inline int gqueue_empty(struct generic_queue *p)
{
    return (p->count == 0);
}

/* Returns the oldest entry in the queue (or NULL if empty). */
static inline void* gqueue_front(struct generic_queue *p)
{
    if (!p->count)
       return NULL;

    return p->buf + p->tail * p->entry_size;
}

/* Returns the newest entry in the queue (or NULL if empty). */
static inline void* gqueue_back(struct generic_queue *p)
{
    if (!p->count)
       return NULL;
    return &p->buf + p->head * p->entry_size;
}

/* Releases the oldest entry from the queue. */
static inline void gqueue_pop(struct generic_queue *p)
{
    p->tail++;

    if(p->tail == p->size)
        p->tail = 0;
    p->count--;
}


#endif
