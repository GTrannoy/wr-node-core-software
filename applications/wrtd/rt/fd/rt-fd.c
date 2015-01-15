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
#include "wrtd-common.h"
#include "hw/fd_channel_regs.h"
#include "hash.h"
#include "loop-queue.h"
#include "wrtd-serializers.h"


/* Structure describing a single pulse in the Fine Delay software output queue */
struct pulse_queue_entry {
/* Trigger that produced the pulse */
	struct wrtd_trigger_entry trig;
/* Origin timestamp cycles count (for latency statistics) */
	int origin_cycles;
/* Rule that produced the pulse */
	struct lrt_output_rule *rule;
};

/* Pulse FIFO for a single Fine Delay output */
struct lrt_pulse_queue 
{
	struct pulse_queue_entry data[FD_MAX_QUEUE_PULSES];
	int head, tail, count;
};

/* State of a single Trigger Output */
struct lrt_output {
/* Base address of the corresponding Fine Delay output block */
	uint32_t base_addr;
/* Index of the output */
	int index;
/* Executed pulses counter */
	int hits;
/* Missed pulses counters, due to: too big latency, exceeded dead time, queue overflow (too big delay), no WR timing */
	int miss_timeout, miss_deadtime, miss_overflow, miss_no_timing;
/* Last executed trigger (i.e. the last one that produced a pulse at the FD output). */
	struct wrtd_trigger_entry last_executed;
/* Last enqueued trigger (i.e. the last one that entered the FD output queue). */
	struct wrtd_trigger_entry last_enqueued;
/* Last received trigger (i.e. last received packet from Etherbone). */
	struct wrtd_trigger_entry last_received;
/* Last timestamp value written to FD output config */
	struct wr_timestamp last_programmed;
/* Last timestamp value written to FD output config */
	struct wrtd_trigger_entry last_lost;
/* Idle flag */
	int idle;
/* Arm state */
	int state;
/* Trigger mode */
	int mode;
/* Flags (logging, etc) */
	uint32_t flags;
/* Current logging level */
	uint32_t log_level;
/* Dead time (8ns cycles) */
	int dead_time;
/* Pulse width (8ns cycles) */
	int width_cycles;
/* Output pulse queue */
	struct lrt_pulse_queue queue;
};

/* A pointer/handle to a trigger rule in the hash table. Used by the host to
   quickly access/modify a particular trigger entry */
struct lrt_trigger_handle {
/* Main trigger event */
	struct lrt_hash_entry *trig;
/* Conditional arm event */
	struct lrt_hash_entry *cond;
/* CPU core for which the trigger is assigned (not used for the moment) */
	int cpu;
/* Output channel for which the trigger is assigned */
	int channel;
};

/* Output state array */
struct lrt_output outputs[FD_NUM_CHANNELS];
/* Received message counters */
static int rx_ebone = 0, rx_loopback = 0;

/* Adjusts the timestamp in-place by adding cycles/frac value */
static inline void ts_adjust_delay(struct wr_timestamp *ts, uint32_t cycles, uint32_t frac)
{
	ts->frac += frac;

	if (ts->frac & 0x1000)
		ts->ticks++;

	ts->frac &= 0xfff;
	ts->ticks += cycles;

	if (ts->ticks >= 125000000) {
		ts->ticks -= 125000000;
		ts->seconds++;
	}
}

/* Initializes an empty pulse queue */
void pulse_queue_init(struct lrt_pulse_queue *p)
{
	p->head = 0;
	p->tail = 0;
	p->count = 0;
}

/* Requests a new entry in a pulse queue. Returns pointer to the ne
   entry or NULL if the queue is full. */
struct pulse_queue_entry *pulse_queue_push(struct lrt_pulse_queue *p)
{
	if (p->count == FD_MAX_QUEUE_PULSES)
		return NULL;

	struct pulse_queue_entry *ent = &p->data[p->head];

	p->count++;
	p->head++;

	if (p->head == FD_MAX_QUEUE_PULSES)
		p->head = 0;

	return ent;
}

/* Returns non-0 if pulse queue p contains any pulses. */
int pulse_queue_empty(struct lrt_pulse_queue *p)
{
	return (p->count == 0);
}

