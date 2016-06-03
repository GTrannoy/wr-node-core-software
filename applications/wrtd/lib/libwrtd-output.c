/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *         inspired by a draft of Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * License: GPL v3
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libmockturtle.h>
#include <libwrtd-internal.h>


/* Basic header for synchronous messages */
static const struct trtl_proto_header hdr_base_sync = {
	.slot_io = (WRTD_IN_FD_CONTROL << 4) |(WRTD_OUT_FD_CONTROL & 0xF),
	.flags = TRTL_PROTO_FLAG_SYNC,
};

/*
 * Internal helper to send and receive synchronous messages to/from the WRNC
 */
static inline int wrtd_out_send_and_receive_sync(struct wrtd_desc *wrtd,
						struct trtl_msg *msg)
{
	/* Send the message and get answer */
	struct trtl_hmq *hmq;
	int err;

	hmq = trtl_hmq_open(wrtd->trtl, WRTD_IN_FD_CONTROL, TRTL_HMQ_INCOMING);
	if (!hmq)
		return -1;

	err = trtl_hmq_send_and_receive_sync(hmq, WRTD_OUT_FD_CONTROL, msg,
					     WRTD_DEFAULT_TIMEOUT);

	trtl_hmq_close(hmq);

	return err < 0 ? err : 0; /* Ignore timeout */
}


/**
 * It retrieves the trigger index where you can write.
 * If the trigger with ID 'tid' already exists it returns its index,
 * otherwise it return the first free index.
 */
static int wrtd_out_trigger_index_get(struct wrtd_desc *wrtd,
				      struct wrtd_trig_id *tid)
{
	struct trtl_proto_header hdr = {
		.msg_id = WRTD_OUT_ACTION_TRIG_IDX,
		.slot_io = (WRTD_IN_FD_CONTROL << 4) |
			   (WRTD_OUT_FD_CONTROL & 0xF),
		.flags = TRTL_PROTO_FLAG_SYNC,
		.len = sizeof(struct wrtd_trig_id) / 4,
	};
	struct trtl_msg msg;
	void *data;
	int err;

	memset(&msg, 0, sizeof(struct trtl_msg));
	data = &msg.data[sizeof(struct trtl_proto_header) / 4];
	memcpy(data, tid, sizeof(struct wrtd_trig_id));
	trtl_message_header_set(&msg, &hdr);
	msg.datalen = sizeof(struct trtl_proto_header) / 4 + hdr.len;
	err = wrtd_out_send_and_receive_sync(wrtd, &msg);
	if (err)
		return -1;
	trtl_message_header_get(&msg, &hdr);
	if (hdr.len != 1 || hdr.msg_id == RT_ACTION_SEND_NACK) {
		errno = EWRTD_NOFOUND_TRIGGER;
		return -1;
	}
	if (hdr.msg_id != WRTD_OUT_ACTION_TRIG_IDX &&
	    hdr.msg_id != WRTD_OUT_ACTION_TRIG_FRE) {
		errno = ETRTL_INVALID_MESSAGE;
		return -1;
	}

	/* return the trigger index */
	return msg.data[sizeof(struct trtl_proto_header) / 4];
}

static int wrtd_out_action_one_word(struct wrtd_desc *wrtd,
				    uint8_t msgid, uint32_t word)
{
	struct trtl_proto_header hdr = {
		.msg_id = msgid,
		.slot_io = (WRTD_IN_FD_CONTROL << 4) |
			   (WRTD_OUT_FD_CONTROL & 0xF),
		.flags = TRTL_PROTO_FLAG_SYNC,
		.len = 1,
	};
	struct trtl_msg msg;
	uint32_t *data;
	int err;

	memset(&msg, 0, sizeof(struct trtl_msg));
	data = &msg.data[sizeof(struct trtl_proto_header) / 4];
	data[0] = word;
	trtl_message_header_set(&msg, &hdr);
	msg.datalen = sizeof(struct trtl_proto_header) / 4 + hdr.len;
	err = wrtd_out_send_and_receive_sync(wrtd, &msg);
	if (err)
		return -1;
	trtl_message_header_get(&msg, &hdr);

	if (hdr.msg_id != RT_ACTION_SEND_ACK) {
		errno = EWRTD_INVALID_ANSWER_ACK;
		return -1;
	}

	return 0;
}

