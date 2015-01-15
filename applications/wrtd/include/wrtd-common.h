/*
 * This work is part of the White Rabbit Node Core project.
 *
 * Copyright (C) 2013-2014 CERN (www.cern.ch)
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * Released according to the GNU GPL, version 2 or any later version.
 */

#ifndef __WRTD_COMMON_H
#define __WRTD_COMMON_H

/* WR Node CPU Core indices */
#define WRTD_CPU_TDC 0			/* Core 0 controls the TDC mezzanine */
#define WRTD_CPU_FD 1			/* Core 0 controls the FD mezzanine */

#define WRTD_IN_MAX		2
#define WRTD_IN_TDC_CONTROL	0
#define WRTD_IN_FD_CONTROL	1

#define WRTD_OUT_MAX		4
#define WRTD_OUT_TDC_CONTROL	0
#define WRTD_OUT_FD_CONTROL	1
#define WRTD_OUT_TDC_LOGGING	2
#define WRTD_OUT_FD_LOGGING	3


#define WRTD_REMOTE_IN_MAX	1
#define WRTD_REMOTE_IN_FD	0

#define WRTD_REMOTE_OUT_MAX	1
#define WRTD_REMOTE_OUT_TDC	0


/* Command and log message IDs */
#define WRTD_LOG_RAW_INPUT	  	 0x1
#define WRTD_LOG_SENT_TRIGGER 	 0x2

#define WRTD_CMD_TDC_CHAN_ENABLE		0x1
#define WRTD_CMD_TDC_CHAN_SET_DEAD_TIME		0x2
#define WRTD_CMD_TDC_CHAN_SET_DELAY 		0x3
#define WRTD_CMD_TDC_CHAN_GET_STATE 		0x4
#define WRTD_CMD_TDC_CHAN_ARM	 		0x5
#define WRTD_CMD_TDC_CHAN_SET_MODE 		0x7
#define WRTD_CMD_TDC_CHAN_SET_SEQ 	       	0x8
#define WRTD_CMD_TDC_CHAN_ASSIGN_TRIGGER 	0x9
#define WRTD_CMD_TDC_CHAN_SET_FLAGS 		0xa
#define WRTD_CMD_TDC_CHAN_SET_TIMEBASE_OFFSET	0xb
#define WRTD_CMD_TDC_PING                     0xc
#define WRTD_CMD_TDC_SOFTWARE_TRIGGER         0xd
#define WRTD_CMD_TDC_CHAN_SET_LOG_LEVEL       0xe
#define WRTD_CMD_TDC_CHAN_RESET_COUNTERS      0xf


#define WRTD_CMD_FD_CHAN_ENABLE               0x1
#define WRTD_CMD_FD_CHAN_ASSIGN_TRIGGER       0x2
#define WRTD_CMD_FD_READ_HASH	    	      0x3
#define WRTD_CMD_FD_CHAN_REMOVE_TRIGGER       0x4
#define WRTD_CMD_FD_CHAN_GET_STATE            0x5
#define WRTD_CMD_FD_CHAN_SET_DELAY            0x6
#define WRTD_CMD_FD_CHAN_SET_WIDTH            0x7
#define WRTD_CMD_FD_CHAN_SET_MODE             0x8
#define WRTD_CMD_FD_SOFTWARE_TRIGGER          0x9
#define WRTD_CMD_FD_CHAN_ARM                  0xa
#define WRTD_CMD_FD_CHAN_ENABLE_TRIGGER       0xb
#define WRTD_CMD_FD_CHAN_SET_LOG_LEVEL        0xc
#define WRTD_CMD_FD_CHAN_RESET_COUNTERS       0xd

#define WRTD_REP_ACK_ID			0x100
#define WRTD_REP_STATE			0x101
#define WRTD_REP_NACK			0x102
#define WRTD_REP_TRIGGER_HANDLE		0x103
#define WRTD_REP_HASH_ENTRY		0x104
#define WRTD_REP_TIMESTAMP		0x105



#define TDC_NUM_CHANNELS 5
#define TDC_TRIGGER_COALESCE_LIMIT 5

#define FD_NUM_CHANNELS 4
#define FD_HASH_ENTRIES 128
#define FD_MAX_QUEUE_PULSES 16


