/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *         inspired by a draft of Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * License: GPL v3
 */

#include <stdlib.h>
#include <errno.h>
#include <libmockturtle.h>
#include "libwrtd-internal.h"

static const struct trtl_proto_header wrtd_in_hdr_sync = {
	.slot_io = (WRTD_IN_TDC_CONTROL << 4) |	(WRTD_OUT_TDC_CONTROL & 0xF),
	.flags = TRTL_PROTO_FLAG_SYNC,
};

/**
 * Internal helper to send and recevie synchronous messages to/from the TDC
 */
static inline int wrtd_in_send_and_receive_sync(struct wrtd_desc *wrtd,
						struct trtl_msg *msg)
{
	struct trtl_hmq *hmq;
	int err;

	hmq = trtl_hmq_open(wrtd->trtl, WRTD_IN_TDC_CONTROL, TRTL_HMQ_INCOMING);
	if (!hmq)
		return -1;

	/* Send the message and get answer */
        err = trtl_hmq_send_and_receive_sync(hmq, WRTD_OUT_TDC_CONTROL, msg,
					     WRTD_DEFAULT_TIMEOUT);
	trtl_hmq_close(hmq);

	return err < 0 ? err : 0; /* Ignore timeout */
}



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * PROTOTYPEs IMPLEMENTATION * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * FIXME
 * Most of the function's code below can be optimized by using memcpy()
 * or similar operations. For the time being, I'm leaving it like this
 * because data structures are shared with the real-time applications
 */

/**
 * It retreives the current status of a given input channel
 * @param[in] dev device token
 * @param[in] input index (0-based) of the input channel
 * @param[out] state the current status of a channel
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_state_get(struct wrtd_node *dev, unsigned int input,
		      struct wrtd_input_state *state)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	int err;
	struct wrtd_in_channel chan;
	struct trtl_structure_tlv tlv = {
		.index = IN_STRUCT_CHAN_0 + input,
		.size = sizeof(struct wrtd_in_channel),
		.structure = &chan,
	};
	uint32_t variables[] = {IN_VAR_DEVICE_SENT_PACK, 0,
				IN_VAR_DEVICE_DEAD_TIME, 0};
	struct trtl_proto_header hdr = wrtd_in_hdr_sync;

	if (input >= TDC_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	if (state == NULL) {
		errno = ENOMEM;
		return -1;
	}

	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;

	hdr.len = ARRAY_SIZE(variables);
	err = trtl_rt_variable_get(wrtd->trtl, &hdr, variables, 2);
	if (err)
		return err;

	/* Convert to state structure */
	state->input = chan.n;
	state->assigned_id = chan.config.id;
	state->delay = chan.config.delay;
	wrtd_timestamp_endianess_fix(&state->delay);
	state->tdc_timebase_offset = chan.config.timebase_offset;
	wrtd_timestamp_endianess_fix(&state->tdc_timebase_offset);
	state->last_tagged_pulse = chan.stats.last_tagged;
	wrtd_timestamp_endianess_fix(&state->last_tagged_pulse);
	state->flags = chan.config.flags;
	state->log_level = chan.config.log_level;
	state->mode = chan.config.mode;
	state->tagged_pulses = chan.stats.total_pulses;
	state->sent_triggers = chan.stats.sent_pulses;
	state->last_sent = chan.stats.last_sent;
	wrtd_timestamp_endianess_fix(&state->last_sent.ts);

	state->dead_time.seconds = 0;
	state->dead_time.frac = 0;
	state->dead_time.ticks = variables[3] * 2;
	wrtd_timestamp_endianess_fix(&state->dead_time);
	state->sent_packets = variables[1];

	return 0;
}