/* Returns the oldest entry in the pulse queue (or NULL if empty). */
struct pulse_queue_entry* pulse_queue_front(struct lrt_pulse_queue *p)
{
    if (!p->count)
	   return NULL;
    return &p->data[p->tail];
}

/* Returns the newest entry in the pulse queue (or NULL if empty). */
struct pulse_queue_entry* pulse_queue_back(struct lrt_pulse_queue *p)
{
    if (!p->count)
	   return NULL;
    return &p->data[p->head];
}

/* Releases the oldest entry from the pulse queue. */
void pulse_queue_pop(struct lrt_pulse_queue *p)
{
	p->tail++;

	if(p->tail == FD_MAX_QUEUE_PULSES)
		p->tail = 0;
	p->count--;
}

/* Checks if the timestamp of the pulse (ts) does not violate the dead time on the output out
   by comparing it with the last processed pulse timestamp. */
static inline int check_dead_time( struct lrt_output *out, struct wr_timestamp *ts )
{
	if(out->idle)
		return 1;

	int delta_s = ts->seconds - out->last_enqueued.ts.seconds;
	int delta_c = ts->ticks - out->last_enqueued.ts.ticks;

	if(delta_c < 0) {
		delta_c += 125 * 1000 * 1000;
		delta_s--;
	}

	if(delta_s < 0 || delta_c < out->dead_time)
		return 0;

	return 1;
}

/* Writes to FD output registers for output (out) */
static inline void fd_ch_writel(struct lrt_output *out, uint32_t value,
				 uint32_t reg)
{
	dp_writel( value , reg + out->base_addr );
}

/* Reads from FD output registers for output (out) */
static inline uint32_t fd_ch_readl (struct lrt_output *out,  uint32_t reg)
{
	return dp_readl( reg + out->base_addr );
}

static int check_output_timeout (struct lrt_output *out)
{
	struct wr_timestamp tc;

	/* Read the current WR time, order is important: first seconds, then cycles (cycles
	   get latched on reading secs register. */
	tc.seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
	tc.ticks = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);

	int delta = tc.seconds - out->last_programmed.seconds;
	delta *= 125 * 1000 * 1000;
	delta += tc.ticks - out->last_programmed.ticks;

	/* Current time exceeds FD setpoint? */
	return (delta > 0);
}

void update_latency_stats (struct pulse_queue_entry *pq_ent)
{
/* Read the time and calculate the latency */
	volatile uint32_t dummy = lr_readl (WRN_CPU_LR_REG_TAI_SEC);
	int latency = lr_readl (WRN_CPU_LR_REG_TAI_CYCLES) - pq_ent->origin_cycles;

	if (latency < 0)
		latency += 125 * 1000 * 1000;

	struct lrt_output_rule *rule = pq_ent->rule;

	if (latency > rule->latency_worst)
		rule->latency_worst = latency;

	if(rule->latency_avg_sum > 2000 * 1000 * 1000)
	{
		rule->latency_avg_sum = 0;
		rule->latency_avg_nsamples = 0;
	}

	rule->latency_avg_sum += latency;
	rule->latency_avg_nsamples++;
}

/* Output driving function. Reads pulses from the output queue, 
   programs the output and updates the output statistics. */
