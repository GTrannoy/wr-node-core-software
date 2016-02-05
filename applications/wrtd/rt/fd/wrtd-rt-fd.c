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
 * rt-fd.c: real-time CPU application for the FMC Fine Delay mezzanine (Trigger Output)
 */

#include <string.h>

#include "rt.h"
#include "wrtd-common.h"
#include "hw/fd_channel_regs.h"
#include "hw/fd_main_regs.h"

#include "loop-queue.h"
#include "wrtd-serializers.h"

#define OUT_ST_IDLE 0
#define OUT_ST_ARMED 1
#define OUT_ST_TEST_PENDING 2
#define OUT_ST_CONDITION_HIT 3

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

int wr_enable_lock( int enable )
{
	if(enable)
		dp_writel ( FD_TCR_WR_ENABLE, FD_REG_TCR );
	else
		dp_writel ( 0, FD_REG_TCR);

	return 0;
}

void wr_update_link()
{
	switch(wr_state)
	{
		case WR_LINK_OFFLINE:
			if ( wr_link_up() )
			{
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
				pp_printf("WR link up!\n");
				wr_state = WR_LINK_SYNCED;
			}
			break;

		case WR_LINK_SYNCED:
			break;
	}

	if( wr_state != WR_LINK_OFFLINE && !wr_link_up() )
	{
		wr_state = WR_LINK_OFFLINE;
		wr_enable_lock(0);
	}
}

int wr_is_timing_ok()
{
	return (wr_state == WR_LINK_SYNCED);
}



/**
 * Rule defining the behaviour of a trigger output upon reception of a
 * trigger message with matching ID
 */
struct lrt_output_rule {
	uint32_t delay_cycles; /**< Delay to add to the timestamp enclosed
				  within the trigger message */
	uint16_t delay_frac;
	uint16_t state; /**< State of the rule (empty, disabled,
			   conditional action, condition, etc.) */
	struct lrt_hash_entry *cond_ptr; /**< Pointer to conditional action.
					     Used for rules that define
					     triggering conditions. */
	uint32_t latency_worst; /**< Worst-case latency (in 8ns ticks)*/
	uint32_t latency_avg_sum; /**< Average latency accumulator and
				     number of samples */
	uint32_t latency_avg_nsamples;
	int hits; /**< Number of times the rule has successfully produced
		     a pulse */
	int misses; /**< Number of times the rule has missed a pulse
		       (for any reason) */
};

#define ENTRY_FLAG_VALID (1 << 0)
/**
 * Trigger handled by this real-time application
 */
struct lrt_hash_entry {
	unsigned int flags;
	struct wrtd_trig_id id; /**< trigger identifier */
	struct lrt_output_rule ocfg[FD_NUM_CHANNELS]; /**< specific rule
							 for each channel*/
};

struct lrt_hash_entry raw_tlist[FD_HASH_ENTRIES];
unsigned int tlist_count = 0; /**< number of valid trigger entry
					in tlist */
struct lrt_hash_entry *ord_tlist[FD_HASH_ENTRIES]; /**< list of triggers
						      ordered by 'id' */


int trigger_search(struct lrt_hash_entry **tlist,
		  struct wrtd_trig_id *id,
		  unsigned int min, unsigned int max,
		  unsigned int *mid)
{
	int cmp;
#if 0
	/* Binary search */
	while (max > min)
	{
		*mid = min + (max - min) / 2;
		cmp = memcmp(&tlist[*mid]->id, id, sizeof(struct wrtd_trig_id));
		if(cmp == 0)
			return 1;
		else if (cmp < 0)
			min = *mid + 1;
		else
			max = *mid - 1;
	}

	/* When we do not find our element, then mid + 1 is the ideal position
	 for a the new entry */
	*mid++;
#else

	  /* FIXME Temporary HACK to be able to use memcmp
	     (smem not byte addressable - VHDL problem) */
	  volatile struct wrtd_trig_id tmp;
	  tmp.system = id->system;
	  tmp.source_port = id->source_port;
	  tmp.trigger = id->trigger;

	  for (*mid = min; *mid < max; (*mid)++) {
		  cmp = memcmp(&tlist[*mid]->id, &tmp, sizeof(struct wrtd_trig_id));
		  if (cmp == 0)
			  return 1;
		  else if (cmp > 0)
			  break;
	  }

#endif

#ifdef RTDEBUG
	pp_printf("%s:%d %d %d %d %d - trig ID %d:%d:%d == %d:%d:%d ?\n",
		  __func__, __LINE__,
		  cmp, min, *mid, max,
		  tlist[*mid]->id.system, tlist[*mid]->id.source_port, tlist[*mid]->id.trigger,
		  id->system, id->source_port, id->trigger);
#endif
	return 0;
}


