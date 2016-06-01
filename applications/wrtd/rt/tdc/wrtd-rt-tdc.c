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
 * rt-tdc.c: real-time CPU application for the FMC TDC mezzanine (Trigger Input)
 */

#include <string.h>

#include "mockturtle-rt.h"
#include "wrtd-common.h"
#include "hw/fmctdc-direct.h"
#include "hw/tdc_regs.h"
#include "loop-queue.h"

#include <libmockturtle-rt.h>

#define DEFAULT_DEAD_TIME (80000/16)

#define BASE_DP_TDC_REGS    0x2000
#define BASE_DP_TDC_DIRECT  0x8000

enum wrtd_in_wr_link {
	WR_LINK_OFFLINE = 1,
	WR_LINK_ONLINE,
	WR_LINK_SYNCING,
	WR_LINK_SYNCED,
	WR_LINK_TDC_WAIT,
};

static struct wrtd_in wrtd_in_dev;
static struct wrtd_in_channel wrtd_in_channels[TDC_NUM_CHANNELS];


uint32_t seq = 0; /**< global sequence number */
uint32_t sent_packets = 0; /**< Total number of packets sent */
int coalesce_count = 0; /**< RMQ coalescing counter */
int wr_state = 0; /**< White-Rabbit link status */
uint32_t tai_start = 0;


static inline int wr_link_up()
{
	return dp_readl(BASE_DP_TDC_REGS + TDC_REG_WR_STAT) & TDC_WR_STAT_LINK;
}

static inline int wr_time_locked()
{
	return dp_readl(BASE_DP_TDC_REGS + TDC_REG_WR_STAT) & TDC_WR_STAT_AUX_LOCKED;
}

static inline int wr_time_ready()
{
	return dp_readl(BASE_DP_TDC_REGS + TDC_REG_WR_STAT) & TDC_WR_STAT_TIME_VALID;
}

static inline int wr_is_timing_ok()
{
	return (wr_state == WR_LINK_SYNCED);
}


/**
 * It sends a log entry to the host system
 */
static int wrtd_in_trigger_log(int type, int miss_reason,
			       struct wrtd_in_channel *st,
			       struct wrtd_trigger_entry *ent)
{
	struct trtl_msg out_buf;
	struct wrtd_log_entry *log;
	struct trtl_proto_header hdr = {
		.rt_app_id = 0,
		.msg_id = WRTD_IN_ACTION_LOG,
		.slot_io = WRTD_OUT_TDC_LOGGING,
		.seq = seq,
		.len = sizeof(struct wrtd_log_entry) / 4,
		.flags = 0x0,
		.trans = 0x0,
		.time = 0x0,
	};

	if (!(st->config.log_level & type))
		return -1;

	out_buf = rt_mq_claim_out(&hdr);
	log = (struct wrtd_log_entry *)rt_proto_payload_get(out_buf.data);
	log->type = type;
	log->channel = st->n;
	log->miss_reason = miss_reason;
	log->seq = ent->seq;
	log->id = ent->id;
	log->ts = ent->ts;
	rt_proto_header_set((void *) out_buf.data, &hdr);
	rt_mq_msg_send(&out_buf);

	return 0;
}


/**
 * Creates a trigger message with timestamp (ts) for the channel (ch) and
 * pushes it to the RMQ output (without sending) and sends immediately through
 * the loopback queue
 */
static inline void send_trigger (struct wrtd_trigger_entry *ent)
{
	volatile struct wrtd_trigger_message *msg;

	msg =mq_map_out_buffer(1, WRTD_REMOTE_OUT_TDC);
	msg->triggers[coalesce_count].id = ent->id;
	msg->triggers[coalesce_count].seq = ent->seq;
	msg->triggers[coalesce_count].ts = ent->ts;

	loop_queue_push(&ent->id, ent->seq, &ent->ts);

	coalesce_count++;
}


/**
 * Flushes the triggerrs in the RMQ output buffer to the WR Network
 */
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

	mq_send(1, WRTD_REMOTE_OUT_TDC, 7 + 7 * coalesce_count);
	coalesce_count = 0;
	sent_packets++;
}


