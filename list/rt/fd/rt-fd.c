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
#include "hw/fd_channel_regs.h"
#include "hash.h"

struct lrt_pulse_queue 
{
    struct list_timestamp data[FD_MAX_QUEUE_PULSES];
    int head, tail, count;
};

struct lrt_output {
    uint32_t base_addr;
    int index;
    int hits, miss_timeout, miss_deadtime, miss_overflow;
    struct list_id last_id;
    struct list_timestamp last_executed;
    struct list_timestamp last_enqueued;
    struct list_timestamp last_programmed, l1, l2, l3;
    int idle;
    int state;
    int mode;
    uint32_t flags;
    uint32_t log_level;
    int dead_time;
    int width_cycles;
    struct lrt_pulse_queue queue;
    int worst_latency;
    int pgms;
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

int pulse_queue_push(struct lrt_pulse_queue *p, struct list_timestamp *ts)
{
    if(p->count == FD_MAX_QUEUE_PULSES)
	   return -1;

    p->count++;
    p->data[p->head] = *ts;
    p->head++;
    if(p->head == FD_MAX_QUEUE_PULSES)
	   p->head = 0;

    return 0;
}

int pulse_queue_empty(struct lrt_pulse_queue *p)
{
    return p->count == 0;
}

struct list_timestamp* pulse_queue_front(struct lrt_pulse_queue *p)
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

    int delta_s = ts->seconds - out->last_enqueued.seconds;
    int delta_c = ts->cycles - out->last_enqueued.cycles;

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
static int rx_total = 0;

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

static int last_cyc=0;

static struct list_timestamp prev_ts,prev_tc;

void do_output( struct lrt_output *out )
{
        
    struct lrt_pulse_queue *q = &out->queue;
    uint32_t dcr = fd_ch_readl(out, FD_REG_DCR);

    if(!out->idle) {
        if (!(dcr & FD_DCR_PG_TRIG)) // still waiting for trigger
        {
            struct list_timestamp tc;

            tc.cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
            tc.seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);


     int delta = tc.seconds - prev_tc.seconds;
    delta *= 125000000;
    delta += tc.cycles - prev_tc.cycles;

    if(delta < 0)
	pp_printf("FUCK-fd!");

	    prev_tc = tc;
	    delta = tc.seconds - out->last_programmed.seconds;
	    
	    delta *= 125000000;
	    delta += tc.cycles - out->last_programmed.cycles;

	    if(delta > 0)
	    {
                out->miss_timeout ++;
		out->idle = 1;
	        fd_ch_writel(out, FD_DCR_MODE, FD_REG_DCR);

    		out->l1 = out->last_programmed;//tc.seconds; //delta;
		out->l2 = tc;
		out->l3 = prev_ts;

	    }

	    last_cyc = tc.cycles;


        } else {
	    out->hits++;
	    out->idle = 1;
	}

	return;
    } 
    

    if(pulse_queue_empty(q))
        return;

    struct list_timestamp *ts = pulse_queue_front(q);
 
    pulse_queue_pop(q);

    gpio_set(0);
    gpio_clear(0);

//    out->pgms++;
        
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


    // fixme: hardware miss detection?
    prev_ts = out->last_programmed;

    out->last_programmed = *ts;
    out->idle = 0;

    gpio_set(0);
    gpio_clear(0);
}




void enqueue_trigger ( int output, struct lrt_output_rule *rule, struct list_id *id, struct list_timestamp *ts, int seq)
{
    struct lrt_output *out = &outputs[output];
    struct list_timestamp adjusted = *ts;

    ts_adjust_delay (&adjusted, rule->delay_cycles, rule->delay_frac);

//    if(!rule->enabled)
//	return;

    if(!check_dead_time(out, &adjusted))
    {
        out->miss_deadtime ++;
        return;
    }

    if( pulse_queue_push( &out->queue, &adjusted ) < 0)
    {
        out->miss_overflow ++;
        return;
    }

    out->last_enqueued = *ts;

    gpio_set(0);
    gpio_clear(0);

}

void do_rx()
{
    if( !rmq_poll( FD_IN_SLOT_REMOTE) )
	return;
	
    gpio_set(1);
    gpio_clear(1);
    struct list_trigger_message *msg = mq_map_in_buffer(1, FD_IN_SLOT_REMOTE) - sizeof(struct rmq_message_addr);
    int i, j;
    int cnt = msg->count;

    //struct list_timestamp tc;

    //tc.seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
    //tc.cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
    //pp_printf("Trx %d %d", tc.seconds, tc.cycles);
    //pp_printf("%d triggers\n", cnt);
    for(i=0; i < cnt; i++)
    {
        struct list_id id = msg->triggers[i].id;
        struct lrt_hash_entry *ent = hash_search( &id, NULL );

        rx_total++;

        if(ent)
        {
            struct list_timestamp ts = msg->triggers[i].ts;
            int seq = msg->triggers[i].seq;
            for(j = 0; j < FD_NUM_CHANNELS; j++)
                if(ent->ocfg[j].state != HASH_ENT_EMPTY)
                {
                    gpio_set(0);
                    gpio_clear(0);
                    enqueue_trigger ( j, &ent->ocfg[j], &id, &ts, seq );
                }
        } else {

        }
    }
    //pp_printf("RX %d %d %d!", msg->triggers[0].id.system, msg->triggers[0].id.source_port, msg->triggers[0].id.trigger);
    mq_discard(1, FD_IN_SLOT_REMOTE);
    
}

