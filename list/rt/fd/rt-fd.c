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
 * rt-fd.c: real-time CPU application for the FMC Fine Delay mezzanine (Trigger Output)
 */


#include <string.h>

#include "rt.h"

#include "list-common.h"
#include "list-serializers.h"

#include "hw/fd_channel_regs.h"
#include "hash.h"
#include "loop-queue.h"

struct pulse_queue_entry {
    struct list_trigger_entry trig;
    int origin_cycles;
    struct lrt_output_rule *rule;
};

struct lrt_pulse_queue 
{
    struct pulse_queue_entry data[FD_MAX_QUEUE_PULSES];
    int head, tail, count;
};

struct lrt_output {
    uint32_t base_addr;
    int index;
    int hits;
    int miss_timeout, miss_deadtime, miss_overflow, miss_no_timing;
    struct list_trigger_entry last_executed;
    struct list_trigger_entry last_enqueued;
    struct list_trigger_entry last_received;
    struct list_timestamp last_programmed;
    struct list_trigger_entry last_lost;
    int idle;
    int state;
    int mode;
    uint32_t flags;
    uint32_t log_level;
    int dead_time;
    int width_cycles;
    struct lrt_pulse_queue queue;
};

struct lrt_trigger_handle {
    struct lrt_hash_entry *trig;
    struct lrt_hash_entry *cond;
    int cpu;
    int channel;
};

static inline void ts_adjust_delay(struct list_timestamp *ts, uint32_t cycles, uint32_t frac)
{
    ts->frac += frac;
    
    if(ts->frac & 0x1000)
	ts->cycles++;

    ts->frac &= 0xfff;
    ts->cycles += cycles;

    if(ts->cycles >= 125000000)
    {
	ts->cycles -= 125000000;
	ts->seconds++;
    }
}

void pulse_queue_init(struct lrt_pulse_queue *p)
{
    p->head = 0;
    p->tail = 0;
    p->count = 0;
}

struct pulse_queue_entry *pulse_queue_push(struct lrt_pulse_queue *p)
{
    if(p->count == FD_MAX_QUEUE_PULSES)
	   return NULL;

    p->count++;
    struct pulse_queue_entry *ent = &p->data[p->head];
    p->head++;
    if(p->head == FD_MAX_QUEUE_PULSES)
	   p->head = 0;

    return ent;
}

int pulse_queue_empty(struct lrt_pulse_queue *p)
{
    return p->count == 0;
}

struct pulse_queue_entry* pulse_queue_front(struct lrt_pulse_queue *p)
{
    if(!p->count)
	   return NULL;
    return &p->data[p->tail];
}

void pulse_queue_pop(struct lrt_pulse_queue *p)
{
    p->tail++;
    if(p->tail == FD_MAX_QUEUE_PULSES)
	   p->tail = 0;
    p->count--;
}




static inline int check_dead_time( struct lrt_output *out, struct list_timestamp *ts )
{
    if(out->idle)
        return 1;

    int delta_s = ts->seconds - out->last_enqueued.ts.seconds;
    int delta_c = ts->cycles - out->last_enqueued.ts.cycles;

    if(delta_c < 0)
    {
        delta_c += 125 * 1000 * 1000;
        delta_s--;
    }

    if(delta_s < 0 || delta_c < out->dead_time)
        return 0;
    return 1;
}


struct lrt_output outputs[FD_NUM_CHANNELS];
static int rx_ebone = 0, rx_loopback = 0;

void init_outputs()
{
    int i;

    for(i=0; i<FD_NUM_CHANNELS;i++)
    {
        outputs[i].base_addr = 0x100 + i * 0x100;
        outputs[i].index = i;
        outputs[i].idle = 1;
        outputs[i].dead_time = 80000 / 8; // 80 us
        outputs[i].width_cycles = 1250; // 1us
    }
}

static inline void fd_ch_writel (struct lrt_output *out, uint32_t value, uint32_t reg)
{
    dp_writel( value , reg + out->base_addr );
}

static inline uint32_t fd_ch_readl (struct lrt_output *out,  uint32_t reg)
{
    return dp_readl( reg + out->base_addr );
}