static inline int wrtd_out_trigger_insert(struct wrtd_desc *wrtd, uint32_t tid)
{
	return wrtd_out_action_one_word(wrtd, WRTD_OUT_ACTION_TRIG_ADD, tid);
}

static inline int wrtd_out_trigger_remove(struct wrtd_desc *wrtd, uint32_t tid)
{
	return wrtd_out_action_one_word(wrtd, WRTD_OUT_ACTION_TRIG_DEL, tid);
}


static inline int wrtd_out_rt_disable(struct wrtd_desc *wrtd,
				      unsigned int output)
{
	return wrtd_out_action_one_word(wrtd, WRTD_OUT_ACTION_DISABLE, output);
}

static struct trtl_structure_tlv wrtd_trigger_tlv(uint32_t index,
						  struct wrtd_out_trigger *trig)
{
	struct trtl_structure_tlv tlv = {
		.index = __WRTD_OUT_STRUCT_MAX + index,
		.size = sizeof(struct wrtd_out_trigger),
		.structure = trig,
	};

	return tlv;
}
static struct trtl_structure_tlv wrtd_channel_tlv(uint32_t index,
						  struct wrtd_out_channel *chan)
{
	struct trtl_structure_tlv tlv = {
		.index = OUT_STRUCT_CHAN_0 + index,
		.size = WRTD_OUT_CHANNEL_PUBLIC_SIZE,
		.structure = chan,
	};

	return tlv;
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
	struct wrtd_out out;
	struct wrtd_out_channel chan;
	struct trtl_structure_tlv tlv = wrtd_channel_tlv(output, &chan);
	struct trtl_proto_header hdr = hdr_base_sync;
	int err;

