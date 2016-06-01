/*
 * This work is part of the White Rabbit Node Core project.
 *
 * Copyright (C) 2013-2014 CERN (www.cern.ch)
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * Released according to the GNU GPL, version 2 or any later version.
 */


/*.
 * White Rabbit Trigge Distribution
 *
 * wrtd-serializers.h: API for serializing HMQ messages specific for WR Trigger Distribution system.
 */

#ifndef __WRTD_SERIALIZERS_H
#define __WRTD_SERIALIZERS_H

#include "wrtd-common.h"

#ifdef WRNODE_RT
#include "rt-message.h"
#endif

static inline int _trtl_msg_check_buffer( struct trtl_msg *buf, int n_words )
{
#ifndef WRNODE_RT
    if(buf->error) 
	return -1;

    if(buf->direction == WRNC_MSG_DIR_SEND &&
       buf->datalen + n_words > buf->max_size)
    {
#ifdef DEBUG
	pp_printf("Error: HMQ buffer send overflow: %d vs %d\n", buf->datalen + n_words, buf->max_size );
#endif
	buf->error = 1;
	return -1;
    } else if (buf->direction == WRNC_MSG_DIR_RECEIVE &&
	       buf->offset + n_words > buf->datalen ) {

#ifdef DEBUG
	pp_printf("Error: HMQ buffer recv overflow: %d vs %d\n", buf->offset + n_words, buf->datalen );
#endif
	buf->error = 1;
	return -1;
    }
#endif

    return 0;
}

static inline int trtl_msg_int32 ( struct trtl_msg *buf, int *value )
{
    if ( _trtl_msg_check_buffer ( buf, 1 ) < 0 )
	return -1;

    if (buf->direction == WRNC_MSG_DIR_SEND)
    {
	buf->data[buf->datalen] = *value;
	buf->datalen++;
    } else {
	*value = buf->data[buf->offset];
	buf->offset++;
    }

    return 0;
}


static inline int trtl_msg_int16 ( struct trtl_msg *buf, int16_t *value )
{
    if ( _trtl_msg_check_buffer ( buf, 1 ) < 0 )
	return -1;

    if (buf->direction == WRNC_MSG_DIR_SEND)
    {
	buf->data[buf->datalen] = *value;
	buf->datalen++;
    } else {
	*value = (int16_t) buf->data[buf->offset];
	buf->offset++;
    }

    return 0;
}

static inline int trtl_msg_uint32 ( struct trtl_msg *buf, uint32_t *value )
{
    return trtl_msg_int32( buf, (int *) value);
}

static inline int trtl_msg_uint16 ( struct trtl_msg *buf, uint16_t *value )
{
    return trtl_msg_int16( buf, (int16_t *) value);
}


static inline int trtl_msg_header ( struct trtl_msg *buf, uint32_t *id, uint32_t *seq_no )
{
    if (_trtl_msg_check_buffer ( buf, 2 ) < 0)
	return -1;

    if (buf->direction == WRNC_MSG_DIR_SEND)
    {
	buf->data[buf->datalen + 0] = *id;
	buf->data[buf->datalen + 1] = *seq_no;
        buf->datalen += 2;
    } else {
	*id =		buf->data[buf->offset + 0];
	*seq_no = 	buf->data[buf->offset + 1];
        buf->offset += 2;
    }
    
    return 0;
}

static inline void trtl_msg_skip ( struct trtl_msg *buf, int n_words )
{
    _trtl_msg_check_buffer ( buf, n_words );
    if (buf->direction == WRNC_MSG_DIR_SEND)
	buf->datalen += n_words;
    else
	buf->offset += n_words;
}

static inline void trtl_msg_seek ( struct trtl_msg *buf, int pos )
{
    buf->offset = pos;
    buf->datalen = pos;
}

static inline int wrtd_msg_timestamp ( struct trtl_msg *buf, struct wr_timestamp *ts )
{
    if (_trtl_msg_check_buffer ( buf, 4 ) < 0)
	return -1;

    if (buf->direction == WRNC_MSG_DIR_SEND)
    {
        buf->data[buf->datalen + 0] =	(ts->seconds >> 32) & 0xFFFFFFFF;
	buf->data[buf->datalen + 1] =	ts->seconds & 0xFFFFFFFF;
        buf->data[buf->datalen + 2] = 	ts->ticks;
        buf->data[buf->datalen + 3] = 	ts->frac;
	buf->datalen += 4;
    } else {
	ts->seconds = 	((uint64_t)buf->data[buf->offset + 0]) << 32;
       ts->seconds |=	buf->data[buf->offset + 1];
	ts->ticks = 	buf->data[buf->offset + 2];
        ts->frac = 	buf->data[buf->offset + 3];
	buf->offset += 4;
    }

    return 0;
}

static inline int wrtd_msg_trig_id ( struct trtl_msg *buf, struct wrtd_trig_id *id )
{
    if (_trtl_msg_check_buffer ( buf, 3 ) < 0)
	return -1;
    
    if (buf->direction == WRNC_MSG_DIR_SEND)
    {
	buf->data[buf->datalen + 0] = id->system;
        buf->data[buf->datalen + 1] = id->source_port;
        buf->data[buf->datalen + 2] = id->trigger;
	buf->datalen += 3;
    } else {
	id->system =		buf->data[buf->offset + 0];
        id->source_port =	buf->data[buf->offset + 1];
        id->trigger =		buf->data[buf->offset + 2];
	buf->offset += 3;
    }

    return 0;
}

static inline int wrtd_msg_trigger_entry ( struct trtl_msg *buf, struct wrtd_trigger_entry *ent )
{
    if (wrtd_msg_timestamp (buf, &ent->ts) < 0)
	return -1;
    if (wrtd_msg_trig_id (buf, &ent->id) < 0)
	return -1;
    
    return trtl_msg_int32 (buf, (int *) &ent->seq);
}

static inline struct trtl_msg trtl_msg_init(int max_size)
{
    struct trtl_msg b;

    b.direction = WRNC_MSG_DIR_SEND;
    b.max_size = max_size;
    b.offset = 0;
    b.datalen = 0;
    b.error = 0;

    return b;
}

static inline int trtl_msg_check_error (struct trtl_msg *buf)
{
    return buf->error;
}

#endif
