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
 * rt-tdc.c: real-time CPU application for the FMC TDC mezzanine (Trigger Input)
 */

#include <string.h>

#include "rt.h"
#include "wrtd-common.h"
#include "wrtd-serializers.h"
#include "hw/fmctdc-direct.h"
#include "hw/tdc_regs.h"
#include "loop-queue.h"

#define DEFAULT_DEAD_TIME (80000/16)

#define BASE_DP_TDC_REGS    0x31000
#define BASE_DP_TDC_DIRECT  0x10000

/* Structure describing state of each TDC channel*/
struct tdc_channel_state {
/* Currently assigned trigger ID */
    struct wrtd_trig_id id;
/* Trigger delay, added to each timestamp */
    struct wr_timestamp delay;
/* Internal time base offset. Used to compensate the TDC-to-WR timebase lag.
   Not exposed to the public, set from the internal calibration data of the TDC driver. */
    struct wr_timestamp timebase_offset;
/* Timestamp of the last tagged pulse */
    struct wr_timestamp last_tagged;
/* Last transmitted trigger */
    struct wrtd_trigger_entry last_sent;
/* Channel flags (enum wrnc_io_flags) */
    uint32_t flags;
/* Log level (enum wrnc_log_level) */
    uint32_t log_level;
/* Triggering mode (enum wrtd_triger_mode) */
    enum wrtd_trigger_mode mode;
/* Number of the channel described in this structure */
    int n;
/* Total tagged pulses */
    uint32_t total_pulses;
/* Total sent pulses */
    uint32_t sent_pulses;
/* Total missed pulses (no WR) */
    uint32_t miss_no_timing;
/* Trigger message sequence counter */
    uint32_t seq;
/* TDC dead time, in 8ns ticks */
    uint32_t dead_time;
};

/* States of all TDC channels */
static struct tdc_channel_state channels[TDC_NUM_CHANNELS];
/* Total number of packets sent */
static uint32_t sent_packets = 0;
/* RMQ coalescing counter */
static int coalesce_count = 0;

/*
 * Timestamp Processing part
 */

 static void log_trigger(int type, int miss_reason, struct tdc_channel_state *st, struct wr_timestamp *ts, struct wrtd_trigger_entry *ent)
{
    uint32_t id = WRTD_REP_LOG_MESSAGE;
    uint32_t seq = 0;

    if ( !(st->log_level & type))
        return;

    struct wrnc_msg buf = hmq_msg_claim_out (WRTD_OUT_TDC_LOGGING, 16);

    wrnc_msg_header (&buf, &id, &seq);
    wrnc_msg_int32 (&buf, &type);
    wrnc_msg_int32 (&buf, &st->n);
    wrnc_msg_int32 (&buf, &miss_reason);
    if(ent)
    {
        wrtd_msg_trigger_entry (&buf, ent);
    } else {
        struct wrtd_trig_id fake_id;
        
        wrtd_msg_timestamp (&buf, ts);
        wrtd_msg_trig_id (&buf, &fake_id);
        wrnc_msg_uint32 (&buf, &st->total_pulses);
    }

    hmq_msg_send (&buf);
}


/* Creates a trigger message with timestamp (ts) for the channel (ch) and pushes it
   to the RMQ output (without sending) and sends immediately through the loopback queue */
static inline void send_trigger (struct wrtd_trigger_entry *ent)
{
    volatile struct wrtd_trigger_message *msg = mq_map_out_buffer(1, WRTD_REMOTE_OUT_TDC);

    msg->triggers[coalesce_count].id = ent->id;
    msg->triggers[coalesce_count].seq = ent->seq;
    msg->triggers[coalesce_count].ts = ent->ts;

    loop_queue_push(&ent->id, ent->seq, &ent->ts);

    coalesce_count++;
}


/* Prepares the RMQ output slot for transmission of trigger message */
static inline void claim_tx()
{
    mq_claim(1, WRTD_REMOTE_OUT_TDC);
}

/* Flushes the triggerrs in the RMQ output buffer to the WR Network */
static inline void flush_tx ()
{
    volatile struct wrtd_trigger_message *msg = mq_map_out_buffer(1, WRTD_REMOTE_OUT_TDC);

    msg->hdr.target_ip = 0xffffffff;    /* broadcast */
    msg->hdr.target_port = 0xebd0;      /* port */
    msg->hdr.target_offset = 0x4000;    /* target EB slot */

    /* Embed transmission time for latency measyurement */
    msg->transmit_seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
    msg->transmit_cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
    msg->count = coalesce_count;

    mq_send(1, WRTD_REMOTE_OUT_TDC, 6 + 7 * coalesce_count);
    coalesce_count = 0;
    sent_packets ++;
}

