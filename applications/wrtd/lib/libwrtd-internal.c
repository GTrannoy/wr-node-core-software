/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *         inspired by a draft of Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * License: GPL v3
 */

#include <stdlib.h>
#include <errno.h>
#include <string.h>
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
 * It compares two triggers id. The output is the same of memcmp(2)
 * @param[in] id1 first id to compare
 * @param[in] id2 second id to compare
 * @return like memcmp(2)
 */
int wrtd_trig_id_cmp(struct wrtd_trig_id *id1, struct wrtd_trig_id *id2)
{
	return memcmp(id1, id2, sizeof(struct wrtd_trig_id));
}


/*
 * Internal helper to send and receive synchronous messages to/from the WRNC
 */
int wrtd_send_and_receive_sync(struct wrtd_desc *wrtd,
			       struct wrnc_msg *msg,
			       enum wrtd_core core)
{
	/* Send the message and get answer */
	struct wrnc_hmq *hmq;
	unsigned int hmq_send = core ? WRTD_IN_FD_CONTROL : WRTD_IN_TDC_CONTROL;
	unsigned int hmq_recv = core ? WRTD_OUT_FD_CONTROL :
				       WRTD_OUT_TDC_CONTROL;
	int err;

	hmq = wrnc_hmq_open(wrtd->wrnc, hmq_send, WRNC_HMQ_INCOMING);
	if (!hmq)
		return -1;

	err = wrnc_hmq_send_and_receive_sync(hmq, hmq_recv, msg,
					     WRTD_DEFAULT_TIMEOUT);

	wrnc_hmq_close(hmq);

	return err < 0 ? err : 0; /* ignore timeout */
}

/**
 * It performs a simple request to a given core which will only answer
 * with an ACK
 */
int wrtd_trivial_request(struct wrtd_node *dev,
			 struct wrnc_msg *request_msg,
			 enum wrtd_core core)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	int err;

	err = wrtd_send_and_receive_sync(wrtd, request_msg, core);
        if (err)
		return err;

	return wrtd_validate_acknowledge(request_msg);
}


/**
 * The embedded core is big endian, convert it to little endian (host)
 */
void wrtd_timestamp_endianess_fix(struct wr_timestamp *ts)
{
	ts->seconds = ((ts->seconds & 0xFFFFFFFF) << 32) |
		      ((ts->seconds >> 32) & 0xFFFFFFFF);
}

void wrtd_output_rule_endianess_fix(struct lrt_output_rule *rule)
{
	uint16_t tmp = rule->state;

	rule->state = (rule->delay_frac & 0x00FF) << 8 |
		(rule->delay_frac & 0xFF00) >> 8;
	rule->delay_frac = (tmp & 0x00FF) << 8 |
		(tmp & 0xFF00) >> 8;
}