struct lrt_hash_entry *rtfd_trigger_entry_find(struct wrtd_trig_id *id)
{
	unsigned int index;
	int ret;

	ret = trigger_search(ord_tlist, id, 0, tlist_count, &index);

	return ret ? ord_tlist[index] : NULL;
}


/**
 * It updates a trigger entry. If it does not exist yet, it creates a new
 * entry. Creating a new entry is not fast as a hash table but actually it
 * is not important to be fast for this operation.
 */
struct lrt_hash_entry * rtfd_trigger_entry_update(struct wrtd_trig_id *id,
						  int output,
						  struct lrt_output_rule *rule)
{
	unsigned int index = 0;
	int i = -1, k, ret = 0;

	ret = trigger_search(ord_tlist, id, 0, tlist_count, &index);

	if (!ret) { /* entry does not exists, add it */
		/* Look for an empty slot */
		for (i = 0; i < FD_HASH_ENTRIES; ++i)
			if (!(raw_tlist[i].flags & ENTRY_FLAG_VALID))
				break;
		if (i >= FD_HASH_ENTRIES)
			return NULL; /* array is full */

		/* Save new entry */
		raw_tlist[i].flags |= ENTRY_FLAG_VALID;
		raw_tlist[i].id = *id;
		raw_tlist[i].ocfg[output] = *rule;

		/* Order it! */
		for (k = tlist_count - 1; k >= index; --k)
			ord_tlist[k + 1] = ord_tlist[k];
		ord_tlist[index] = &raw_tlist[i];

		tlist_count++;
	} else { /* entry exists, update its channel rule */
		memcpy(&ord_tlist[index]->ocfg[output], rule,
		       sizeof(struct lrt_output_rule));
	}

#ifdef RTDEBUG
	pp_printf("%s:%d (%d)[%d] = %p %d:%d:%d\n", __func__, __LINE__,
		  i, index, ord_tlist[index], ord_tlist[index]->id.system,
		  ord_tlist[index]->id.source_port, ord_tlist[index]->id.trigger);
#endif

	return ord_tlist[index];
}


/**
 * It removes an entry from the trigger list.
 */
void rtfd_trigger_entry_remove(struct lrt_hash_entry *ent, unsigned int output)
{
	unsigned int index = 0;
	int i, ret;

	ret = trigger_search(ord_tlist, &ent->id, 0, tlist_count, &index);
	if (!ret) {
#ifdef RTDEBUG
		pp_printf("%s:%d %d:%d:%d not found\n", __func__, __LINE__,
			  ent->id.system, ent->id.source_port, ent->id.trigger);
#endif
		return; /* entry not found */
	}
#ifdef RTDEBUG
	pp_printf("%s:%d [%d] = %p %d:%d:%d\n", __func__, __LINE__,
		  index, ord_tlist[index], ord_tlist[index]->id.system,
		  ord_tlist[index]->id.source_port, ord_tlist[index]->id.trigger);
#endif
	/* Remove output rule for the given channel */
	memset(&ord_tlist[index]->ocfg[output], 0,
	       sizeof(struct lrt_output_rule));

	/* Remove the entire entry when we don't have rule */
	for (i = 0; i < FD_NUM_CHANNELS; i++)
		if (ord_tlist[index]->ocfg[i].state != HASH_ENT_EMPTY)
			break;
	if (i < FD_NUM_CHANNELS)
		return;
	memset(ord_tlist[index], 0, sizeof(struct lrt_hash_entry));

	/* Move all trigger back */
	for (i = index; i < tlist_count - 1; i++)
		ord_tlist[i] = ord_tlist[i + 1];
	ord_tlist[i] = NULL;
	tlist_count--;

#ifdef RTDEBUG
	pp_printf("[%d] = %p  --> %d:%d:%d\n",
		  index, ord_tlist[i], ord_tlist[i]->id.system,
		  ord_tlist[i]->id.source_port, ord_tlist[i]->id.trigger);
#endif
}


int rtfd_trigger_entry_rules_count(unsigned int ch)
{
	int i, count = 0;

	for (i = 0; i < tlist_count; ++i) {
		if (ord_tlist[i]->ocfg[ch].state != HASH_ENT_EMPTY)
			count++;
	}

	return count;
}


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
/* Pending conditonal trigger */
	struct lrt_output_rule *pending_trig;
