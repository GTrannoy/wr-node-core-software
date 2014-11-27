/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *         inspired by a draft of Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * License: GPL v3
 */

#include <stdlib.h>
#include <errno.h>
#include <libwrnc.h>
#include "libwrtd-internal.h"


/**
 * Internal helper to send and recevie synchronous messages to/from the TDC
 */
static inline int wrtd_in_send_and_receive_sync(struct wrtd_desc *wrtd,
						struct wrnc_msg *msg)
{
	int err;

	err = wrnc_hmq_open(wrtd->wrnc, WRTD_IN_TDC_CONTROL, WRNC_HMQ_INCOMING);
	if (err)
		return err;

	/* Send the message and get answer */
        err = wrnc_slot_send_and_receive_sync(wrtd->wrnc,
					      WRTD_IN_TDC_CONTROL,
					      WRTD_OUT_TDC_CONTROL,
					      msg,
					      WRTD_DEFAULT_TIMEOUT);
	wrnc_hmq_close(wrtd->wrnc, WRTD_IN_TDC_CONTROL, WRNC_HMQ_INCOMING);

	return err;
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
	struct wrnc_msg msg;
	int err;

  	if (input >= WRTD_IN_MAX) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	if (state == NULL) {
		errno = ENOMEM;
		return -1;
	}

	/* Build the message */
	msg.datalen = 3;
	msg.data[0] = WRTD_CMD_TDC_CHAN_GET_STATE;
	msg.data[1] = 0;
	msg.data[2] = input;

	/* Send the message and get answer */
	err = wrtd_in_send_and_receive_sync(wrtd, &msg);
        if (err || msg.datalen != 29 || msg.data[0] != WRTD_REP_STATE) {
		errno = EWRTD_INVALD_ANSWER_STATE;
		return -1;
	}

	/* Copy status information */
	state->input = input;
	state->flags = msg.data[15];
	state->log_level = msg.data[16];
	state->mode = msg.data[17];
	state->tagged_pulses = msg.data[18];
	state->sent_triggers = msg.data[19];
	state->dead_time.seconds = 0;
	state->dead_time.bins = 0;
	state->dead_time.ticks = msg.data[20] * 2;

	state->assigned_id.system = msg.data[3];
	state->assigned_id.source_port = msg.data[4];
	state->assigned_id.trigger = msg.data[5];

	unbag_ts(msg.data, 6, &state->delay);
	unbag_ts(msg.data, 12, &state->last);
	unbag_ts(msg.data, 21, &state->last_sent.ts);
	state->last_sent.id.system = msg.data[24];
	state->last_sent.id.source_port = msg.data[25];
	state->last_sent.id.trigger = msg.data[26];
	state->last_sent.seq = msg.data[27];
	state->sent_packets = msg.data[28];

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
	struct wrnc_msg msg;
	int err;

	if (input >= WRTD_IN_MAX) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	/* Build the message */
	msg.datalen = 4;
	msg.data[0] = WRTD_CMD_TDC_CHAN_ENABLE;
	msg.data[1] = 0;
	msg.data[2] = input;
	msg.data[3] = !!enable;

	/* Send the message and get answer */
	err = wrtd_in_send_and_receive_sync(wrtd, &msg);
	if (err)
		return err;

	return wrtd_validate_acknowledge(&msg);
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
	struct wrnc_msg msg;
	int err;

	if (input >= WRTD_IN_MAX) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	/* Build the message */
	msg.datalen = 7;
	msg.data[0] = WRTD_CMD_TDC_CHAN_ASSIGN_TRIGGER;
	msg.data[1] = 0;
	msg.data[2] = input;
	/* '? :' should be optimized by the compiler */
	msg.data[3] = trig_id ? 1 : 0;
	msg.data[4] = trig_id ? trig_id->system : 0;
	msg.data[5] = trig_id ? trig_id->source_port : 0;
	msg.data[6] = trig_id ? trig_id->trigger : 0;

	/* Send the message and get answer */
	err = wrtd_in_send_and_receive_sync(wrtd, &msg);
	if (err)
		return err;

	return wrtd_validate_acknowledge(&msg);
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
	struct wrnc_msg msg;
	int err;

	if (input >= WRTD_IN_MAX) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	/* Build the message */
	msg.datalen = 4;
	msg.data[0] = WRTD_CMD_TDC_CHAN_SET_MODE;
	msg.data[1] = 0;
	msg.data[2] = input;
	msg.data[3] = mode;

