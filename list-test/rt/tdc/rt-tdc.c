#define WRNODE_APP

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "mqueue.h"

#include "hw/fmctdc-direct.h"

#include "pp-printf.h"

#include "rt-common.h"

#define TDC_CORE_BASE 0x200000
#define CPU_LR_BASE 0x100000

static inline uint32_t tdc_readl ( uint32_t reg )
{
  return *(volatile uint32_t *) ( reg + TDC_CORE_BASE );
}

static inline uint32_t lr_readl ( uint32_t reg )
{
  return *(volatile uint32_t *) ( reg + CPU_LR_BASE );
}

static inline void tdc_writel ( uint32_t value, uint32_t reg )
{
     *(volatile uint32_t *) ( reg + TDC_CORE_BASE ) = value;
}


int puts(const char *p)
{
    int i;
    volatile uint32_t *buf = mq_map_out_buffer(0, 1);
    
    mq_claim(0, 1);
    
    buf[0] = 0xdeadbeef;
    for(i=0;i<127;i++,p++)
      {
  	   if(*p)
	       buf[i+1] = *p;
	     else
	       break;
      }

    mq_send(0, 1, i + 1);
    return i;
}

static void delay(int n)
{
    int i;
    for(i=0;i<n;i++) asm volatile("nop");
}



struct tdc_channel_state channels[TDC_NUM_CHANNELS];

static inline void ts_add(struct list_timestamp *a, const struct list_timestamp *b)
{
    a->frac += b->frac;
    if(a->frac >= 4096)
    {
	a->frac -= 4096;
	a->cycles ++;
    }

    a->cycles += b->cycles;

    if(a->cycles >= 125000000)
    {
	a->cycles -= 125000000;
	a->seconds++;
    }

    a->seconds += b->seconds;
}

static inline void ts_sub(struct list_timestamp *a, const struct list_timestamp *b)
{
    a->frac -= b->frac;
    if(a->frac < 0)
    {
	a->frac += 4096;
	a->cycles --;
    }

    a->cycles -= b->cycles;

    if(a->cycles < 0)
    {
	a->cycles += 125000000;
	a->seconds--;
}

    a->seconds -= b->seconds;
}


struct list_log_message {
    int type;
    int channel;
    struct list_timestamp ts;
    uint32_t seq;
    struct list_id id;
};

static inline void log_raw_timestamp (struct tdc_channel_state *ch, struct list_timestamp *ts)
{
    struct list_log_message *msg = mq_map_out_buffer(1, TDC_OUT_SLOT_LOGGING);
    msg->type = ID_TDC_LOG_RAW_TIMESTAMP;
    msg->channel = ch->n;

}

static inline void send_trigger (struct tdc_channel_state *ch, struct list_timestamp *ts)
{
//    struct tdc_channel_state *ch = &channels[channel];
    struct list_trigger_message *msg = mq_map_out_buffer(1, TDC_OUT_SLOT_REMOTE);

    mq_claim(1, TDC_OUT_SLOT_REMOTE);
    msg->hdr.target_ip = 0xffffffff; // broadcast
    msg->hdr.target_port = 0xebd0;   // port
    msg->hdr.target_offset = 0x20000;// target EB slot
    msg->transmit_cycles = 0; //lr_readl();
    msg->transmit_seconds = 0;
    msg->count = 1;
    msg->triggers[0].id = ch->id;
    msg->triggers[0].seq = ch->seq;
    msg->triggers[0].ts = *ts;
    mq_send(1, TDC_OUT_SLOT_REMOTE, 20); // fixme
}

static inline void log_sent_trigger(struct tdc_channel_state *ch, struct list_timestamp *ts)
{
    uint32_t *buf = mq_map_out_buffer(0, TDC_OUT_SLOT_LOGGING);
    
    mq_claim(0, TDC_OUT_SLOT_LOGGING);
    buf[0] = ch->n;
    buf[1] = ch->seq;
    buf[2] = ch->id.system;
    buf[3] = ch->id.source_port;
    buf[4] = ch->id.trigger;
    buf[5] = 0;
    buf[6] = 0;
    buf[7] = ts->seconds;
    buf[8] = ts->cycles;
    buf[9] = ts->frac;
    
    mq_send(0, TDC_OUT_SLOT_LOGGING, 10);
}


