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

static void unbag_ts(uint32_t *buf, int offset, struct wr_timestamp *ts)
{
    ts->seconds = buf[offset];
    ts->ticks = buf[offset + 1];
    ts->bins = buf[offset + 2];
}

static const uint32_t application_id[] = {
	0x115790de,
};

static const char *wrtd_errors[] = {
	"Received an invalid answer from white-rabbit-node-code CPU",
	"You are using an invalid binary",
};

const char *wrtd_strerror(int err)
{
	if (err < EWRTD_INVALD_ANSWER_ACK || err >= __EWRTD_MAX_ERROR_NUMBER)
		return wrnc_strerror(err);

	return wrtd_errors[err - EWRTD_INVALD_ANSWER_ACK];
}

int wrtd_init()
{
	int err;

	err = wrnc_init();
	if (err)
		return err;

	return 0;
}

void wrtd_exit()
{
	wrnc_exit();
}

struct wrtd_node *wrtd_open_by_fmc(uint32_t device_id)
{
	struct wrtd_desc *wrtd;

	wrtd = malloc(sizeof(struct wrtd_desc));
	if (!wrtd)
		return NULL;

	wrtd->wrnc = wrnc_open_by_fmc(device_id);
	if (!wrtd->wrnc)
		goto out;

	return (struct wrtd_node *)wrtd;

out:
	free(wrtd);
	return NULL;
}

void wrtd_close(struct wrtd_node *dev)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;

	wrnc_close(wrtd->wrnc);
	free(wrtd);
	dev = NULL;
}

struct wrnc_dev *wrtd_get_wrnc_dev(struct wrtd_node *dev)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;

	return (struct wrnc_dev *)wrtd->wrnc;
}

int wrtd_load_application(struct wrtd_node *dev, char *rt_tdc,
			  char *rt_fd)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	uint32_t reg_old;
	int err;

	if (!rt_tdc || !rt_fd) {
		errno = EWRTD_INVALD_BINARY;
		return -1;
	}
	err = wrnc_cpu_reset_get(wrtd->wrnc, &reg_old);
	if (err)
		return err;

	/* Keep the CPUs in reset state */
	err = wrnc_cpu_reset_set(wrtd->wrnc,
				 (1 << WRTD_CPU_TDC) | (1 << WRTD_CPU_FD));
	if (err)
		return err;

	/* Program CPUs application */
	err = wrnc_cpu_load_application_file(wrtd->wrnc, WRTD_CPU_TDC, rt_tdc);
	if (err)
		return err;
	err = wrnc_cpu_load_application_file(wrtd->wrnc, WRTD_CPU_FD, rt_tdc);
	if (err)
		return err;

	/* Re-enable the CPUs */
	err = wrnc_cpu_reset_set(wrtd->wrnc, reg_old);
	if (err)
		return err;

	return 0;
}

static inline int wrtd_send_and_receive_sync(struct wrtd_desc *wrtd, struct wrnc_msg *msg)
{
	/* Send the message and get answer */
	return wrnc_slot_send_and_receive_sync(wrtd->wrnc,
					       WRTD_IN_TDC_CONTROL,
					       WRTD_OUT_TDC_CONTROL,
					       msg,
					       WRTD_DEFAULT_TIMEOUT);
}

int wrtd_in_state_get(struct wrtd_node *dev, unsigned int input,
		      struct wrtd_input_state *state)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg msg;
	int err;

  	if (input >= WRTD_IN_MAX || state == NULL) {
		errno = -EINVAL;
		return -1;
	}

	/* Build the message */
	msg.datalen = 3;
	msg.data[0] = WRTD_CMD_TDC_CHAN_GET_STATE;
	msg.data[1] = 0;
	msg.data[2] = input;

	/* Send the message and get answer */
	err = wrtd_send_and_receive_sync(wrtd, &msg);
        if (msg.datalen != 29 || msg.data[0] != WRTD_REP_STATE) {
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

int wrtd_in_enable(struct wrtd_node *dev, unsigned int input, int enable)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg msg;
	int err;

	if (input >= WRTD_IN_MAX) {
		errno = -EINVAL;
		return -1;
	}

	/* Build the message */
	msg.datalen = 4;
	msg.data[0] = WRTD_CMD_TDC_CHAN_ENABLE;
	msg.data[1] = 0;
	msg.data[2] = input;
	msg.data[3] = !!enable;

	/* Send the message and get answer */
	err = wrtd_send_and_receive_sync(wrtd, &msg);
	if (err)
		return err;

	if (msg.datalen != 2 || msg.data[0] != WRTD_REP_ACK_ID) {
		errno = EWRTD_INVALD_ANSWER_ACK;
		return -1;
	}

	return 0;
}