	/* Send the message and get answer */
	err = wrtd_in_send_and_receive_sync(wrtd, &msg);
	if (err)
		return err;

	return wrtd_validate_acknowledge(&msg);
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
	struct wrnc_msg msg;
	uint32_t *buf = (uint32_t *)trigger;
	int err, i;

	if (trigger == NULL) {
		errno = EWRTD_INVALID_TRIG_ID;
		return -1;
	}

	/* Build the message */
	msg.datalen = 2 + (sizeof(struct wrtd_trigger_entry) / 4);
	msg.data[0] = WRTD_CMD_TDC_SOFTWARE_TRIGGER;
	msg.data[1] = 0;
	for (i = 2; i < msg.datalen; ++i)
		msg.data[i] = buf[i - 2];

	/* Send the message and get answer */
	err = wrtd_in_send_and_receive_sync(wrtd, &msg);
	if (err)
		return err;

	return wrtd_validate_acknowledge(&msg);
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
	struct wrnc_msg msg;
	int err;

	if (input >= WRTD_IN_MAX) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	/* Build the message */
	msg.datalen = 4;
	msg.data[0] = WRTD_CMD_TDC_CHAN_ARM;
	msg.data[1] = 0;
	msg.data[2] = input;
	msg.data[3] = !!armed;

	/* Send the message and get answer */
	err = wrtd_in_send_and_receive_sync(wrtd, &msg);
	if (err)
		return err;

	return wrtd_validate_acknowledge(&msg);
}


/**
 * Disarm the WRTD input. It is just an helper that internally use wrtd_in_arm()
 * @param[in] dev device token
 * @param[in] input index (0-based) of input channel
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_disarm(struct wrtd_node *dev, unsigned int input)
{
	return wrtd_in_arm(dev, input, 0);
}


/**
 * Set the dead time (the minimum gap between input pulses, below which
 * the TDC ignores the subsequent pulses; limits maximum input pulse rate,
 * 16 ns granularity)
 * @param[in] dev device token
 * @param[in] input index (0-based) of input channel
 * @param[in] dead_time_ps dead time in pico-seconds
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_dead_time_set(struct wrtd_node *dev, unsigned int input,
				 uint64_t dead_time_ps)
{
  	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg msg;
	int err, dead_time_cycles;

	if (input >= WRTD_IN_MAX) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	/* Convert dead-times in cycles/ticks */
	dead_time_cycles = dead_time_ps / 16000;
	if(dead_time_cycles < 5000 || dead_time_cycles > 10000000 ) {
		errno = EWRTD_INVALD_DEAD_TIME;
		return -1;
	}

	/* Build the message */
	msg.datalen = 4;
	msg.data[0] = WRTD_CMD_TDC_CHAN_SET_DEAD_TIME;
	msg.data[1] = 0;
	msg.data[2] = input;
	msg.data[3] = dead_time_cycles;

	/* Send the message and get answer */
	err = wrtd_in_send_and_receive_sync(wrtd, &msg);
	if (err)
		return err;

	return wrtd_validate_acknowledge(&msg);
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
	struct wr_timestamp t;
	struct wrnc_msg msg;
	int err;

	if (input >= WRTD_IN_MAX) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

        t = picos_to_ts(delay_ps);

	/* Build the message */
	msg.datalen = 6;
	msg.data[0] = WRTD_CMD_TDC_CHAN_SET_DELAY;
	msg.data[1] = 0;
	msg.data[2] = input;
	msg.data[3] = t.seconds;
	msg.data[4] = t.ticks;
	msg.data[5] = t.bins;

	/* Send the message and get answer */
	err = wrtd_in_send_and_receive_sync(wrtd, &msg);
	if (err)
		return err;

	return wrtd_validate_acknowledge(&msg);
}