/**
 * Hardware enable/disable a WRTD input channel.
 * @param[in] dev pointer to open node device.
 * @param[in] input index (0-based) of the input channel
 * @param[in] enable 1 to enable the input, 0 disables it.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_enable(struct wrtd_node *dev, unsigned int input, int enable)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrtd_in_channel chan;
	struct trtl_structure_tlv tlv = {
		.index = IN_STRUCT_CHAN_0 + input,
		.size = sizeof(struct wrtd_in_channel),
		.structure = &chan,
	};
	uint32_t variables[] = {IN_VAR_DEVICE_CHAN_ENABLE, 0};
	struct trtl_proto_header hdr = wrtd_in_hdr_sync;
	int err;

	if (input >= TDC_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;

	hdr.len = ARRAY_SIZE(variables);
	err = trtl_rt_variable_get(wrtd->trtl, &hdr, variables, 1);
	if (err)
		return err;


	if (enable) {
		variables[1] |= (1 << input);
		chan.config.flags |= WRTD_ENABLED;
	} else {
		variables[1] &= ~(1 << input);
		chan.config.flags &= ~WRTD_ENABLED;
	}


	err = trtl_rt_structure_set(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;
	hdr.len = ARRAY_SIZE(variables);
	return trtl_rt_variable_set(wrtd->trtl, &hdr, variables, 1);
}


/**
 * Assign (unassign) a trigger ID to a given WRTD input. Passing a NULL trig_id
 * un-assigns the current trigger (the input will be tagging pulses and
 * logging them, but they will not be sent as triggers to the WR network).
 * @param[in] dev device token
 * @param[in] input index (0-based) of the input channel
 * @param[in] trig_id the trigger to be sent upon reception of a pulse on the
 *            given input.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_trigger_assign(struct wrtd_node *dev, unsigned int input,
			   struct wrtd_trig_id *trig_id)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrtd_in_channel chan;
	struct trtl_structure_tlv tlv = {
		.index = IN_STRUCT_CHAN_0 + input,
		.size = sizeof(struct wrtd_in_channel),
		.structure = &chan,
	};
	struct trtl_proto_header hdr = wrtd_in_hdr_sync;
	int err;

	if (input >= TDC_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;

	if (trig_id) {
		chan.config.id = *trig_id;
		chan.config.flags |= WRTD_TRIGGER_ASSIGNED;
		chan.config.flags &= ~WRTD_LAST_VALID;
	} else {
		memset(&chan.config.id, 0, sizeof(struct wrtd_trig_id));
		chan.config.flags &= ~WRTD_TRIGGER_ASSIGNED;
	}

	return trtl_rt_structure_set(wrtd->trtl, &hdr, &tlv, 1);
}


/**
 * It un-assign the trigger on an input channel. It is just an helper that
 * internally use wrtd_in_trigger_unassign()
 * @param[in] dev device token
 * @param[in] input index (0-based) of the input channel
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_trigger_unassign(struct wrtd_node *dev,
			     unsigned int input)
{
	return wrtd_in_trigger_assign(dev, input, NULL);
}


/**
 * Set trigger mode for a given WRTD input. Note that the input must be armed
 * by calling wrtd_in_arm() at least once before it can send triggers.
 *
 * The mode can be single shot or continuous. Single shot means the input will
 * trigger on the first incoming pulse and will ignore the subsequent pulses
 * until re-armed.
 *
 * @param[in] dev device token
 * @param[in] input (0-based) of the input channel
 * @param[in] mode triggering mode.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_trigger_mode_set(struct wrtd_node *dev, unsigned int input,
			     enum wrtd_trigger_mode mode)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrtd_in_channel chan;
	struct trtl_structure_tlv tlv = {
		.index = IN_STRUCT_CHAN_0 + input,
		.size = sizeof(struct wrtd_in_channel),
		.structure = &chan,
	};
	struct trtl_proto_header hdr = wrtd_in_hdr_sync;
	int err;

	if (input >= TDC_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;

	chan.config.mode = mode;

	return trtl_rt_structure_set(wrtd->trtl, &hdr, &tlv, 1);
}


/**
 * Software-trigger the input at a given TAI value
 * @param[in] dev device token
 * @param[in] trigger trigger to enumlate
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_trigger_software(struct wrtd_node *dev,
			     struct wrtd_trigger_entry *trigger)
{
   	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrtd_trigger_entry ltrig = *trigger;
	struct trtl_msg msg;
	struct trtl_proto_header hdr = {
		.msg_id = WRTD_IN_ACTION_SW_TRIG,
		.slot_io = (WRTD_IN_TDC_CONTROL << 4) |
			   (WRTD_OUT_TDC_CONTROL & 0xF),
		.flags = TRTL_PROTO_FLAG_SYNC,
		.len = sizeof(struct wrtd_trigger_entry) / 4,
	};
	int err;

	if (trigger == NULL) {
		errno = EWRTD_INVALID_TRIG_ID;
		return -1;
	}

	memset(&msg, 0, sizeof(struct trtl_msg));

	wrtd_timestamp_endianess_fix(&ltrig.ts);
	trtl_message_pack(&msg, &hdr, &ltrig);

	/* Send the message and get answer */
	err = wrtd_in_send_and_receive_sync(wrtd, &msg);
	if (err)
		return err;
	trtl_message_unpack(&msg, &hdr, NULL);
	if (hdr.msg_id != RT_ACTION_SEND_ACK)
		return -1;
	return 0;
}