/* Processes a pulse with timestamp (ts) arriving on channel (channel) */
static inline void do_channel (int channel, struct wr_timestamp *ts)
{
    struct tdc_channel_state *ch = &channels[channel];

/* Apply timebase offset to align TDC time with WR timebase */
    ts_sub(ts, &ch->timebase_offset);
    ch->last_tagged = *ts;

/* Log raw value if needed */
    log_trigger (WRTD_LOG_RAW, 0, ch, ts, NULL);

    ch->total_pulses++;
   
/* Apply trigger delay */
    ts_add(ts, &ch->delay);

/* Enable/Arm/Trigger logic */
    if( (ch->flags & WRTD_TRIGGER_ASSIGNED ) && (ch->flags & WRTD_ARMED) )
    {
        struct wrtd_trigger_entry ent;
        
        ent.ts = *ts;
        ent.id = ch->id;
        ent.seq = ch->seq;

        if(!wr_is_timing_ok())
        {
            ch->miss_no_timing ++;
            log_trigger (WRTD_LOG_MISSED, WRTD_MISS_NO_WR, ch, NULL, &ent);
            return;
        }

    	ch->seq++;
    	ch->flags |= WRTD_TRIGGERED;
    	if(ch->mode == WRTD_TRIGGER_MODE_SINGLE )
    	    ch->flags &= ~WRTD_ARMED;

        ch->sent_pulses++;
        ch->last_sent = ent;

        send_trigger(&ent);
        log_trigger (WRTD_LOG_SENT, 0, ch, NULL, &ent);

        ch->flags |= WRTD_LAST_VALID;

    }
}

/* Handles input timestamps from all TDC channels, calling do_output() on incoming pulses */
static inline void do_input ()
{
    int i;

    /* Prepare for message transmission */
    claim_tx();

    /* We can send up to TDC_TRIGGER_COALESCE_LIMIT triggers in a single message - the loop will iterate
       up to this limit or exit immediately if there's no more input pulses in the TDC FIFO */

    for(i = 0; i < TDC_TRIGGER_COALESCE_LIMIT; i++)
    {
        uint32_t fifo_sr = dp_readl (BASE_DP_TDC_DIRECT + DR_REG_FIFO_CSR);
        struct wr_timestamp ts;
        int meta;

        /* Poll the FIFO and read the timestamp */
        if(fifo_sr & DR_FIFO_CSR_EMPTY)
            break;

        ts.seconds = dp_readl(BASE_DP_TDC_DIRECT + DR_REG_FIFO_R0);
        ts.ticks  = dp_readl(BASE_DP_TDC_DIRECT + DR_REG_FIFO_R1);
        meta   = dp_readl(BASE_DP_TDC_DIRECT + DR_REG_FIFO_R2);

        /* Convert from ACAM bins (81ps) to WR time format. Numerical hack used
           to avoid time-consuming division. */
        ts.frac = ( (meta & 0x3ffff) * 5308 ) >> 7;
    	ts.ticks += ts.frac >> 12;
    	ts.frac &= 0xfff;

        /* Make sure there's no overflow after conversion */
    	if (ts.ticks >= 125000000)
    	{
    		ts.ticks -= 125000000;
    		ts.seconds ++;
    	}

    	int channel = (meta >> 19) & 0x7;

        /* Pass the timestamp to triggering/TX logic */
        do_channel( channel, &ts );
    }

    /* Flush the RMQ buffer if it contains anything */
    if(coalesce_count)
        flush_tx();

};


/*
 * WRTD Command Handlers
 */


/* Creates a hmq_buf serializing object for the control output slot */
static inline struct wrnc_msg ctl_claim_out_buf()
{
    return hmq_msg_claim_out (WRTD_OUT_TDC_CONTROL, 128);
}

/* Creates a hmq_buf deserializing object for the control input slot */
static inline struct wrnc_msg ctl_claim_in_buf()
{
    return hmq_msg_claim_in (WRTD_IN_TDC_CONTROL, 16);
}


/* Sends an acknowledgement reply */
static inline void ctl_ack( uint32_t seq )
{
    struct wrnc_msg buf = ctl_claim_out_buf();
    uint32_t id_ack = WRTD_REP_ACK_ID;

    wrnc_msg_header (&buf, &id_ack, &seq);
    hmq_msg_send (&buf);
}