/**
 * Processes a pulse with timestamp (ts) arriving on channel (channel)
 */
static inline void do_channel(int channel, struct wr_timestamp *ts)
{
	struct wrtd_in_channel *ch = &wrtd_in_channels[channel];
	struct wrtd_trigger_entry ent = {
		.ts = *ts,
	};

	/* Apply timebase offset to align TDC time with WR timebase */
	ts_sub(ts, &ch->config.timebase_offset);
	ch->stats.last_tagged = *ts;

	/* Log raw value if needed */
	wrtd_in_trigger_log(WRTD_LOG_RAW, 0, ch, &ent);

	ch->stats.total_pulses++;

	/* Apply trigger delay */
	ts_add(ts, &ch->config.delay);

	/* Enable/Arm/Trigger logic */
	if (!((ch->config.flags & WRTD_TRIGGER_ASSIGNED ) &&
	      (ch->config.flags & WRTD_ARMED)))
		return;

	if (!wr_is_timing_ok()) {
		ch->stats.miss_no_timing++;
		wrtd_in_trigger_log(WRTD_LOG_MISSED, WRTD_MISS_NO_WR, ch, &ent);
		return;
	}

	ent.ts = *ts;
	ent.id = ch->config.id;
	ent.seq = ch->stats.seq++;

	ch->config.flags |= WRTD_TRIGGERED;
	if (ch->config.mode == WRTD_TRIGGER_MODE_SINGLE )
		ch->config.flags &= ~WRTD_ARMED;

	ch->stats.sent_pulses++;
	ch->stats.last_sent = ent;

	send_trigger(&ent);
	wrtd_in_trigger_log(WRTD_LOG_SENT, 0, ch, &ent);

	ch->config.flags |= WRTD_LAST_VALID;
}

/**
 * Handles input timestamps from all TDC channels, calling do_output() on
 * incoming pulses
 */
static inline void do_input(void)
{
	int i;

	/* Prepare for message transmission */
	mq_claim(1, WRTD_REMOTE_OUT_TDC);

	/* We can send up to TDC_TRIGGER_COALESCE_LIMIT triggers in a
	   single message - the loop will iterate up to this limit or exit
	   immediately if there's no more input pulses in the TDC FIFO */
	for (i = 0; i < TDC_TRIGGER_COALESCE_LIMIT; i++) {
		uint32_t fifo_sr = dp_readl(BASE_DP_TDC_DIRECT + DR_REG_FIFO_CSR);
		struct wr_timestamp ts;
		int meta;

		/* Poll the FIFO and read the timestamp */
		if(fifo_sr & DR_FIFO_CSR_EMPTY)
			break;

		ts.seconds = dp_readl(BASE_DP_TDC_DIRECT + DR_REG_FIFO_R0);
		ts.ticks = dp_readl(BASE_DP_TDC_DIRECT + DR_REG_FIFO_R1);
		meta = dp_readl(BASE_DP_TDC_DIRECT + DR_REG_FIFO_R2);

		/* Convert from ACAM bins (81ps) to WR time format. Numerical
		   hack used  to avoid time-consuming division. */
		ts.frac = ( (meta & 0x3ffff) * 5308 ) >> 7;
		ts.ticks += ts.frac >> 12;
		ts.frac &= 0xfff;

		/* Make sure there's no overflow after conversion */
		if (ts.ticks >= 125000000) {
			ts.ticks -= 125000000;
			ts.seconds ++;
		}

		int channel = (meta >> 19) & 0x7;

		/* Pass the timestamp to triggering/TX logic */
		do_channel(channel, &ts);
	}

	/* Flush the RMQ buffer if it contains anything */
	if(coalesce_count)
		flush_tx();
}


/**
 * It generate a software trigger accorging to the trigger entry coming
 * from the user space. The user space entry must provide the trigger
 * delay, then this function will add it to the current time to compute
 * the correct fire time.
 */
static int wrtd_in_trigger_sw(struct trtl_proto_header *hin, void *pin,
			      struct trtl_proto_header *hout, void *pout)
{
	struct wrtd_trigger_entry ent;
	struct wr_timestamp ts;