	if (output >= FD_NUM_CHANNELS) {
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

	/* Copy to state structure */
	state->output = chan.n;
	state->executed_pulses = chan.stats.hits;
	state->missed_pulses_late = chan.stats.miss_timeout;
	state->missed_pulses_deadtime = chan.stats.miss_deadtime;
	state->missed_pulses_overflow = chan.stats.miss_overflow;
	state->missed_pulses_no_timing = chan.stats.miss_no_timing;

	state->last_executed = chan.stats.last_executed;
	state->last_enqueued = chan.stats.last_enqueued;
	state->last_lost = chan.stats.last_lost;
	wrtd_timestamp_endianess_fix(&state->last_executed.ts);
	wrtd_timestamp_endianess_fix(&state->last_enqueued.ts);
	wrtd_timestamp_endianess_fix(&state->last_lost.ts);

	state->mode = chan.config.mode;
	state->flags = chan.config.flags;
	state->log_level = chan.config.log_level;

	state->pulse_width.seconds = 0;
	state->pulse_width.frac = 0;
	state->pulse_width.ticks = chan.config.width_cycles;

	state->dead_time.seconds = 0;
	state->dead_time.frac = 0;
	state->dead_time.ticks = chan.config.dead_time;

	hdr.len = 0; /* reset len */
	tlv.index = OUT_STRUCT_DEVICE;
	tlv.size = sizeof(struct wrtd_out);
	tlv.structure = &out;
	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;
	state->received_messages = out.counter_etherbone;
	state->received_loopback = out.counter_loopback;
	state->last_received = out.last_received;
	wrtd_timestamp_endianess_fix(&state->last_received.ts);

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
	struct wrtd_out_channel chan;
	struct trtl_structure_tlv tlv = wrtd_channel_tlv(output, &chan);
	struct trtl_proto_header hdr = hdr_base_sync;
	int err;

	if (output >= FD_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;

	if (enable) {
		chan.config.flags |= WRTD_ENABLED;
	} else {
		chan.config.flags &= ~(WRTD_ENABLED | WRTD_ARMED |
				       WRTD_TRIGGERED | WRTD_LAST_VALID);
		chan.config.state = OUT_ST_IDLE;

		err = wrtd_out_rt_disable(wrtd, output);
		if (err)
			return -1;
	}

	return trtl_rt_structure_set(wrtd->trtl, &hdr, &tlv, 1);
}


static int wrtd_out_trig_assign_condition_by_index(struct wrtd_node *dev,
						   unsigned int output,
						   uint32_t trig_idx,
						   uint32_t cond_idx)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrtd_out_trigger trig;
	struct trtl_structure_tlv tlv;
	struct trtl_proto_header hdr = hdr_base_sync;
	int err;

	if (output >= FD_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	tlv = wrtd_trigger_tlv(trig_idx, &trig);
	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;
	wrtd_output_rule_endianess_fix(&trig.ocfg[output]);

	if (!(trig.flags & ENTRY_FLAG_VALID)) {
		/* If the given trigger is not valid, then skip */
		errno = EINVAL;
		return -1;
	}

	trig.ocfg[output].state &= ~HASH_ENT_DIRECT;
	trig.ocfg[output].state |= HASH_ENT_CONDITIONAL;
	trig.ocfg[output].cond_ptr = cond_idx;

	wrtd_output_rule_endianess_fix(&trig.ocfg[output]);
	err = trtl_rt_structure_set(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;

	tlv = wrtd_trigger_tlv(cond_idx, &trig);
	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;
	wrtd_output_rule_endianess_fix(&trig.ocfg[output]);

	if (!(trig.flags & ENTRY_FLAG_VALID)) {
		/* If the given trigger is not valid, then skip */
		errno = EINVAL;
		return -1;
	}

	trig.ocfg[output].state &= ~HASH_ENT_DIRECT;
	trig.ocfg[output].state |= HASH_ENT_CONDITION;
	trig.ocfg[output].cond_ptr = -1;

	wrtd_output_rule_endianess_fix(&trig.ocfg[output]);
	return trtl_rt_structure_set(wrtd->trtl, &hdr, &tlv, 1);
}


/**
 * It sets the given bitmaks (it means that it does OR with the current value)
 */
static int wrtd_out_flag_set(struct wrtd_node *dev, unsigned int output,
			     uint32_t flags)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrtd_out_channel chan;
	struct trtl_structure_tlv tlv = wrtd_channel_tlv(output, &chan);
	struct trtl_proto_header hdr = hdr_base_sync;
	int err;

	if (output >= FD_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;

	chan.config.flags |= flags;

	return trtl_rt_structure_set(wrtd->trtl, &hdr, &tlv, 1);
}

/**
 * It sets the given bitmaks (it means that it does AND NOT with the current
 * value)
 */
static int wrtd_out_flag_clr(struct wrtd_node *dev, unsigned int output,
			     uint32_t flags)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrtd_out_channel chan;
	struct trtl_structure_tlv tlv = wrtd_channel_tlv(output, &chan);
	struct trtl_proto_header hdr = hdr_base_sync;
	int err;

	if (output >= FD_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;

	chan.config.flags &= ~flags;

	return trtl_rt_structure_set(wrtd->trtl, &hdr, &tlv, 1);
}

static int wrtd_out_trig_assign_one(struct wrtd_node *dev, unsigned int output,
				    struct wrtd_trigger_handle *handle,
				    struct wrtd_trig_id *tid)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrtd_out_trigger trig;
	struct trtl_structure_tlv tlv;
	struct trtl_proto_header hdr = hdr_base_sync;
	int err, ret;

	handle->channel = output;
	if (handle->channel >= FD_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

        ret = wrtd_out_trigger_index_get(wrtd, tid);
	if (ret < 0)
		return ret;
	handle->ptr_trig = ret;

	tlv = wrtd_trigger_tlv(handle->ptr_trig, &trig);
	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;

	trig.flags |= ENTRY_FLAG_VALID;
	trig.id = *tid;
	memset(&trig.ocfg[handle->channel], 0, sizeof(struct lrt_output_rule));
	trig.ocfg[handle->channel].delay_cycles = 100000000 / 8000;
	trig.ocfg[handle->channel].state = HASH_ENT_DISABLED;
	trig.ocfg[handle->channel].state |= HASH_ENT_DIRECT;
	trig.ocfg[handle->channel].cond_ptr = -1;
	wrtd_output_rule_endianess_fix(&trig.ocfg[handle->channel]);

	err = trtl_rt_structure_set(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;

	err = wrtd_out_trigger_insert(wrtd, handle->ptr_trig);
	if (err)
		return err;

	return wrtd_out_flag_set(dev, handle->channel, WRTD_TRIGGER_ASSIGNED);
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
			 struct wrtd_trig_id *tid,
			 struct wrtd_trig_id *condition)
{
	struct wrtd_trigger_handle tmp_handle;
	int err;

	if (output >= FD_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	err = wrtd_out_trig_assign_one(dev, output, handle, tid);
	if (err)
		return err;
	if (!condition)
		return 0;
	err = wrtd_out_trig_assign_one(dev, output, &tmp_handle, condition);
	if (err)
		return err;

	handle->ptr_cond = tmp_handle.ptr_trig;
	return wrtd_out_trig_assign_condition_by_index(dev, output,
						       handle->ptr_trig,
						       handle->ptr_cond);
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
	struct wrtd_output_trigger_state triggers[256];
	struct wrtd_out_trigger trig;
	struct trtl_structure_tlv tlv = wrtd_trigger_tlv(handle->ptr_trig,
							 &trig);
	struct trtl_proto_header hdr = hdr_base_sync;
	struct wrtd_output_trigger_state state;
	int err, cnt = 0, i;
	volatile int cond;

	if (handle->channel >= FD_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;
	wrtd_output_rule_endianess_fix(&trig.ocfg[handle->channel]);

	cond = trig.ocfg[handle->channel].cond_ptr;
	memset(&trig.ocfg[handle->channel], 0, sizeof(struct lrt_output_rule));
	for (i = 0; i < FD_NUM_CHANNELS; i++)
		if (trig.ocfg[i].state == HASH_ENT_EMPTY)
			cnt++;
	if (cnt == FD_NUM_CHANNELS)
		trig.flags &= ~ENTRY_FLAG_VALID;

	wrtd_output_rule_endianess_fix(&trig.ocfg[handle->channel]);
        err = trtl_rt_structure_set(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;
	err = wrtd_out_trigger_remove(wrtd, handle->ptr_trig);
	if (err)
		return err;

	err = wrtd_out_trig_get_all(dev, handle->channel, triggers, 256);
	if (err < 0)
		return -1;
	if (err == 0){
		err = wrtd_out_flag_clr(dev, handle->channel,
					WRTD_TRIGGER_ASSIGNED);
	} else {
		err = 0;
	}

	if (cond < 0 || cond > FD_HASH_ENTRIES)
		return err;

	/* Remove also its condition */
	err = wrtd_out_trig_state_get_by_index(dev, cond,
					       handle->channel, &state);
	if (err)
		return err;

	return wrtd_out_trig_unassign(dev, &state.handle);
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
	struct wrtd_output_trigger_state tmp;
	int err = 0, i, count = 0;

	if (output >= FD_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	for (i = 0; count < max_count && i < FD_HASH_ENTRIES; i++) {
		err = wrtd_out_trig_state_get_by_index(dev, i, output, &tmp);
		if (err) {
			if (errno == EWRTD_NOFOUND_TRIGGER) {
				err = 0;
				continue;
			} else {
			        break;
			}
		} else {
			memcpy(&triggers[count], &tmp,
			       sizeof(struct wrtd_output_trigger_state));
			count++;
		}

	}

	/* Do not count trigger with error */
	if (err)
		count--;

	return count > 0 ? count : err;
}


/**
 * It returns a trigger state from a given handle.
 * @param[in] dev pointer to open node device.
 * @param[in] handle trigger where act on
 * @param[out] trigger trigger status
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_trig_state_get_by_handle(struct wrtd_node *dev,
				      struct wrtd_trigger_handle *handle,
				      struct wrtd_output_trigger_state *trigger)
{
	return wrtd_out_trig_state_get_by_index(dev, handle->ptr_trig,
						handle->channel, trigger);
}


/**
 * It returns a trigget from a given identifier.
 * Whenever is possible you should prefer wrtd_out_trig_state_get_by_handle()
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 * @param[in] id identifier of the trigger to retrieve
 * @param[out] trigger trigger status
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_trig_state_get_by_id(struct wrtd_node *dev,
				  unsigned int output,
				  struct wrtd_trig_id *tid,
				  struct wrtd_output_trigger_state *trigger)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	int index;

	index = wrtd_out_trigger_index_get(wrtd, tid);
	if (index < 0) {
		errno = EWRTD_NOFOUND_TRIGGER;
		return -1;
	}

	return wrtd_out_trig_state_get_by_index(dev, index, output, trigger);
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
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrtd_out_trigger trig;
	struct trtl_structure_tlv tlv = wrtd_trigger_tlv(index, &trig);
	struct trtl_proto_header hdr = hdr_base_sync;
	struct wrtd_output_trigger_state cond;
	int err;

	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;
	wrtd_output_rule_endianess_fix(&trig.ocfg[output]);

	if (!(trig.flags & ENTRY_FLAG_VALID) ||
	    (trig.ocfg[output].state == HASH_ENT_EMPTY)) {
		errno = EWRTD_NOFOUND_TRIGGER;
		return -1;
	}

	memset(trigger, 0, sizeof(struct wrtd_output_trigger_state));
	trigger->handle.channel = output;
	trigger->handle.ptr_trig = index;
	trigger->handle.ptr_cond = (uint32_t)trig.ocfg[output].cond_ptr;
	trigger->is_conditional = (0 <= trigger->handle.ptr_cond && trigger->handle.ptr_cond <= FD_HASH_ENTRIES);
	trigger->enabled = !(trig.ocfg[output].state & HASH_ENT_DISABLED);
	trigger->trigger = trig.id;
	trigger->delay_trig.ticks = trig.ocfg[output].delay_cycles;
	trigger->delay_trig.frac = trig.ocfg[output].delay_frac;
	if (trig.ocfg[output].latency_avg_nsamples) {
		trigger->latency_average_us = (trig.ocfg[output].latency_avg_sum /
					       trig.ocfg[output].latency_avg_nsamples + 124) / 125;
	} else {
		trigger->latency_average_us = 0;
	}
	trigger->latency_worst_us = (trig.ocfg[output].latency_worst + 124)
				  / 125;
	trigger->executed_pulses = trig.ocfg[output].hits;
	trigger->missed_pulses = trig.ocfg[output].misses;

	if (!trigger->is_conditional)
		return 0;

	/* Get conditional trigger */
	err =wrtd_out_trig_state_get_by_index(dev, trigger->handle.ptr_cond,
					      output, &cond);
	if (err)
		return err;
	memcpy(&trigger->condition, &cond.trigger, sizeof(struct wrtd_trig_id));
	memcpy(&trigger->delay_cond, &cond.delay_trig,
	       sizeof(struct wr_timestamp));

	return 0;
}

static int wrtd_out_rule_delay_set(struct wrtd_node *dev,
			    	   int output,
			    	   uint32_t trig_idx,
			    	   uint64_t delay_ps)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrtd_out_trigger trig;
	struct trtl_structure_tlv tlv = wrtd_trigger_tlv(trig_idx, &trig);
	struct trtl_proto_header hdr = hdr_base_sync;
	struct wr_timestamp t;
	int err;

	if (output >= FD_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	if (delay_ps > (1000 * 1000 * 1000 * 1000ULL - 1000ULL)) {
		errno = EWRTD_INVALID_DELAY;
		return -1;

	}

	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;
	wrtd_output_rule_endianess_fix(&trig.ocfg[output]);

	if (!(trig.flags & ENTRY_FLAG_VALID)) {
		errno = EWRTD_NOFOUND_TRIGGER;
		return -1;
	}

	wrtd_pico_to_ts(&delay_ps, &t);
	trig.ocfg[output].delay_cycles = t.ticks;
	trig.ocfg[output].delay_frac = t.frac;

	wrtd_output_rule_endianess_fix(&trig.ocfg[output]);
	return trtl_rt_structure_set(wrtd->trtl, &hdr, &tlv, 1);
}


/**
 * It sets the delay to apply for a given trigger
 * @param[in] dev pointer to open node device.
 * @param[in] handle trigger where act on
 * @param[in] delay_ps delay in pico-seconds in range [0, 999999999000]
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_trig_delay_set(struct wrtd_node *dev,
			    struct wrtd_trigger_handle *handle,
			    uint64_t delay_ps)
{
	return wrtd_out_rule_delay_set (dev, handle->channel, handle->ptr_trig, delay_ps);
}

/**
 * Sets the pulse width for a given output channel.
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 * @param[in] width_ps pulse width in pico-seconds (from 250ns to 1s)
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_pulse_width_set(struct wrtd_node *dev, unsigned int output,
			   uint64_t width_ps)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrtd_out_channel chan;
	struct trtl_structure_tlv tlv = wrtd_channel_tlv(output, &chan);
	struct trtl_proto_header hdr = hdr_base_sync;
	int err;

	if (output >= FD_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	if (width_ps < 1000ULL * 250 ||
	    width_ps >= 1000ULL * 1000 * 1000 * 1000) {
		errno = EWRTD_INVALID_PULSE_WIDTH;
		return -1;
	}

	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;

	chan.config.width_cycles = width_ps / 8000ULL;

	return trtl_rt_structure_set(wrtd->trtl, &hdr, &tlv, 1);
}


/**
 * It set the dead time for a given output channel. so, it applies on all
 * triggers assigned to the given output channel.
 *
 * The function will round the value, so it may happen that you read back a
 * different value. The reason is that the RT application measure the dead
 * time in ticks, which are 8ns steps. So this function will internally
 * convert the dead time in ticks. The function accept pico-seconds only to
 * be consistent with the API.
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 * @param[in] dead_time_ps dead time in pico-seconds
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_dead_time_set(struct wrtd_node *dev, unsigned int output,
			   uint64_t dead_time_ps)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrtd_out_channel chan;
	struct trtl_structure_tlv tlv = wrtd_channel_tlv(output, &chan);
	struct trtl_proto_header hdr = hdr_base_sync;
	int err;

	if (output >= FD_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;

	chan.config.dead_time = dead_time_ps / 8000;

	return trtl_rt_structure_set(wrtd->trtl, &hdr, &tlv, 1);
}


/**
 * @param[in] dev pointer to open node device.
 * @param[in] handle trigger where act on
 * @param[in] delay_ps delay in pico-seconds in range [0, 999999999000]
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_trig_condition_delay_set(struct wrtd_node *dev,
				      struct wrtd_trigger_handle *handle,
				      uint64_t delay_ps)
{
	if (handle->ptr_cond == 0)
	{
		errno = EWRTD_NO_TRIGGER_CONDITION;
		return -1;
	}

	return wrtd_out_rule_delay_set (dev, handle->channel, handle->ptr_cond, delay_ps);
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
	struct wrtd_out_trigger trig;
	struct trtl_structure_tlv tlv = wrtd_trigger_tlv(handle->ptr_trig,
							 &trig);
	struct trtl_proto_header hdr = hdr_base_sync;
	int err;

	if (handle->channel >= FD_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;
	wrtd_output_rule_endianess_fix(&trig.ocfg[handle->channel]);

	if (!(trig.flags & ENTRY_FLAG_VALID)) {
		errno = EWRTD_NOFOUND_TRIGGER;
		return -1;
	}

	if (enable)
		trig.ocfg[handle->channel].state &= ~HASH_ENT_DISABLED;
	else
		trig.ocfg[handle->channel].state |= HASH_ENT_DISABLED;

	wrtd_output_rule_endianess_fix(&trig.ocfg[handle->channel]);
	return trtl_rt_structure_set(wrtd->trtl, &hdr, &tlv, 1);
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
	struct wrtd_out_channel chan;
	struct trtl_structure_tlv tlv = wrtd_channel_tlv(output, &chan);
	struct trtl_proto_header hdr = hdr_base_sync;
	int err;

	if (output >= FD_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;

	chan.config.mode = mode;
	chan.config.flags &= ~(WRTD_TRIGGERED | WRTD_LAST_VALID);
	if (chan.config.mode == WRTD_TRIGGER_MODE_SINGLE) {
		chan.config.flags &= ~WRTD_ARMED;
		chan.config.state = OUT_ST_IDLE;
	}

	return trtl_rt_structure_set(wrtd->trtl, &hdr, &tlv, 1);
}


/**
 * It arms (un-arms) a given output channel
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 * @param[in] armed 1 to arm, 0 to un-arm
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_arm(struct wrtd_node *dev, unsigned int output, int armed)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrtd_out_channel chan;
	struct trtl_structure_tlv tlv = wrtd_channel_tlv(output, &chan);
	struct trtl_proto_header hdr = hdr_base_sync;
	int err;

	if (output >= FD_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;

	chan.config.flags &= ~WRTD_TRIGGERED;
	if (armed) {
		chan.config.flags |= WRTD_ARMED;
		chan.config.state = OUT_ST_ARMED;
	} else {
		chan.config.flags &= ~WRTD_ARMED;
		chan.config.state = OUT_ST_IDLE;
	}

	return trtl_rt_structure_set(wrtd->trtl, &hdr, &tlv, 1);
}


/**
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_counters_reset(struct wrtd_node *dev, unsigned int output)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrtd_out_channel chan;
	struct trtl_structure_tlv tlv = wrtd_channel_tlv(output, &chan);
	struct trtl_proto_header hdr = hdr_base_sync;
	int err;

	if (output >= FD_NUM_CHANNELS) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	err = trtl_rt_structure_get(wrtd->trtl, &hdr, &tlv, 1);
	if (err)
		return err;

	chan.stats.miss_timeout = 0;
	chan.stats.miss_deadtime = 0;
	chan.stats.miss_no_timing = 0;
	chan.stats.miss_overflow = 0;
	chan.config.flags &= ~WRTD_LAST_VALID;

	return trtl_rt_structure_set(wrtd->trtl, &hdr, &tlv, 1);
}


/**
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_check_triggered(struct wrtd_node *dev, unsigned int output)
{
	struct wrtd_output_state st;
	int err;

	err = wrtd_out_state_get(dev, output, &st);
	if(err)
		return err;

	return st.flags & WRTD_TRIGGERED ? 1 : 0;
}


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
	struct wrtd_output_state state;
	int err;

	err = wrtd_out_state_get(dev, output, &state);
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
 * Check the trigger assigned status on a trigger output. If you provide
 * a trigger identifier then the function checks that the given trigger
 * is assigned to the given channel. Otherwise it will tell you if there
 * is any trigger assigned to the channel.
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 * @param[in] id trigger identifier (optional)
 * @param[out] armed 1 if it is enabled, 0 otherwise
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_has_trigger(struct wrtd_node *dev, unsigned int output,
			 struct wrtd_trig_id *id, unsigned int *assigned)
{
	struct wrtd_output_trigger_state triggers[256];
	struct wrtd_output_state state;
	int ret, i;

	/* Set default output */
	*assigned = 0;

	ret = wrtd_out_state_get(dev, output, &state);
	if (ret)
		return -1;

	if (!id) {
		/* Check only if there is at least one trigger */
		*assigned = !!(state.flags & WRTD_TRIGGER_ASSIGNED);
		return 0;
	}

	/* Look for the id among all assigned trigger */
	ret = wrtd_out_trig_get_all(dev, output, triggers, 256);
	if (ret < 0)
		return -1;
	for (i = 0; i < ret; i++) {
		if (wrtd_trig_id_cmp(id, &triggers[i].trigger) == 0) {
			*assigned = 1;
			return 0;
		}
	}

	return 0;
}


/**
 * It check if the output real-time application is alive
 * @param[in] dev device token
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_ping(struct wrtd_node *dev)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;

	return trtl_rt_ping(wrtd->trtl, WRTD_IN_FD_CONTROL,
			    WRTD_OUT_FD_CONTROL);
}

/**
 * It gets the output base time
 * @param[in] dev device token
 * @param[out] ts output device base time
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_base_time(struct wrtd_node *dev, struct wr_timestamp *ts)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	uint32_t variables[] = {OUT_VAR_DEVICE_TIME_S, 0,
				OUT_VAR_DEVICE_TIME_T, 0};
	struct trtl_proto_header hdr = hdr_base_sync;
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
int wrtd_out_version(struct wrtd_node *dev, struct trtl_rt_version *version)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;

	return trtl_rt_version_get(wrtd->trtl, version,
				   WRTD_IN_FD_CONTROL, WRTD_OUT_FD_CONTROL);
}