static inline void ctl_chan_enable (uint32_t seq, struct wrnc_msg *ibuf)
{
    int channel, enable;

    /* Deserailize the request */
    wrnc_msg_int32 (ibuf, &channel);
    wrnc_msg_int32 (ibuf, &enable);

    uint32_t mask = dp_readl(BASE_DP_TDC_DIRECT + DR_REG_CHAN_ENABLE);
    struct tdc_channel_state *ch = &channels[channel];

    if(enable)
    {
	   mask |= (1 << channel);
       ch->flags |= WRTD_ENABLED;
    } else {
	   mask &= ~(1 << channel);
       ch->flags &= ~WRTD_ENABLED | WRTD_ARMED | WRTD_TRIGGERED | WRTD_LAST_VALID;
    }

    /* Update TDC FIFO channel mask */
    dp_writel(mask, BASE_DP_TDC_DIRECT + DR_REG_CHAN_ENABLE);

    ctl_ack(seq);
}

static inline void ctl_chan_set_dead_time (uint32_t seq, struct wrnc_msg *ibuf)
{
    int channel, dead_time, i;

    /* Deserailize the request */
    wrnc_msg_int32 (ibuf, &channel);
    wrnc_msg_int32 (ibuf, &dead_time);

    dp_writel( dead_time, BASE_DP_TDC_DIRECT + DR_REG_DEAD_TIME);

    for(i=0; i < TDC_NUM_CHANNELS; i++)
        channels[i].dead_time = dead_time;

    ctl_ack(seq);
}

static inline void ctl_ping (uint32_t seq, struct wrnc_msg *ibuf)
{
    ctl_ack(seq);
}

static inline void ctl_chan_set_delay (uint32_t seq, struct wrnc_msg *ibuf)
{
    int channel;

    wrnc_msg_int32(ibuf, &channel);
    wrtd_msg_timestamp(ibuf, &channels[channel].delay);

    ctl_ack(seq);
}

static inline void ctl_chan_set_timebase_offset (uint32_t seq, struct wrnc_msg *ibuf)
{
    int channel;

    wrnc_msg_int32(ibuf, &channel);
    wrtd_msg_timestamp(ibuf, &channels[channel].timebase_offset);

    ctl_ack(seq);
}

static inline void ctl_chan_get_state (uint32_t seq, struct wrnc_msg *ibuf)
{
    int channel;
    uint32_t id_state = WRTD_REP_STATE;

    wrnc_msg_int32(ibuf, &channel);

    struct tdc_channel_state *st = &channels[channel];
    struct wrnc_msg obuf = ctl_claim_out_buf();

    wrnc_msg_header (&obuf, &id_state, &seq);
    wrnc_msg_int32 (&obuf, &channel);
    wrtd_msg_trig_id (&obuf, &st->id);
    wrtd_msg_timestamp (&obuf, &st->delay);
    wrtd_msg_timestamp (&obuf, &st->timebase_offset);
    wrtd_msg_timestamp (&obuf, &st->last_tagged);

    uint32_t flags = st->flags;

    if( !wr_is_timing_ok() )
        flags |= WRTD_NO_WR;

    wrnc_msg_uint32 (&obuf, &flags);
    wrnc_msg_uint32 (&obuf, &st->log_level);
    wrnc_msg_int32 (&obuf, (int *) &st->mode);
    wrnc_msg_uint32 (&obuf, &st->total_pulses);
    wrnc_msg_uint32 (&obuf, &st->sent_pulses);
    wrnc_msg_uint32 (&obuf, &st->dead_time);
    wrtd_msg_trigger_entry (&obuf, &st->last_sent);
    wrnc_msg_uint32 (&obuf, &sent_packets);

    hmq_msg_send (&obuf);
}

static inline void ctl_software_trigger (uint32_t seq, struct wrnc_msg *ibuf)
{
    struct wrtd_trigger_message *msg = mq_map_out_buffer(1, WRTD_REMOTE_OUT_TDC);

    mq_claim(1, WRTD_REMOTE_OUT_TDC);

    /* assemble a trigger message on the spot */
    msg->hdr.target_ip = 0xffffffff; // broadcast
    msg->hdr.target_port = 0xebd0;   // port
    msg->hdr.target_offset = 0x4000; // target EB slot
    msg->transmit_seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
    msg->transmit_cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
    msg->count = 1;

    /* and dumbly copy the trigger entry */
    struct wrtd_trigger_entry ent;
    wrtd_msg_trigger_entry(ibuf, &ent);

    msg->triggers[0] = ent;

    mq_send(1, WRTD_REMOTE_OUT_TDC, sizeof(struct wrtd_trigger_message) / 4); // fixme

    ctl_ack(seq);
}

