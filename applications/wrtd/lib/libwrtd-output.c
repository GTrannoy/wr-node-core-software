/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *         inspired by a draft of Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * License: GPL v3
 */

#include <stdlib.h>
#include <errno.h>
#include <libwrnc.h>
#include <libwrtd-internal.h>

/**
 * Internal helper to send and receive synchronous messages to/from the WRNC
 */
static inline int wrtd_out_send_and_receive_sync(struct wrtd_desc *wrtd,
						struct wrnc_msg *msg)
{
	/* Send the message and get answer */
	struct wrnc_hmq *hmq;
	int err;

	hmq = wrnc_hmq_open(wrtd->wrnc, WRTD_IN_FD_CONTROL, WRNC_HMQ_INCOMING);
	if (!hmq)
		return -1;

	err = wrnc_hmq_send_and_receive_sync(hmq, WRTD_OUT_FD_CONTROL, msg,
					     WRTD_DEFAULT_TIMEOUT);

	wrnc_hmq_close(hmq);

	return err;
}

/**
 * It retreive a list of assigned trigger to an output channel
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 */
static int wrtd_out_trig_get(struct wrtd_node *dev, unsigned int output,
			     unsigned int bucket, unsigned int index,
			     struct wrtd_output_trigger_state *trigger)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg msg;
	uint32_t state;
	int err;

  	if (output >= WRTD_OUT_MAX) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	/* Build the message */
	msg.datalen = 5;
	msg.data[0] = WRTD_CMD_FD_READ_HASH;
	msg.data[1] = 0;
	msg.data[2] = bucket;
	msg.data[3] = index;
	msg.data[4] = output;

	/* Send the message and get answer */
	err = wrtd_out_send_and_receive_sync(wrtd, &msg);
	if (err)
		return err;

	/* Valdiate the answer */
	if (msg.datalen != 17 || msg.data[0] != WRTD_REP_HASH_ENTRY) {
		errno = EWRTD_INVALD_ANSWER_HASH;
		return -1;
	}
	/* Check if the message content is valid */
	if (!msg.data[2]) {
		errno = EWRTD_INVALD_ANSWER_HASH_CONT;
		return -1;
	}

	state = msg.data[8];
	if (state == HASH_ENT_EMPTY || (state & HASH_ENT_CONDITIONAL))
		return 0;

	trigger->handle.channel = output;
	trigger->handle.ptr_cond = msg.data[9];
	trigger->handle.ptr_trig = msg.data[10];

	trigger->trigger.system = msg.data[3];
	trigger->trigger.source_port = msg.data[4];
	trigger->trigger.trigger = msg.data[5];
	trigger->delay_trig.seconds = 0;
	trigger->delay_trig.ticks = msg.data[6];
	trigger->delay_trig.frac = msg.data[7];
	trigger->is_conditional = 0;
	trigger->worst_latency_us = (msg.data[17] + 124) / 125;

	if(msg.data[9]) { /* condition assigned? */
		trigger->is_conditional = 1;
		trigger->condition.system = msg.data[11];
		trigger->condition.source_port = msg.data[12];
		trigger->condition.trigger = msg.data[13];
		trigger->delay_cond.seconds = 0;
		trigger->delay_cond.ticks = msg.data[14];
		trigger->delay_cond.frac = msg.data[15];
	}

	trigger->enabled = (state & HASH_ENT_DISABLED) ? 0 : 1;


	return 0;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * PROTOTYPEs IMPLEMENTATION * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * It retreives the current output status of a given channel
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 * @param[out] state channel status
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_state_get(struct wrtd_node *dev, unsigned int output,
			 struct wrtd_output_state *state)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg msg;
	int err;

  	if (output >= WRTD_OUT_MAX) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	if (state == NULL) {
		errno = ENOMEM;
		return -1;
	}

	/* Build the message */
	msg.datalen = 3;
	msg.data[0] = WRTD_CMD_FD_CHAN_GET_STATE;
	msg.data[1] = 0;
	msg.data[2] = output;

	/* Send the message and get answer */
	err = wrtd_out_send_and_receive_sync(wrtd, &msg);
        if (err || msg.datalen != 28 || msg.data[0] != WRTD_REP_STATE) {
		errno = EWRTD_INVALD_ANSWER_STATE;
		return -1;
	}

	/* Copy status information */
	state->output = output;
	state->executed_pulses = msg.data[3];
	state->missed_pulses_late = msg.data[4];
	state->missed_pulses_deadtime = msg.data[5];
	state->missed_pulses_overflow = msg.data[6];

	state->rx_packets = msg.data[27];
	state->rx_loopback = msg.data[28];

	unbag_ts(msg.data, 10, &state->last_executed.ts);
	unbag_ts(msg.data, 13, &state->last_enqueued.ts);
	unbag_ts(msg.data, 16, &state->last_programmed.ts);

	return 0;
}


