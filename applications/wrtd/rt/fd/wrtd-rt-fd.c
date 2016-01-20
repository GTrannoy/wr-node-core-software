/*
 * This work is part of the White Rabbit Node Core project.
 *
 * Copyright (C) 2013-2014 CERN (www.cern.ch)
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *
 * Released according to the GNU GPL, version 2 or any later version.
 */


/*
 * LHC Instability Trigger Distribution (LIST) Firmware
 *
 * Real-Time application for the FMC Fine Delay mezzanine (Trigger Output)
 */

#include <string.h>

#include "rt.h"
#include "wrtd-common.h"
#include "hw/fd_channel_regs.h"
#include "hw/fd_main_regs.h"

#include "loop-queue.h"

#include <librt.h>

#define OUT_TIMEOUT 10

static const uint32_t version = GIT_VERSION;
static uint32_t promiscuous_mode = 0;



#define WR_LINK_OFFLINE		1
#define WR_LINK_ONLINE		2
#define WR_LINK_SYNCING		3
#define WR_LINK_SYNCED		4

static int wr_state;

int wr_link_up()
{
	return dp_readl ( FD_REG_TCR ) & FD_TCR_WR_LINK;
}

int wr_time_locked()
{
	return dp_readl ( FD_REG_TCR ) & FD_TCR_WR_LOCKED;
}

int wr_time_ready()
{
	return 1;
}

int wr_enable_lock(int enable)
{
	if(enable)
		dp_writel(FD_TCR_WR_ENABLE, FD_REG_TCR);
	else
		dp_writel(0, FD_REG_TCR);
}

/**
 * It updates the White-Rabbit link status
 */
void wr_update_link(void)
{
	switch(wr_state) {
	case WR_LINK_OFFLINE:
		if (wr_link_up())
			wr_state = WR_LINK_ONLINE;
		break;
	case WR_LINK_ONLINE:
		if (wr_time_ready()) {
			wr_state = WR_LINK_SYNCING;
			wr_enable_lock(1);
		}
		break;
	case WR_LINK_SYNCING:
		if (wr_time_locked()) {
			pp_printf("WR link up!\n");
			wr_state = WR_LINK_SYNCED;
		}
		break;
	case WR_LINK_SYNCED:
		break;
	}

	if( wr_state != WR_LINK_OFFLINE && !wr_link_up() ) {
		pp_printf("rt-out: WR sync lost\n");
		wr_state = WR_LINK_OFFLINE;
		wr_enable_lock(0);
	}
}

int wr_is_timing_ok()
{
	return (wr_state == WR_LINK_SYNCED);
}


struct wrtd_out_trigger triggers[FD_HASH_ENTRIES]; /**< list of triggers */
struct wrtd_out_trigger *ht[FD_HASH_ENTRIES]; /* hash table */
unsigned int tlist_count = 0; /**< number of valid trigger entry
					in tlist */
static struct wrtd_out_channel wrtd_out_channels[FD_NUM_CHANNELS]; /**< Output
								      state
								      array */
static struct wrtd_out wrtd_out_device;

static inline int wr_is_timing_ok(void);

/**
 * Writes to FD output registers for output (out)
 */
static inline void fd_ch_writel(struct wrtd_out_channel *out, uint32_t value,
				 uint32_t reg)
{
	dp_writel(value , reg + out->base_addr);
}

/**
 * Reads from FD output registers for output (out)
 */
static inline uint32_t fd_ch_readl (struct wrtd_out_channel *out,  uint32_t reg)
{
	return dp_readl(reg + out->base_addr);
}


/**
 * Compare two trigger identifiers.
 * It does not use memcmp() because the shared memory use word addressing, so
 * it will not work if one of the variable is located on the shared memory
 */
static inline int trig_eq(struct wrtd_trig_id *id1, struct wrtd_trig_id *id2)
{
	return id1->system == id2->system &&
		id1->source_port == id2->source_port &&
		id1->trigger == id2->trigger;
}