void do_output( struct lrt_output *out )
{
        
    struct lrt_pulse_queue *q = &out->queue;
    uint32_t dcr = fd_ch_readl(out, FD_REG_DCR);

    if(!out->idle) {
        if (!(dcr & FD_DCR_PG_TRIG)) // still waiting for trigger
        {
            struct list_timestamp tc;

            tc.seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
            tc.cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);

	    int delta = tc.seconds - out->last_programmed.seconds;
	    delta *= 125000000;
	    delta += tc.cycles - out->last_programmed.cycles;

	    if(delta > 0)
	    {
        	pulse_queue_pop(q);
	        
	        out->miss_timeout ++;
		out->idle = 1;
	        fd_ch_writel(out, FD_DCR_MODE, FD_REG_DCR);

	    }
        } else {
	    out->last_executed = pulse_queue_front(q)->trig;
	    pulse_queue_pop(q);
	    out->hits++;
	    out->idle = 1;
	}

	return;
    } 
    

    if(pulse_queue_empty(q))
        return;

    struct pulse_queue_entry *pq_ent = pulse_queue_front(q);
    struct list_timestamp *ts = &pq_ent->trig.ts;

    gpio_set(0);
    gpio_clear(0);

    fd_ch_writel(out, ts->seconds, FD_REG_U_STARTL);
    fd_ch_writel(out, ts->cycles, FD_REG_C_START);
    fd_ch_writel(out, ts->frac, FD_REG_F_START);
    
    ts->cycles += out->width_cycles;
    if(ts->cycles >= 125000000)
    {
        ts->cycles -= 125000000;
        ts->seconds++;
    }

    fd_ch_writel(out, ts->seconds, FD_REG_U_ENDL);
    fd_ch_writel(out, ts->cycles, FD_REG_C_END);
    fd_ch_writel(out, ts->frac, FD_REG_F_END);
    fd_ch_writel(out, 0, FD_REG_RCR);
    fd_ch_writel(out, FD_DCR_MODE, FD_REG_DCR);
    fd_ch_writel(out, FD_DCR_MODE | FD_DCR_UPDATE, FD_REG_DCR);
    fd_ch_writel(out, FD_DCR_MODE | FD_DCR_PG_ARM | FD_DCR_ENABLE, FD_REG_DCR);

    ts->cycles += 1000;
    if(ts->cycles >= 125000000)
    {
        ts->cycles -= 125000000;
        ts->seconds++;
    }

    volatile uint32_t dummy = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
    int latency = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES) - pq_ent->origin_cycles;

    if(latency < 0)
	latency += 125000000;

    if( pq_ent->rule->worst_latency < latency)
	 pq_ent->rule->worst_latency = latency;

    // fixme: hardware miss detection?

    out->last_programmed = *ts;
    
    out->idle = 0;

    gpio_set(0);
    gpio_clear(0);
}




void enqueue_trigger ( int output, struct lrt_output_rule *rule, struct list_id *id, struct list_timestamp *ts, int seq)
{
    struct lrt_output *out = &outputs[output];
    struct list_timestamp adjusted = *ts;
    struct pulse_queue_entry *pq_ent;
    ts_adjust_delay (&adjusted, rule->delay_cycles, rule->delay_frac);

//    if(!rule->enabled)
//	return;

    if(!check_dead_time(out, &adjusted))
    {
        out->miss_deadtime ++;
        return;
    }

    if( (pq_ent = pulse_queue_push( &out->queue )) == NULL)
    {
        out->miss_overflow ++;
        return;
    }

    pq_ent->trig.ts = adjusted;
    pq_ent->trig.id = *id;
    pq_ent->trig.seq = seq;
    pq_ent->origin_cycles = ts->cycles;
    pq_ent->rule = rule;

    out->last_enqueued.ts = *ts;
    out->last_enqueued.id = *id;
    out->last_enqueued.seq = seq;

    gpio_set(0);
    gpio_clear(0);

}

