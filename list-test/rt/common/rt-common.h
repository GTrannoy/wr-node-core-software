#ifndef __RT_COMMON_H
#define __RT_COMMON_H


#define TDC_IN_SLOT_CONTROL 0
#define TDC_OUT_SLOT_CONTROL 0
#define TDC_OUT_SLOT_LOGGING 1
#define TDC_OUT_SLOT_REMOTE 0

#define ID_TDC_LOG_RAW		 0x1
#define ID_TDC_LOG_OUTGOING	 0x2

#define ID_TDC_CMD_CHAN_ENABLE	 		        0x1
#define ID_TDC_CMD_CHAN_SET_DEAD_TIME       0x2
#define ID_TDC_CMD_CHAN_SET_DELAY 		      0x3
#define ID_TDC_CMD_CHAN_GET_STATE 		      0x4
#define ID_TDC_CMD_CHAN_ARM	 		            0x5
#define ID_TDC_CMD_CHAN_DISARM	 		        0x6
#define ID_TDC_CMD_CHAN_SET_MODE 		        0x7
#define ID_TDC_CMD_CHAN_RESET_SEQ 	       	0x8
#define ID_TDC_CMD_CHAN_ASSIGN_TRIGGER 		  0x9
#define ID_TDC_CMD_CHAN_SET_FLAGS 		      0xa
#define ID_TDC_CMD_CHAN_SET_TIMEBASE_OFFSET	0xb
#define ID_TDC_CMD_PING                   0xc
#define ID_TDC_CMD_SOFTWARE_TRIGGER     0xd

#define ID_TDC_LOG_RAW_TIMESTAMP		0x102
#define ID_TDC_LOG_TRIGGER			0x103

#define ID_REP_ACK				0x100
#define ID_REP_STATE			0x101

#define TDC_NUM_CHANNELS 5

#define TDC_TRIGGER_COALESCE_LIMIT 5

#define TDC_CHAN_LOG_RAW	      	(1<<0)
#define TDC_CHAN_LOG_TRIGGERS     	(1<<1)
#define TDC_CHAN_TRIGGER_ASSIGNED 	(1<<2)
#define TDC_CHAN_ARMED 		        (1<<3)
#define TDC_CHAN_TRIGGERED 		    (1<<4)
#define TDC_CHAN_ENABLED 		    (1<<5)

#define TDC_CHAN_MODE_SINGLE    	0
#define TDC_CHAN_MODE_CONTINUOUS	1

struct list_timestamp {
  int32_t seconds;
  int32_t cycles;
  int32_t frac;
};

struct list_id 
{
  uint32_t system;
  uint32_t source_port;
  uint32_t trigger;
};

struct list_trigger_entry {
     struct list_timestamp ts;
     struct list_id id;
     uint32_t seq;
};

#ifdef WRNODE_APP
struct list_trigger_message {
  struct rmq_message_addr hdr;
  uint32_t transmit_seconds;
  uint32_t transmit_cycles;
  int count;
  struct list_trigger_entry triggers[TDC_TRIGGER_COALESCE_LIMIT];
};
#endif

struct tdc_channel_state {
    struct list_id id;
    struct list_timestamp delay;
    struct list_timestamp timebase_offset;
    struct list_timestamp last;
    int32_t flags;
    int32_t mode;
    int32_t n;
    int32_t total_pulses;
    int32_t sent_pulses;
    int32_t worst_latency;
    uint32_t seq;
};

struct tdc_channel_state_msg 
{
    int seq;
    struct tdc_channel_state state;
};


#endif