/**
 * It looks for a trigger entry within the hash table
 */
static int wrtd_out_hash_table_find(struct wrtd_trig_id *tid)
{
	int hidx;

	for (hidx = wrtd_hash_func(tid); hidx < FD_HASH_ENTRIES; hidx++)
		if (trig_eq(tid, &ht[hidx]->id))
			return hidx;

        return -1;
}


/**
 * It looks for a trigger entry within the hash table
 */
static inline struct wrtd_out_trigger *rtfd_trigger_entry_find(struct wrtd_trig_id *tid)
{
	int hidx = wrtd_out_hash_table_find(tid);

	return hidx >= 0 ? ht[hidx] : NULL;
}


/**
 * Adjusts the timestamp in-place by adding cycles/frac value
 */
static void ts_adjust_delay(struct wr_timestamp *ts, uint32_t cycles, uint32_t frac)
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

/**
 * Puts a trigger message in the log buffer
 */
static void log_trigger(int type, int miss_reason, struct wrtd_out_channel *out,
			struct wrtd_trigger_entry *ent)
{
	struct wrnc_proto_header hdr = {
		.rt_app_id = 0,
		.msg_id = WRTD_OUT_ACTION_LOG,
		.slot_io = WRTD_OUT_FD_LOGGING,
		.seq = ent->seq,
		.len = sizeof(struct wrtd_log_entry) / 4,
		.flags = 0x0,
		.trans = 0x0,
		.time = 0x0,
	};
	struct wrnc_msg out_buf;
	struct wrtd_log_entry *log;

	if (!out && (type != WRTD_LOG_PROMISC || !promiscuous_mode ))
		return;
	if (out && !(out->config.log_level & type))
		return;

	out_buf = rt_mq_claim_out(&hdr);
	log = (struct wrtd_log_entry *)rt_proto_payload_get(out_buf.data);
	log->type = type;
	log->channel = out ? out->n : -1;
	log->miss_reason = miss_reason;
	log->seq = ent->seq;
	log->id = ent->id;
	log->ts = ent->ts;
	rt_proto_header_set((void *) out_buf.data, &hdr);
	rt_mq_msg_send(&out_buf);
}


/**
 * Initializes an empty pulse queue
 */
static void pulse_queue_init(struct lrt_pulse_queue *p)
{
	p->head = 0;
	p->tail = 0;
	p->count = 0;
}


/**
 * Requests a new entry in a pulse queue. Returns pointer to the ne
 * entry or NULL if the queue is full.
 */
struct pulse_queue_entry *pulse_queue_push(struct lrt_pulse_queue *p)
{
	struct pulse_queue_entry *ent;

	if (p->count == FD_MAX_QUEUE_PULSES)
		return NULL;

	ent = &p->data[p->head];
	p->count++;
	p->head++;

	if (p->head == FD_MAX_QUEUE_PULSES)
		p->head = 0;

	return ent;
}


/**
 * Returns non-0 if pulse queue p contains any pulses.
 */
static inline int pulse_queue_empty(struct lrt_pulse_queue *p)
{
	return (p->count == 0);
}


/**
 * Returns the oldest entry in the pulse queue (or NULL if empty).
 */
static struct pulse_queue_entry* pulse_queue_front(struct lrt_pulse_queue *p)
{
    if (!p->count)
	   return NULL;
    return &p->data[p->tail];
}


/**
 * Returns the newest entry in the pulse queue (or NULL if empty).
 */
static struct pulse_queue_entry* pulse_queue_back(struct lrt_pulse_queue *p)
{
    if (!p->count)
	   return NULL;
    return &p->data[p->head];
}


/**
 * Releases the oldest entry from the pulse queue.
 */
static void pulse_queue_pop(struct lrt_pulse_queue *p)
{
	p->tail++;

	if(p->tail == FD_MAX_QUEUE_PULSES)
		p->tail = 0;
	p->count--;
}

