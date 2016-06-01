/*
 * Copyright (C) 2015 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include <libmockturtle.h>
#include <libwrtd-internal.h>
#include <wrtd-serializers.h>

/**
 * It returns a human readable string that describe a given log level
 * @param[in] lvl log level
 * @return a string if the log level is mapped, otherwise an empty string
 */
const char *wrtd_strlogging(enum wrtd_log_level lvl)
{
	switch (lvl) {
	case WRTD_LOG_NOTHING:
		return "off";
	case WRTD_LOG_RAW:
		return "raw";
	case WRTD_LOG_SENT:
		return "sent";
	case WRTD_LOG_PROMISC:
		return "promiscuous";
	case WRTD_LOG_FILTERED:
		return "filtered";
	case WRTD_LOG_EXECUTED:
		return "executed";
	case WRTD_LOG_MISSED:
		return "missed";
	case WRTD_LOG_ALL:
		return "all";
	}

	return "n/a";
}


/**
 * It returns the full string describing the log_level in use
 * @param[out] buf where write the string
 * @param[in] log_level the log level to describe
 */
void wrtd_strlogging_full(char *buf, uint32_t log_level)
{
	enum wrtd_log_level lvl;

	if (!log_level) { /* No log level */
		strcpy(buf, wrtd_strlogging(log_level));
		return;
	}

	strcpy(buf,"");
	for (lvl = 0x1; lvl <= WRTD_LOG_MISSED; lvl <<= 1) {
		if (lvl & log_level) {
			strcat(buf, wrtd_strlogging(lvl));
			strcat(buf, " ");
		}
	}
}


/**
 * It converts a given logging string into a log_level
 * @param[in] log string log level
 * @return the correspondent log level enum
 */
enum wrtd_log_level wrtd_strlogging_to_level(char *log)
{
	if(!strcmp(log, "all"))
		return WRTD_LOG_ALL;
        if(!strcmp(log, "promiscuous"))
		return WRTD_LOG_PROMISC;
        if(!strcmp(log, "raw"))
		return WRTD_LOG_RAW;
        if(!strcmp(log, "executed"))
		return WRTD_LOG_EXECUTED;
        if(!strcmp(log, "missed"))
		return WRTD_LOG_MISSED;
        if(!strcmp(log, "sent"))
		return WRTD_LOG_SENT;
        if(!strcmp(log, "filtered"))
		return WRTD_LOG_FILTERED;

	return WRTD_LOG_NOTHING;
}


/**
 * It opens the logging interface for a given divice. The  default
 * logging level will be applied to all device channels. You can change it
 * later using wrtd_log_level_set()
 * @param[in] dev device token
 * @param[in] input channel number [-1, 4]. [-1] for all channels, [0,4] for a
 *                  specific one.
 * @param[in] core WRTD core to use
 * @return a HMQ token on success, NULL on error and errno is set appropriately
 */
static struct wrnc_hmq *wrtd_log_open(struct wrtd_node *dev,
				      int channel,
				      enum wrtd_core core)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg_filter filter = {
		.operation = WRNC_MSG_FILTER_EQ,
		.word_offset = 3, /* channel field */
		.mask = 0xFFFF, /* entire field */
		.value = channel, /* required channel */
	};
	struct wrnc_hmq *hmq = NULL;
	int err;
	int n_chan = core ? FD_NUM_CHANNELS : TDC_NUM_CHANNELS;
	unsigned int hmq_back_index = core ? WRTD_OUT_FD_LOGGING :
					     WRTD_OUT_TDC_LOGGING;

	if (channel < -1 || channel >= n_chan) {
		errno = EWRTD_INVALID_CHANNEL;
		return NULL;
	}

	hmq = wrnc_hmq_open(wrtd->wrnc, hmq_back_index, 0);
	if (!hmq)
		return NULL;

	if (channel > -1) {
		if (core == WRTD_CORE_IN) {
			/* On input side we have the header */
			filter.word_offset = 6;
		}
		/* the user want to filter per channel */
		err = wrnc_hmq_filter_add(hmq, &filter);
		if (err)
			goto out_close;
	}

	return hmq;