	/* Verify that the size is correct */
	if (hin->len * 4 != sizeof(struct wrtd_trigger_entry)) {
		rt_send_nack(hin, pin, hout, pout);
		return -1;
	}

	/* Copy the trigger instance and ack the message */
	memcpy(&ent, pin, hin->len * 4);
	rt_send_ack(hin, pin, hout, pout);

	/* trigger entity ts from the host contains the delay.
	   So add to it the current time */

	struct wr_timestamp ts_orig;
	
	ts_orig.seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
	ts_orig.ticks = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);

	ts.seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
	ts.ticks = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
	ts.frac = 0;
	ts_add(&ent.ts, &ts);


	if( ent.ts.seconds > ts_orig.seconds + 1000 )
	{
    	    pp_printf("Warning: suspiciously long software trigger delay %d tai-secs %d", (int)ent.ts.seconds, (int)ts_orig.seconds);
	}
	
	/* Send trigger */
	send_trigger(&ent);

	return 0;
}

void wr_enable_lock(int enable)
{
	dp_writel(TDC_CTRL_DIS_ACQ, BASE_DP_TDC_REGS + TDC_REG_CTRL);
	if (enable)
		dp_writel(TDC_WR_CTRL_ENABLE, BASE_DP_TDC_REGS + TDC_REG_WR_CTRL);
	else
		dp_writel(0, BASE_DP_TDC_REGS + TDC_REG_WR_CTRL);
	dp_writel(TDC_CTRL_EN_ACQ, BASE_DP_TDC_REGS + TDC_REG_CTRL);
}

/**
 * It updates the White-Rabbit link status
 */
void wr_update_link(void)
{

	switch (wr_state) {
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
			pp_printf("rt-tdc: WR synced, waiting for TDC plumbing to catch up...\n");
			wr_state = WR_LINK_TDC_WAIT;
			tai_start = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
		}
		break;
	case WR_LINK_TDC_WAIT:
		if (lr_readl(WRN_CPU_LR_REG_TAI_SEC) == (tai_start + 4)) {
			pp_printf("rt-tdc: WR TDC synced\n");
			wr_state = WR_LINK_SYNCED;
		}
		break;
	case WR_LINK_SYNCED:
		break;
	}

	if ( wr_state != WR_LINK_OFFLINE && !wr_link_up() ) {
	        pp_printf("rt-tdc: WR sync lost\n");
		wr_state = WR_LINK_OFFLINE;
		wr_enable_lock(0);
	}
}


/**
 * Data structures to export to host system
 */
static struct rt_structure wrtd_in_structures[] = {
	[IN_STRUCT_DEVICE] = {
		.struct_ptr = &wrtd_in_dev,
		.len = sizeof(struct wrtd_in),
	},
	[IN_STRUCT_CHAN_0] = {
		.struct_ptr = &wrtd_in_channels[0],
		.len = sizeof(struct wrtd_in_channel),
	},
	[IN_STRUCT_CHAN_1] = {
		.struct_ptr = &wrtd_in_channels[1],
		.len = sizeof(struct wrtd_in_channel),
	},
	[IN_STRUCT_CHAN_2] = {
		.struct_ptr = &wrtd_in_channels[2],
		.len = sizeof(struct wrtd_in_channel),
	},
	[IN_STRUCT_CHAN_3] = {
		.struct_ptr = &wrtd_in_channels[3],
		.len = sizeof(struct wrtd_in_channel),
	},
	[IN_STRUCT_CHAN_4] = {
		.struct_ptr = &wrtd_in_channels[4],
		.len = sizeof(struct wrtd_in_channel),
	},
};

