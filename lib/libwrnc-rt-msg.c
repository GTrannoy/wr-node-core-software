/*
 * Copyright (C) 2015 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *
 * Released according to the GNU GPL, version 3
 */

#include <libwrnc.h>
#include <errno.h>

/**
 * Retrieve the current Real-Time Application version running. This is a
 * synchronous message.
 * @param[in] wrnc device token
 * @param[out] version FPGA, RT and GIT version
 * @param[in] hmq_in hmq slot index where send the message
 * @param[in] hmq_out hmq slot index where you expect the answer
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrnc_rt_version_get(struct wrnc_dev *wrnc, struct wrnc_rt_version *version,
			unsigned int hmq_in, unsigned int hmq_out)
{
	struct wrnc_proto_header hdr;
	struct wrnc_hmq *hmq;
	struct wrnc_msg msg;
	int err;

	memset(&msg, 0, sizeof(struct wrnc_msg));
	memset(&hdr, 0, sizeof(struct wrnc_proto_header));
	hdr.msg_id = RT_ACTION_RECV_VERSION;
	hdr.slot_io = (hmq_in << 4) | hmq_out;
	hdr.flags = WRNC_PROTO_FLAG_SYNC;
	hdr.len = 0;
	wrnc_message_pack(&msg, &hdr, NULL);

	hmq = wrnc_hmq_open(wrnc, hmq_in, WRNC_HMQ_INCOMING);
	if (!hmq)
		return -1;

	/* Send the message and get answer */
        err = wrnc_hmq_send_and_receive_sync(hmq, hmq_out, &msg,
					     wrnc_default_timeout_ms);
	wrnc_hmq_close(hmq);

	wrnc_message_unpack(&msg, &hdr, version);
	if (hdr.msg_id != RT_ACTION_SEND_VERSION) {
		errno = EWRNC_INVALID_MESSAGE;
		return -1;
	}

	return 0;
}


/**
 * It checks if a Real-Time application is running and answering to messages
 * @param[in] wrnc device token
 * @param[in] hmq_in hmq slot index where send the message
 * @param[in] hmq_out hmq slot index where you expect the answer
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrnc_rt_ping(struct wrnc_dev *wrnc,
		 unsigned int hmq_in, unsigned int hmq_out)
{
	struct wrnc_proto_header hdr;
	struct wrnc_hmq *hmq;
	struct wrnc_msg msg;
	int err;

	memset(&hdr, 0, sizeof(struct wrnc_proto_header));
	hdr.msg_id = RT_ACTION_RECV_PING;
	hdr.slot_io = (hmq_in << 4) | hmq_out;
	hdr.flags = WRNC_PROTO_FLAG_SYNC;
	hdr.len = 0;
	wrnc_message_pack(&msg, &hdr, NULL);

	hmq = wrnc_hmq_open(wrnc, hmq_in, WRNC_HMQ_INCOMING);
	if (!hmq)
		return -1;

	/* Send the message and get answer */
        err = wrnc_hmq_send_and_receive_sync(hmq, hmq_out, &msg,
					     wrnc_default_timeout_ms);
	wrnc_hmq_close(hmq);
	if (hdr.msg_id != RT_ACTION_SEND_ACK) {
		errno = EWRNC_INVALID_MESSAGE;
		return -1;
	}

	return 0;
}


/**
 * Real implementation to read/write variables
 */
static inline int wrnc_rt_variable(struct wrnc_dev *wrnc,
				   unsigned int hmq_in, unsigned int hmq_out,
				   uint8_t msg_id_in, uint8_t msg_id_out,
				   uint32_t *variables,
				   unsigned int n_variables,
				   unsigned int sync)
{
	struct wrnc_proto_header hdr;
	struct wrnc_msg msg;
	struct wrnc_hmq *hmq;
	int err;

	hmq = wrnc_hmq_open(wrnc, hmq_in, WRNC_HMQ_INCOMING);
	if (!hmq)
		return -1;

	memset(&msg, 0, sizeof(struct wrnc_msg));
	memset(&hdr, 0, sizeof(struct wrnc_proto_header));
	hdr.msg_id = msg_id_in;
	hdr.slot_io = (hmq->index << 4) | (hmq_out & 0xF);
	hdr.len = n_variables * 2;
	hdr.flags = sync ? WRNC_PROTO_FLAG_SYNC : 0;

	/* Send asynchronous message, we do not wait for answers  */
	wrnc_message_pack(&msg, &hdr, variables);
	if (hdr.flags & WRNC_PROTO_FLAG_SYNC) {
		err = wrnc_hmq_send_and_receive_sync(hmq, hmq_out, &msg, 1000);
		wrnc_message_unpack(&msg, &hdr, variables);
		/* Check if it is the answer that we are expecting */
		if (!err && hdr.msg_id != msg_id_out)
			err = -1;
	} else {
		err = wrnc_hmq_send(hmq, &msg);
	}
	wrnc_hmq_close(hmq);

	return err;
}


/**
 * It sends/receive a set of variables to/from the Real-Time application.
 *
 * The 'variables' field data format is the following
 *
 *     0     1     2     3     4     5    ...
 *  +-----+-----+-----+-----+-----+-----+
 *  | IDX | VAL | IDX | VAL | IDX | VAL | ...
 *  +-----+-----+-----+-----+-----+-----+
 *
 * IDX is the variable index defined by the real-time application
 * VAL is the associated value
 *
 * By setting the flag 'sync' you will send a synchronous message, otherwise
 * it is asyncrhonous. When synchronous the 'variables' field will be
 * overwritten by the syncrhonous answer; the answer contains the read back
 * values for the requested variable after the set operation. You can use
 * this to verify. You can use synchrounous messages to verify that you
 * variable are properly set.
 * @param[in] wrnc device token
 * @param[in] hmq_in input slot index [0, 15]
 * @param[in] hmq_out output slot index [0, 15]
 * @param[in|out] variables
 * @param[in] n_variables number of variables to set. In other words,
 *            the number of indexes you have in the 'variables' fields.
 * @param[in] sync set if you want a synchronous message
 */