/* Output pulse queue */
	struct lrt_pulse_queue queue;
/* Last enqueued trigger + delay (for dead time checking). */
	struct wr_timestamp prev_pulse;
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
static struct lrt_output outputs[FD_NUM_CHANNELS];
/* Received message counters */
static int rx_ebone = 0, rx_loopback = 0;
/* Last received trigger (i.e. last received packet from Etherbone). */
static struct wrtd_trigger_entry last_received;


/* Adjusts the timestamp in-place by adding cycles/frac value */
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

/* Puts a trigger message in the log buffer */
static void log_trigger(int type, int miss_reason, struct lrt_output *out, struct wrtd_trigger_entry *ent)
{
    	uint32_t id = WRTD_REP_LOG_MESSAGE;
    	uint32_t seq = 0;
	int chan;

	if (!out && (type != WRTD_LOG_PROMISC || !promiscuous_mode ))
		return;
	if (out && !(out->log_level & type))
		return;

	struct wrnc_msg buf = hmq_msg_claim_out (WRTD_OUT_FD_LOGGING, 16);
	chan = out ? out->index : -1;
	wrnc_msg_header (&buf, &id, &seq);
	wrnc_msg_int32 (&buf, &type);
	wrnc_msg_int32 (&buf, &chan);
	wrnc_msg_int32 (&buf, &miss_reason);
	wrtd_msg_trigger_entry (&buf, ent);

   	hmq_msg_send (&buf);
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
static int check_dead_time( struct lrt_output *out, struct wr_timestamp *ts )
{
	int delta_s = ts->seconds - out->prev_pulse.seconds;
	int delta_c = ts->ticks - out->prev_pulse.ticks;

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
	__attribute__((unused))
		volatile uint32_t dummy = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
	int latency = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES) - pq_ent->origin_cycles;

	if (latency < 0)
		latency += 125 * 1000 * 1000;

	struct lrt_output_rule *rule = pq_ent->rule;

	if(!rule)
		return;

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

void drop_trigger( struct lrt_output *out, struct pulse_queue_entry *pq_ent, struct lrt_pulse_queue *q, int reason)
{
	out->idle = 1;

	if(pulse_queue_empty(q))
	    return;

	/* Drop the pulse */
	pulse_queue_pop(q);
	pq_ent->rule->misses ++;

	if(reason == WRTD_MISS_TIMEOUT)
		out->miss_timeout ++;
	else if (reason == WRTD_MISS_NO_WR)
		out->miss_no_timing ++;

	out->last_lost = pq_ent->trig;

	/* Disarm the FD output */
	fd_ch_writel(out, FD_DCR_MODE, FD_REG_DCR);

	if(out->state == OUT_ST_TEST_PENDING)
		out->state = OUT_ST_IDLE;

	log_trigger (WRTD_LOG_MISSED, reason,
		     out, &pq_ent->trig);
}

/* Output driving function. Reads pulses from the output queue,
   programs the output and updates the output statistics. */
void do_output (struct lrt_output *out)
{
	struct lrt_pulse_queue *q = &out->queue;
	struct pulse_queue_entry *pq_ent = pulse_queue_front(q);
	uint32_t dcr = fd_ch_readl(out, FD_REG_DCR);

	/* Check if the output has triggered */
	if(!out->idle) {

		if( !wr_is_timing_ok() ) {
			drop_trigger(out, pq_ent, q, WRTD_MISS_NO_WR);
		}
		else if (!(dcr & FD_DCR_PG_TRIG)) { /* Nope, armed but still waiting for trigger */
			if (check_output_timeout (out))	{
				drop_trigger(out, pq_ent, q, WRTD_MISS_TIMEOUT);
			}
		} else {
			out->last_executed = pq_ent->trig;
			pq_ent->rule->hits ++;
			pulse_queue_pop(q);
			out->hits++;
			out->idle = 1;
			out->flags |= WRTD_TRIGGERED;

			if(out->state == OUT_ST_TEST_PENDING)
				out->state = OUT_ST_IDLE;

			log_trigger (WRTD_LOG_EXECUTED, 0, out, &pq_ent->trig);
		}
		return;
	}

	/* Output is idle: check if there's something in the queue to execute */
	if(pulse_queue_empty(q))
        	return;

	pq_ent = pulse_queue_front(q);
	struct wr_timestamp *ts = &pq_ent->trig.ts;

	if( !wr_is_timing_ok() )
		drop_trigger(out, pq_ent, q, WRTD_MISS_NO_WR);


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

#ifdef RTDEBUG
	pp_printf("%s:%d ch %d 0x%x\n", __func__, __LINE__, output, rule->state);
#endif
	if(rule->state & HASH_ENT_DISABLED)
		return;

	if (!(out->flags & WRTD_ENABLED))
		return;

	ts_adjust_delay (&adjusted, rule->delay_cycles, rule->delay_frac);

	struct wrtd_trigger_entry ent;
	ent.ts = *ts;
	ent.id = *id;
	ent.seq = seq;

	if (!check_dead_time(out, &adjusted)) {
		rule->misses ++;
		out->miss_deadtime ++;
		log_trigger (WRTD_LOG_MISSED, WRTD_MISS_DEAD_TIME, out, &ent);
		return;
	}

	if (!wr_is_timing_ok()) {
		rule->misses ++;
		out->miss_no_timing ++;
		log_trigger (WRTD_LOG_MISSED, WRTD_MISS_NO_WR, out, &ent);
		return;
	}

	switch(out->state)
	{
		case OUT_ST_IDLE:
			return; // output not armed

		case OUT_ST_ARMED:
			if (rule->state & HASH_ENT_CONDITION)
			{
				out->pending_trig = &rule->cond_ptr->ocfg[output];
				out->state = OUT_ST_CONDITION_HIT;
				return;
			} else if (out->mode == WRTD_TRIGGER_MODE_SINGLE) {
				out->flags &= ~WRTD_ARMED;
				out->state = OUT_ST_IDLE;
			}
			break;
		case OUT_ST_TEST_PENDING:
			return;

		case OUT_ST_CONDITION_HIT:
			if (rule != out->pending_trig)
				return;

			if (out->mode == WRTD_TRIGGER_MODE_SINGLE) {
				out->flags &= ~WRTD_ARMED;
				out->state = OUT_ST_IDLE;
			} else {
				out->state = OUT_ST_ARMED;
			}
			break;
		default:
			break;

	}

	if ((pq_ent = pulse_queue_push (&out->queue)) == NULL) {
		struct wrtd_trigger_entry ent;
		ent.ts = *ts;
		ent.id = *id;
		ent.seq = seq;
		log_trigger (WRTD_LOG_MISSED, WRTD_MISS_OVERFLOW, out, &ent);

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

	out->prev_pulse = adjusted;

	log_trigger (WRTD_LOG_FILTERED, 0, out, &out->last_enqueued);

}

static void filter_trigger(struct wrtd_trigger_entry *trig)
{
	struct lrt_hash_entry *ent = rtfd_trigger_entry_find(&trig->id);
	int j;

	log_trigger(WRTD_LOG_PROMISC, 0, NULL, trig);
	last_received = *trig;
#ifdef RTDEBUG
	pp_printf("%s:%d Trigger %d:%d:%d - entry %p\n",
		  __func__, __LINE__,
		  trig->id.system, trig->id.source_port, trig->id.trigger, ent);
#endif
	if(ent)
	{
		struct wr_timestamp ts = trig->ts;
		struct wrtd_trig_id id = trig->id;
		int seq = trig->seq;
		for(j = 0; j < FD_NUM_CHANNELS; j++)
			if(ent->ocfg[j].state != HASH_ENT_EMPTY)
				enqueue_trigger (j, &ent->ocfg[j], &id, &ts, seq);
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
		out->state = OUT_ST_IDLE;
		/* clear the pulse queue and disable the physical output */
		pulse_queue_init ( &out->queue );
		fd_ch_writel(out, FD_DCR_MODE, FD_REG_DCR);
	}

//	pp_printf("chan-enable %d %d flags %x\n", ch, enabled, out->flags);

	ctl_ack (seq);
}


static inline void ctl_trig_assign (uint32_t seq, struct wrnc_msg *ibuf)
{
	uint32_t id_trigger_handle = WRTD_REP_TRIGGER_HANDLE;
	int ch, is_cond;
	struct wrnc_msg obuf;
	struct wrtd_trig_id id, cond;
	struct lrt_output_rule rule;
	struct lrt_trigger_handle handle = {NULL, NULL, 0, 0};
	struct lrt_output *out;

	wrnc_msg_int32(ibuf, &ch);
	handle.channel = ch;

	/* Get the trigger ID (direct) */
	wrtd_msg_trig_id(ibuf, &id);
	wrnc_msg_int32(ibuf, &is_cond);
	wrtd_msg_trig_id(ibuf, &cond);

	int n_req = is_cond ? 2 : 1;

	/* We need at least one or two hash entries */
	if (tlist_count + n_req >= FD_HASH_ENTRIES) {
		ctl_nack(seq, -1);
		return;
	}

	/* Set condition trigger */
	if (is_cond) {
		memset(&rule, 0, sizeof(struct lrt_output_rule));
		rule.delay_cycles = 100000000 / 8000;
		rule.state = HASH_ENT_CONDITION | HASH_ENT_DISABLED;
		handle.cond = rtfd_trigger_entry_update(&cond, ch, &rule);
	}

	/* Create an empty rule with default edelay of 100 us */
	memset(&rule, 0, sizeof(struct lrt_output_rule));
	rule.delay_cycles = 100000000 / 8000;
	rule.state |= HASH_ENT_DISABLED;
	if (is_cond) {
		rule.state |= HASH_ENT_CONDITIONAL;
		rule.cond_ptr = handle.cond;
	} else {
		rule.state |= HASH_ENT_DIRECT;
		rule.cond_ptr = NULL;
	}
	handle.trig = rtfd_trigger_entry_update( &id, ch, &rule );

	/* Notify that there is at least one trigger
	   assigned to the current channel */
	out = &outputs[ch];
	out->flags |= WRTD_TRIGGER_ASSIGNED;

	/* Create the response */
	obuf = ctl_claim_out_buf();
	wrnc_msg_header(&obuf, &id_trigger_handle, &seq);
        wrnc_msg_int32(&obuf, &handle.channel);
        wrnc_msg_uint32(&obuf, (uint32_t *) &handle.cond);
        wrnc_msg_uint32(&obuf, (uint32_t *) &handle.trig);
        hmq_msg_send (&obuf);
}

static inline void ctl_trig_remove (uint32_t seq, struct wrnc_msg *ibuf)
{
	struct lrt_trigger_handle handle;
	struct lrt_output *out;

	wrnc_msg_int32(ibuf, &handle.channel);
	wrnc_msg_uint32(ibuf, (uint32_t *) &handle.cond);
	wrnc_msg_uint32(ibuf, (uint32_t *) &handle.trig);

	out = &outputs[handle.channel];
	if(handle.cond)
	{
		/* check if this wasn't a pending conditional trigger and re-arm the output
		   if so */
		if(out->state == OUT_ST_CONDITION_HIT && out->pending_trig == (struct lrt_output_rule *)handle.trig)
		{
			out->state = OUT_ST_ARMED;
		}

		rtfd_trigger_entry_remove(handle.cond, handle.channel);

	}
	rtfd_trigger_entry_remove(handle.trig, handle.channel);

	/* Remove assigned flag when no trigger is assigned to a channel */
	if (rtfd_trigger_entry_rules_count(handle.channel) == 0)
		out->flags &= ~WRTD_TRIGGER_ASSIGNED;

	ctl_ack(seq);
}

static inline void ctl_chan_enable_trigger (uint32_t seq, struct wrnc_msg *ibuf)
{
	int ch, enable;
	struct lrt_hash_entry *ent;

	wrnc_msg_int32(ibuf, &ch);
	wrnc_msg_int32(ibuf, &enable);
	wrnc_msg_uint32(ibuf, (uint32_t *) &ent);

	if (!ent)
		ctl_nack(seq, -1);
	struct lrt_output_rule *rule = &ent->ocfg[ch];

#ifdef RTDEBUG
	pp_printf("%s:%d ch:%d en:%d ptr:%p rule:%p\n", __func__, __LINE__,
		  ch, enable, ent, rule);
#endif

	if (enable)
		rule->state &= ~HASH_ENT_DISABLED;
	else
		rule->state |= HASH_ENT_DISABLED;

	// fixme: purge pending pulses conditional pulses from the queue
	ctl_ack(seq);
}

static void bag_hash_entry ( struct wrtd_trig_id *id, struct lrt_output_rule *rule, struct wrnc_msg *obuf )
{
	wrnc_msg_uint16 (obuf, &rule->state);
	wrtd_msg_trig_id (obuf, id);
	wrnc_msg_uint32 (obuf, &rule->delay_cycles);
	wrnc_msg_uint16 (obuf, &rule->delay_frac);
	wrnc_msg_uint32 (obuf, &rule->latency_worst);
	wrnc_msg_uint32 (obuf, &rule->latency_avg_sum);
	wrnc_msg_uint32 (obuf, &rule->latency_avg_nsamples);
	wrnc_msg_int32 (obuf, &rule->hits);
	wrnc_msg_int32 (obuf, &rule->misses);
}

static struct lrt_hash_entry *rtfd_trigger_get_first(unsigned int start,
						     unsigned int ch)
{
	int i;

	for (i = start; i < tlist_count; i++) {
		if ((ord_tlist[i]->flags & ENTRY_FLAG_VALID) &&
		    ord_tlist[i]->ocfg[ch].state != HASH_ENT_EMPTY)
			return ord_tlist[i];

	}

	return NULL;
}
void send_hash_entry (uint32_t seq, int ch, int valid, struct lrt_hash_entry *ent)
{
	int is_conditional, i;
	struct wrnc_msg obuf = ctl_claim_out_buf();
	uint32_t id_hash_entry = WRTD_REP_HASH_ENTRY, next;
        struct lrt_hash_entry *cond = NULL;

#ifdef RTDEBUG
	pp_printf("%s:%d %p %d\n", __func__, __LINE__, ent, ch);
	if (ent)
		pp_printf("%s:%d     %d:%d:%d\n", __func__, __LINE__,
		ent->id.system, ent->id.source_port, ent->id.trigger);
#endif
	/* Create the response */
	wrnc_msg_header(&obuf, &id_hash_entry, &seq);
	wrnc_msg_int32(&obuf, &valid);

	if(valid) {
		cond = ent->ocfg[ch].cond_ptr;
		is_conditional = (cond ? 1 : 0);
		wrnc_msg_int32(&obuf, &is_conditional);

		wrnc_msg_uint32(&obuf, (uint32_t *) &ent);
		wrnc_msg_uint32(&obuf, (uint32_t *) &cond);

		/* Send triggers information*/
		bag_hash_entry(&ent->id, &ent->ocfg[ch], &obuf);
		if (cond)
			wrtd_msg_trig_id(&obuf, &cond->id);

		/* Look for the next trigger declared (this is only used
		   to get the full list of triggers ) */
		for (i = 0; i < tlist_count; ++i)
			if (ord_tlist[i] == ent)
				break;
		next = (uint32_t) rtfd_trigger_get_first(i + 1, ch);
		wrnc_msg_uint32(&obuf, (uint32_t *) &next);
	}

	hmq_msg_send (&obuf);
}

static inline void ctl_read_hash (uint32_t seq, struct wrnc_msg *ibuf )
{
	int ch, entry_ok;
	uint32_t ptr;
	struct lrt_hash_entry *ent;

	wrnc_msg_int32(ibuf, &ch);
	wrnc_msg_uint32(ibuf, &ptr);

	/* Get the first one if not specified */
	ent = !ptr ? rtfd_trigger_get_first(0, ch) : (void *) ptr;
	entry_ok = ent && ent->ocfg[ch].state != HASH_ENT_EMPTY;

	send_hash_entry (seq, ch, entry_ok, ent);
}

static inline void ctl_chan_get_trigger_by_id (uint32_t seq, struct wrnc_msg *ibuf )
{
	struct wrtd_trig_id id;
	int ch;

	wrnc_msg_int32(ibuf, &ch);
	wrtd_msg_trig_id(ibuf, &id);

	struct lrt_hash_entry *ent = rtfd_trigger_entry_find(&id);

	send_hash_entry (seq, ch, ent ? 1 : 0, ent);
}

static inline void ctl_chan_get_trigger_state (uint32_t seq, struct wrnc_msg *ibuf)
{
	struct lrt_trigger_handle handle;

	wrnc_msg_int32(ibuf, &handle.channel);
	wrnc_msg_uint32(ibuf, (uint32_t *) &handle.cond);
	wrnc_msg_uint32(ibuf, (uint32_t *) &handle.trig);

	send_hash_entry (seq, handle.channel, 1, handle.cond ? handle.cond : handle.trig );
}

static inline void ctl_chan_reset_counters (uint32_t seq, struct wrnc_msg *ibuf)
{
	int ch;
	wrnc_msg_int32(ibuf, &ch);

	struct lrt_output *st = &outputs[ch];

	st->hits = 0;
	st->miss_timeout = 0;
	st->miss_deadtime = 0;
	st->miss_no_timing = 0;
	st->miss_overflow = 0;
	st->flags &= ~WRTD_LAST_VALID;

	ctl_ack (seq);
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

static inline void ctl_base_time (uint32_t seq, struct wrnc_msg *ibuf)
{
	struct wrnc_msg buf = ctl_claim_out_buf();
	uint32_t id_ack = WRTD_REP_BASE_TIME_ID;
	struct wr_timestamp ts;

	ts.seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
	ts.ticks = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
	ts.frac = 0;
	wrnc_msg_header (&buf, &id_ack, &seq);
	wrtd_msg_timestamp(&buf, &ts);
	hmq_msg_send (&buf);
}

static inline void ctl_version(uint32_t seq, struct wrnc_msg *ibuf)
{
	struct wrnc_msg buf = ctl_claim_out_buf();
	uint32_t id_ack = WRTD_REP_VERSION;

	wrnc_msg_header(&buf, &id_ack, &seq);
	wrnc_msg_uint32(&buf, &version);
	hmq_msg_send(&buf);
}

static inline void ctl_chan_set_delay (uint32_t seq, struct wrnc_msg *ibuf)
{
	int ch;
	struct lrt_hash_entry *ent;

	wrnc_msg_int32(ibuf, &ch);
	wrnc_msg_uint32(ibuf, (uint32_t*) &ent);

	if(ent->ocfg[ch].state != HASH_ENT_EMPTY)
	{
		wrnc_msg_uint32(ibuf, &ent->ocfg[ch].delay_cycles);
		wrnc_msg_uint16(ibuf, &ent->ocfg[ch].delay_frac);
	}

	ctl_ack(seq);
}

static inline void ctl_chan_get_state (uint32_t seq, struct wrnc_msg *ibuf)
{
	int ch;

	wrnc_msg_int32(ibuf, &ch);

	struct lrt_output *st = &outputs[ch];
	struct wrnc_msg obuf = ctl_claim_out_buf();

	uint32_t id_state = WRTD_REP_STATE;

	if(st->state != OUT_ST_IDLE)
		st->flags |= WRTD_ARMED;


	st->flags &= ~WRTD_NO_WR;

	if( !wr_is_timing_ok() )
		st->flags |= WRTD_NO_WR;

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
	wrtd_msg_trigger_entry(&obuf, &last_received);
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
		wrtd_msg_timestamp(ibuf, &tc);
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

	wrtd_msg_timestamp(&obuf, &tc);
	hmq_msg_send (&obuf);

	struct lrt_output *out = &outputs[ch];
	struct pulse_queue_entry *pq_ent;

	/* clear all pulses from the queue */
	pulse_queue_init(&out->queue);
	pq_ent = pulse_queue_push (&out->queue);

	pq_ent->trig.ts = tc;
	pq_ent->trig.id.system = 0xffffffff;
	pq_ent->trig.id.source_port = 0xffffffff;
	pq_ent->trig.id.trigger = 0xffffffff;
	pq_ent->trig.seq = 0xffffffff;
	pq_ent->origin_cycles = tc.ticks;
	pq_ent->rule = NULL;

	out->last_enqueued.ts = tc;
	out->last_enqueued.id = pq_ent->trig.id;
	out->last_enqueued.seq = pq_ent->trig.seq;
	out->state = OUT_ST_TEST_PENDING;

}


static inline void ctl_chan_set_mode (uint32_t seq, struct wrnc_msg *ibuf)
{
	int ch;
	wrnc_msg_int32(ibuf, &ch);

	struct lrt_output *st = &outputs[ch];
	wrnc_msg_int32(ibuf, &st->mode);

	st->flags &= ( WRTD_TRIGGERED | WRTD_LAST_VALID) ;
	if( st->mode == WRTD_TRIGGER_MODE_SINGLE ) {
		st->flags &= ~WRTD_ARMED;
		st->state = OUT_ST_IDLE;
	}


	ctl_ack(seq);
}

static inline void ctl_chan_set_width (uint32_t seq, struct wrnc_msg *ibuf)
{
	int ch;
	wrnc_msg_int32(ibuf, &ch);

	struct lrt_output *st = &outputs[ch];
	wrnc_msg_int32(ibuf, &st->width_cycles);

	ctl_ack(seq);
}


static inline void ctl_chan_arm (uint32_t seq, struct wrnc_msg *ibuf)
{
	int ch, armed;
	wrnc_msg_int32(ibuf, &ch);
	wrnc_msg_int32(ibuf, &armed);

	struct lrt_output *st = &outputs[ch];

	st->flags &= ~WRTD_TRIGGERED;

	if(armed) {
		st->flags |= WRTD_ARMED;
		st->state = OUT_ST_ARMED;
	} else {
		st->flags &= ~WRTD_ARMED;
		st->state = OUT_ST_IDLE;
	}
	ctl_ack(seq);
}

static inline void ctl_chan_set_log_level (uint32_t seq, struct wrnc_msg *ibuf)
{
	int ch, i;
	wrnc_msg_int32(ibuf, &ch);

	struct lrt_output *st = &outputs[ch];
	wrnc_msg_uint32(ibuf, &st->log_level);

	ctl_ack(seq);

	/* Set promiscuous_mode - so it's enable if at least one channel
	   has enable it */
	promiscuous_mode = 0;
	for (i = 0; i < FD_NUM_CHANNELS; i++)
		promiscuous_mode |= (outputs[i].log_level & WRTD_LOG_PROMISC);
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

//	pp_printf("cmd %x\n", cmd);

#define _CMD(id, func)          \
    case id:                    \
    {                           \
        func(seq, &ibuf);       \
        break;                  \
    }

	switch(cmd) {
	_CMD(WRTD_CMD_FD_TRIG_ASSIGN,			ctl_trig_assign)
	_CMD(WRTD_CMD_FD_TRIG_REMOVE,			ctl_trig_remove)
	_CMD(WRTD_CMD_FD_TRIG_ENABLE,			ctl_chan_enable_trigger)
	_CMD(WRTD_CMD_FD_TRIG_SET_DELAY,		ctl_chan_set_delay)
	_CMD(WRTD_CMD_FD_TRIG_GET_BY_ID,		ctl_chan_get_trigger_by_id)
	_CMD(WRTD_CMD_FD_TRIG_GET_STATE,		ctl_chan_get_trigger_state)

	_CMD(WRTD_CMD_FD_CHAN_RESET_COUNTERS,		ctl_chan_reset_counters)
	_CMD(WRTD_CMD_FD_CHAN_ENABLE,			ctl_chan_enable)
	_CMD(WRTD_CMD_FD_CHAN_SET_MODE,			ctl_chan_set_mode)
	_CMD(WRTD_CMD_FD_CHAN_ARM,			ctl_chan_arm)
	_CMD(WRTD_CMD_FD_CHAN_GET_STATE,		ctl_chan_get_state)
	_CMD(WRTD_CMD_FD_CHAN_SET_LOG_LEVEL,		ctl_chan_set_log_level)
	_CMD(WRTD_CMD_FD_CHAN_SET_WIDTH,		ctl_chan_set_width)

	_CMD(WRTD_CMD_FD_READ_HASH,			ctl_read_hash)
	_CMD(WRTD_CMD_FD_BASE_TIME,                     ctl_base_time)
	_CMD(WRTD_CMD_FD_PING,                          ctl_ping)
	_CMD(WRTD_CMD_FD_CHAN_DEAD_TIME,                ctl_chan_set_dead_time)
	_CMD(WRTD_CMD_FD_VERSION,                       ctl_version)
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
	        outputs[i].mode = WRTD_TRIGGER_MODE_AUTO;
	        outputs[i].pending_trig = NULL;
	        outputs[i].state = OUT_ST_IDLE;
	        outputs[i].idle = 1;
	        outputs[i].dead_time = 80000 / 8; // 80 us
	        outputs[i].width_cycles = 1250; // 1us
    }
}


void init()
{
	int i;

	rx_ebone = 0;
	rx_loopback = 0;
	promiscuous_mode = 0;

	memset(&last_received, 0, sizeof(struct wrtd_trigger_entry));
	tlist_count = 0;
	wr_state = WR_LINK_OFFLINE;
	wr_enable_lock(0);

	init_outputs();
	for (i = 0; i < FD_HASH_ENTRIES; i++) {
		memset(&raw_tlist[i], 0, sizeof(struct lrt_hash_entry));
		ord_tlist[i] = NULL;
	}

	pp_printf("rt-fd firmware initialized.\n");
}

int main()
{
	pp_printf("Running %s from commit 0x%x.\n", __FILE__, version);
	init();

	for(;;) {
		do_rx();
		do_outputs();
		do_control();
		wr_update_link();
	}

	return 0;
}