static struct rt_variable wrtd_in_variables[] = {
	[IN_VAR_CHAN_ENABLE] = {
		.addr = CPU_DP_BASE + BASE_DP_TDC_DIRECT + DR_REG_CHAN_ENABLE,
		.mask = 0x2F,
		.offset = 16,
	},
	[IN_VAR_DEVICE_TIME_S] = {
		.addr = CPU_LR_BASE + WRN_CPU_LR_REG_TAI_SEC,
		.mask = 0xFFFFFFFF,
		.offset = 0,
	},
	[IN_VAR_DEVICE_TIME_T] = {
		.addr = CPU_LR_BASE + WRN_CPU_LR_REG_TAI_CYCLES,
		.mask = 0xFFFFFFFF,
		.offset = 0,
	},
	[IN_VAR_DEVICE_SENT_PACK] = {
		.addr = (uint32_t)&sent_packets,
		.mask = 0xFFFFFFFF,
		.offset = 0,
	},
	[IN_VAR_DEVICE_DEAD_TIME] = {
		.addr = CPU_DP_BASE + BASE_DP_TDC_DIRECT + DR_REG_DEAD_TIME,
		.mask = 0xFFFFFFFF,
		.offset = 0,
	},
	[IN_VAR_DEVICE_CHAN_ENABLE] = {
		.addr = CPU_DP_BASE + BASE_DP_TDC_DIRECT + DR_REG_CHAN_ENABLE,
		.mask = 0x1F,
		.offset = 0,
	},
};

static action_t *wrtd_in_actions[] = {
	[RT_ACTION_RECV_PING] = rt_recv_ping,
	[RT_ACTION_RECV_VERSION] = rt_version_getter,
	[RT_ACTION_RECV_FIELD_SET] = rt_variable_setter,
	[RT_ACTION_RECV_FIELD_GET] = rt_variable_getter,
	[RT_ACTION_RECV_STRUCT_SET] = rt_structure_setter,
	[RT_ACTION_RECV_STRUCT_GET] = rt_structure_getter,
	[WRTD_IN_ACTION_SW_TRIG] = wrtd_in_trigger_sw,
};

enum rt_slot_name {
	IN_CMD_IN = 0,
	IN_CMD_OUT,
	IN_LOG,
};

struct rt_mq mq[] = {
	[IN_CMD_IN] = {
		.index = 0,
		.flags = RT_MQ_FLAGS_LOCAL | RT_MQ_FLAGS_INPUT,
	},
	[IN_CMD_OUT] = {
		.index = 0,
		.flags = RT_MQ_FLAGS_LOCAL | RT_MQ_FLAGS_OUTPUT,
	},
	[IN_LOG] = {
		.index = 2,
		.flags = RT_MQ_FLAGS_LOCAL | RT_MQ_FLAGS_OUTPUT,
	},
};

struct rt_application app = {
	.name = "wrtd-input",
	.version = {
		.fpga_id = 0x115790de,
		.rt_id = WRTD_IN_RT_ID,
		.rt_version = RT_VERSION(2, 0),
		.git_version = GIT_VERSION
	},
	.mq = mq,
	.n_mq = ARRAY_SIZE(mq),

	.structures = wrtd_in_structures,
	.n_structures = ARRAY_SIZE(wrtd_in_structures),

	.variables = wrtd_in_variables,
	.n_variables = ARRAY_SIZE(wrtd_in_variables),

	.actions = wrtd_in_actions,
	.n_actions = ARRAY_SIZE(wrtd_in_actions),
};


static void init(void)
{
	int i;

	seq = 0;
	sent_packets = 0;
	coalesce_count = 0;
	wr_state = 0;
	tai_start = 0;

	loop_queue_init();

	wr_state = WR_LINK_OFFLINE;
	wr_enable_lock(0);

	/* Initialize the TDC FIFO (channels disabled, default dead time) */
	dp_writel(0x0, BASE_DP_TDC_DIRECT + DR_REG_CHAN_ENABLE);
	dp_writel(DEFAULT_DEAD_TIME, BASE_DP_TDC_DIRECT + DR_REG_DEAD_TIME);

	/* Set up channel states to safe default values */
	memset(wrtd_in_channels, 0,
	       sizeof(struct wrtd_in_channel) * TDC_NUM_CHANNELS);
	for(i = 0; i < TDC_NUM_CHANNELS; i++) {
		wrtd_in_channels[i].n = i;
		wrtd_in_channels[i].config.mode = WRTD_TRIGGER_MODE_AUTO;
	}
}

int main(void)
{
	init();
	rt_init(&app);

	while (1) {
		do_input();
		rt_mq_action_dispatch(IN_CMD_IN);
		wr_update_link();
	}

	return 0;
}