/**
 * It enables/disables a trigger output line
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 * @param[in] enable 1 to enable the output, 0 disables it.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_enable(struct wrtd_node *dev, unsigned int output,
		    int enable)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg msg;
	int err;

	if (output >= WRTD_OUT_MAX) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	/* Build the message */
	msg.datalen = 4;
	msg.data[0] = WRTD_CMD_FD_CHAN_ENABLE;
	msg.data[1] = 0;
	msg.data[2] = output;
	msg.data[3] = !!enable;

	/* Send the message and get answer */
	err = wrtd_out_send_and_receive_sync(wrtd, &msg);
	if (err)
		return err;

	return wrtd_validate_acknowledge(&msg);
}


/**
 * It assign a trigger to an output channel
 * @param[in] dev pointer to open node device.
 * @param[in] output index (0-based) of output channel
 * @param[out] handle
 * @param[in] trig trigger id to assign
 * @param[in] condition trigger id to assign to the condition
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_trig_assign(struct wrtd_node *dev, int output,
			 struct wrtd_trigger_handle *handle,
			 struct wrtd_trig_id *trig,
			 struct wrtd_trig_id *condition)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg msg;
	int err;

  	if (output >= WRTD_OUT_MAX) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}
	/* Build the message */
	msg.datalen = 10;
	msg.data[0] = WRTD_CMD_FD_CHAN_ASSIGN_TRIGGER;
	msg.data[1] = 0;
	msg.data[2] = output;
	msg.data[3] = trig->system;
	msg.data[4] = trig->source_port;
	msg.data[5] = trig->trigger;
	msg.data[6] = condition ? 1 : 0;

	if (condition) {
		msg.data[7] = condition->system;
		msg.data[8] = condition->source_port;
		msg.data[9] = condition->trigger;
	}

	/* Send the message and get answer */
	err = wrtd_out_send_and_receive_sync(wrtd, &msg);
        if (err)
		return err;

	if (msg.datalen != 5 || msg.data[0] != WRTD_REP_TRIGGER_HANDLE) {
		errno = EWRTD_INVALD_ANSWER_TRIG;
		return -1;
	}

	if (handle) {
		handle->channel = msg.data[2];
		handle->ptr_cond = msg.data[3];
		handle->ptr_trig = msg.data[4];
	}

	return 0;
}


/**
 * Un-assign a give trigger
 * @param[in] dev pointer to open node device.
 * @param[in] handle
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_trig_unassign(struct wrtd_node *dev,
			   struct wrtd_trigger_handle *handle)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg msg;
	int err;

	/* Build the message */
	msg.datalen = 5;
	msg.data[0] = WRTD_CMD_FD_CHAN_REMOVE_TRIGGER;
	msg.data[1] = 0;
	msg.data[2] = handle->channel;
	msg.data[3] = handle->ptr_cond;
	msg.data[4] = handle->ptr_trig;

	/* Send the message and get answer */
	err = wrtd_out_send_and_receive_sync(wrtd, &msg);
        if (err)
		return err;

	return wrtd_validate_acknowledge(&msg);
}