/**
 * Checks if the timestamp of the pulse (ts) does not violate the
 * dead time on the output out by comparing it with the last processed
 * pulse timestamp.
 */
static int check_dead_time(struct wrtd_out_channel *out,
			   struct wr_timestamp *ts)
{
	int delta_s = ts->seconds - out->priv.prev_pulse.seconds;
	int delta_c = ts->ticks - out->priv.prev_pulse.ticks;

	if (delta_c < 0) {
		delta_c += 125 * 1000 * 1000;
		delta_s--;
	}

	return (delta_s < 0 || delta_c < out->config.dead_time) ? 0 : 1;
}


/**
 * Checks if the timestamp of the last programmed pulse is lost because
 * of timeout
 */
static int check_output_timeout (struct wrtd_out_channel *out)
{
	struct lrt_pulse_queue *q = &out->queue;
	struct pulse_queue_entry *pq_ent = pulse_queue_front(q);
	struct wr_timestamp tc;
	int delta;

	/*
	 * Read the current WR time, order is important: first seconds,
	 * then cycles (cycles get latched on reading secs register.
	 */
	tc.seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
	tc.ticks = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);

	if( out->stats.last_programmed.seconds > tc.seconds + OUT_TIMEOUT ) {
	    pp_printf("Enqueued timestamp very far in the future [id %x:%x:%x]. Dropping.",
		      pq_ent->trig.id.system,
		      pq_ent->trig.id.source_port,
		      pq_ent->trig.id.trigger);
	    pp_printf("Offending TS: %d:%d",
		      out->last_programmed.seconds,
		      out->last_programmed.ticks);
	}

	delta = tc.seconds - out->stats.last_programmed.seconds;
	delta *= 125 * 1000 * 1000;
	delta += tc.ticks - out->stats.last_programmed.ticks;

	/* Current time exceeds FD setpoint? */
	return (delta > 0);
}


/**
 * It updates the latency stats
 */
void update_latency_stats(struct pulse_queue_entry *pq_ent)
{
	/* Read the time and calculate the latency */
	struct lrt_output_rule *rule = pq_ent->rule;
	__attribute__((unused))
		volatile uint32_t dummy = lr_readl (WRN_CPU_LR_REG_TAI_SEC);
	int latency;

	if(!rule)
		return;
	latency = lr_readl (WRN_CPU_LR_REG_TAI_CYCLES) - pq_ent->origin_cycles;
	if (latency < 0)
		latency += 125 * 1000 * 1000;

	if (latency > rule->latency_worst)
		rule->latency_worst = latency;

	if (rule->latency_avg_sum > 2000 * 1000 * 1000) {
		rule->latency_avg_sum = 0;
		rule->latency_avg_nsamples = 0;
	}

	rule->latency_avg_sum += latency;
	rule->latency_avg_nsamples++;
}


/**
 * Drop the given enqueued trigger
 */
static void drop_trigger(struct wrtd_out_channel *out,
			 struct pulse_queue_entry *pq_ent,
			 struct lrt_pulse_queue *q, int reason)
{
	out->priv.idle = 1;

	if (pulse_queue_empty(q))
	    return;

	/* Drop the pulse */
	pulse_queue_pop(q);
	pq_ent->rule->misses ++;

	switch (reason) {
	case WRTD_MISS_TIMEOUT:
		out->stats.miss_timeout++;
		break;
	case WRTD_MISS_NO_WR:
		out->stats.miss_no_timing ++;
		break;
	}

	out->stats.last_lost = pq_ent->trig;

	/* Disarm the FD output */
	fd_ch_writel(out, FD_DCR_MODE, FD_REG_DCR);

	if(out->config.state == OUT_ST_TEST_PENDING)
		out->config.state = OUT_ST_IDLE;

	log_trigger(WRTD_LOG_MISSED, reason, out, &pq_ent->trig);
}


/**
 * Output driving function. Reads pulses from the output queue,
 * programs the output and updates the output statistics.
 */
