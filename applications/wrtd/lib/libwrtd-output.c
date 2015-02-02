/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *         inspired by a draft of Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * License: GPL v3
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libwrnc.h>
#include <libwrtd-internal.h>

#include "wrtd-serializers.h"

/*
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


#define WRTD_HASH_ENTRY_OK 0
#define WRTD_HASH_ENTRY_CONDITIONAL 1
#define WRTD_HASH_ENTRY_EMPTY 2

/**
 * Retreives a trigger entry specified by a position in the hash table 
 * (bucket/index number) or by its handle (if handle parameter is not null)
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 * @param[in] bucket bucket in the hash table to read
 * @param[in] index specifies which entry in the selected bucket will be read
 * @param[in] handle handle of the trigger to read (if null, takes bucket/index)
 * @param[out] trigger retrieved trigger entry
 */

static int wrtd_out_trig_get(struct wrtd_node *dev, unsigned int output,
			     unsigned int bucket, unsigned int index,
			     struct wrtd_trigger_handle *handle,
			     struct wrtd_output_trigger_state *trigger)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg msg = wrnc_msg_init (16);
	int err, is_conditional = 0, entry_ok = 0;
	uint32_t seq = 0, id ;
	uint32_t state, latency_worst, latency_avg_sum, latency_avg_nsamples, ptr = 0;

	if (output > FD_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	/* Build the message */
	id = WRTD_CMD_FD_READ_HASH;
	wrnc_msg_header (&msg, &id, &seq);
	wrnc_msg_uint32 (&msg, &bucket);
	wrnc_msg_uint32 (&msg, &index);
	wrnc_msg_uint32 (&msg, &output);

	if(handle)
		ptr = handle->ptr_cond ? handle->ptr_cond : handle->ptr_trig;

	wrnc_msg_uint32 (&msg, &ptr);

	/* Send the message and get answer */
	err = wrtd_out_send_and_receive_sync (wrtd, &msg);
	if (err) {
		errno = EWRTD_INVALID_ANSWER_STATE;
		return -1;
	}

	/* Deserialize and check the answer */
	wrnc_msg_header (&msg, &id, &seq);
	if (id != WRTD_REP_HASH_ENTRY)
	{
		errno = EWRTD_INVALID_ANSWER_STATE;
		return -1;
	}

	wrnc_msg_int32 (&msg, &entry_ok);
	wrnc_msg_int32 (&msg, &is_conditional);

	if(!entry_ok)
		return WRTD_HASH_ENTRY_EMPTY;

	trigger->is_conditional = 0;

	trigger->handle.channel = output;
	wrnc_msg_uint32 (&msg, (uint32_t *) &trigger->handle.ptr_trig);
	wrnc_msg_uint32 (&msg, (uint32_t *) &trigger->handle.ptr_cond);

	if(is_conditional)
	{
		trigger->is_conditional = 1;

		wrnc_msg_uint32 (&msg, &state);
		wrtd_msg_trig_id (&msg, &trigger->condition);
		trigger->delay_cond.ticks = 0;
		wrnc_msg_uint32 (&msg, &trigger->delay_cond.ticks);
		wrnc_msg_uint32 (&msg, &trigger->delay_cond.frac);
		/* skip the condition latency stats for the time being */
		wrnc_msg_skip(&msg, 5);
	}

	wrnc_msg_uint32 (&msg, &state);

	if (state & HASH_ENT_CONDITIONAL)
		return WRTD_HASH_ENTRY_CONDITIONAL;

	wrtd_msg_trig_id (&msg, &trigger->trigger);
	trigger->delay_trig.ticks = 0;
	wrnc_msg_uint32 (&msg, &trigger->delay_trig.ticks);
	wrnc_msg_uint32 (&msg, &trigger->delay_trig.frac);
	wrnc_msg_uint32 (&msg, &latency_worst);
	wrnc_msg_uint32 (&msg, &latency_avg_sum);
	wrnc_msg_uint32 (&msg, &latency_avg_nsamples);
	wrnc_msg_uint32 (&msg, &trigger->executed_pulses);
	wrnc_msg_uint32 (&msg, &trigger->missed_pulses);

	if(latency_avg_nsamples == 0)
		trigger->latency_average_us = 0;
	else
		trigger->latency_average_us = (latency_avg_sum / latency_avg_nsamples + 124) / 125;

	trigger->latency_worst_us = (latency_worst + 124) / 125;

	trigger->enabled = (state & HASH_ENT_DISABLED) ? 0 : 1;

	if ( wrnc_msg_check_error(&msg) ) {
		errno = EWRTD_INVALID_ANSWER_STATE;
		return -1;
	}

	return WRTD_HASH_ENTRY_OK;
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
	struct wrnc_msg msg = wrnc_msg_init (16);
	int err, dummy = 0;
	uint32_t seq = 0, id;
	uint32_t dead_time_ticks, pulse_width_ticks;

	if (output > FD_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	if (state == NULL) {
		errno = ENOMEM;
		return -1;
	}

	/* Build the message */
	id = WRTD_CMD_FD_CHAN_GET_STATE;
	wrnc_msg_header (&msg, &id, &seq);
   	wrnc_msg_uint32 (&msg, &output);

	/* Send the message and get answer */
	err = wrtd_out_send_and_receive_sync(wrtd, &msg);
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

	wrnc_msg_int32(&msg, &state->output);

	wrnc_msg_uint32(&msg, &state->executed_pulses);
	wrnc_msg_uint32(&msg, &state->missed_pulses_late);
	wrnc_msg_uint32(&msg, &state->missed_pulses_deadtime);
	wrnc_msg_uint32(&msg, &state->missed_pulses_overflow);
	wrnc_msg_uint32(&msg, &state->missed_pulses_no_timing);

	wrtd_msg_trigger_entry(&msg, &state->last_executed);
	wrtd_msg_trigger_entry(&msg, &state->last_enqueued);
	wrtd_msg_trigger_entry(&msg, &state->last_received);
	wrtd_msg_trigger_entry(&msg, &state->last_lost);

	wrnc_msg_int32(&msg, &dummy);
	wrnc_msg_int32(&msg, &dummy);
	wrnc_msg_int32(&msg, (int *) &state->mode);
	wrnc_msg_uint32(&msg, &state->flags);
	wrnc_msg_uint32(&msg, &state->log_level);
	wrnc_msg_uint32(&msg, &dead_time_ticks);
	wrnc_msg_uint32(&msg, &pulse_width_ticks);
	wrnc_msg_uint32(&msg, &state->received_messages);
	wrnc_msg_uint32(&msg, &state->received_loopback);

	/* Check for deserialization errors (buffer underflow/overflow) */
	if ( wrnc_msg_check_error(&msg) ) {
		errno = EWRTD_INVALID_ANSWER_STATE;
		return -1;
	}

	state->pulse_width.seconds = 0;
	state->pulse_width.frac = 0;
	state->pulse_width.ticks = pulse_width_ticks;

	state->dead_time.seconds = 0;
	state->dead_time.frac = 0;
	state->dead_time.ticks = dead_time_ticks;

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

	if (output > FD_NUM_CHANNELS) {
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
int wrtd_out_trig_assign(struct wrtd_node *dev, unsigned int output,
			 struct wrtd_trigger_handle *handle,
			 struct wrtd_trig_id *trig,
			 struct wrtd_trig_id *condition)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg msg = wrnc_msg_init (16);
	int err, tmp = 0;
	uint32_t seq = 0, id;
	struct wrtd_trigger_handle tmp_handle;

	if (output > FD_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	/* Build the message */
	id = WRTD_CMD_FD_CHAN_ASSIGN_TRIGGER;
	wrnc_msg_header (&msg, &id, &seq);
   	wrnc_msg_uint32 (&msg, &output);
   	wrtd_msg_trig_id (&msg, trig);

   	tmp = condition ? 1 : 0;

   	wrnc_msg_int32 (&msg, &tmp);
   	if (condition)
	   	wrtd_msg_trig_id (&msg, condition );

	/* Send the message and get answer */
	err = wrtd_out_send_and_receive_sync(wrtd, &msg);
	if (err) {
		errno = EWRTD_INVALID_ANSWER_STATE;
		return -1;
	}
	
	if (msg.data[0] != WRTD_REP_TRIGGER_HANDLE) {
		errno = EWRTD_INVALID_ANSWER_TRIG;
		return -1;
	}

	wrnc_msg_int32(&msg, &tmp_handle.channel);
	wrnc_msg_uint32(&msg, &tmp_handle.ptr_cond);
	wrnc_msg_uint32(&msg, &tmp_handle.ptr_trig);

	if ( wrnc_msg_check_error(&msg) ) {
		errno = EWRTD_INVALID_ANSWER_HANDLE;
		return -1;
	}

	if (handle)
		*handle = tmp_handle;

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
 * @return number of triggers on success, -1 on error and
 *         errno is set appropriately
 */
int wrtd_out_trig_get_all(struct wrtd_node *dev, unsigned int output,
			  struct wrtd_output_trigger_state *triggers,
			  int max_count)
{
	int status, bucket, count = 0, index;

	if (output > FD_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

#define WRTD_HASH_ENTRY_OK 0
#define WRTD_HASH_ENTRY_CONDITIONAL 1
#define WRTD_HASH_ENTRY_LAST 2

	for (bucket = 0; bucket < FD_HASH_ENTRIES; bucket++) {
		index = 0;
		while (count < max_count) {
			status = wrtd_out_trig_get(dev, output, bucket, index,
						NULL, &triggers[count]);

			if(status == WRTD_HASH_ENTRY_OK)
				count++;
			else if (status == WRTD_HASH_ENTRY_LAST)
				break;
			else if (status < 0)
				return -1;
			index++;
		}
	}

	return count;
}


/**
 * It returns a trigger state from a given handle.
 * @param[in] dev pointer to open node device.
 * @param[in] handle trigger where act on
 * @param[out] state trigger status
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_trig_state_get_by_handle(struct wrtd_node *dev,
				      struct wrtd_trigger_handle *handle,
				      struct wrtd_output_trigger_state *state)
{
	int err;

	err = wrtd_out_trig_get(dev, handle->channel, 0, 0, handle, state);
	if (err)
		return -1;

	return 0;
}


/**
 * It returns a trigget from a given index. The index may change due to trigger
 * assing and un-assing. So, before use this function you have to check the
 * current trigger's indexes. Note that this is not thread safe.
 * Whenever is possible you should prefer wrtd_out_trig_state_get_by_handle()
 * @param[in] dev device token
 * @param[in] id identifier of the trigger to retrieve
 * @param[out] trigger trigger status
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_trig_state_get_by_id(struct wrtd_node *dev,
				  struct wrtd_trig_id *id,
				  struct wrtd_output_trigger_state *trigger)
{
	struct wrtd_output_trigger_state triggers[256];
	int ret, i, k;

	for (k = 0; k < FD_NUM_CHANNELS; ++k) {
		/* Get triggers for a given channel */
		ret = wrtd_out_trig_get_all(dev, k, triggers, 256);
		if (ret < 0) {
			return -1;
		}

		/* Look for trigger ID */
		for (i = 0; i < 256; ++i) {
			if (memcmp(id, &triggers[i].trigger,
				   sizeof(struct wrtd_trig_id)) == 0) {
				*trigger = triggers[i];
				return 0;
			}
		}
	}
	errno = EWRTD_NOFOUND_TRIGGER;
	return -1;
}


/**
 * It returns a trigget from a given index. The index may change due to trigger
 * assing and un-assing. So, before use this function you have to check the
 * current trigger's indexes. Note that this is not thread safe.
 * Whenever is possible you should prefer wrtd_out_trig_state_get_by_handle()
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 * @param[out] trigger trigger status
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_trig_state_get_by_index(struct wrtd_node *dev, unsigned int index,
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
	struct wrnc_msg msg = wrnc_msg_init (16);
	int err;
	uint32_t id, seq = 0;

	if (delay_ps > (1000 * 1000 * 1000 * 1000ULL - 1000ULL))
	{
		errno = -EWRTD_INVALID_DELAY;
		return -1;

	}

	t = picos_to_ts(delay_ps);

	id = WRTD_CMD_FD_CHAN_SET_DELAY;
	wrnc_msg_header (&msg, &id, &seq);
   	wrnc_msg_int32 (&msg, &handle->channel);
	wrnc_msg_uint32 (&msg, &handle->ptr_trig);
	wrnc_msg_uint32 (&msg, &t.ticks);
	wrnc_msg_uint32 (&msg, &t.frac);

	/* Send the message and get answer */
	err = wrtd_out_send_and_receive_sync(wrtd, &msg);
        if (err)
		return err;

	return wrtd_validate_acknowledge(&msg);
}

/**
 * Sets the pulse width for a given output channel. 
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 * @param[in] width_ps pulse width in pico-seconds (from 1us to 1s)
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_pulse_width_set(struct wrtd_node *dev, unsigned int output,
			   uint64_t width_ps)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg msg = wrnc_msg_init (16);
	int err, tmp = 0;
	uint32_t seq = 0, id;
	
	if (output > FD_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	if (width_ps < 1000ULL * 250 || width_ps >= 1000ULL * 1000 * 1000 * 1000 )
	{
		//fixme : add errno
		return -1;
	}

	/* Build the message */
	id = WRTD_CMD_FD_CHAN_SET_WIDTH;
	wrnc_msg_header (&msg, &id, &seq);
   	wrnc_msg_uint32 (&msg, &output);
   	
   	tmp = width_ps / 8000ULL;

   	wrnc_msg_int32 (&msg, &tmp);

	/* Send the message and get answer */
	err = wrtd_out_send_and_receive_sync(wrtd, &msg);
	if (err) {
		errno = EWRTD_INVALID_ANSWER_STATE;
		return -1;
	}
	
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
 * @param[in] enable 1 to enable, 0 to disable
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_trig_enable(struct wrtd_node *dev,
			 struct wrtd_trigger_handle *handle, int enable)
{

	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg msg = wrnc_msg_init (16);
	uint32_t id = WRTD_CMD_FD_CHAN_ENABLE_TRIGGER, seq = 0;
	int err;

	wrnc_msg_header (&msg, &id, &seq);
	wrnc_msg_int32 (&msg, &handle->channel);
	wrnc_msg_int32 (&msg, &enable);

	if(handle->ptr_cond)
		wrnc_msg_uint32 (&msg, (uint32_t *) &handle->ptr_cond);
	else
		wrnc_msg_uint32 (&msg, (uint32_t *) &handle->ptr_trig);

	/* Send the message and get answer */
	err = wrtd_out_send_and_receive_sync(wrtd, &msg);
	if (err)
		return err;

	return wrtd_validate_acknowledge(&msg);
}


/**
 * It opens the logging interface for the output device. You can provide a
 * default logging level.
 * @todo reduce code duplication wrtd_in_log_open()
 * @param[in] dev device token
 * @param[in] lvl default logging level
 * @param[in] output channel number [-1, 3]. [-1] for all channels, [0,3] for a
 *                   specific one.
 * @return a HMQ token on success, NULL on error and errno is set appropriately
 */
struct wrnc_hmq *wrtd_out_log_open(struct wrtd_node *dev,
				   uint32_t lvl,
				   int output)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg_filter filter = {
		.operation = WRNC_MSG_FILTER_AND,
		.word_offset = 2, /* channel field */
		.mask = 0xFFFF, /* entire field */
		.value = output, /* required channel */
	};
	struct wrnc_hmq *hmq = NULL;
	int i, err;

	if (output < -1 || output >= FD_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return NULL;
	}

	if (output > -1) {
		err = wrtd_out_log_level_set(dev, output, lvl);
		if (err)
			return NULL;

		hmq = wrnc_hmq_open(wrtd->wrnc, WRTD_OUT_FD_LOGGING, 0);
		if (!hmq)
			return NULL;

		err = wrnc_hmq_filter_add(hmq, &filter);
		if (err) {
			wrnc_hmq_close(hmq);
			return NULL;
		}
	} else {
		/* Set the same logging level to all channels */
		for (i = 0; i < TDC_NUM_CHANNELS; ++i) {
			err = wrtd_out_log_level_set(dev, i, lvl);
			if (err)
				return NULL;
		}
		hmq = wrnc_hmq_open(wrtd->wrnc, WRTD_OUT_FD_LOGGING, 0);
	}

	return hmq;
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
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg msg = wrnc_msg_init(4);
	uint32_t seq = 0, id;
	int err;

	if (output > FD_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	/* Build the message */
	id = WRTD_CMD_FD_CHAN_SET_LOG_LEVEL;
	wrnc_msg_header(&msg, &id, &seq);
	wrnc_msg_uint32(&msg, &output);
	wrnc_msg_uint32(&msg, &log_level);

	/* Send the message and get answer */
	err = wrtd_out_send_and_receive_sync(wrtd, &msg);
        if (err)
		return err;

	return wrtd_validate_acknowledge(&msg);
}

/**
 * It sets the trigger mode of a given output channel
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 * @param[in] mode output mode
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_trigger_mode_set(struct wrtd_node *dev,
			      unsigned int output,
			      enum wrtd_trigger_mode mode)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg msg  = wrnc_msg_init(4);;
	uint32_t seq = 0, id;
	int err;

	if (output > FD_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	/* Build the message */
	id = WRTD_CMD_FD_CHAN_SET_MODE;
	wrnc_msg_header(&msg, &id, &seq);
	wrnc_msg_uint32(&msg, &output);
	wrnc_msg_uint32(&msg, &mode);

	/* Send the message and get answer */
	err = wrtd_out_send_and_receive_sync(wrtd, &msg);
	if (err)
		return err;

	return wrtd_validate_acknowledge(&msg);
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
int wrtd_out_counters_reset(struct wrtd_node *dev, unsigned int output)
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

/**
 * Check the enable status on a trigger output.
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 * @param[out] enable 1 if it is enabled, 0 otherwise
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_is_enabled(struct wrtd_node *dev, unsigned int output,
			unsigned int *enable)
{
	struct wrtd_input_state state;
	int err;

	err = wrtd_in_state_get(dev, output, &state);
	if (err)
		return -1;
	*enable = !!(state.flags & WRTD_ENABLED);

	return 0;
}


/**
 * Check the armed status on a trigger output.
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 * @param[out] armed 1 if it is enabled, 0 otherwise
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_is_armed(struct wrtd_node *dev, unsigned int output,
		      unsigned int *armed)
{
	struct wrtd_output_state state;
	int err;

	err = wrtd_out_state_get(dev, output, &state);
	if (err)
		return -1;
	*armed = !!(state.flags & WRTD_ARMED);

	return 0;
}

/**
 * Check the trigger assigned status on a trigger output.
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 * @param[out] armed 1 if it is enabled, 0 otherwise
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_has_trigger(struct wrtd_node *dev, unsigned int output,
			 unsigned int *assigned)
{
	struct wrtd_output_state state;
	int err;

	err = wrtd_out_state_get(dev, output, &state);
	if (err)
		return -1;
	*assigned = !!(state.flags & WRTD_TRIGGER_ASSIGNED);

	return 0;
}


/**
 * It check if the input real-time application is alive
 * @param[in] dev device token
 * @return 0 on success, -1 on error and errno is set appropriately
 */
#if 0
int wrtd_out_ping(struct wrtd_node *dev)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg msg = wrnc_msg_init(2);
	uint32_t id, seq = 0;
	int err;

	id = WRTD_CMD_FD_PING;
	wrnc_msg_header(&msg, &id, &seq);

	/* Send the message and get answer */
	err = wrtd_in_send_and_receive_sync(wrtd, &msg);
        if (err) {
		errno = EWRTD_INVALID_ANSWER_STATE;
		return -1;
	}

	return wrtd_validate_acknowledge(&msg);
}
#else
int wrtd_out_ping(struct wrtd_node *dev)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}
#endif