/**
 * Set the time offset on a given input channel
 * @param[in] dev device token
 * @param[in] input index (0-based) of input channel
 * @param[in] offset time offset in pico seconds
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_timebase_offset_set(struct wrtd_node *dev, unsigned int input,
				uint64_t offset)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wr_timestamp t;
	struct wrnc_msg msg;
	int err;

  	if (input >= WRTD_OUT_MAX) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	t = picos_to_ts(offset);

	/* Build the message */
	msg.datalen = 6;
	msg.data[0] = WRTD_CMD_TDC_CHAN_SET_TIMEBASE_OFFSET;
	msg.data[1] = 0;
	msg.data[2] = input;
	msg.data[3] = t.seconds;
	msg.data[4] = t.ticks;
	msg.data[5] = t.bins;

	/* Send the message and get answer */
	err = wrtd_in_send_and_receive_sync(wrtd, &msg);
        if (err)
		return err;

	return wrtd_validate_acknowledge(&msg);
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
	struct wrnc_msg msg;
	int err;

	if (input >= WRTD_IN_MAX) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	/* Build the message */
	msg.datalen = 3;
	msg.data[0] = WRTD_CMD_TDC_CHAN_RESET_COUNTERS;
	msg.data[1] = 0;
	msg.data[2] = input;

	/* Send the message and get answer */
	err = wrtd_in_send_and_receive_sync(wrtd, &msg);
	if (err)
		return err;

	return wrtd_validate_acknowledge(&msg);
}


/**
 * Set the log level of a given input channel
 * @param[in] dev device token
 * @param[in] input index (0-based) of input channel
 * @param[in] log_level log level to apply to the logging messages
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_log_level_set(struct wrtd_node *dev, unsigned int input,
			  uint32_t log_level)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg msg;
	int err;

  	if (input >= WRTD_IN_MAX) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	/* Build the message */
	msg.datalen = 3;
	msg.data[0] = WRTD_CMD_TDC_CHAN_SET_LOG_LEVEL;
	msg.data[1] = 0;
	msg.data[2] = input;
	msg.data[3] = log_level;

	/* Send the message and get answer */
	err = wrtd_in_send_and_receive_sync(wrtd, &msg);
        if (err)
		return err;

	return wrtd_validate_acknowledge(&msg);
}


/**
 * Log every trigger pulse sent out to the network. Each log message contains
 * the input number, sequence ID, trigger ID, trigger counter (since arm) and
 * origin timestamp.
 * @param[in] dev device token
 * @param[out] log log message
 * @param[in] flags
 * @param[in] input_mask bit mask of channel where read
 * @param[in] count number of messages to read
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_read_log(struct wrtd_node *dev, struct wrtd_log_entry *log,
		     int flags, int input_mask, int count)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	int remaining = count;
	int n_read = 0;
	struct wrtd_log_entry *cur = log;
	struct wrnc_msg *msg;

	while (remaining) {
		msg = wrnc_slot_receive(wrtd->wrnc, WRTD_OUT_FD_LOGGING);
		if (!msg)
			break;

		cur->type = msg->data[0];
		cur->channel = msg->data[2];


		if ((cur->type & flags) && (cur->channel & input_mask)) {
			cur->seq = msg->data[1];
    			cur->id.system = msg->data[3];
    			cur->id.source_port = msg->data[4];
    			cur->id.trigger = msg->data[5];
    			cur->ts.seconds = msg->data[6];
    			cur->ts.ticks = msg->data[7];
    			cur->ts.bins = msg->data[8];

    			remaining--;
			n_read++;
			cur++;
		}
		free(msg);
	}

	return n_read;
}



/**
 * Check the enable status on a trigger input.
 * @param[in] dev device token
 * @param[in] input index (0-based) of input channel
 * @param[in] enable 1 enables the input, 0 disables it.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_is_enabled(struct wrtd_node *dev, unsigned int input)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}


/**
 * Get the dead time (the minimum gap between input pulses, below which
 * the TDC ignores the subsequent pulses; limits maximum input pulse rate,
 * 16 ns granularity)
 * @param[in] dev device token
 * @param[in] input index (0-based) of input channel
 * @param[out] dead_time_ps dead time in pico-seconds
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_dead_time_get(struct wrtd_node *dev, unsigned int input,
			  uint64_t *dead_time_ps)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}


/**
 * Get the offset (for compensating cable delays), in 10 ps steps.
 * @param[in] dev device token
 * @param[in] input index (0-based) of input channel
 * @param[out] delay_ps delay in pico-seconds
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_delay_get(struct wrtd_node *dev, unsigned int input,
		      uint64_t *delay_ps)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}


/**
 * Get/set the Sequence ID counter (counting up at every pulse)
 * @param[in] dev device token
 * @param[in] input index (0-based) of input channel
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_seq_counter_set(struct wrtd_node *dev, unsigned int input)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}