static void do_output (struct wrtd_out_channel *out)
{
	struct lrt_pulse_queue *q = &out->priv.queue;
	struct pulse_queue_entry *pq_ent = pulse_queue_front(q);
	uint32_t dcr = fd_ch_readl(out, FD_REG_DCR);
	struct wr_timestamp *ts;

	/* Check if the output has triggered */
	if(!out->priv.idle) {
		if (!wr_is_timing_ok()) {
			drop_trigger(out, pq_ent, q, WRTD_MISS_NO_WR);
		}
		else if (!(dcr & FD_DCR_PG_TRIG)) {
			/* Nope, armed but still waiting for trigger */
			if (check_output_timeout (out))
				drop_trigger(out, pq_ent, q, WRTD_MISS_TIMEOUT);
		} else {
			out->stats.last_executed = pq_ent->trig;
			pq_ent->rule->hits ++;
			pulse_queue_pop(q);
			out->stats.hits++;
			out->priv.idle = 1;
			out->config.flags |= WRTD_TRIGGERED;

			if(out->config.state == OUT_ST_TEST_PENDING)
				out->config.state = OUT_ST_IDLE;

			log_trigger (WRTD_LOG_EXECUTED, 0, out, &pq_ent->trig);
		}
		return;
	}

	/* Output is idle: check if there's something in the queue to execute */
	if (pulse_queue_empty(q))
        	return;

	pq_ent = pulse_queue_front(q);
	ts = &pq_ent->trig.ts;

	if (!wr_is_timing_ok())
		drop_trigger(out, pq_ent, q, WRTD_MISS_NO_WR);


	/* Program the output start time */
	fd_ch_writel(out, ts->seconds, FD_REG_U_STARTL);
	fd_ch_writel(out, ts->ticks, FD_REG_C_START);
	fd_ch_writel(out, ts->frac, FD_REG_F_START);

	/* Adjust pulse width and program the output end time */
	ts->ticks += out->config.width_cycles;
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
	fd_ch_writel(out, FD_DCR_MODE | FD_DCR_PG_ARM | FD_DCR_ENABLE,
		     FD_REG_DCR);

	ts->ticks += 1000;
	if (ts->ticks >= 125000000) {
		ts->ticks -= 125000000;
		ts->seconds++;
	}

	/*
	 * Store the last programmed timestamp (+ some margin) and mark
	 * the output as busy
	 */
	out->stats.last_programmed = *ts;
	out->priv.idle = 0;

	update_latency_stats (pq_ent);
}


/**
 * If the given trigger can be generated, then push it into the queue
 */
static void enqueue_trigger(int output, struct lrt_output_rule *rule,
			    struct wrtd_trig_id *id,
			    struct wr_timestamp *ts, int seq)
{
	struct wrtd_out_channel *out = &wrtd_out_channels[output];
	struct wr_timestamp adjusted = *ts;
	struct pulse_queue_entry *pq_ent;
	struct wrtd_trigger_entry ent;

#ifdef RTDEBUG
	pp_printf("%s:%d ch %d 0x%x\n", __func__, __LINE__, output, rule->state);
#endif
	if(rule->state & HASH_ENT_DISABLED)
		return;

	if (!(out->config.flags & WRTD_ENABLED))
		return;

	ts_adjust_delay(&adjusted, rule->delay_cycles, rule->delay_frac);

	ent.ts = *ts;
	ent.id = *id;
	ent.seq = seq;

	if (!check_dead_time(out, &adjusted)) {
		rule->misses ++;
		out->stats.miss_deadtime++;
		log_trigger(WRTD_LOG_MISSED, WRTD_MISS_DEAD_TIME, out, &ent);
		return;
	}

	if (!wr_is_timing_ok()) {
		rule->misses++;
		out->stats.miss_no_timing++;
		log_trigger(WRTD_LOG_MISSED, WRTD_MISS_NO_WR, out, &ent);
		return;
	}