void do_output (struct lrt_output *out)
{
	struct lrt_pulse_queue *q = &out->queue;
	uint32_t dcr = fd_ch_readl(out, FD_REG_DCR);

	/* Check if the output has triggered */
	if(!out->idle) {
		if (!(dcr & FD_DCR_PG_TRIG)) { /* Nope, armed but still waiting for trigger */
			if (check_output_timeout (out))
			{
				/* Drop the pulse */
				pulse_queue_pop(q);
				out->miss_timeout ++;
				out->idle = 1;
				/* Disarm the FD output */
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

	/* Output is idle: check if there's something in the queue to execute */
	if(pulse_queue_empty(q))
        	return;

	struct pulse_queue_entry *pq_ent = pulse_queue_front(q);
	struct wr_timestamp *ts = &pq_ent->trig.ts;

	/* Program the output start time */
	fd_ch_writel(out, ts->seconds, FD_REG_U_STARTL);
	fd_ch_writel(out, ts->ticks, FD_REG_C_START);
	fd_ch_writel(out, ts->frac, FD_REG_F_START);

	/* Adjust pulse width and program the output end time */
	ts->ticks += out->width_cycles;
	if (ts->ticks >= 125000000) {
		ts->ticks -= 125000000;
		ts->seconds++;
	}

	fd_ch_writel(out, ts->seconds, FD_REG_U_ENDL);
	fd_ch_writel(out, ts->ticks, FD_REG_C_END);
	fd_ch_writel(out, ts->frac, FD_REG_F_END);
	fd_ch_writel(out, 0, FD_REG_RCR);
	fd_ch_writel(out, FD_DCR_MODE, FD_REG_DCR);
	fd_ch_writel(out, FD_DCR_MODE | FD_DCR_UPDATE, FD_REG_DCR);
	fd_ch_writel(out, FD_DCR_MODE | FD_DCR_PG_ARM | FD_DCR_ENABLE, FD_REG_DCR);

	ts->ticks += 1000;
	if (ts->ticks >= 125000000) {
		ts->ticks -= 125000000;
		ts->seconds++;
	}

	/* Store the last programmed timestamp (+ some margin) and mark the output as busy */
	out->last_programmed = *ts;
	out->idle = 0;

	update_latency_stats (pq_ent);
}

void enqueue_trigger(int output, struct lrt_output_rule *rule,
		       struct wrtd_trig_id *id, struct wr_timestamp *ts, int seq)
{
	struct lrt_output *out = &outputs[output];
	struct wr_timestamp adjusted = *ts;
	struct pulse_queue_entry *pq_ent;

	if(rule->state & HASH_ENT_DISABLED)
		return;

	ts_adjust_delay (&adjusted, rule->delay_cycles, rule->delay_frac);

	if (!check_dead_time(out, &adjusted)) {
		out->miss_deadtime ++;
		return;
	}

// put arm and condition logic here

	if ((pq_ent = pulse_queue_push (&out->queue)) == NULL) {
		out->miss_overflow ++;
		return;
	}

	pq_ent->trig.ts = adjusted;
	pq_ent->trig.id = *id;
	pq_ent->trig.seq = seq;
	pq_ent->origin_cycles = ts->ticks;
	pq_ent->rule = rule;

	out->last_enqueued.ts = *ts;
	out->last_enqueued.id = *id;
	out->last_enqueued.seq = seq;
}

static inline void filter_trigger(struct wrtd_trigger_entry *trig)
{
	struct lrt_hash_entry *ent = hash_search (&trig->id, NULL);
	int j;

	if(ent)
	{
		struct wr_timestamp ts = trig->ts;
		struct wrtd_trig_id id = trig->id;
		int seq = trig->seq;
		for(j = 0; j < FD_NUM_CHANNELS; j++)
			if(ent->ocfg[j])
				enqueue_trigger (j, ent->ocfg[j], &id, &ts, seq);
	}
}

void do_rx()
{
	if (rmq_poll( WRTD_REMOTE_IN_FD)) {
		struct wrtd_trigger_message *msg = mq_map_in_buffer (1, WRTD_REMOTE_IN_FD) - sizeof(struct rmq_message_addr);
		int i, cnt = msg->count;

		for (i = 0; i < cnt; i++)
			filter_trigger (&msg->triggers[i]);

		mq_discard (1, WRTD_REMOTE_IN_FD);
		rx_ebone++;
	}

	struct wrtd_trigger_entry *ent = loop_queue_pop();

	if (ent) {
		filter_trigger (ent);
		rx_loopback++;
	}
}

void do_outputs()
{
	int i;
	for (i = 0;i < FD_NUM_CHANNELS; i++)
		do_output(&outputs[i]);
}

/*.
 * WRTD Command Handlers
 */


/* Creates a hmq_buf serializing object for the control output slot */
static inline struct wrnc_msg ctl_claim_out_buf()
{
    return hmq_msg_claim_out (WRTD_OUT_FD_CONTROL, 128);
}

/* Creates a hmq_buf deserializing object for the control input slot */
static inline struct wrnc_msg ctl_claim_in_buf()
{
    return hmq_msg_claim_in (WRTD_IN_FD_CONTROL, 16);
}

/* Sends an acknowledgement reply */
static inline void ctl_ack( uint32_t seq )
{
    struct wrnc_msg buf = ctl_claim_out_buf();
    uint32_t id_ack = WRTD_REP_ACK_ID;

    wrnc_msg_header (&buf, &id_ack, &seq);
    hmq_msg_send (&buf);
}

/* Sends an NACK (error) reply */
static inline void ctl_nack( uint32_t seq, int err )
{
    struct wrnc_msg buf = ctl_claim_out_buf();
    uint32_t id_nack = WRTD_REP_NACK;

    wrnc_msg_header (&buf, &id_nack, &seq);
    wrnc_msg_int32 (&buf, &err);
    hmq_msg_send (&buf);
}

static inline void ctl_chan_enable (uint32_t seq, struct wrnc_msg *ibuf)
{
	int ch, enabled;

	wrnc_msg_int32(ibuf, &ch);
	wrnc_msg_int32(ibuf, &enabled);

	struct lrt_output *out = &outputs[ch];

	if(enabled) {
		out->flags |= WRTD_ENABLED;
	} else {
		out->flags &= ~(WRTD_ENABLED | WRTD_ARMED | WRTD_TRIGGERED | WRTD_LAST_VALID);
		/* clear the pulse queue and disable the physical output */
		pulse_queue_init ( &out->queue );
		fd_ch_writel(out, FD_DCR_MODE, FD_REG_DCR);
	}
	ctl_ack (seq);
}


static inline void ctl_chan_assign_trigger (uint32_t seq, struct wrnc_msg *ibuf)
{
	int ch, is_cond;
	struct wrtd_trig_id id;
	struct lrt_output_rule rule;
	struct lrt_trigger_handle handle;

	wrnc_msg_int32(ibuf, &ch);
	wrnc_msg_int32(ibuf, &is_cond);

	int n_req = is_cond ? 2 : 1;

	/* We need at least one or two hash entries */
	if (hash_free_count() < n_req) {
		ctl_nack(seq, -1);
		return;
	}

	/* Get the trigger ID (direct) */
	wrtd_msg_trig_id(ibuf, &id);

	/* Create an empty rule with default delay of 100 us */
	rule.delay_cycles = 100000000 / 8000;
	rule.delay_frac = 0;
	rule.state = (is_cond ? HASH_ENT_CONDITIONAL : HASH_ENT_DIRECT) | HASH_ENT_DISABLED;
	rule.cond_ptr = NULL;

	handle.channel = ch;
	handle.cond = NULL;
	handle.trig = hash_add ( &id, ch, &rule );
	
	struct wrnc_msg obuf = ctl_claim_out_buf();
	uint32_t id_trigger_handle = WRTD_REP_TRIGGER_HANDLE;


	/* Create the response */
	wrnc_msg_header(&obuf, &id_trigger_handle, &seq);

	if(!is_cond) // unconditional trigger
	{
	    wrnc_msg_int32(&obuf, &handle.channel);
	    wrnc_msg_uint32(&obuf, (uint32_t *) &handle.cond);
	    wrnc_msg_uint32(&obuf, (uint32_t *) &handle.trig);
	    hmq_msg_send (&obuf);
	    return;
	} 

	/* Get the condition ID */
	wrtd_msg_trig_id(ibuf, &id);
	
	/* Create default conditional rule */

	rule.delay_cycles = 100000000 / 8000;
	rule.delay_frac = 0;
	rule.state = HASH_ENT_CONDITION | HASH_ENT_DISABLED; 
	rule.cond_ptr = (struct lrt_output_rule *) handle.trig;

	handle.cond = hash_add ( &id, ch, &rule );

        wrnc_msg_int32(&obuf, &handle.channel);
        wrnc_msg_uint32(&obuf, (uint32_t *) &handle.cond);
        wrnc_msg_uint32(&obuf, (uint32_t *) &handle.trig);
        hmq_msg_send (&obuf);
}

static inline void ctl_chan_remove_trigger (uint32_t seq, struct wrnc_msg *ibuf)
{
	struct lrt_trigger_handle handle;

	wrnc_msg_int32(ibuf, &handle.channel);
	wrnc_msg_uint32(ibuf, (uint32_t *) &handle.cond);
	wrnc_msg_uint32(ibuf, (uint32_t *) &handle.trig);

	if(handle.cond)
		hash_remove(handle.cond, handle.channel);
	hash_remove(handle.trig, handle.channel);

	// fixme: purge pending pulses conditional pulses from the queue
	ctl_ack(seq);
}

static inline void ctl_chan_enable_trigger (uint32_t seq, struct wrnc_msg *ibuf)
{
	int ch, enable;
	struct lrt_hash_entry *ent;

	wrnc_msg_int32(ibuf, &ch);
	wrnc_msg_int32(ibuf, &enable);
	wrnc_msg_uint32(ibuf, (uint32_t *) &ent);

	struct lrt_output_rule *rule = ent->ocfg[ch];
	if(enable)
		rule->state &= ~HASH_ENT_DISABLED;
	else
		rule->state |= ~HASH_ENT_DISABLED;

	// fixme: purge pending pulses conditional pulses from the queue
	ctl_ack(seq);
}

static inline void ctl_read_hash (uint32_t seq, struct wrnc_msg *ibuf)
{
	int ch, bucket, pos;
	uint32_t state_tmp;

	wrnc_msg_int32(ibuf, &bucket);
	wrnc_msg_int32(ibuf, &pos);
	wrnc_msg_int32(ibuf, &ch);

	struct wrnc_msg obuf = ctl_claim_out_buf();
	uint32_t id_hash_entry = WRTD_REP_HASH_ENTRY;

	/* Create the response */
	wrnc_msg_header(&obuf, &id_hash_entry, &seq);

	struct lrt_hash_entry *ent = hash_get_entry (bucket, pos);
        struct lrt_hash_entry *cond = NULL;

	int entry_ok = ent && ent->ocfg[ch];

	wrnc_msg_int32(&obuf, &entry_ok);

	if(entry_ok) {
		cond = (struct lrt_hash_entry *) ent->ocfg[ch]->cond_ptr;
		if(cond)
		{
			state_tmp = (ent->ocfg[ch]->state & HASH_ENT_DISABLED) | HASH_ENT_CONDITION;

			wrtd_msg_trig_id(&obuf, &cond->id);
			wrnc_msg_uint32(&obuf, &cond->ocfg[ch]->delay_cycles);
			wrnc_msg_uint16(&obuf, &cond->ocfg[ch]->delay_frac);
			wrnc_msg_uint32(&obuf, &state_tmp);
			wrnc_msg_uint32(&obuf, (uint32_t *) ent);
			wrnc_msg_uint32(&obuf, (uint32_t *) cond);
			wrtd_msg_trig_id(&obuf, &ent->id);
			wrnc_msg_uint32(&obuf, &ent->ocfg[ch]->delay_cycles);
			wrnc_msg_uint16(&obuf, &ent->ocfg[ch]->delay_frac);

    			state_tmp = HASH_ENT_CONDITIONAL;
			wrnc_msg_uint32(&obuf, &state_tmp);
		} else {
			wrtd_msg_trig_id(&obuf, &ent->id);
			wrnc_msg_uint32(&obuf, &ent->ocfg[ch]->delay_cycles);
			wrnc_msg_uint16(&obuf, &ent->ocfg[ch]->delay_frac);
			wrnc_msg_uint16(&obuf, &ent->ocfg[ch]->state);
			wrnc_msg_uint32(&obuf, (uint32_t *) ent);
			wrnc_msg_uint32(&obuf, (uint32_t *) cond);
		}
	}

	hmq_msg_send (&obuf);
}

static inline void ctl_chan_set_dead_time (uint32_t seq, struct wrnc_msg *ibuf)
{
	int ch;
	wrnc_msg_int32(ibuf, &ch);
	wrnc_msg_int32(ibuf, &outputs[ch].dead_time);
	ctl_ack(seq);
}


static inline void ctl_ping (uint32_t seq, struct wrnc_msg *ibuf)
{
	ctl_ack(seq);
}

static inline void ctl_chan_set_delay (uint32_t seq, struct wrnc_msg *ibuf)
{
	int ch;
	struct lrt_hash_entry *ent;
	
	wrnc_msg_int32(ibuf, &ch);
	wrnc_msg_uint32(ibuf, (uint32_t*) &ent);

	if(ent->ocfg[ch])
	{
		wrnc_msg_uint32(ibuf, &ent->ocfg[ch]->delay_cycles);
		wrnc_msg_uint16(ibuf, &ent->ocfg[ch]->delay_frac);
	}

	ctl_ack(seq);
}

static inline void ctl_chan_get_state (uint32_t seq, struct wrnc_msg *ibuf)
{
	int ch;;
	wrnc_msg_int32(ibuf, &ch);

	struct lrt_output *st = &outputs[ch];
	struct wrnc_msg obuf = ctl_claim_out_buf();

	uint32_t id_state = WRTD_REP_STATE;

	/* Create the response */
	wrnc_msg_header(&obuf, &id_state, &seq);
	wrnc_msg_int32(&obuf, &ch);

	wrnc_msg_int32(&obuf, &st->hits);
	wrnc_msg_int32(&obuf, &st->miss_timeout);
	wrnc_msg_int32(&obuf, &st->miss_deadtime);
	wrnc_msg_int32(&obuf, &st->miss_overflow);
	wrnc_msg_int32(&obuf, &st->miss_no_timing);

	wrtd_msg_trigger_entry(&obuf, &st->last_executed);
	wrtd_msg_trigger_entry(&obuf, &st->last_enqueued);
	wrtd_msg_trigger_entry(&obuf, &st->last_received);
	wrtd_msg_trigger_entry(&obuf, &st->last_lost);

	wrnc_msg_int32(&obuf, &st->idle);
	wrnc_msg_int32(&obuf, &st->state);
	wrnc_msg_int32(&obuf, &st->mode);
	wrnc_msg_uint32(&obuf, &st->flags);
	wrnc_msg_uint32(&obuf, &st->log_level);
	wrnc_msg_int32(&obuf, &st->dead_time);
	wrnc_msg_int32(&obuf, &st->width_cycles);
	wrnc_msg_int32(&obuf, &rx_ebone);
	wrnc_msg_int32(&obuf, &rx_loopback);

	hmq_msg_send (&obuf);
}

static inline void ctl_software_trigger (uint32_t seq, struct wrnc_msg *ibuf)
{
	int ch, now;
	struct wr_timestamp tc;

	wrnc_msg_int32(ibuf, &ch);
	wrnc_msg_int32(ibuf, &now);
	
	if(now)
	{
		/* Prepare a pulse at (now + some margin) */
		tc.seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
		tc.ticks = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
		tc.frac = 0;
		tc.ticks += 10000;
	} else {
		wrnc_msg_timestamp(ibuf, &tc);
	}

	struct wrnc_msg obuf = ctl_claim_out_buf();

	uint32_t id_timestamp = WRTD_REP_TIMESTAMP;

	/* Create the response */
	wrnc_msg_header(&obuf, &id_timestamp, &seq);
	wrnc_msg_int32(&obuf, &ch);

	if (tc.ticks >= 125000000) {
		tc.ticks -= 125000000;
		tc.seconds++;
	}

	wrnc_msg_timestamp(&obuf, &tc);
	hmq_msg_send (&obuf);

	struct lrt_output *out = &outputs[ch];

	/* Program the output start time */
	fd_ch_writel(out, tc.seconds, FD_REG_U_STARTL);
	fd_ch_writel(out, tc.ticks, FD_REG_C_START);
	fd_ch_writel(out, 0, FD_REG_F_START);

	tc.ticks += out->width_cycles;
	if (tc.ticks >= 125000000) {
		tc.ticks -= 125000000;
		tc.seconds++;
	}

	fd_ch_writel(out, tc.seconds, FD_REG_U_ENDL);
	fd_ch_writel(out, tc.ticks, FD_REG_C_END);
	fd_ch_writel(out, 0, FD_REG_F_END);
	fd_ch_writel(out, 0, FD_REG_RCR);
	fd_ch_writel(out, FD_DCR_MODE, FD_REG_DCR);
	fd_ch_writel(out, FD_DCR_MODE | FD_DCR_UPDATE, FD_REG_DCR);
	fd_ch_writel(out, FD_DCR_MODE | FD_DCR_PG_ARM | FD_DCR_ENABLE, FD_REG_DCR);
}


static inline void ctl_chan_set_mode (uint32_t seq, struct wrnc_msg *ibuf)
{
	int ch;
	wrnc_msg_int32(ibuf, &ch);

	struct lrt_output *st = &outputs[ch];
	wrnc_msg_int32(ibuf, &st->mode);

	st->flags &= ~(WRTD_ARMED | WRTD_TRIGGERED | WRTD_LAST_VALID) ;
	ctl_ack(seq);
}


static inline void ctl_chan_arm (uint32_t seq, struct wrnc_msg *ibuf)
{
	int ch, armed;
	wrnc_msg_int32(ibuf, &ch);
	wrnc_msg_int32(ibuf, &armed);

	struct lrt_output *st = &outputs[ch];

	if(armed)
		st->flags |= WRTD_ARMED;
	else
		st->flags &= ~WRTD_ARMED;

	st->flags &= ~WRTD_TRIGGERED;

	ctl_ack(seq);
}

static inline void ctl_chan_set_log_level (uint32_t seq, struct wrnc_msg *ibuf)
{
	int ch;
	wrnc_msg_int32(ibuf, &ch);

	struct lrt_output *st = &outputs[ch];
	wrnc_msg_uint32(ibuf, &st->log_level);

	ctl_ack(seq);
}
/* Receives command messages and call matching command handlers */
static inline void do_control()
{
	uint32_t cmd, seq;
	uint32_t p = mq_poll();

	/* HMQ control slot empty? */
	if(! ( p & ( 1<< WRTD_IN_FD_CONTROL )))
		return;

	struct wrnc_msg ibuf = ctl_claim_in_buf();

	wrnc_msg_header(&ibuf, &cmd, &seq);

#define _CMD(id, func)          \
    case id:                    \
    {                           \
        func(seq, &ibuf);       \
        break;                  \
    }

	switch(cmd) {
	_CMD(WRTD_CMD_FD_CHAN_ASSIGN_TRIGGER,	ctl_chan_assign_trigger)
	_CMD(WRTD_CMD_FD_CHAN_REMOVE_TRIGGER,	ctl_chan_remove_trigger)
	_CMD(WRTD_CMD_FD_CHAN_ENABLE_TRIGGER,	ctl_chan_enable_trigger)
	_CMD(WRTD_CMD_FD_CHAN_ENABLE,		ctl_chan_enable)
	_CMD(WRTD_CMD_FD_CHAN_SET_DELAY,	ctl_chan_set_delay)
	_CMD(WRTD_CMD_FD_READ_HASH,		ctl_read_hash)
	_CMD(WRTD_CMD_FD_CHAN_SET_MODE,		ctl_chan_set_mode)
	_CMD(WRTD_CMD_FD_CHAN_ARM,		ctl_chan_arm)
	_CMD(WRTD_CMD_FD_CHAN_GET_STATE,	ctl_chan_get_state)
	_CMD(WRTD_CMD_FD_CHAN_SET_LOG_LEVEL,	ctl_chan_set_log_level)
	default:
	break;
	}

	/* Drop the message once handled */
	mq_discard(0, WRTD_IN_FD_CONTROL);
}

void init_outputs()
{
    int i;

	for (i = 0; i < FD_NUM_CHANNELS; i++) {
	memset(&outputs[i], 0, sizeof(struct lrt_output));
        outputs[i].base_addr = 0x100 + i * 0x100;
        outputs[i].index = i;
        outputs[i].idle = 1;
        outputs[i].dead_time = 80000 / 8; // 80 us
        outputs[i].width_cycles = 1250; // 1us
    }
}

void init() 
{
	hash_init();
	init_outputs();

	/* reset Fine Delay */
	dp_writel( (0xdead << 16) | 0x0, 0x0);
	delay(1000);
	dp_writel( (0xdead << 16) | 0x3, 0x0);
	delay(1000);

	pp_printf("RT_FD locking WR\n");
	/* Lock to white-rabbit */
	dp_writel((1 << 1), 0xC);

	pp_printf("RT_FD firmware initialized.\n");

}

int main()
{
	init();

	for(;;) {
		do_rx();
		do_outputs();
		do_control();
	}

	return 0;
}