out_close:
	wrnc_hmq_close(hmq);
	return NULL;
}


/**
 * It reads one or more log entry from a given hmq_log. The user of this
 * function must check that the hmq_log used correspond to a logging interface
 * @param[in] hmq_log logging HMQ.
 * @param[out] log log message
 * @param[in] count number of messages to read
 * @param[in] poll_timeout poll(2) timeout argument. Negative means infinite.
 * @return number of read messages on success (check errno if it returns less
 *         messages than expected), -1 on error and errno is set appropriately
 */
int wrtd_log_read(struct wrnc_hmq *hmq_log, struct wrtd_log_entry *log,
		  int count, int poll_timeout)
{
	struct wrnc_proto_header hdr;
	struct wrtd_log_entry *cur = log;
	struct wrnc_msg *msg;
	struct pollfd p;
	int remaining = count;
	int n_read = 0, ret;
	uint32_t id = 0, seq = 0;

	p.fd = hmq_log->fd;
	p.events = POLLIN;

	/* Clean up errno to be able to distinguish between error cases and
	   normal behaviour when the function return less messages
	   than expected */
	errno = 0;
	while (remaining) {
		struct wrtd_trigger_entry ent;
		ret = poll(&p, 1, poll_timeout);
		if (ret <= 0 || !(p.revents & POLLIN))
			break;

		msg = wrnc_hmq_receive(hmq_log);
		if (!msg)
			break;

		if (hmq_log->index == WRTD_OUT_FD_LOGGING) {
			/* Output */
			wrnc_msg_header (msg, &id, &seq);

			if (id != WRTD_REP_LOG_MESSAGE)
			{
				free(msg);
				errno = EWRTD_INVALID_ANSWER_STATE;
				break;
			}

			wrnc_msg_uint32 (msg, &cur->type);
			wrnc_msg_int32 (msg, &cur->channel);
			wrnc_msg_uint32 (msg, &cur->miss_reason);
			wrtd_msg_trigger_entry(msg, &ent);

			cur->ts = ent.ts;
			cur->seq = ent.seq;
			cur->id = ent.id;

			if ( wrnc_msg_check_error(msg) ) {
				free(msg);
				errno = EWRTD_INVALID_ANSWER_STATE;
				break;
			}
		} else {
			/* Input */
			wrnc_message_unpack(msg, &hdr, cur);
			if (hdr.msg_id != WRTD_IN_ACTION_LOG) {
				free(msg);
				errno = EWRTD_INVALID_ANSWER_STATE;
				break;
			}
			wrtd_timestamp_endianess_fix(&cur->ts);
		}

		remaining--;
		n_read++;
		cur++;
		free(msg);
	}

	return (n_read > 0 || errno == 0 ? n_read : -1);
}

/**
 * It closes the logging interface
 * @param[in] hmq HMQ token to close
 */
void wrtd_log_close(struct wrnc_hmq *hmq)
{
	wrnc_hmq_close(hmq);
}


/**
 * @param[in] dev device token
 * @param[in] channel 0-based channel index
 * @param[in] log_level log level to apply to the logging messages
 * @return 0 on success, -1 on error and errno is set appropriately
 */