	switch (out->config.state) {
	case OUT_ST_IDLE: /* output not armed */
		return;

	case OUT_ST_ARMED:
		if (rule->state & HASH_ENT_CONDITION) {
			/* FIXME select the correct channel */
			out->priv.pending_trig = &triggers[rule->cond_ptr].ocfg[0];
			out->config.state = OUT_ST_CONDITION_HIT;
			return;
		} else if (out->config.mode == WRTD_TRIGGER_MODE_SINGLE) {
			out->config.flags &= ~WRTD_ARMED;
			out->config.state = OUT_ST_IDLE;
		}
		break;
	case OUT_ST_TEST_PENDING:
		return;

	case OUT_ST_CONDITION_HIT:
		if (rule != out->priv.pending_trig)
			return;

		if (out->config.mode == WRTD_TRIGGER_MODE_SINGLE) {
			out->config.flags &= ~WRTD_ARMED;
			out->config.state = OUT_ST_IDLE;
		} else {
			out->config.state = OUT_ST_ARMED;
		}
		break;
	default:
		break;

	}

	pq_ent = pulse_queue_push(&out->priv.queue);
	if (!pq_ent) {
		ent.ts = *ts;
		ent.id = *id;
		ent.seq = seq;
		log_trigger (WRTD_LOG_MISSED, WRTD_MISS_OVERFLOW, out, &ent);

		out->stats.miss_overflow++;
		return;
	}

	pq_ent->trig.ts = adjusted;
	pq_ent->trig.id = *id;
	pq_ent->trig.seq = seq;
	pq_ent->origin_cycles = ts->ticks;
	pq_ent->rule = rule;

	out->stats.last_enqueued.ts = *ts;
	out->stats.last_enqueued.id = *id;
	out->stats.last_enqueued.seq = seq;

	out->priv.prev_pulse = adjusted;

	log_trigger(WRTD_LOG_FILTERED, 0, out, &out->stats.last_enqueued);

}


static void filter_trigger(struct wrtd_trigger_entry *trig)
{
	struct wrtd_out_trigger *ent = rtfd_trigger_entry_find(&trig->id);
	int j;

	log_trigger(WRTD_LOG_PROMISC, 0, NULL, trig);
	wrtd_out_device.last_received = *trig;
#ifdef RTDEBUG
	pp_printf("%s:%d Trigger %d:%d:%d - entry %p\n",
		  __func__, __LINE__,
		  trig->id.system, trig->id.source_port, trig->id.trigger, ent);
#endif
	if (!ent)
		return;

	for(j = 0; j < FD_NUM_CHANNELS; j++)
		if(ent->ocfg[j].state != HASH_ENT_EMPTY)
			enqueue_trigger(j, &ent->ocfg[j],
					&trig->id, &trig->ts, trig->seq);

}

void do_rx(void)
{
	struct wrtd_trigger_entry *ent;
	struct wrtd_trigger_message *msg;
	int i;

	/* check from etherbone */
	if (rmq_poll(WRTD_REMOTE_IN_FD)) {
		msg = mq_map_in_buffer(1, WRTD_REMOTE_IN_FD)
			- sizeof(struct rmq_message_addr);

		for (i = 0; i < msg->count; i++)
			filter_trigger (&msg->triggers[i]);

		mq_discard (1, WRTD_REMOTE_IN_FD);
		wrtd_out_device.counter_etherbone++;
	}

	/* Check from the loopback */
	ent = loop_queue_pop();
	if (ent) {
		filter_trigger (ent);
		wrtd_out_device.counter_loopback++;
	}
}

void do_outputs(void)
{
	int i;

	for (i = 0;i < FD_NUM_CHANNELS; i++)
		do_output(&wrtd_out_channels[i]);
}

/*.
 * WRTD Command Handlers
 */

struct wrtd_trigger_sw {
	uint32_t chan;
	uint32_t now;
	struct wr_timestamp tstamp;
};


/**
 * It generate a software trigger accorging to the trigger entry coming
 * from the user space.
 */