static inline void filter_trigger(struct list_trigger_entry *trig)
{
    struct lrt_hash_entry *ent = hash_search( &trig->id, NULL );
    int j;
    
    if(ent)
    {
        struct list_timestamp ts = trig->ts;
        struct list_id id = trig->id;
        int seq = trig->seq;
        for(j = 0; j < FD_NUM_CHANNELS; j++)
	    if(ent->ocfg[j].state != HASH_ENT_EMPTY)
    	    {
        	gpio_set(0);
                gpio_clear(0);
                enqueue_trigger ( j, &ent->ocfg[j], &id, &ts, seq );
            }
    }
}

void do_rx()
{
//    wr_up = 

    if( rmq_poll( FD_IN_SLOT_REMOTE) )
    {
	struct list_trigger_message *msg = mq_map_in_buffer(1, FD_IN_SLOT_REMOTE) - sizeof(struct rmq_message_addr);
        int i, cnt = msg->count;

	for(i=0; i < cnt; i++)
    	    filter_trigger( &msg->triggers[i] );

	mq_discard(1, FD_IN_SLOT_REMOTE);
	rx_ebone ++;
    }
    
    struct list_trigger_entry *ent = loop_queue_pop();
    
    if(ent)
    {
	filter_trigger(ent);
	rx_loopback++;
    }
    
    gpio_set(1);
    gpio_clear(1);
}

void do_outputs()
{
    int i;
    
    for(i=0;i < FD_NUM_CHANNELS;i++)
        do_output(&outputs[i]);
}


static inline struct mq_buffer ctl_claim_out()
{
    mq_claim(0, FD_OUT_SLOT_CONTROL);
    return mq_buffer_init_out( 64, 0, FD_OUT_SLOT_CONTROL );
}

static inline void ctl_ack( uint32_t seq )
{
    struct mq_buffer buf = ctl_claim_out();

    bag_int ( &buf, ID_REP_ACK );
    bag_int ( &buf, seq );

    mq_buffer_send ( &buf, 0, FD_OUT_SLOT_CONTROL );
}

static inline void ctl_nack( uint32_t seq, int err )
{
    struct mq_buffer buf = ctl_claim_out();

    bag_int ( &buf, ID_REP_NACK );
    bag_int ( &buf, seq );
    bag_int ( &buf, err );

    mq_buffer_send ( &buf, 0, FD_OUT_SLOT_CONTROL );
}

static inline void ctl_chan_assign_trigger (int seq, uint32_t *buf)
{
    struct list_id id;
    struct lrt_output_rule rule;
    struct lrt_trigger_handle handle;
    int ch = buf[0];
    int is_cond = buf[4];
    int n_req = is_cond ? 2 : 1;


//    pp_printf("assign ch %d, fc %d", ch, hash_free_count());

    if(hash_free_count() < n_req)
    {
        ctl_nack(seq, -1);
        return;
    }

    id.system = buf[1];
    id.source_port = buf[2];
    id.trigger = buf[3];

    rule.delay_cycles = 100000000 / 8000;
    rule.delay_frac = 0;
    rule.state = (is_cond ? HASH_ENT_CONDITIONAL : HASH_ENT_DIRECT) | HASH_ENT_DISABLED;
    rule.cond_ptr = NULL;

    handle.channel = ch;
    handle.cond = NULL;
    handle.trig = hash_add ( &id, ch, &rule );
        
    if(!is_cond) // unconditional trigger - send its handle to the host app
    {
	struct mq_buffer obuf = ctl_claim_out();

        bag_int ( &obuf, ID_REP_TRIGGER_HANDLE );
        bag_int ( &obuf, seq );
        bag_int ( &obuf, handle.channel );
        bag_int ( &obuf, (uint32_t) handle.cond );
        bag_int ( &obuf, (uint32_t) handle.trig );

        mq_buffer_send ( &obuf, 0, FD_OUT_SLOT_CONTROL );
        return;
    } 
    
    id.system = buf[5];
    id.source_port = buf[6];
    id.trigger = buf[7];

    rule.delay_cycles = 100000000 / 8000;
    rule.delay_frac = 0;
    rule.state = HASH_ENT_CONDITION | HASH_ENT_DISABLED; 
    rule.cond_ptr = (struct lrt_output_rule *) handle.trig;

    handle.cond = hash_add ( &id, ch, &rule );


    struct mq_buffer obuf = ctl_claim_out();
    
    bag_int ( &obuf, ID_REP_TRIGGER_HANDLE );
    bag_int ( &obuf, seq );
    bag_int ( &obuf, handle.channel );
    bag_int ( &obuf, (uint32_t) handle.cond );
    bag_int ( &obuf, (uint32_t) handle.trig );

    mq_buffer_send ( &obuf, 0, FD_OUT_SLOT_CONTROL );
}

