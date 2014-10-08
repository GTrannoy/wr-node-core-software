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
#include "list-common.h"
#include "hw/fmctdc-direct.h"

#define DEFAULT_DEAD_TIME (80000/16)

struct tdc_channel_state {
    struct list_id id;
    struct list_timestamp delay;
    struct list_timestamp timebase_offset;
    struct list_timestamp last;
    struct list_timestamp prev;
    struct list_trigger_entry last_sent;
    uint32_t flags;
    uint32_t log_level;
    enum list_trigger_mode mode;
    int32_t n;
    int32_t total_pulses;
    int32_t sent_pulses;
    uint32_t seq;
    uint32_t dead_time;
    
};

static struct tdc_channel_state channels[TDC_NUM_CHANNELS];
static int sent_packets = 0;

static inline void log_raw_timestamp (struct tdc_channel_state *ch, struct list_timestamp *ts)
{
    volatile struct list_log_entry *msg = mq_map_out_buffer(0, TDC_OUT_SLOT_LOGGING);
    
    mq_claim(0, TDC_OUT_SLOT_LOGGING);
    msg->type = LIST_LOG_RAW;
    msg->channel = ch->n;
    msg->seq = ch->total_pulses;
    msg->ts = *ts;
    mq_send(0, TDC_OUT_SLOT_LOGGING, sizeof(struct list_log_entry) / 4);
}

static int coalesce_count = 0;

    
static inline void send_trigger (struct tdc_channel_state *ch, struct list_timestamp *ts)
{
    volatile struct list_trigger_message *msg = mq_map_out_buffer(1, TDC_OUT_SLOT_REMOTE);

    msg->triggers[coalesce_count].id = ch->id;
    msg->triggers[coalesce_count].seq = ch->seq;
    msg->triggers[coalesce_count].ts = *ts; 
    
    ch->last = *ts;
    coalesce_count++;
}

static inline void claim_tx()
{
    mq_claim(1, TDC_OUT_SLOT_REMOTE);
}

static inline void flush_tx ()
{
    volatile struct list_trigger_message *msg = mq_map_out_buffer(1, TDC_OUT_SLOT_REMOTE);

    msg->hdr.target_ip = 0xffffffff; // broadcast
    msg->hdr.target_port = 0xebd0;   // port
    msg->hdr.target_offset = 0x4000;// target EB slot
    msg->transmit_seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
    msg->transmit_cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
    msg->count = coalesce_count;
    mq_send(1, TDC_OUT_SLOT_REMOTE, 6 + 7 * coalesce_count);
    coalesce_count = 0;
    sent_packets ++;
}



static inline void log_sent_trigger(struct tdc_channel_state *ch, struct list_timestamp *ts)
{
    volatile struct list_log_entry *msg =  mq_map_out_buffer(0, TDC_OUT_SLOT_LOGGING);
    
    mq_claim(0, TDC_OUT_SLOT_LOGGING);
    msg->type = ID_LOG_SENT_TRIGGER;
    msg->seq = ch->seq;
    msg->id = ch->id;
    msg->ts = *ts;
    
    mq_send(0, TDC_OUT_SLOT_LOGGING, sizeof(struct list_log_entry) / 4);
}


static inline void do_channel (int channel, struct list_timestamp *ts)
{
    struct tdc_channel_state *ch = &channels[channel];

    ts_sub(ts, &ch->timebase_offset);    
    ch->last = *ts;

	if(ch->log_level & LIST_LOG_RAW)
        log_raw_timestamp(ch, ts);

    ch->last = *ts;
    ch->total_pulses++;

    ts_add(ts, &ch->delay);

    int delta = ts->seconds - ch->prev.seconds;
    delta *= 125000000;    
    delta += ts->cycles - ch->prev.cycles;

    if(delta < 0)
	pp_printf("FUCK!");

    ch->prev = *ts;

    if( (ch->flags & LIST_TRIGGER_ASSIGNED ) && (ch->flags & LIST_ARMED) )
    {
    	ch->seq++;
    	ch->flags |= LIST_TRIGGERED;
    	if(ch->mode == LIST_TRIGGER_MODE_SINGLE )
    	    ch->flags &= ~LIST_ARMED;
    	
        ch->sent_pulses++;
        send_trigger(ch, ts);
        ch->last_sent.ts  = *ts;
        ch->last_sent.id = ch->id;
        ch->last_sent.seq = ch->seq;
        ch->flags |= LIST_LAST_VALID;

	    if(ch->log_level & LIST_LOG_SENT)
            log_sent_trigger(ch, ts);
    }     
}