static int wrtd_out_trigger_sw(struct wrnc_proto_header *hin, void *pin,
			       struct wrnc_proto_header *hout, void *pout)
{
	struct wrtd_trigger_sw *tsw = pin;
	struct pulse_queue_entry *pq_ent;

	if (tsw->now) {
		/* Prepare a pulse at (now + some margin) */
		tsw->tstamp.seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
		tsw->tstamp.ticks = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
		tsw->tstamp.frac = 0;
		tsw->tstamp.ticks += 10000;
	}

	/* clear all pulses from the queue */
	pulse_queue_init(&wrtd_out_channels[tsw->chan].priv.queue);
	pq_ent = pulse_queue_push (&wrtd_out_channels[tsw->chan].priv.queue);

	pq_ent->trig.ts = tsw->tstamp;
	pq_ent->trig.id.system = 0xffffffff;
	pq_ent->trig.id.source_port = 0xffffffff;
	pq_ent->trig.id.trigger = 0xffffffff;
	pq_ent->trig.seq = 0xffffffff;
	pq_ent->origin_cycles = tsw->tstamp.ticks;
	pq_ent->rule = NULL;

	wrtd_out_channels[tsw->chan].stats.last_enqueued.ts = tsw->tstamp;
	wrtd_out_channels[tsw->chan].stats.last_enqueued.id = pq_ent->trig.id;
	wrtd_out_channels[tsw->chan].stats.last_enqueued.seq = pq_ent->trig.seq;
	wrtd_out_channels[tsw->chan].config.state = OUT_ST_TEST_PENDING;

	rt_send_ack(hin, pin, hout, NULL);
	return 0;
}


/**
 * It gets the first available row in the hash table for the given
 * trigger ID
 */
static int wrtd_out_hash_table_free(struct wrtd_trig_id *tid)
{
	int hidx;

	for (hidx = wrtd_hash_func(tid); hidx < FD_HASH_ENTRIES; hidx++) {
		if (!ht[hidx])
			return hidx;
		/* Check if it already exists */
		if (trig_eq(tid, &ht[hidx]->id))
			break;
	}
        return -1;
}


/**
 * Insert trigger entry in the hash table
 */
static int wrtd_out_hash_table_insert(struct wrnc_proto_header *hin, void *pin,
				      struct wrnc_proto_header *hout, void *pout)
{
	uint32_t tidx = *(uint32_t *)pin;
	int hidx;

	if (tlist_count >= FD_HASH_ENTRIES) {
		pp_printf("Too many triggers %d/%d\n",
			  tlist_count, FD_HASH_ENTRIES);
		return -1;
	}
	if (!(triggers[tidx].flags & ENTRY_FLAG_VALID))
		return; /* nothing to do is a valid trigger */

	hidx = wrtd_out_hash_table_free(&triggers[tidx].id);
	if  (hidx < 0)
		goto out;

	if (hidx >= FD_HASH_ENTRIES)
		return -1;

	tlist_count++;
	ht[hidx] = &triggers[tidx];
out:
	rt_send_ack(hin, pin, hout, pout);

	return 0;
}


/**
 * Remove trigger entry from the hash table
 */
static int wrtd_out_hash_table_remove(struct wrnc_proto_header *hin, void *pin,
				      struct wrnc_proto_header *hout, void *pout)
{
	uint32_t tidx = *(uint32_t *)pin;
	int hidx;

	if (tlist_count <= 0) {
		pp_printf("No trigger to remove %d/%d\n",
			  tlist_count, FD_HASH_ENTRIES);
		return -1;
	}

	if (triggers[tidx].flags & ENTRY_FLAG_VALID)
		return; /* nothing to do is a valid trigger */

	hidx = wrtd_out_hash_table_find(&triggers[tidx].id);
	if (hidx < 0)
		return -1;

	tlist_count--;
	ht[hidx] = NULL;
	rt_send_ack(hin, pin, hout, pout);

	return 0;
}


