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

#define WRTD_IN_RT_ID  0x347D0000
#define WRTD_OUT_RT_ID 0x347D0001

/* WR Node CPU Core indices */
#define WRTD_CPU_TDC 0			/* Core 0 controls the TDC mezzanine */
#define WRTD_CPU_FD 1			/* Core 1 controls the FD mezzanine */

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
#define WRTD_CMD_TDC_BASE_TIME                0x10
#define WRTD_CMD_TDC_VERSION                  0x11

#define WRTD_CMD_FD_TRIG_ENABLE          0x10
#define WRTD_CMD_FD_TRIG_REMOVE          0x11
#define WRTD_CMD_FD_TRIG_ASSIGN          0x12
#define WRTD_CMD_FD_TRIG_GET_BY_ID       0x13
#define WRTD_CMD_FD_TRIG_GET_STATE       0x14
#define WRTD_CMD_FD_TRIG_SET_COND_DELAY  0x15
#define WRTD_CMD_FD_TRIG_SET_DELAY       0x16
#define WRTD_CMD_FD_TRIG_RESET_COUNTERS  0x17

#define WRTD_CMD_FD_CHAN_ENABLE               0x1
#define WRTD_CMD_FD_READ_HASH	    	      0x2
#define WRTD_CMD_FD_CHAN_GET_STATE            0x3
#define WRTD_CMD_FD_CHAN_SET_WIDTH            0x4
#define WRTD_CMD_FD_CHAN_SET_MODE             0x5
#define WRTD_CMD_FD_SOFTWARE_TRIGGER          0x6
#define WRTD_CMD_FD_CHAN_ARM                  0x7
#define WRTD_CMD_FD_CHAN_SET_LOG_LEVEL        0x8
#define WRTD_CMD_FD_CHAN_RESET_COUNTERS       0x9

#define WRTD_CMD_FD_PING                     0xa
#define WRTD_CMD_FD_BASE_TIME                0xb
#define WRTD_CMD_FD_CHAN_DEAD_TIME           0xc
#define WRTD_CMD_FD_VERSION                  0xd


#define WRTD_REP_ACK_ID			0x100
#define WRTD_REP_STATE			0x101
#define WRTD_REP_NACK			0x102
#define WRTD_REP_TRIGGER_HANDLE	0x103
#define WRTD_REP_HASH_ENTRY		0x104
#define WRTD_REP_TIMESTAMP		0x105
#define WRTD_REP_LOG_MESSAGE    0x106
#define WRTD_REP_BASE_TIME_ID           0x107
#define WRTD_REP_VERSION                0x108




#define TDC_NUM_CHANNELS 5
#define TDC_TRIGGER_COALESCE_LIMIT 5

#define FD_NUM_CHANNELS 4
#define FD_HASH_ENTRIES 64
#define FD_MAX_QUEUE_PULSES 16

enum wrtd_in_actions {
	WRTD_IN_ACTION_SW_TRIG = __RT_ACTION_RECV_STANDARD_NUMBER,
	WRTD_IN_ACTION_LOG,
};
enum wrtd_out_actions {
	WRTD_OUT_ACTION_SW_TRIG = __RT_ACTION_RECV_STANDARD_NUMBER,
	WRTD_OUT_ACTION_LOG,
};

enum wrtd_in_variables_indexes {
	IN_VAR_CHAN_ENABLE = 0,
	IN_VAR_DEVICE_TIME_S,
	IN_VAR_DEVICE_TIME_T,
	IN_VAR_DEVICE_SENT_PACK,
	IN_VAR_DEVICE_DEAD_TIME,
	IN_VAR_DEVICE_CHAN_ENABLE,
	__WRTD_IN_VAR_MAX,
};
enum wrtd_in_structures_indexes {
	IN_STRUCT_DEVICE = 0,
	IN_STRUCT_CHAN_0,
	IN_STRUCT_CHAN_1,
	IN_STRUCT_CHAN_2,
	IN_STRUCT_CHAN_3,
	IN_STRUCT_CHAN_4,
	__WRTD_IN_STRUCT_MAX,
};
enum wrtd_out_variables_indexes {
	OUT_VAR_DEVICE_TIME_S=0,
	OUT_VAR_DEVICE_TIME_T,
	__WRTD_OUT_VAR_MAX,
};
enum wrtd_out_structures_indexes {
	OUT_STRUCT_DEVICE = 0,
	OUT_STRUCT_CHAN_0,
	OUT_STRUCT_CHAN_1,
	OUT_STRUCT_CHAN_2,
	OUT_STRUCT_CHAN_3,
	OUT_STRUCT_CHAN_4,
	__WRTD_OUT_STRUCT_MAX,
};

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
enum wrtd_io_flags {
	WRTD_ENABLED = (1 << 0),          /*!< I/O is enabled */
	WRTD_TRIGGER_ASSIGNED = (1 << 1), /*!< I/O is has a trigger assigned */
	WRTD_LAST_VALID = (1 << 2),       /*!< I/O processed at least one pulse.
					    It's timestamp/ID is in the "last"
					    field. */
	WRTD_ARMED = (1 << 3),            /*!< I/O is armed */
	WRTD_TRIGGERED = (1 << 4),        /*!< I/O has triggered */
	WRTD_NO_WR = (1 << 5),            /*!< I/O has no WR timing */

};