/**
 * availables trigger mode
 */
enum wrtd_trigger_mode {
    WRTD_TRIGGER_MODE_SINGLE = 1, /**< In SINGLE mode, the input/output will
				     trigger only on the 1st pulse/trigger
				     message after arming.*/
    WRTD_TRIGGER_MODE_AUTO = 2, /**< In AUTO mode, the input/output will
				   trigger on every pulse/trigger message.*/
};


/**
 * This enum is used in list_input_state / list_output_state
 * structures to pass state information
 */
enum wrnc_io_flags {
    WRTD_ENABLED = (1 << 0),          /*!< I/O is physically enabled */
    WRTD_TRIGGER_ASSIGNED = (1 << 1), /*!< I/O is has a trigger assigned */
    WRTD_LAST_VALID = (1 << 2),       /*!< I/O processed at least one pulse.
					It's timestamp/ID is in the "last"
					field. */
    WRTD_ARMED = (1 << 3),            /*!< I/O is armed */
    WRTD_TRIGGERED = (1 << 4),        /*!< I/O has triggered */
};


enum wrtd_log_level {
    WRTD_LOG_NOTHING = 0,
    WRTD_LOG_RAW = (1 << 0), /**< Input only: log all pulses coming to
				the TDC input */
    WRTD_LOG_SENT = (1 << 1), /**< Input only: log all sent triggers */
    WRTD_LOG_PROMISC = (1 << 2), /**< Output only: promiscious mode -
				    log all trigger messages received
				    from WR network */
    WRTD_LOG_FILTERED = (1 << 3), /**< Output only: log all trigger
				     messages that have been assigned
				     to the output */
    WRTD_LOG_EXECUTED = (1 << 4), /**< Output only: log all triggers
				     executed on the output */
    WRTD_LOG_MISSED   = (1 << 5), /**< Output only: log all triggers
				     missed by the output */

    WRTD_LOG_ALL = 0xff,
};


#define HASH_ENT_EMPTY          (0 << 0)
#define HASH_ENT_DIRECT         (1 << 0)
#define HASH_ENT_CONDITION      (1 << 1)
#define HASH_ENT_CONDITIONAL    (1 << 2)
#define HASH_ENT_DISABLED       (1 << 3)

struct wr_timestamp {
	uint64_t seconds;
	uint64_t ticks;
	uint64_t frac;
};

struct wrtd_trig_id {
	uint32_t system;
	uint32_t source_port;
	uint32_t trigger;
};

struct wrtd_trigger_entry {
     struct wr_timestamp ts;
     struct wrtd_trig_id id;
     uint32_t seq;
};


struct wrtd_log_entry {
    uint32_t type;
    uint32_t seq;
    int channel;
    struct wrtd_trig_id id;
    struct wr_timestamp ts;
};

#ifdef WRNODE_RT
struct wrtd_trigger_message {
  struct rmq_message_addr hdr;
  uint32_t transmit_seconds;
  uint32_t transmit_cycles;
  int count;
  struct wrtd_trigger_entry triggers[TDC_TRIGGER_COALESCE_LIMIT];
};
#endif


#ifdef WRNODE_RT
static inline void ts_add(struct wr_timestamp *a, const struct wr_timestamp *b)
{
    a->frac += b->frac;

    if(a->frac >= 4096)
    {
    	a->frac -= 4096;
    	a->ticks ++;
    }

    a->ticks += b->ticks;

    if(a->ticks >= 125000000)
    {
    	a->ticks -= 125000000;
    	a->seconds++;
    }

    a->seconds += b->seconds;
}

static inline void ts_sub(struct wr_timestamp *a, const struct wr_timestamp *b)
{
    a->frac -= b->frac;

    if(a->frac < 0)
    {
    	a->frac += 4096;
    	a->ticks --;
    }

    a->ticks -= b->ticks;

    if(a->ticks < 0)
    {
    	a->ticks += 125000000;
    	a->seconds--;
    }

    a->seconds -= b->seconds;

    if(a->seconds == -1)
    {
      a->seconds = 0;
      a->ticks -= 125000000;
    }
}
#endif

#endif