void do_outputs()
{
    int i;
    
    for(i=0;i < FD_NUM_CHANNELS;i++)
        do_output(&outputs[i]);
}


static inline uint32_t *ctl_claim_out()
{
    mq_claim(0, FD_OUT_SLOT_CONTROL);
    return mq_map_out_buffer( 0, FD_OUT_SLOT_CONTROL );
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
    buf[1] = seq;
    mq_send(0, FD_OUT_SLOT_CONTROL, 2);
}

static inline void ctl_nack( uint32_t seq, int err )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_NACK;
    buf[1] = seq;
    buf[2] = err;
    mq_send(0, FD_OUT_SLOT_CONTROL, 3);
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
        
//    pp_printf("st-t %x\n", rule.state);
    if(!is_cond) // unconditional trigger
    {
//	pp_printf("add-dt %x:%x:%x out %d->%p", id.system, id.source_port, id.trigger, ch, handle.trig);
        uint32_t *obuf = ctl_claim_out();
        obuf[0] = ID_REP_TRIGGER_HANDLE;
        obuf[1] = seq;
        obuf[2] = handle.channel;
        obuf[3] = (uint32_t) handle.cond;
        obuf[4] = (uint32_t) handle.trig;
        mq_send(0, FD_OUT_SLOT_CONTROL, 5);
        
	    return;
    }
    
    id.system = buf[5];
    id.source_port = buf[6];
    id.trigger = buf[7];

    rule.delay_cycles = 100000000 / 8000;
    rule.delay_frac = 0;
    rule.state = HASH_ENT_CONDITION | HASH_ENT_DISABLED; 
    rule.cond_ptr = (struct lrt_output_rule *) handle.trig;

//    pp_printf("add-cond %x:%x:%x out %d->%p", id.system, id.source_port, id.trigger, ch, handle.trig);
//    pp_printf("st-c %x\n", rule.state);

    handle.cond = hash_add ( &id, ch, &rule );

    uint32_t *obuf = ctl_claim_out();
    obuf[0] = ID_REP_TRIGGER_HANDLE;
    obuf[1] = seq;    
    obuf[2] = handle.channel;
    obuf[3] = (uint32_t) handle.cond;
    obuf[4] = (uint32_t) handle.trig;
    mq_send(0, FD_OUT_SLOT_CONTROL, 5);
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

static void bag_timestamp(uint32_t *buf, struct list_timestamp *ts )
{
    buf[0] = ts->seconds;
    buf[1] = ts->cycles;
    buf[2] = ts->frac;
}

static void bag_id(uint32_t *buf, struct list_id *id )
{
    buf[0] = id->system;
    buf[1] = id->source_port;
    buf[2] = id->trigger;
}


static inline void ctl_read_hash(int seq, uint32_t *buf)
{
    int bucket = buf[0];
    int pos = buf[1];
    int ch = buf[2];

    uint32_t *obuf = ctl_claim_out();

    struct lrt_hash_entry *ent = hash_get_entry (bucket, pos);
    struct lrt_hash_entry *cond = NULL;
    
    obuf[0] = ID_REP_HASH_ENTRY;
    obuf[1] = seq;
    obuf[2] = ent ? 1 : 0;

    if(ent)
    {
        cond = (struct lrt_hash_entry *) ent->ocfg[ch].cond_ptr;
        obuf[9] = (uint32_t) cond;
        obuf[10] = (uint32_t) ent;
        
        if(cond)
        {    	
       	    bag_id(obuf + 3, &cond->id);
            obuf[6] = cond->ocfg[ch].delay_cycles;
    	    obuf[7] = cond->ocfg[ch].delay_frac;
    	    obuf[8] = (ent->ocfg[ch].state & HASH_ENT_DISABLED) | HASH_ENT_CONDITION;
            bag_id(obuf+11, &ent->id);
    	    obuf[14] = ent->ocfg[ch].delay_cycles;
    	    obuf[15] = ent->ocfg[ch].delay_frac;
    	    obuf[16] = HASH_ENT_CONDITIONAL;
    	} else {
            bag_id(obuf + 3, &ent->id);
    	    obuf[6] = ent->ocfg[ch].delay_cycles;
    	    obuf[7] = ent->ocfg[ch].delay_frac;
    	    obuf[8] = ent->ocfg[ch].state;
        }
    }

    mq_send(0, FD_OUT_SLOT_CONTROL, 17);
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
    bag_id(obuf + 7, &st->last_id);

    bag_timestamp(obuf + 10, &st->l1);
    bag_timestamp(obuf + 13, &st->l2);
    bag_timestamp(obuf + 16, &st->l3);

//    bag_timestamp(obuf + 10, &st->last_executed);
//    bag_timestamp(obuf + 13, &st->last_enqueued);
//    bag_timestamp(obuf + 16, &st->last_programmed);
    obuf[19] = st->idle;
    obuf[20] = st->state;
    obuf[21] = st->mode;
    obuf[22] = st->flags;
    obuf[23] = st->log_level;
    obuf[24] = st->dead_time;
    obuf[25] = st->width_cycles;
    obuf[26] = st->worst_latency;
    obuf[27] = rx_total;

    mq_send(0, FD_OUT_SLOT_CONTROL, 28);
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

    pp_printf("RT_FD locking WR");
    /* Lock to white-rabbit */
    dp_writel((1 << 1), 0xC);

    pp_printf("RT_FD firmware initialized.");

    
    for(;;)
    {
        do_rx();
        do_outputs();
        do_control();
    }

}