static inline void do_input ()
{
    int i;

    claim_tx();

    for(i = 0; i < TDC_TRIGGER_COALESCE_LIMIT; i++)
    {
        uint32_t fifo_sr = dp_readl (DR_REG_FIFO_CSR);
        struct list_timestamp ts;
        int meta;

        if(fifo_sr & DR_FIFO_CSR_EMPTY)
            break;
        
        ts.seconds = dp_readl(DR_REG_FIFO_R0);
        ts.cycles  = dp_readl(DR_REG_FIFO_R1);
        meta   = dp_readl(DR_REG_FIFO_R2);

        // convert from ACAM bins (81ps) to WR time format
        ts.frac = ( (meta & 0x3ffff) * 5308 ) >> 7;
    
#if 0
        if(ts.cycles >= 125000000) // fixme: fix in hw (Eva working on the issue)
        {
            ts.cycles -= 125000000;
            ts.seconds --;
        }
#endif

    	ts.cycles += ts.frac >> 12;
    	ts.frac &= 0xfff;
        
    	if (ts.cycles >= 125000000)
    	{
    		ts.cycles -= 125000000;
    		ts.seconds ++;
    	}

    	int channel = (meta >> 19) & 0x7;

        do_channel( channel, &ts );
    }
    
    if(coalesce_count)
        flush_tx();

};

static inline uint32_t *ctl_claim_out()
{
    mq_claim(0, TDC_OUT_SLOT_CONTROL);
    return mq_map_out_buffer( 0, TDC_OUT_SLOT_CONTROL );
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
    buf[1] = seq;
    mq_send(0, TDC_OUT_SLOT_CONTROL, 2);
}

static inline void ctl_chan_enable (int seq, uint32_t *buf)
{
    uint32_t mask = dp_readl(DR_REG_CHAN_ENABLE);
    int channel = buf[0];
    struct tdc_channel_state *ch = &channels[channel];
    
    int enable = buf[1];

    if(enable)
    {
	   mask |= (1<<channel);
       ch->flags |= LIST_ENABLED;
    } else {
	   mask &= ~(1<<channel);
       ch->flags &= ~LIST_ENABLED | LIST_ARMED | LIST_TRIGGERED | LIST_LAST_VALID;
    } 

    dp_writel(mask, DR_REG_CHAN_ENABLE);

    ctl_ack(seq);
}

static inline void ctl_chan_set_dead_time (int seq, uint32_t *buf)
{
    int i;
    dp_writel( buf[1], DR_REG_DEAD_TIME);
    for(i=0; i < TDC_NUM_CHANNELS; i++)
        channels[i].dead_time = buf[1];
    ctl_ack(seq);
}


static inline void ctl_ping (int seq, uint32_t *buf)
{
    ctl_ack(seq);
}

static inline void ctl_chan_set_delay (int seq, uint32_t *buf)
{
    int channel = buf[0];
    struct tdc_channel_state *ch = &channels[channel];

    ch->delay.seconds = buf[1];
    ch->delay.cycles = buf[2];
    ch->delay.frac = buf[3];
    ctl_ack(seq);
}

static inline void ctl_chan_set_timebase_offset (int seq, uint32_t *buf)
{
    int channel = buf[0];
    struct tdc_channel_state *ch = &channels[channel];

    ch->timebase_offset.seconds = buf[1];
    ch->timebase_offset.cycles = buf[2];
    ch->timebase_offset.frac = buf[3];

    ctl_ack(seq);
}