/**
 * Arm (disarm) a WRTD input for triggering. By arming the input, you are making
 * it ready to accept/send triggers
 * @param[in] dev device token
 * @param[in] input index (0-based) of input channel
 * @param[in] armed 1 arms the input, 0 disarms the input.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_arm(struct wrtd_node *dev, unsigned int input, int armed)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrtd_in_channel chan;
	struct trtl_structure_tlv tlv = {
		.index = IN_STRUCT_CHAN_0 + input,
		.size = sizeof(struct wrtd_in_channel),
		.structure = &chan,
	};
	struct trtl_proto_header hdr = wrtd_in_hdr_sync;
	int err;

	if (input >= TDC_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;

	if (armed)
		chan.config.flags |= WRTD_ARMED;
	else
		chan.config.flags &= ~WRTD_ARMED;
	chan.config.flags &= ~WRTD_TRIGGERED;

	return trtl_rt_structure_set(wrtd->trtl, &hdr, &tlv, 1);
}


/**
 * Set the dead time (the minimum gap between input pulses, below which
 * the TDC ignores the subsequent pulses; limits maximum input pulse rate,
 * 16 ns granularity)
 * @param[in] dev device token
 * @param[in] input index (0-based) of input channel
 * @param[in] dead_time_ps dead time in pico-seconds [80000000, 160000000000]
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_dead_time_set(struct wrtd_node *dev, unsigned int input,
			  uint64_t dead_time_ps)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	uint32_t variables[] = {IN_VAR_DEVICE_DEAD_TIME, 0};
	struct trtl_proto_header hdr = wrtd_in_hdr_sync;
	uint32_t dead_time_cycles;

	if (input >= TDC_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	/* Convert dead-times in cycles/ticks */
	dead_time_cycles = dead_time_ps / 16000;
	if(dead_time_cycles < 5000 || dead_time_cycles > 10000000 ) {
		errno = EWRTD_INVALID_DEAD_TIME;
		return -1;
	}
	variables[1] =  dead_time_cycles; // FIXME wrong API

	return trtl_rt_variable_set(wrtd->trtl, &hdr, variables, 1);
}