int wrnc_rt_variable_set(struct wrnc_dev *wrnc,
			 unsigned int hmq_in, unsigned int hmq_out,
			 uint32_t *variables,
			 unsigned int n_variables,
			 unsigned int sync)
{
	return wrnc_rt_variable(wrnc, hmq_in, hmq_out,
				RT_ACTION_RECV_FIELD_SET,
				RT_ACTION_SEND_FIELD_GET,
				variables, n_variables, sync);
}


/**
 * It receive a set of variables from the Real-Time application.
 *
 * The 'variables' field data format is the following
 *
 *     0     1     2     3     4     5    ...
 *  +-----+-----+-----+-----+-----+-----+
 *  | IDX | VAL | IDX | VAL | IDX | VAL | ...
 *  +-----+-----+-----+-----+-----+-----+
 *
 * IDX is the variable index defined by the real-time application
 * VAL is the associated value
 *
 * This kind of message is always synchronous. The 'variables' field will be
 * overwritten by the syncrhonous answer; the answer contains the read back
 * values for the requested variables.
 * @param[in] wrnc device token
 * @param[in] hmq_in input slot index [0, 15]
 * @param[in] hmq_out output slot index [0, 15]
 * @param[in|out] variables
 * @param[in] n_variables number of variables to set. In other words,
 *            the number of indexes you have in the 'variables' fields
 */
int wrnc_rt_variable_get(struct wrnc_dev *wrnc,
			 unsigned int hmq_in, unsigned int hmq_out,
			 uint32_t *variables,
			 unsigned int n_variables)
{
	return wrnc_rt_variable(wrnc, hmq_in, hmq_out,
				RT_ACTION_RECV_FIELD_GET,
				RT_ACTION_SEND_FIELD_GET,
				variables, n_variables, 1);
}


/**
 * Real implementation to read/write structures
 */
static int wrnc_rt_structure(struct wrnc_dev *wrnc,
			     unsigned int hmq_in, unsigned int hmq_out,
			     uint8_t msg_id_in, uint8_t msg_id_out,
			     struct wrnc_structure_tlv *tlv,
			     unsigned int n_tlv,
			     unsigned int sync)
{
	struct wrnc_proto_header hdr;
	struct wrnc_msg msg;
	struct wrnc_hmq *hmq;
	int err, i;

	hmq = wrnc_hmq_open(wrnc, hmq_in, WRNC_HMQ_INCOMING);
	if (!hmq)
		return -1;

	memset(&msg, 0, sizeof(struct wrnc_msg));
	memset(&hdr, 0, sizeof(struct wrnc_proto_header));
	hdr.msg_id = msg_id_in;
	hdr.slot_io = (hmq->index << 4) | (hmq_out & 0xF);
	hdr.flags = sync ? WRNC_PROTO_FLAG_SYNC : 0;

	/* Send asynchronous message, we do not wait for answers  */
	for (i = 0; i < n_tlv; ++i)
		wrnc_message_structure_push(&msg, &hdr, &tlv[i]);

	if (hdr.flags & WRNC_PROTO_FLAG_SYNC) {
		err = wrnc_hmq_send_and_receive_sync(hmq, hmq_out, &msg, 1000);
		for (i = 0; i < n_tlv; ++i)
			wrnc_message_structure_pop(&msg, &hdr, &tlv[i]);
		/* Check if it is the answer that we are expecting */
		if (!err && hdr.msg_id != msg_id_out)
			err = -1;
	} else {
		err = wrnc_hmq_send(hmq, &msg);
	}
	wrnc_hmq_close(hmq);

	return err;
}


/**
 * It sends/receives a set of structures within TLV records
 * @param[in] wrnc device token
 * @param[in] hmq_in input slot index [0, 15]
 * @param[in] hmq_out output slot index [0, 15]
 * @param[in|out] structures
 */
int wrnc_rt_structure_set(struct wrnc_dev *wrnc,
			  unsigned int hmq_in, unsigned int hmq_out,
			  struct wrnc_structure_tlv *tlv, unsigned int n_tlv,
			  unsigned int sync)
{
	return wrnc_rt_structure(wrnc, hmq_in, hmq_out,
				 RT_ACTION_RECV_STRUCT_SET,
				 RT_ACTION_SEND_STRUCT_GET,
				 tlv, n_tlv, sync);
}

/**
 * It receives a set of structures within TLV records
 * @param[in] wrnc device token
 * @param[in] hmq_in input slot index [0, 15]
 * @param[in] hmq_out output slot index [0, 15]
 * @param[in|out] structures
 */
int wrnc_rt_structure_get(struct wrnc_dev *wrnc,
			  unsigned int hmq_in, unsigned int hmq_out,
			  struct wrnc_structure_tlv *tlv, unsigned int n_tlv)
{
	return wrnc_rt_structure(wrnc, hmq_in, hmq_out,
				 RT_ACTION_RECV_STRUCT_GET,
				 RT_ACTION_SEND_STRUCT_GET,
				 tlv, n_tlv, 1);
}