static inline void ctl_chan_get_state (int seq, uint32_t *buf)
{
    int channel = buf[0];
    struct tdc_channel_state *st = &channels[channel];
    volatile uint32_t *obuf = ctl_claim_out();
  
    obuf[0] = ID_REP_STATE;
    obuf[1] = seq;
    obuf[2] = channel;

    obuf[3] = st->id.system;
    obuf[4] = st->id.source_port;
    obuf[5] = st->id.trigger;
    
    obuf[6] = st->delay.seconds;
    obuf[7] = st->delay.cycles;
    obuf[8] = st->delay.frac;
    
    obuf[9] = st->timebase_offset.seconds;
    obuf[10] = st->timebase_offset.cycles;
    obuf[11] = st->timebase_offset.frac;
    
    obuf[12] = st->last.seconds;
    obuf[13] = st->last.cycles;
    obuf[14] = st->last.frac;
    
    obuf[15] = st->flags;
    obuf[16] = st->log_level;
    obuf[17] = st->mode;

    obuf[18] = st->total_pulses;
    obuf[19] = st->sent_pulses;
    obuf[20] = st->dead_time;

    obuf[21] = st->last_sent.ts.seconds;
    obuf[22] = st->last_sent.ts.cycles;
    obuf[23] = st->last_sent.ts.frac;
    obuf[24] = st->last_sent.id.system;
    obuf[25] = st->last_sent.id.source_port;
    obuf[26] = st->last_sent.id.trigger;
    obuf[27] = st->last_sent.seq;
    obuf[28] = sent_packets;

    mq_send(0, TDC_OUT_SLOT_CONTROL, 29);
}

static inline void ctl_software_trigger (int seq, uint32_t *buf)
{
    struct list_trigger_message *msg = mq_map_out_buffer(1, TDC_OUT_SLOT_REMOTE);

    mq_claim(1, TDC_OUT_SLOT_REMOTE);
    
    msg->hdr.target_ip = 0xffffffff; // broadcast
    msg->hdr.target_port = 0xebd0;   // port
    msg->hdr.target_offset = 0x4000;// target EB slot
    msg->transmit_seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
    msg->transmit_cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
    msg->count = 1;

    memcpy(&msg->triggers[0], buf, sizeof(struct list_trigger_entry) );
    
    mq_send(1, TDC_OUT_SLOT_REMOTE, sizeof(struct list_trigger_message) / 4); // fixme
    ctl_ack(seq);
}

static inline void ctl_chan_set_mode (int seq, uint32_t *buf)
{
    int channel = buf[0];

    struct tdc_channel_state *ch = &channels[channel];

    ch->mode = buf[1];
    ch->flags &= ~(LIST_ARMED | LIST_TRIGGERED | LIST_LAST_VALID) ;

    ctl_ack(seq);
}


static inline void ctl_chan_set_seq (int seq, uint32_t *buf)
{
    int channel = buf[0];

    struct tdc_channel_state *ch = &channels[channel];
    ch->seq = buf[1];
    ctl_ack(seq);
}


static inline void ctl_chan_arm (int seq, uint32_t *buf)
{
    int channel = buf[0];

    struct tdc_channel_state *ch = &channels[channel];

    if(buf[1]) {
        ch->flags |= LIST_ARMED;
        ch->flags &= ~LIST_TRIGGERED;
    }
    else
        ch->flags &= ~(LIST_ARMED | LIST_TRIGGERED);

    ctl_ack(seq);
}