static inline void ctl_chan_remove_trigger (int seq, uint32_t *buf)
{
    struct lrt_trigger_handle handle;
    
    handle.channel = buf[0];
    handle.cond = (struct lrt_hash_entry *) buf[1];
    handle.trig = (struct lrt_hash_entry *) buf[2];

    if(handle.cond)
	   hash_remove(handle.cond, handle.channel);
    hash_remove(handle.trig, handle.channel);

    // fixme: purge pending pulses
    ctl_ack(seq);
}



static inline void ctl_read_hash(int seq, uint32_t *buf)
{
    int bucket = buf[0];
    int pos = buf[1];
    int ch = buf[2];

    struct mq_buffer obuf = ctl_claim_out();

    struct lrt_hash_entry *ent = hash_get_entry (bucket, pos);
    struct lrt_hash_entry *cond = NULL;
    
    bag_int( &buf, ID_REP_HASH_ENTRY );
    bag_int( &buf, seq );
    bag_int( &buf, ent ? 1 : 0 );

    if(ent)
    {
        cond = (struct lrt_hash_entry *) ent->ocfg[ch].cond_ptr;
    
	if(cond)
	{
	    bag_id ( &buf, &cond->id );
	    bag_int ( &buf, cond->ocfg[ch].delay_cycles );
	    bag_int ( &buf, cond->ocfg[ch].delay_frac );
    	    bag_int ( &buf, (ent->ocfg[ch].state & HASH_ENT_DISABLED) | HASH_ENT_CONDITION );
    	    bag_int ( &buf, (uint32_t) cond );
    	    bag_int ( &buf, (uint32_t) ent );
            bag_id ( &buf, &ent->id);
    	    bag_int( &buf, ent->ocfg[ch].delay_cycles );
    	    bag_int( &buf, ent->ocfg[ch].delay_frac );
    	    bag_int( &buf, HASH_ENT_CONDITIONAL );
	} else {
	    bag_id ( &buf, &ent->id );
	    bag_int ( &buf, ent->ocfg[ch].delay_cycles );
	    bag_int ( &buf, ent->ocfg[ch].delay_frac );
    	    bag_int ( &buf, ent->ocfg[ch].state );
    	    bag_int ( &buf, (uint32_t) cond );
    	    bag_int ( &buf, (uint32_t) ent );
    	    bag_skip ( &buf, 6);
	}
    }


    mq_send(0, FD_OUT_SLOT_CONTROL, 18);
}

static inline void ctl_chan_set_dead_time (int seq, uint32_t *buf)
{
    int i;
    int channel = buf[0];

    outputs[channel].dead_time = buf[1];
   
    ctl_ack(seq);
}


static inline void ctl_ping (int seq, uint32_t *buf)
{
    ctl_ack(seq);
}

static inline void ctl_chan_set_delay (int seq, uint32_t *buf)
{
    int channel = buf[0];
    
    struct lrt_hash_entry *ent = (struct lrt_hash_entry *) buf[1];
    
    ent->ocfg[channel].delay_cycles = buf[2];
    ent->ocfg[channel].delay_frac = buf[3];

    ctl_ack(seq);
}