static inline void do_channel (int channel, struct list_timestamp *ts)
{
    struct tdc_channel_state *ch = &channels[channel];

    ts_sub(ts, &ch->timebase_offset);    
    ch->last = *ts;

    if( ch->flags & TDC_CHAN_LOG_RAW )
	   log_raw_timestamp(ch, ts);

    ts_add(ts, &ch->delay);

    if( (ch->flags & TDC_CHAN_TRIGGER_ASSIGNED) && (ch->flags & TDC_CHAN_ARMED) )
    {
    	ch->seq++;
    	ch->flags |= TDC_CHAN_TRIGGERED;
    	if(ch->mode == TDC_CHAN_MODE_SINGLE)
    	    ch->flags &= ~TDC_CHAN_ARMED;
    	
        send_trigger(ch, ts);
    	log_sent_trigger(channel, ts);
    }
        
}

static inline void do_input ()
{
    uint32_t fifo_sr = tdc_readl (DR_REG_FIFO_CSR);
    
    if(! (fifo_sr & DR_FIFO_CSR_EMPTY))
    {
        struct list_timestamp ts;

        ts.seconds = tdc_readl(DR_REG_FIFO_R0);
        ts.cycles  = tdc_readl(DR_REG_FIFO_R1);
        int meta   = tdc_readl(DR_REG_FIFO_R2);

    	ts.frac = (meta & 0x3ffff) * 81 * 64 / 125;
    	ts.cycles += ts.frac >> 12;
    	ts.frac &= 0x3ff;

    	if (ts.cycles >= 125000000)
    	{
    		ts.cycles -= 125000000;
    		ts.seconds ++;
    	}

    	int channel = (meta >> 19) & 0x7;

    	do_channel( channel, &ts );
    }
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
    uint32_t mask = tdc_readl(DR_REG_CHAN_ENABLE);

    int channel = buf[0];
    int enable = buf[1];

    if(enable)
	   mask |= (1<<channel);
    else
	   mask &= ~(1<<channel);
    tdc_writel(mask, DR_REG_CHAN_ENABLE);

    ctl_ack(seq);
}

static inline void ctl_chan_set_dead_time (int seq, uint32_t *buf)
{
    //pp_printf("SDT %d", buf[0]);
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
    struct tdc_channel_state *ch = &channels[channel];
    struct tdc_channel_state_msg *msg = (struct tdc_channel_state_msg *) ctl_claim_out();


    msg->seq = seq;
    msg->state = *ch;
}

static inline void ctl_software_trigger (int seq, uint32_t *buf)
{
    struct list_trigger_message *msg = mq_map_out_buffer(1, TDC_OUT_SLOT_REMOTE);

    mq_claim(1, TDC_OUT_SLOT_REMOTE);
    msg->hdr.target_ip = 0xffffffff; // broadcast
    msg->hdr.target_port = 0xebd0;   // port
    msg->hdr.target_offset = 0x20000;// target EB slot
    msg->transmit_seconds = 0;
    msg->transmit_cycles = 0;
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
    ch->flags &= ~TDC_CHAN_ARMED;

    ctl_ack(seq);
}

static inline void ctl_chan_arm (int seq, uint32_t *buf)
{
    int channel = buf[0];

    struct tdc_channel_state *ch = &channels[channel];

    if(buf[1])
        ch->flags |= TDC_CHAN_ARMED;
    else
        ch->flags &= ~TDC_CHAN_ARMED;

    ctl_ack(seq);
}

static inline void ctl_chan_assign_trigger (int seq, uint32_t *buf)
{
    int channel = buf[0];

    struct tdc_channel_state *ch = &channels[channel];    

    ch->id.system = buf[1];
    ch->id.source_port = buf[2];
    ch->id.trigger = buf[3];

    ch->flags |= TDC_CHAN_TRIGGER_ASSIGNED;

    ctl_ack(seq);
}


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
        _CMD(ID_TDC_CMD_CHAN_ASSIGN_TRIGGER,    ctl_chan_assign_trigger)
        _CMD(ID_TDC_CMD_CHAN_SET_MODE,              ctl_chan_set_mode)
        _CMD(ID_TDC_CMD_CHAN_ARM,                   ctl_chan_arm)
        _CMD(ID_TDC_CMD_PING,                       ctl_ping)
        default:
		  break;
	}

	mq_discard(0, TDC_IN_SLOT_CONTROL);
}

void init()
{
    int i;

    tdc_writel( 0x0, DR_REG_CHAN_ENABLE);
    tdc_writel( 0x1, DR_REG_DEAD_TIME);

    for(i=0;i<TDC_NUM_CHANNELS;i++)
    {
    	memset(&channels[i], 0, sizeof(struct tdc_channel_state));
        channels[i].n = i;
    }
}


main()
{   
    int i = 0;
    init();

    pp_printf("RT_TDC firmware initialized.");

    for(;;)
    {
        do_input();
        do_control();
    }

}