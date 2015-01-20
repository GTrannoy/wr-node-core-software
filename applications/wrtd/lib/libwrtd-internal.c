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
 * It validates the answer of a synchronous message
 * @param[in] msg message to validate
 * @return 0 if it is valid, -1 otherwise and errno is appropriately set
 */
int wrtd_validate_acknowledge(struct wrnc_msg *msg)
{
	if (msg->datalen != 2 || msg->data[0] != WRTD_REP_ACK_ID) {
		errno = EWRTD_INVALID_ANSWER_ACK;
		return -1;
	}

	return 0;
}


/**
 * It converts a pico-second value to a wr_timestamp structure
 * @param[in] p pico-second value to convert
 * @return a wr_timestamp structure corresponding to the given pico-second value
 */
struct wr_timestamp picos_to_ts(uint64_t p)
{
	struct wr_timestamp t;

	t.seconds = p / (1000ULL * 1000ULL * 1000ULL * 1000ULL);
	p %= (1000ULL * 1000ULL * 1000ULL * 1000ULL);
	t.ticks = p / 8000;
	p %= 8000;
	t.frac = p * 4096 / 8000;

	return t;
}


/**
 * It extracts a wr_timestamp from a given buffer (arriving from a real-time
 * application)
 * @param[in] buf answer of the real time application
 * @param[in] offset offset of the timestamp inside the answer buffer
 * @param[out] ts where write the wr_timestamp
 */
void unbag_ts(uint32_t *buf, int offset, struct wr_timestamp *ts)
{
    ts->seconds = buf[offset];
    ts->ticks = buf[offset + 1];
    ts->frac = buf[offset + 2];
}


/**
 * It reads one or more log entry from a given hmq_log. The user of this function
 * must check that the hmq_log used correspond to a logging interface
 * @param[in] hmq_log logging HMQ
 * @param[out] log log message
 * @param[in] count number of messages to read
 * @return number of read messages on success, -1 on error and errno is set
 *         appropriately
 */
int wrtd_log_read(struct wrnc_hmq *hmq_log, struct wrtd_log_entry *log,
		  int count)
{
	struct wrtd_log_entry *cur = log;
	struct wrnc_msg *msg;
	int remaining = count;
	int n_read = 0;

	while (remaining) {
		msg = wrnc_hmq_receive(hmq_log);
		if (!msg)
			break;
		/*FIXME optimize with serialization */
		cur->type = msg->data[0];
		cur->channel = msg->data[2];

		cur->seq = msg->data[1];
		cur->id.system = msg->data[3];
		cur->id.source_port = msg->data[4];
		cur->id.trigger = msg->data[5];
		cur->ts.seconds = msg->data[6];
		cur->ts.ticks = msg->data[7];
		cur->ts.frac = msg->data[8];

		remaining--;
		n_read++;
		cur++;
		free(msg);
	}

	return n_read ? n_read : -1;
}