/**
 * Set the offset (for compensating cable delays), in 10 ps steps.
 * @param[in] dev device token
 * @param[in] input index (0-based) of input channel
 * @param[in] delay_ps delay in pico-seconds
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_delay_set(struct wrtd_node *dev, unsigned int input,
		      uint64_t delay_ps)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrtd_in_channel chan;
	struct trtl_structure_tlv tlv = {
		.index = IN_STRUCT_CHAN_0 + input,
		.size = sizeof(struct wrtd_in_channel),
		.structure = &chan,
	};
	struct trtl_proto_header hdr = wrtd_in_hdr_sync;
	struct wr_timestamp t;
	int err;

	if (input >= TDC_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;

	wrtd_pico_to_ts(&delay_ps, &t);
	memcpy(&chan.config.delay, &t, sizeof(struct wr_timestamp));

	return trtl_rt_structure_set(wrtd->trtl, &hdr, &tlv, 1);
}


/**
 * Set the time offset on a given input channel. The time offset is between
 * the White-Rabbit timescale and the ACAM TDC timescale. This information
 * is only known by the TDC driver which has access the calibration data
 * on the TDC eeprom. So, it is necessary to inform the RealTime application
 * about this offset as soon as the RealTime application start to run.
 * @param[in] dev device token
 * @param[in] input index (0-based) of input channel
 * @param[in] offset time offset in pico seconds
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_timebase_offset_set(struct wrtd_node *dev, unsigned int input,
				uint64_t offset)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrtd_in_channel chan;
	struct trtl_structure_tlv tlv = {
		.index = IN_STRUCT_CHAN_0 + input,
		.size = sizeof(struct wrtd_in_channel),
		.structure = &chan,
	};
	struct trtl_proto_header hdr = wrtd_in_hdr_sync;
	struct wr_timestamp t;
	int err;

	if (input >= TDC_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;

	wrtd_pico_to_ts(&offset, &t);
	memcpy(&chan.config.timebase_offset, &t, sizeof(struct wr_timestamp));

	return trtl_rt_structure_set(wrtd->trtl, &hdr, &tlv, 1);
}


/**
 * Reset all counters on a given input channel
 * @param[in] dev device token
 * @param[in] input index (0-based) of input channel
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_counters_reset(struct wrtd_node *dev, unsigned int input)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrtd_in_channel chan;
	struct trtl_structure_tlv tlv = {
		.index = IN_STRUCT_CHAN_0 + input,
		.size = sizeof(struct wrtd_in_channel),
		.structure = &chan,
	};
	struct trtl_proto_header hdr = wrtd_in_hdr_sync;
	int err;

	if (input >= TDC_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;

	chan.stats.total_pulses = 0;
	chan.stats.sent_pulses = 0;
	chan.stats.miss_no_timing = 0;
	chan.config.flags &= ~WRTD_LAST_VALID;

	return trtl_rt_structure_set(wrtd->trtl, &hdr, &tlv, 1);
}


/**
 * Check the enable status on a trigger input.
 * @param[in] dev device token
 * @param[in] input index (0-based) of input channel
 * @param[out] enable 1 if it is enabled, 0 otherwise
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_is_enabled(struct wrtd_node *dev, unsigned int input,
		       unsigned int *enable)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrtd_in_channel chan;
	struct trtl_structure_tlv tlv = {
		.index = IN_STRUCT_CHAN_0 + input,
		.size = sizeof(struct wrtd_in_channel),
		.structure = &chan,
	};
	struct trtl_proto_header hdr = wrtd_in_hdr_sync;
	int err;

	if (input >= TDC_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;

	*enable = !!(chan.config.flags & WRTD_ENABLED);

	return 0;
}


/**
 * Check the armed status on a trigger input.
 * @param[in] dev device token
 * @param[in] input index (0-based) of input channel
 * @param[out] armed 1 if it is enabled, 0 otherwise
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_is_armed(struct wrtd_node *dev, unsigned int input,
		     unsigned int *armed)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrtd_in_channel chan;
	struct trtl_structure_tlv tlv = {
		.index = IN_STRUCT_CHAN_0 + input,
		.size = sizeof(struct wrtd_in_channel),
		.structure = &chan,
	};
	struct trtl_proto_header hdr = wrtd_in_hdr_sync;
	int err;

	if (input >= TDC_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;

	*armed = !!(chan.config.flags & WRTD_ARMED);

	return 0;
}

/**
 * Check the trigger assigned status on a trigger input.
 * @param[in] dev device token
 * @param[in] input index (0-based) of input channel
 * @param[out] armed 1 if it is enabled, 0 otherwise
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_has_trigger(struct wrtd_node *dev, unsigned int input,
			unsigned int *assigned)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrtd_in_channel chan;
	struct trtl_structure_tlv tlv = {
		.index = IN_STRUCT_CHAN_0 + input,
		.size = sizeof(struct wrtd_in_channel),
		.structure = &chan,
	};
	struct trtl_proto_header hdr = wrtd_in_hdr_sync;
	int err;

	if (input >= TDC_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;

	*assigned = !!(chan.config.flags & WRTD_TRIGGER_ASSIGNED);

	return 0;
}


/**
 * Get the dead time (the minimum gap between input pulses, below which
 * the TDC ignores the subsequent pulses; limits maximum input pulse rate,
 * 16 ns granularity)
 * @param[in] dev device token
 * @param[in] input index (0-based) of input channel
 * @param[out] dead_time_ps dead time in pico-seconds
 * @return 0 on success, -1 on error and errno is set appropriately
 *
 * @todo to be implemented
 */