static inline void ctl_chan_set_mode (uint32_t seq, struct wrnc_msg *ibuf)
{
    int channel, mode;

    wrnc_msg_int32(ibuf, &channel);
    wrnc_msg_int32(ibuf, &mode);

    struct tdc_channel_state *ch = &channels[channel];

    ch->mode = mode;
    /* Changing the mode resets triggering state */
    ch->flags &= ~(WRTD_ARMED | WRTD_TRIGGERED | WRTD_LAST_VALID) ;

    ctl_ack(seq);
}


static inline void ctl_chan_set_seq (uint32_t seq, struct wrnc_msg *ibuf)
{
    int channel;

    wrnc_msg_int32(ibuf, &channel);
    wrnc_msg_uint32(ibuf, &channels[channel].seq);

    ctl_ack(seq);
}


static inline void ctl_chan_arm (uint32_t seq, struct wrnc_msg *ibuf)
{
    int channel, arm;

    wrnc_msg_int32(ibuf, &channel);
    wrnc_msg_int32(ibuf, &arm);

    struct tdc_channel_state *ch = &channels[channel];

    if(arm)
        ch->flags |= WRTD_ARMED;
    else
        ch->flags &= ~WRTD_ARMED;

    /* Arming clears triggered flag */
    ch->flags &= ~WRTD_TRIGGERED;

    ctl_ack(seq);
}

static inline void ctl_chan_assign_trigger (uint32_t seq, struct wrnc_msg *ibuf)
{
    int channel, assign;
    struct wrtd_trig_id id;

    wrnc_msg_int32(ibuf, &channel);
    wrnc_msg_int32(ibuf, &assign);

    struct tdc_channel_state *ch = &channels[channel];

    if(assign)
    {
        wrtd_msg_trig_id(ibuf, &ch->id);

        ch->flags |= WRTD_TRIGGER_ASSIGNED;
        ch->flags &= ~WRTD_LAST_VALID;
    } else {
        ch->id.system = 0;
        ch->id.source_port = 0;
        ch->id.trigger = 0;

        ch->flags &= ~WRTD_TRIGGER_ASSIGNED;
    }

    ctl_ack(seq);
}

static inline void ctl_chan_reset_counters (uint32_t seq, struct wrnc_msg *ibuf)
{
    int channel;

    wrnc_msg_int32(ibuf, &channel);

    struct tdc_channel_state *ch = &channels[channel];

    ch->total_pulses = 0;
    ch->sent_pulses = 0;
    ch->miss_no_timing = 0;
    sent_packets = 0;
    ch->flags &= ~WRTD_LAST_VALID;

    ctl_ack(seq);
}

static inline void ctl_chan_set_log_level (uint32_t seq, struct wrnc_msg *ibuf)
{
    int channel;
    uint32_t log_level;

    wrnc_msg_int32(ibuf, &channel);
    wrnc_msg_uint32(ibuf, &log_level);

    channels[channel].log_level = log_level;

    ctl_ack(seq);
}

/* Receives command messages and call matching command handlers */
static inline void do_control()
{
    uint32_t cmd, seq;
    uint32_t p = mq_poll();

    /* HMQ control slot empty? */
    if(! ( p & ( 1<< WRTD_IN_TDC_CONTROL )))
        return;


    struct wrnc_msg ibuf = ctl_claim_in_buf();

    wrnc_msg_header(&ibuf, &cmd, &seq);

#define _CMD(id, func)          \
    case id:                    \
    {                           \
        func(seq, &ibuf);       \
        break;                  \
    }

	switch(cmd)
	{
	_CMD(WRTD_CMD_TDC_CHAN_ENABLE,                ctl_chan_enable)
	_CMD(WRTD_CMD_TDC_CHAN_SET_DEAD_TIME,         ctl_chan_set_dead_time)
	_CMD(WRTD_CMD_TDC_CHAN_SET_DELAY,             ctl_chan_set_delay)
	_CMD(WRTD_CMD_TDC_CHAN_SET_TIMEBASE_OFFSET,   ctl_chan_set_timebase_offset)
	_CMD(WRTD_CMD_TDC_CHAN_GET_STATE,             ctl_chan_get_state)
	_CMD(WRTD_CMD_TDC_SOFTWARE_TRIGGER,           ctl_software_trigger)
	_CMD(WRTD_CMD_TDC_CHAN_ASSIGN_TRIGGER,        ctl_chan_assign_trigger)
	_CMD(WRTD_CMD_TDC_CHAN_SET_MODE,              ctl_chan_set_mode)
	_CMD(WRTD_CMD_TDC_CHAN_ARM,                   ctl_chan_arm)
	_CMD(WRTD_CMD_TDC_CHAN_SET_SEQ,               ctl_chan_set_seq)
	_CMD(WRTD_CMD_TDC_CHAN_SET_LOG_LEVEL,         ctl_chan_set_log_level)
	_CMD(WRTD_CMD_TDC_CHAN_RESET_COUNTERS,        ctl_chan_reset_counters)
	_CMD(WRTD_CMD_TDC_PING,                       ctl_ping)
	default:
		  break;
	}

    /* Drop the message once handled */
	mq_discard(0, WRTD_IN_TDC_CONTROL);
}