static inline void ctl_chan_assign_trigger (int seq, uint32_t *buf)
{
    int channel = buf[0];

    struct tdc_channel_state *ch = &channels[channel];    

    int do_assign = buf[1];
    
    if(do_assign)
    {
        ch->id.system = buf[2];
        ch->id.source_port = buf[3];
        ch->id.trigger = buf[4];
        ch->flags |= LIST_TRIGGER_ASSIGNED;
        ch->flags &= ~LIST_LAST_VALID;
    } else {
        ch->id.system = 0;
        ch->id.source_port = 0;
        ch->id.trigger = 0;
        ch->flags &= ~LIST_TRIGGER_ASSIGNED;
    }

    ctl_ack(seq);
}

static inline void ctl_chan_reset_counters (int seq, uint32_t *buf)
{
    int channel = buf[0];
    struct tdc_channel_state *ch = &channels[channel];    
    
    ch->total_pulses = 0;
    ch->sent_pulses = 0;
    ch->flags &= ~LIST_LAST_VALID;

    ctl_ack(seq);
}

static inline void ctl_chan_set_log_level (int seq, uint32_t *buf)
{
    int channel = buf[0];
    struct tdc_channel_state *ch = &channels[channel];    
    
    ch->log_level = buf[1];
    ctl_ack(seq);
}

int n = 0;

static inline void do_control()
{
    uint32_t p = mq_poll();


    if(! ( p & ( 1<< TDC_IN_SLOT_CONTROL )))
        return;

    uint32_t *buf = mq_map_in_buffer( 0, TDC_IN_SLOT_CONTROL );

	int cmd = buf[0];
	int seq = buf[1];
    
#define _CMD(id, func)          \
    case id:                    \
    {                           \
        func(seq, buf + 2);     \
        break;                  \
    }

	switch(cmd)
	{
        _CMD(ID_TDC_CMD_CHAN_ENABLE,                ctl_chan_enable)
	_CMD(ID_TDC_CMD_CHAN_SET_DEAD_TIME,         ctl_chan_set_dead_time)
	_CMD(ID_TDC_CMD_CHAN_SET_DELAY,             ctl_chan_set_delay)
        _CMD(ID_TDC_CMD_CHAN_SET_TIMEBASE_OFFSET,   ctl_chan_set_timebase_offset)
        _CMD(ID_TDC_CMD_CHAN_GET_STATE,             ctl_chan_get_state)
        _CMD(ID_TDC_CMD_SOFTWARE_TRIGGER,           ctl_software_trigger)
        _CMD(ID_TDC_CMD_CHAN_ASSIGN_TRIGGER,        ctl_chan_assign_trigger)
        _CMD(ID_TDC_CMD_CHAN_SET_MODE,              ctl_chan_set_mode)
        _CMD(ID_TDC_CMD_CHAN_ARM,                   ctl_chan_arm)
        _CMD(ID_TDC_CMD_CHAN_SET_SEQ,               ctl_chan_set_seq)
        _CMD(ID_TDC_CMD_CHAN_SET_LOG_LEVEL,         ctl_chan_set_log_level)
        _CMD(ID_TDC_CMD_CHAN_RESET_COUNTERS,        ctl_chan_reset_counters)
        _CMD(ID_TDC_CMD_PING,                       ctl_ping)
        default:
		  break;
	}

	mq_discard(0, TDC_IN_SLOT_CONTROL);
}

void init()
{
    int i;

    dp_writel( 0x0, DR_REG_CHAN_ENABLE);
    dp_writel( DEFAULT_DEAD_TIME, DR_REG_DEAD_TIME);

    for(i=0;i<TDC_NUM_CHANNELS;i++)
    {
    	memset(&channels[i], 0, sizeof(struct tdc_channel_state));
        channels[i].n = i;
        channels[i].mode = LIST_TRIGGER_MODE_AUTO;
        channels[i].dead_time = DEFAULT_DEAD_TIME;
    }
}


main()
{   
    int i = 0;
    rt_set_debug_slot(TDC_OUT_SLOT_CONTROL);
    init();

    pp_printf("RT_TDC firmware initialized.");

    gpio_set(24);
    gpio_set(25);
    gpio_set(26);
    
    for(;;)
    {
        do_input();
        do_control();
    }

}