int wrtd_in_dead_time_get(struct wrtd_node *dev, unsigned int input,
			  uint64_t *dead_time_ps)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	uint32_t variables[] = {IN_VAR_DEVICE_DEAD_TIME, 0};
	struct trtl_proto_header hdr = wrtd_in_hdr_sync;
	int err;

	if (input >= TDC_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	err = trtl_rt_variable_get(wrtd->trtl, &hdr, variables, 1);
	if (err)
		return err;

	*dead_time_ps = variables[1] * 16000;

	return 0;
}


/**
 * Get the offset (for compensating cable delays), in 10 ps steps.
 * @param[in] dev device token
 * @param[in] input index (0-based) of input channel
 * @param[out] delay_ps delay in pico-seconds
 * @return 0 on success, -1 on error and errno is set appropriately
 *
 * @todo to be implemented
 */
int wrtd_in_delay_get(struct wrtd_node *dev, unsigned int input,
		      uint64_t *delay_ps)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrtd_in_channel chan;
	struct trtl_structure_tlv tlv = {
		.index = IN_STRUCT_CHAN_0 + input,
		.size = sizeof(struct wrtd_in_channel),
		.structure = &chan,
	};
	struct trtl_proto_header hdr = wrtd_in_hdr_sync;
	int err;

	if (input >= TDC_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;

	wrtd_ts_to_pico(&chan.config.delay, delay_ps);

	return 0;
}


/**
 * Get/set the Sequence ID counter (counting up at every pulse)
 * @param[in] dev device token
 * @param[in] input index (0-based) of input channel
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_seq_counter_set(struct wrtd_node *dev, unsigned int input,
			    unsigned int value)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrtd_in_channel chan;
	struct trtl_structure_tlv tlv = {
		.index = IN_STRUCT_CHAN_0 + input,
		.size = sizeof(struct wrtd_in_channel),
		.structure = &chan,
	};
	struct trtl_proto_header hdr = wrtd_in_hdr_sync;
	int err;

	if (input >= TDC_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;

	chan.stats.seq = value;

	return trtl_rt_structure_set(wrtd->trtl, &hdr, &tlv, 1);
}


/**
 * It check if the input real-time application is alive
 * @param[in] dev device token
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_ping(struct wrtd_node *dev)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;

	return trtl_rt_ping(wrtd->trtl, WRTD_IN_TDC_CONTROL,
			    WRTD_OUT_TDC_CONTROL);
}


/**
 * It gets the input base time
 * @param[in] dev device token
 * @param[out] ts input device base time
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_base_time(struct wrtd_node *dev, struct wr_timestamp *ts)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	uint32_t variables[] = {IN_VAR_DEVICE_TIME_S, 0,
				IN_VAR_DEVICE_TIME_T, 0};
	struct trtl_proto_header hdr = wrtd_in_hdr_sync;
	int err;

	err = trtl_rt_variable_get(wrtd->trtl, &hdr, variables, 2);
	if (err)
		return err;
	ts->seconds = variables[1];
	ts->ticks = variables[3];

	return 0;
}


/**
 * It gets the output version
 * @param[in] dev device token
 * @param[out] version the RT application version
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_version(struct wrtd_node *dev, struct trtl_rt_version *version)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;

	return trtl_rt_version_get(wrtd->trtl, version,
				   WRTD_IN_TDC_CONTROL, WRTD_OUT_TDC_CONTROL);
}