/**
 * Log level flag description
 */
enum wrtd_log_level {
	WRTD_LOG_NOTHING = 0, /**< disable logging */
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

	WRTD_LOG_ALL = 0xff, /**< all events will be logged */
};


/**
 * Possible causes for missed trigger
 */
enum wrtd_log_miss_reason {
	WRTD_MISS_DEAD_TIME = 0, /**< trigger during dead time period */
	WRTD_MISS_OVERFLOW = 1, /**< too many trigger events, trigger queue
				   overflow */
	WRTD_MISS_NO_WR = 2, /**< No White-Rabbit network */
	WRTD_MISS_TIMEOUT = 3, /**< timeout for trigger generation */
};


#define HASH_ENT_EMPTY          (0 << 0)
#define HASH_ENT_DIRECT         (1 << 0)
#define HASH_ENT_CONDITION      (1 << 1)
#define HASH_ENT_CONDITIONAL    (1 << 2)
#define HASH_ENT_DISABLED       (1 << 3)

/**
 * White-Rabbit Time-Stamp format
 */
struct wr_timestamp {
	uint64_t seconds;
	uint32_t ticks;
	uint32_t frac;
};


/**
 * Trigger identifier
 */
struct wrtd_trig_id {
	uint32_t system;  /**< Unique ID of the WRTD to identify a domain. */
	uint32_t source_port;  /**< System-wide unique ID to intentify the
				input port the trigger comes from. */
	uint32_t trigger;  /**< System-wide unique ID of a particular trigger
			      pulse*/
};


/**
 * Trigger event
 */
struct wrtd_trigger_entry {
	struct wr_timestamp ts; /**< when it fired */
	struct wrtd_trig_id id; /**< which trigger */
	uint32_t seq; /**< its sequence number */
};


/**
 * Log event descriptor
 */
struct wrtd_log_entry {
	uint32_t type; /**< type of logging */
	uint32_t seq; /**< log sequence number */
	int channel; /**< channel that generate the logging message */
	struct wrtd_trig_id id; /**< trigger id associated with the log event */
	struct wr_timestamp ts; /**< when the log message was sent from
				   the RT application*/
	enum wrtd_log_miss_reason miss_reason; /**< trigger failure reason.
						  It is valid only when type
						  is WRTD_LOG_MISSED */
};

#ifdef WRNODE_RT
struct wrtd_trigger_message {
  struct rmq_message_addr hdr;
  uint32_t transmit_seconds;
  uint32_t transmit_cycles;
  int count;
  struct wrtd_trigger_entry triggers[TDC_TRIGGER_COALESCE_LIMIT];
  uint32_t pad; // stupid Etherbone for some reasons drops the last entry on TX
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


/**
 * Structure describing state of each TDC channel
 * All fields must be 32bit (do not use enum because there are no guarantee)
 */
struct wrtd_in_channel_config {
	struct wrtd_trig_id id; /**< Currently assigned trigger ID */
	struct wr_timestamp delay; /**< Trigger delay, added to each timestamp */
	struct wr_timestamp timebase_offset; /* Internal time base offset. Used
						to compensate the TDC-to-WR
						timebase lag. Not exposed to the
						public, set from the internal
						calibration data of the TDC
						driver. */
	uint32_t flags; /**< Channel flags (enum wrnc_io_flags) */
	uint32_t log_level; /**< Log level (enum wrnc_log_level) */
        uint32_t mode; /**< Triggering mode (enum wrtd_triger_mode) */
};

struct wrtd_in_channel_stats {
	struct wr_timestamp last_tagged; /**< Timestamp of the last tagged
					    pulse */
	struct wrtd_trigger_entry last_sent; /**< Last transmitted trigger */
	uint32_t total_pulses; /**< Total tagged pulses */
	uint32_t sent_pulses; /**< Total sent pulses */
	uint32_t miss_no_timing; /**< Total missed pulses (no WR) */
	uint32_t seq;
};

/* Structure describing state of each TDC channel*/
struct wrtd_in_channel {
	int n;
	struct wrtd_in_channel_stats stats;
	struct wrtd_in_channel_config config;
};

struct wrtd_in {
	uint32_t dead_time; /**< TDC dead time, in 8ns ticks */
};

#endif