/**
 * It gets the trigger index for a given trigger ID
 */
static int wrtd_out_trigger_index(struct wrnc_proto_header *hin, void *pin,
				  struct wrnc_proto_header *hout, void *pout)
{
	struct wrtd_trig_id *id = pin;
	uint32_t *index = pout;
	int hidx;
	int ret;

	/* Verify that the size is correct */
	if (hin->len * 4 != sizeof(struct wrtd_trig_id)) {
		pp_printf("%s: wrong incoming message size %d (expected %d)\n",
			  __func__, hin->len * 4, sizeof(struct wrtd_trig_id));
	        goto err;
	}
	if (!pout) {
		pp_printf("%s: missing payload)\n", __func__);
		goto err;
	}


	hidx = wrtd_out_hash_table_find(id);
	if (hidx >= 0) {
		*index = ((int)ht[hidx] - (int)triggers) /
			sizeof(struct wrtd_out_trigger);
	} else {
		/* Get first free */
		for (*index = 0; *index < FD_HASH_ENTRIES; ++(*index)) {
			if (!(triggers[*index].flags & ENTRY_FLAG_VALID)) {
				hout->msg_id = WRTD_OUT_ACTION_TRIG_FRE;
				break;
			}
		}
		if ((*index) == FD_HASH_ENTRIES)
			return -1;
	}

	hout->len = 1;
	return 0;

err:
	return -1;
}


/**
 * It disable the given channel and clear its internal queue
 */
static int wrtd_out_disable(struct wrnc_proto_header *hin, void *pin,
			    struct wrnc_proto_header *hout, void *pout)
{
	uint32_t ch = *((uint32_t *)pin);

	pulse_queue_init(&wrtd_out_channels[ch].priv.queue);
	fd_ch_writel(&wrtd_out_channels[ch], FD_DCR_MODE, FD_REG_DCR);

	rt_send_ack(hin, pin, hout, pout);
	return 0;
}


/**
 * Data structures to export to host system. Initialized dynamically in
 * the function init()
 */
static struct rt_structure wrtd_out_structures[__WRTD_OUT_STRUCT_MAX
					       + FD_HASH_ENTRIES] = {
	[OUT_STRUCT_DEVICE] = {
		.struct_ptr = &wrtd_out_device,
		.len = sizeof(struct wrtd_out),
	},
};

static struct rt_variable wrtd_out_variables[] = {
	[OUT_VAR_DEVICE_TIME_S] = {
		.addr = CPU_LR_BASE + WRN_CPU_LR_REG_TAI_SEC,
		.mask = 0xFFFFFFFF,
		.offset = 0,
	},
	[OUT_VAR_DEVICE_TIME_T] = {
		.addr = CPU_LR_BASE + WRN_CPU_LR_REG_TAI_CYCLES,
		.mask = 0xFFFFFFFF,
		.offset = 0,
	},
};

static action_t *wrtd_out_actions[] = {
	[RT_ACTION_RECV_PING] = rt_recv_ping,
	[RT_ACTION_RECV_VERSION] = rt_version_getter,
	[RT_ACTION_RECV_FIELD_SET] = rt_variable_setter,
	[RT_ACTION_RECV_FIELD_GET] = rt_variable_getter,
	[RT_ACTION_RECV_STRUCT_SET] = rt_structure_setter,
	[RT_ACTION_RECV_STRUCT_GET] = rt_structure_getter,
	[WRTD_OUT_ACTION_SW_TRIG] = wrtd_out_trigger_sw,
	[WRTD_OUT_ACTION_TRIG_IDX] = wrtd_out_trigger_index,
	[WRTD_OUT_ACTION_TRIG_ADD] = wrtd_out_hash_table_insert,
	[WRTD_OUT_ACTION_TRIG_DEL] = wrtd_out_hash_table_remove,
	[WRTD_OUT_ACTION_DISABLE] = wrtd_out_disable,
};

enum rt_slot_name {
	OUT_CMD_IN = 0,
	OUT_CMD_OUT,
	OUT_LOG,
};

