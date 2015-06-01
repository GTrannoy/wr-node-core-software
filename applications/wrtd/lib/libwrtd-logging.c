/*
 * Copyright (C) 2015 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <libwrnc.h>
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
		return "No logging";
	case WRTD_LOG_RAW:
		return "incoming pulse";
	case WRTD_LOG_SENT:
		return "trigger message sent";
	case WRTD_LOG_PROMISC:
		return "received trigger message";
	case WRTD_LOG_FILTERED:
		return "trigger message assigned";
	case WRTD_LOG_EXECUTED:
		return "pulse generated";
	case WRTD_LOG_MISSED:
		return "pulse missed";
	case WRTD_LOG_ALL:
		return "all";
	}

	return "n/a";
}


/**
 * It opens the logging interface for a given divice. The  default
 * logging level will be applied to all device channels. You can change it
 * later using wrtd_log_level_set()
 * @param[in] dev device token
 * @param[in] lvl default logging level
 * @param[in] input channel number [-1, 4]. [-1] for all channels, [0,4] for a
 *                  specific one.
 * @param[in] core WRTD core to use
 * @return a HMQ token on success, NULL on error and errno is set appropriately
 */
static struct wrnc_hmq *wrtd_log_open(struct wrtd_node *dev,
				      uint32_t lvl, int channel,
				      enum wrtd_core core)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg_filter filter = {
		.operation = WRNC_MSG_FILTER_AND,
		.word_offset = 2, /* channel field */
		.mask = 0xFFFF, /* entire field */
		.value = channel, /* required channel */
	};
	struct wrnc_hmq *hmq = NULL;
	int i, err;
	int n_chan = core ? TDC_NUM_CHANNELS : FD_NUM_CHANNELS;
	unsigned int hmq_back_index = core ? WRTD_OUT_TDC_LOGGING :
					     WRTD_OUT_FD_LOGGING;

	if (channel < -1 || channel >= n_chan) {
		errno = EWRTD_INVALID_CHANNEL;
		return NULL;
	}

	if (channel > -1) {
		err = wrtd_in_log_level_set(dev, channel, lvl);
		if (err)
			return NULL;

		hmq = wrnc_hmq_open(wrtd->wrnc, hmq_back_index, 0);
		if (!hmq)
			return NULL;

		err = wrnc_hmq_filter_add(hmq, &filter);
		if (err) {
			wrnc_hmq_close(hmq);
			return NULL;
		}
	} else {
		/* Set the same logging level to all channels */
		for (i = 0; i < n_chan; ++i) {
			err = wrtd_in_log_level_set(dev, i, lvl);
			if (err)
				return NULL;
		}
		hmq = wrnc_hmq_open(wrtd->wrnc, hmq_back_index, 0);
	}

	return hmq;
}


/**
 * It reads one or more log entry from a given hmq_log. The user of this
 * function must check that the hmq_log used correspond to a logging interface
 * @param[in] hmq_log logging HMQ.
 * @param[out] log log message
 * @param[in] count number of messages to read
 * @return number of read messages on success (check errno if it returns less
 *         messages than expected), -1 on error and errno is set appropriately
 */
int wrtd_log_read(struct wrnc_hmq *hmq_log, struct wrtd_log_entry *log,
		  int count)
{
	struct wrtd_log_entry *cur = log;
	struct wrnc_msg *msg;
	int remaining = count;
	int n_read = 0;
	uint32_t id = 0, seq = 0;

	/* Clean up errno to be able to distinguish between error cases and
	   normal behaviour when the function return less messages
	   than expected */
	errno = 0;
	while (remaining) {
		struct wrtd_trigger_entry ent;
		msg = wrnc_hmq_receive(hmq_log);
		if (!msg)
			break;

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

		remaining--;
		n_read++;
		cur++;
		free(msg);
	}

	return n_read ? n_read : -1;
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
	struct wrnc_msg msg = wrnc_msg_init(4);
	uint32_t seq = 0;
	uint32_t id = core ? WRTD_CMD_TDC_CHAN_SET_LOG_LEVEL :
			     WRTD_CMD_FD_CHAN_SET_LOG_LEVEL;
	int n_chan = core ? TDC_NUM_CHANNELS : FD_NUM_CHANNELS;

	if (channel > n_chan) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	/* Build the message */
	wrnc_msg_header(&msg, &id, &seq);
	wrnc_msg_uint32(&msg, &channel);
	wrnc_msg_uint32(&msg, &log_level);

	return wrtd_trivial_request(dev, &msg, core);
}


/**
 * It opens the logging interface for a given divice. The  default
 * logging level will be applied to all device channels. You can change it
 * later using wrtd_out_log_level_set()
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
	return wrtd_log_open(dev, lvl, output, WRTD_CORE_OUT);
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
 * It sets the logging interface shared mode. When the logging interface
 * is shared all users will receive the same messages.
 * Rembember that the logging interface is not "per-channel" but "per-core".
 * Even if it is possible to fake a per-channel behaviour
 * for wrtd_in_log_open(); there is no way to set the shared mode for
 * single channel
 * @param[in] dev device token
 * @param[in] shared 1 share, 0 not share
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_log_share_set(struct wrtd_node *dev, unsigned int shared)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;

	return wrnc_hmq_share_set(wrtd->wrnc, WRNC_HMQ_OUTCOMING,
				  WRTD_OUT_FD_LOGGING, shared);
}


/**
 * It gets the current shared mode status for the logging interface
 * @param[in] dev device token
 * @param[out] shared 1 share, 0 not share
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_out_log_share_get(struct wrtd_node *dev, unsigned int *shared)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;

	return wrnc_hmq_share_get(wrtd->wrnc, WRNC_HMQ_OUTCOMING,
				  WRTD_OUT_FD_LOGGING, shared);
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
struct wrnc_hmq *wrtd_in_log_open(struct wrtd_node *dev,
				  uint32_t lvl,
				  int input)
{
	return wrtd_log_open(dev, lvl, input, WRTD_CORE_IN);
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
 * It sets the logging interface shared mode. When the logging interface
 * is shared all users will receive the same messages.
 * Rembember that the logging interface is not "per-channel" but "per-core".
 * Even if it is possible to fake a per-channel behaviour
 * for wrtd_in_log_open(); there is no way to set the shared mode for
 * single channel
 * @param[in] dev device token
 * @param[in] shared 1 share, 0 not share
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_log_share_set(struct wrtd_node *dev, unsigned int shared)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;

	return wrnc_hmq_share_set(wrtd->wrnc, WRNC_HMQ_OUTCOMING,
				  WRTD_OUT_TDC_LOGGING, shared);
}


/**
 * It gets the current shared mode status for the logging interface
 * @param[in] dev device token
 * @param[out] shared 1 share, 0 not share
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_log_share_get(struct wrtd_node *dev, unsigned int *shared)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;

	return wrnc_hmq_share_get(wrtd->wrnc, WRNC_HMQ_OUTCOMING,
				  WRTD_OUT_TDC_LOGGING, shared);
}