static inline void ctl_chan_get_state (int seq, uint32_t *buf)
{
    int channel = buf[0];

    struct lrt_output *st = &outputs[channel];
    uint32_t *obuf = ctl_claim_out();
  
  
    obuf[0] = ID_REP_STATE;
    obuf[1] = seq;
    obuf[2] = channel;
    obuf[3] = st->hits;
    obuf[4] = st->miss_timeout;
    obuf[5] = st->miss_deadtime; 
    obuf[6] = st->miss_overflow;
    obuf[7] = st->miss_no_timing;

    bag_timestamp(obuf + 8, &st->last_executed);
    bag_timestamp(obuf + 15, &st->last_enqueued);

    obuf[19] = st->idle;
    obuf[20] = st->state;
    obuf[21] = st->mode;
    obuf[22] = st->flags;
    obuf[23] = st->log_level;
    obuf[24] = st->dead_time;
    obuf[25] = st->width_cycles;
    obuf[27] = rx_ebone;
    obuf[28] = rx_loopback;

    bag_timestamp(obuf + 16, &st->last_received);
    bag_timestamp(obuf + 29, &st->last_lost);

    mq_send(0, FD_OUT_SLOT_CONTROL, 29);
}

static inline void ctl_software_trigger (int seq, uint32_t *buf)
{
   

 
    ctl_ack(seq);
}

static inline void ctl_chan_set_mode (int seq, uint32_t *buf)
{
    int channel = buf[0];
    struct lrt_output *st = &outputs[channel];

    st->mode = buf[1];
    st->flags &= ~(LIST_ARMED | LIST_TRIGGERED | LIST_LAST_VALID) ;

    ctl_ack(seq);
}


static inline void ctl_chan_arm (int seq, uint32_t *buf)
{
    int channel = buf[0];
    struct lrt_output *st = &outputs[channel];

    st->flags |= LIST_ARMED;
    st->flags &= ~LIST_TRIGGERED;

    ctl_ack(seq);
}

static inline void ctl_chan_set_log_level (int seq, uint32_t *buf)
{
    int channel = buf[0];
    //struct tdc_channel_state *ch = &channels[channel];    
    
    //ch->log_level = buf[1];
    ctl_ack(seq);
}


static inline void do_control()
{
    uint32_t p = mq_poll();


    if(! ( p & ( 1<< FD_IN_SLOT_CONTROL )))
        return;


    uint32_t *buf = mq_map_in_buffer( 0, FD_IN_SLOT_CONTROL );

    int cmd = buf[0];
    int seq = buf[1];

//    pp_printf("Poll %x Cmd %x\n", p, cmd);
    
#define _CMD(id, func)          \
    case id:                    \
    {                           \
        func(seq, buf + 2);     \
        break;                  \
    }

    switch(cmd)
    {
        _CMD(ID_FD_CMD_CHAN_ASSIGN_TRIGGER,                ctl_chan_assign_trigger)
        _CMD(ID_FD_CMD_CHAN_REMOVE_TRIGGER,                ctl_chan_remove_trigger)
        _CMD(ID_FD_CMD_CHAN_SET_DELAY,	                   ctl_chan_set_delay)
        _CMD(ID_FD_CMD_READ_HASH,                 	   ctl_read_hash)
        _CMD(ID_FD_CMD_CHAN_SET_MODE,                      ctl_chan_set_mode)
        _CMD(ID_FD_CMD_CHAN_ARM,                           ctl_chan_arm)
        _CMD(ID_FD_CMD_CHAN_GET_STATE,                     ctl_chan_get_state)
        
        default:
          break;
    }

    mq_discard(0, FD_IN_SLOT_CONTROL);
}

void init() 
{
    hash_init();
    init_outputs();
}


main()
{   
    rt_set_debug_slot(FD_OUT_SLOT_CONTROL);
    init();

    gpio_set(24);
    gpio_set(25);
    gpio_set(26);

    int loops =0;    

    /* reset Fine Delay */
    dp_writel( (0xdead << 16) | 0x0, 0x0);
    delay(1000);
    dp_writel( (0xdead << 16) | 0x3, 0x0);
    delay(1000);

    pp_printf("RT_FD firmware initialized.");

    
    for(;;)
    {
        do_rx();
        do_outputs();
        do_control();
    }

}