static int wrtd_log_level_set(struct wrtd_node *dev, unsigned int channel,
			      uint32_t log_level, enum wrtd_core core)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_structure_tlv tlv;
	struct wrnc_proto_header hdr;
	struct wrtd_out_channel ochan;
	struct wrtd_in_channel ichan;
	int err;

	hdr.flags = WRNC_PROTO_FLAG_SYNC;
	if (core) {
		if (channel >= FD_NUM_CHANNELS) {
			errno = EWRTD_INVALID_CHANNEL;
			return -1;
		}

		tlv.index = OUT_STRUCT_CHAN_0 + channel;
		tlv.size = sizeof(struct wrtd_out_channel);
		tlv.structure = &ochan;
		hdr.slot_io = (WRTD_IN_FD_CONTROL << 4) |
			(WRTD_OUT_FD_CONTROL & 0xF);
		/* TODO set promiscuous mode */
	} else {
		if (channel >= TDC_NUM_CHANNELS) {
			errno = EWRTD_INVALID_CHANNEL;
			return -1;
		}
		tlv.index = IN_STRUCT_CHAN_0 + channel;
		tlv.size = sizeof(struct wrtd_in_channel);
		tlv.structure = &ichan;
		hdr.slot_io = (WRTD_IN_TDC_CONTROL << 4) |
			(WRTD_OUT_TDC_CONTROL & 0xF);
	}

	err = wrnc_rt_structure_get(wrtd->wrnc, &hdr, &tlv, 1);
	if (err)
		return err;

	if (core)
		ochan.config.log_level = log_level;
	else
		ichan.config.log_level = log_level;

	return wrnc_rt_structure_set(wrtd->wrnc, &hdr, &tlv, 1);
}


/**
 * It opens the logging interface for a given divice. The  default
 * logging level will be applied to all device channels. You can change it
 * later using wrtd_out_log_level_set()
 * @param[in] dev device token
 * @param[in] output channel number [-1, 3]. [-1] for all channels, [0,3] for a
 *                   specific one.
 * @return a HMQ token on success, NULL on error and errno is set appropriately
 */
struct wrnc_hmq *wrtd_out_log_open(struct wrtd_node *dev, int output)
{
	return wrtd_log_open(dev, output, WRTD_CORE_OUT);
}


/**
 * It sets the logging level for an output channel
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 * @param[in] log_level log level to apply to the logging messages
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_log_level_set(struct wrtd_node *dev, unsigned int output,
			   uint32_t log_level)
{
	return wrtd_log_level_set(dev, output, log_level,
				  WRTD_CORE_OUT);
}


/**
 * It gets the logging level for an output channel
 * @param[in] dev device token
 * @param[in] output index (0-based) of output channel
 * @param[out] log_level current log level used by the Real-Time application
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_log_level_get(struct wrtd_node *dev, unsigned int input,
			   uint32_t *log_level)
{
	struct wrtd_output_state state;
	int err;

	err = wrtd_out_state_get(dev, input, &state);
	if (err)
		return err;

	*log_level = state.log_level;

	return 0;
}

/**
 * It opens the logging interface for device a given divice. The  default
 * logging level will be applied to all device channels. You can change it
 * later using wrtd_in_log_level_set()
 * @param[in] dev device token
 * @param[in] lvl default logging level
 * @param[in] input channel number [-1, 4]. [-1] for all channels, [0,4] for a
 *                  specific one.
 * @return a HMQ token on success, NULL on error and errno is set appropriately
 */
struct wrnc_hmq *wrtd_in_log_open(struct wrtd_node *dev, int input)
{
	return wrtd_log_open(dev, input, WRTD_CORE_IN);
}


/**
 * It sets the logging level for an input channel
 * @param[in] dev device token
 * @param[in] input index (0-based) of input channel
 * @param[in] log_level log level to apply to the logging messages
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_log_level_set(struct wrtd_node *dev, unsigned int input,
			  uint32_t log_level)
{
	return wrtd_log_level_set(dev, input, log_level, WRTD_CORE_IN);
}


/**
 * It gets the logging level for an input channel
 * @param[in] dev device token
 * @param[in] input index (0-based) of input channel
 * @param[out] log_level current log level used by the Real-Time application
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_log_level_get(struct wrtd_node *dev, unsigned int input,
			  uint32_t *log_level)
{
	struct wrtd_input_state state;
	int err;

	err = wrtd_in_state_get(dev, input, &state);
	if (err)
		return err;

	*log_level = state.log_level;
	return 0;
}