/**
 * It retreive a given number of triggers from output device
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 * @param[out] triggers list of assigned trigger
 * @param[in] max_count maximum triggers to retreive
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_trig_get_all(struct wrtd_node *dev, unsigned int output,
			  struct wrtd_output_trigger_state *triggers,
			  int max_count)
{
	int err, bucket, count = 0, index;

  	if (output >= WRTD_OUT_MAX) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	for (bucket = 0; bucket < FD_HASH_ENTRIES; bucket++) {
		index = 0;
		while (count < max_count) {
			err = wrtd_out_trig_get(dev, output, bucket, index,
						&triggers[count]);
			if (err && errno == EWRTD_INVALD_ANSWER_HASH_CONT)
				break;
			if (err)
				return -1;
			count++;
			index++;
		}
	}

	return count;
}


/**
 * It returns a trigget from a given index
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 * @param[out] trigger trigger status
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_trig_get_by_index(struct wrtd_node *dev, unsigned int index,
			       unsigned int output,
			       struct wrtd_output_trigger_state *trigger)
{
	struct wrtd_output_trigger_state triggers[256];
	int ret;

	ret = wrtd_out_trig_get_all(dev, output, triggers, 256);
	if (ret < 0 || index >= ret) {
		return -1;
	}

	*trigger = triggers[index];

	return 0;
}


/**
 * It sets the delay to apply for a given trigger
 * @param[in] dev pointer to open node device.
 * @param[in] handle trigger where act on
 * @param[in] delay_ps delay in pico-seconds
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_trig_delay_set(struct wrtd_node *dev,
			    struct wrtd_trigger_handle *handle,
			    uint64_t delay_ps)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wr_timestamp t;
	struct wrnc_msg msg;
	int err;

	t = picos_to_ts(delay_ps);

	/* Build the message */
	msg.datalen = 6;
	msg.data[0] = WRTD_CMD_FD_CHAN_SET_DELAY;
	msg.data[1] = 0;
	msg.data[2] = handle->channel;
	msg.data[3] = handle->ptr_trig;
	msg.data[4] = t.ticks;
	msg.data[5] = t.frac;

	/* Send the message and get answer */
	err = wrtd_out_send_and_receive_sync(wrtd, &msg);
        if (err)
		return err;

	return wrtd_validate_acknowledge(&msg);
}


/**
 * It set the dead time for a given output channel. so, it applies on all
 * triggers assigned to the given output channel
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 * @param[in] dead_time_ps dead time in pico-seconds
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_dead_time_set(struct wrtd_node *dev, unsigned int output,
			   uint64_t dead_time_ps)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}


/**
 * @param[in] dev pointer to open node device.
 * @param[in] handle trigger where act on
 * @param[in] delay_ps delay in pico-seconds
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_trig_condition_delay_set(struct wrtd_node *dev,
				      struct wrtd_trigger_handle *handle,
				      uint64_t delay_ps)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}


/**
 * @param[in] dev pointer to open node device.
 * @param[in] handle trigger where act on
 * @param[out] state trigger status
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_trig_state_get(struct wrtd_node *dev,
			    struct wrtd_trigger_handle *handle,
			    struct wrtd_output_trigger_state *state)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}


/**
 * @param[in] dev pointer to open node device.
 * @param[in] handle trigger where act on
 * @param[in] enable 1 to enable, 0 to disable
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_trig_enable(struct wrtd_node *dev,
			 struct wrtd_trigger_handle *handle, int enable)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
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
int wrtd_out_read_log(struct wrtd_node *dev, struct wrtd_log_entry *log,
		      int flags, unsigned int output_mask, int count)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}

/**
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 * @param[in] log_level log level to apply to the logging messages
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_log_level_set(struct wrtd_node *dev, unsigned int output,
			   uint32_t log_level)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}

/**
 * It sets the trigger mode of a given output channel
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 * @param[in] mode output mode
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_trigger_mode_set(struct wrtd_node *dev,
			      unsigned int output, int mode)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}


/**
 * It arms (un-arms) a given output channel
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 * @param[in] armed 1 to arm, 0 to un-arm
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_arm(struct wrtd_node *dev, unsigned int ouput, int armed)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}


/**
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_reset_counters_reset(struct wrtd_node *dev, unsigned int output)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}


/**
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_check_triggered(struct wrtd_node *dev, unsigned int output)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}
//int wrtd_out_wait_trigger(struct wrtd_node*, int output_mask, struct wrtd_trig_id *id);
