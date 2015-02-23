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

#include "wrtd-serializers.h"

/**
 * Internal helper to send and recevie synchronous messages to/from the TDC
 */
static inline int wrtd_in_send_and_receive_sync(struct wrtd_desc *wrtd,
						struct wrnc_msg *msg)
{
	struct wrnc_hmq *hmq;
	int err;

	hmq = wrnc_hmq_open(wrtd->wrnc, WRTD_IN_TDC_CONTROL, WRNC_HMQ_INCOMING);
	if (!hmq)
		return -1;

	/* Send the message and get answer */
        err = wrnc_hmq_send_and_receive_sync(hmq, WRTD_OUT_TDC_CONTROL, msg,
					     WRTD_DEFAULT_TIMEOUT);
	wrnc_hmq_close(hmq);

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
	struct wrnc_msg msg = wrnc_msg_init(16);
	int err;
	uint32_t id, seq = 0;
	uint32_t dead_time_cycles;

	if (input >= TDC_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	if (state == NULL) {
		errno = ENOMEM;
		return -1;
	}

	/* Build the message */
	id = WRTD_CMD_TDC_CHAN_GET_STATE;
	wrnc_msg_header (&msg, &id, &seq);
   	wrnc_msg_uint32 (&msg, &input);


	/* Send the message and get answer */
	err = wrtd_in_send_and_receive_sync(wrtd, &msg);
        if (err) {
		errno = EWRTD_INVALID_ANSWER_STATE;
		return -1;
	}

	/* Deserialize and check the answer */
	wrnc_msg_header(&msg, &id, &seq);

	if(id != WRTD_REP_STATE)
	{
		errno = EWRTD_INVALID_ANSWER_STATE;
		return -1;
	}

	wrnc_msg_int32		(&msg, &state->input);
	wrtd_msg_trig_id   	(&msg, &state->assigned_id);
	wrtd_msg_timestamp 	(&msg, &state->delay);
	wrtd_msg_timestamp 	(&msg, &state->tdc_timebase_offset);
	wrtd_msg_timestamp 	(&msg, &state->last_tagged_pulse);
	wrnc_msg_uint32    	(&msg, &state->flags);
	wrnc_msg_uint32    	(&msg, &state->log_level);
	wrnc_msg_int32 	   	(&msg, (int *) &state->mode);
	wrnc_msg_uint32 	(&msg, &state->tagged_pulses);
	wrnc_msg_uint32 	(&msg, &state->sent_triggers);
	wrnc_msg_uint32 	(&msg, &dead_time_cycles);
	wrtd_msg_trigger_entry 	(&msg, &state->last_sent);
	wrnc_msg_uint32 	(&msg, &state->sent_packets);

	state->dead_time.seconds = 0;
	state->dead_time.frac = 0;
	state->dead_time.ticks = dead_time_cycles * 2;

	/* Check for deserialization errors (buffer underflow/overflow) */
	if ( wrnc_msg_check_error(&msg) ) {
		errno = EWRTD_INVALID_ANSWER_STATE;
		return -1;
	}

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

	if (input >= TDC_NUM_CHANNELS) {
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

	if (input >= TDC_NUM_CHANNELS) {
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

	if (input >= TDC_NUM_CHANNELS) {
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

	if (input >= TDC_NUM_CHANNELS) {
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

	if (input >= TDC_NUM_CHANNELS) {
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
	msg.data[5] = t.frac;

	/* Send the message and get answer */
	err = wrtd_in_send_and_receive_sync(wrtd, &msg);
	if (err)
		return err;

	return wrtd_validate_acknowledge(&msg);
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
	msg.data[5] = t.frac;

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

	if (input >= TDC_NUM_CHANNELS) {
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

	if (input >= TDC_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	/* Build the message */
	msg.datalen = 4;
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
 * It opens the logging interface for the input device. You can provide a
 * default logging level.
 * @param[in] dev device token
 * @param[in] lvl default logging level
 * @return a HMQ token on success, NULL on error and errno is set appropriately
 */
struct wrnc_hmq *wrtd_in_log_open(struct wrtd_node *dev,
				  uint32_t lvl)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	int i, err;

	/* Set the same logging level to all channels */
	for (i = 0; i < TDC_NUM_CHANNELS; ++i) {
		err = wrtd_in_log_level_set(dev, i, lvl);
		if (err)
			return NULL;
	}

	return wrnc_hmq_open(wrtd->wrnc, WRTD_OUT_TDC_LOGGING, 0);
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
	struct wrtd_input_state state;
	int err;

	err = wrtd_in_state_get(dev, input, &state);
	if (err)
		return -1;
	*enable = !!(state.flags & WRTD_ENABLED);

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
	struct wrtd_input_state state;
	int err;

	err = wrtd_in_state_get(dev, input, &state);
	if (err)
		return -1;
	*armed = !!(state.flags & WRTD_ARMED);

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
	struct wrtd_input_state state;
	int err;

	err = wrtd_in_state_get(dev, input, &state);
	if (err)
		return -1;
	*assigned = !!(state.flags & WRTD_TRIGGER_ASSIGNED);

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
int wrtd_in_seq_counter_set(struct wrtd_node *dev, unsigned int input,
			    unsigned int value)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg msg = wrnc_msg_init(4);
	uint32_t id, seq = 0;
	int err;

	if (input >= TDC_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	id = WRTD_CMD_TDC_CHAN_SET_SEQ;
	wrnc_msg_header(&msg, &id, &seq);
	wrnc_msg_uint32(&msg, &input);
	wrnc_msg_uint32(&msg, &value);

	/* Send the message and get answer */
	err = wrtd_in_send_and_receive_sync(wrtd, &msg);
        if (err) {
		errno = EWRTD_INVALID_ANSWER_STATE;
		return -1;
	}

	return wrtd_validate_acknowledge(&msg);
}


/**
 * It check if the input real-time application is alive
 * @param[in] dev device token
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_ping(struct wrtd_node *dev)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg msg = wrnc_msg_init(2);
	uint32_t id, seq = 0;
	int err;

	id = WRTD_CMD_TDC_PING;
	wrnc_msg_header(&msg, &id, &seq);

	/* Send the message and get answer */
	err = wrtd_in_send_and_receive_sync(wrtd, &msg);
        if (err) {
		errno = EWRTD_INVALID_ANSWER_STATE;
		return -1;
	}

	return wrtd_validate_acknowledge(&msg);
}