#define WR_LINK_OFFLINE		1
#define WR_LINK_ONLINE		2
#define WR_LINK_SYNCING		3
#define WR_LINK_SYNCED		4


static int wr_state;

int wr_link_up()
{
	uint32_t stat = dp_readl (BASE_DP_TDC_REGS + TDC_REG_WR_STAT);
	return stat & TDC_WR_STAT_LINK;
}

int wr_time_locked()
{
	uint32_t stat = dp_readl (BASE_DP_TDC_REGS + TDC_REG_WR_STAT);
	return stat & TDC_WR_STAT_AUX_LOCKED;
}

int wr_time_ready()
{
	uint32_t stat = dp_readl (BASE_DP_TDC_REGS + TDC_REG_WR_STAT);
	return stat & TDC_WR_STAT_TIME_VALID;
}

int wr_enable_lock( int enable )
{
    int i = 0;
	
	//dp_writel (TDC_CTRL_DIS_ACQ, BASE_DP_TDC_REGS + TDC_REG_CTRL);
	if(enable)
		dp_writel (TDC_WR_CTRL_ENABLE, BASE_DP_TDC_REGS + TDC_REG_WR_CTRL);
	else
		dp_writel (0, BASE_DP_TDC_REGS + TDC_REG_WR_CTRL);
   
//	dp_writel (TDC_CTRL_EN_ACQ, BASE_DP_TDC_REGS + TDC_REG_CTRL);
}


void wr_update_link()
{
	switch(wr_state)
	{
		case WR_LINK_OFFLINE:
			if ( wr_link_up() )
			{
				pp_printf("rt-tdc: WR link up\n");
				wr_state = WR_LINK_ONLINE;
			}
			break;
		
		case WR_LINK_ONLINE:
			if (wr_time_ready())
			{
				wr_state = WR_LINK_SYNCING;
				wr_enable_lock(1);
			}
			break;

		case WR_LINK_SYNCING:
			if (wr_time_locked())
			{
				pp_printf("rt-tdc: WR synced\n");
				wr_state = WR_LINK_SYNCED;
			}
			break;

		case WR_LINK_SYNCED:
			break;
	}

	if( wr_state != WR_LINK_OFFLINE && !wr_link_up() )
	{
        pp_printf("rt-tdc: WR sync lost\n");
                
		wr_state = WR_LINK_OFFLINE;
		wr_enable_lock(0);
	} 

}

int wr_is_timing_ok()
{
	return (wr_state == WR_LINK_SYNCED);
}

void init()
{
    int i;

    wr_state = WR_LINK_OFFLINE;
    wr_enable_lock(0);

    /* Initialize the TDC FIFO (channels disabled, default dead time) */
    dp_writel( 0x0, BASE_DP_TDC_DIRECT + DR_REG_CHAN_ENABLE);
    dp_writel( DEFAULT_DEAD_TIME, BASE_DP_TDC_DIRECT + DR_REG_DEAD_TIME);

    /* Set up channel states to safe default values */
    for(i=0;i<TDC_NUM_CHANNELS;i++)
    {
    	memset(&channels[i], 0, sizeof(struct tdc_channel_state));
        channels[i].n = i;
        channels[i].mode = WRTD_TRIGGER_MODE_AUTO;
        channels[i].dead_time = DEFAULT_DEAD_TIME;
    }

    pp_printf("rt-tdc firmware initialized.\n");
}

main()
{
    init();

    for(;;)
    {
        do_input();
        do_control();
        wr_update_link();
    }
    return 0;
}