struct rt_mq mq[] = {
	[OUT_CMD_IN] = {
		.index = 1,
		.flags = RT_MQ_FLAGS_LOCAL | RT_MQ_FLAGS_INPUT,
	},
	[OUT_CMD_OUT] = {
		.index = 1,
		.flags = RT_MQ_FLAGS_LOCAL | RT_MQ_FLAGS_OUTPUT,
	},
	[OUT_LOG] = {
		.index = 3,
		.flags = RT_MQ_FLAGS_LOCAL | RT_MQ_FLAGS_OUTPUT,
	},
};

struct rt_application app = {
	.name = "wrtd-output",
	.version = {
		.fpga_id = 0x115790de,
		.rt_id = WRTD_OUT_RT_ID,
		.rt_version = RT_VERSION(2, 0),
		.git_version = GIT_VERSION
	},
	.mq = mq,
	.n_mq = ARRAY_SIZE(mq),

	.structures = wrtd_out_structures,
	.n_structures = ARRAY_SIZE(wrtd_out_structures),

	.variables = wrtd_out_variables,
	.n_variables = ARRAY_SIZE(wrtd_out_variables),

	.actions = wrtd_out_actions,
	.n_actions = ARRAY_SIZE(wrtd_out_actions),
};


/**
 * Initialize data structures, RT application and variables
 */
void init(void)
{
	int i, j;

	pp_printf("Running %s from commit 0x%x.\n", __FILE__, version);
	rx_ebone = 0;
	rx_loopback = 0;
	promiscuous_mode = 0;
	memset(&last_received, 0, sizeof(struct wrtd_trigger_entry));
	tlist_count = 0;
	wr_state = WR_LINK_OFFLINE;
	wr_enable_lock(0);

	/* device */
	memset(&wrtd_out_device, 0, sizeof(struct wrtd_out));

	/* Channels */
	for (i = 0; i < FD_NUM_CHANNELS; i++) {
		memset(&wrtd_out_channels[i], 0,
		       sizeof(struct wrtd_out_channel));
	        wrtd_out_channels[i].base_addr = 0x100 + i * 0x100;
	        wrtd_out_channels[i].n = i;
	        wrtd_out_channels[i].config.mode = WRTD_TRIGGER_MODE_AUTO;
	        wrtd_out_channels[i].priv.pending_trig = NULL;
	        wrtd_out_channels[i].config.state = OUT_ST_IDLE;
	        wrtd_out_channels[i].priv.idle = 1;
	        wrtd_out_channels[i].config.dead_time = 80000 / 8; // 80 us
	        wrtd_out_channels[i].config.width_cycles = 1250; // 1us
		pp_printf("mode %d %d\n", i , wrtd_out_channels[i].config.mode);
	}

	/* Triggers */
	for (i = 0; i < FD_HASH_ENTRIES; i++) {
		memset(&triggers[i], 0, sizeof(struct wrtd_out_trigger));
		ht[i] = NULL;
	}

	/* librt: dynamically add structures */
	for (i = 0, j = 1; i < FD_NUM_CHANNELS; i++, j++) {
		wrtd_out_structures[j].struct_ptr = &wrtd_out_channels[i];
		wrtd_out_structures[j].len = sizeof(struct wrtd_out_channel)
			- sizeof(struct wrtd_out_channel_private);
	}
	for (i = 0, j = __WRTD_OUT_STRUCT_MAX; i < FD_HASH_ENTRIES; i++, j++) {
		wrtd_out_structures[j].struct_ptr = &triggers[i];
		wrtd_out_structures[j].len = sizeof(struct wrtd_out_trigger);
	}

	pp_printf("rt-output firmware initialized.\n");
}

int main(void)
{
	init();
	rt_init(&app);

	while (1) {
		do_rx();
		do_outputs();
		rt_mq_action_dispatch(OUT_CMD_IN);
		wr_update_link();
	}

	return 0;
}
