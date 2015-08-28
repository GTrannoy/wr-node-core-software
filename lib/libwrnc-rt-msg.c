/*
 * Copyright (C) 2015 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *
 * Released according to the GNU GPL, version 3
 */

#include <libwrnc.h>
#include <errno.h>

/**
 * It embeds the header into the message
 * @param[in] msg the wrnc message
 * @param[in] hdr the header you want to embed into the message
 */
void wrnc_message_header_set(struct wrnc_msg *msg,
			     struct wrnc_proto_header *hdr)
{
	uint32_t *hdr32 = (uint32_t *) hdr;

	msg->data[0] = htobe32((hdr32[0] & 0xFFFF0000) |
			       ((hdr32[0] >> 8) & 0x00FF) |
			       ((hdr32[0] << 8) & 0xFF00));
	msg->data[1] = hdr32[1];
	msg->data[2] = htobe32(hdr32[2]);
	msg->data[3] = hdr32[3];
	msg->datalen = sizeof(struct wrnc_proto_header) / 4;
	msg->max_size = msg->datalen;
}

/**
 * It retrieves the header from the message
 * @param[in] msg the wrnc message
 * @param[out] hdr the header from the message
 */
void wrnc_message_header_get(struct wrnc_msg *msg,
			     struct wrnc_proto_header *hdr)
{
	uint32_t *hdr32 = (uint32_t *) hdr;

	hdr32[0] = be32toh((msg->data[0] & 0x0000FFFF) |
			   ((msg->data[0] >> 8) & 0x00FF0000) |
			   ((msg->data[0] << 8) & 0xFF000000));
	hdr32[1] = msg->data[1];
	hdr32[2] = be32toh(msg->data[2]);
	hdr32[3] = msg->data[3];
}


/**
 * It packs a message to send to the HMQ. The function uses the header to get
 * the payload size. Rembember that the payload length unit is the 32bit word.
 * Remind also that the WRNC VHDL code, will convert a given 32bit word between
 * little endian and big endian
 * @param[out] msg raw message
 * @param[in] hdr message header
 * @param[in] payload data
 */
void wrnc_message_pack(struct wrnc_msg *msg,
		       struct wrnc_proto_header *hdr,
		       void *payload)
{
	void *data = msg->data;

	wrnc_message_header_set(msg, hdr);
	if (!payload)
		return;
	memcpy(data + sizeof(struct wrnc_proto_header), payload,
	       hdr->len * 4);
	msg->datalen += hdr->len;
	msg->max_size = msg->datalen;
}


/**
 * it unpacks a message coming from the HMQ by separating the header from
 * the payload. You will find the payload size in the header.
 * Rembember that the payload length unit is the 32bit word.
 * Remind also that the WRNC VHDL code, will convert a given 32bit word between
 * little endian and big endian
 * @param[in] msg raw message
 * @param[out] hdr message header
 * @param[out] payload data
 */
void wrnc_message_unpack(struct wrnc_msg *msg,
			 struct wrnc_proto_header *hdr,
			 void *payload)
{
	void *data = msg->data;

	wrnc_message_header_get(msg, hdr);
	if (!payload)
		return;
	memcpy(payload, data + sizeof(struct wrnc_proto_header),
	       hdr->len * 4);
}



void wrnc_message_structure_push(struct wrnc_msg *msg,
				 struct wrnc_proto_header *hdr,
				 struct wrnc_structure_tlv *tlv)
{
	unsigned int offset = sizeof(struct wrnc_proto_header) / 4 + hdr->len;
	void *data;

	msg->data[offset++] = tlv->index;
	msg->data[offset++] = tlv->size;
	data = &msg->data[offset];

	memcpy(data, tlv->structure, tlv->size);

	hdr->len += 2 + (tlv->size / 4);

	wrnc_message_header_set(msg, hdr);
	msg->datalen = hdr->len + sizeof(struct wrnc_proto_header) / 4;
}

/**
 * A TLV record containing a structure will be take from the message head.
 * The function will update the message lenght in the header by removing
 * the size occupied by the last record.
 * @param[in] msg raw message
 * @param[in] hdr message header
 * @param[out] tlv TLV record containing a structure
 */
void wrnc_message_structure_pop(struct wrnc_msg *msg,
				struct wrnc_proto_header *hdr,
				struct wrnc_structure_tlv *tlv)
{
	unsigned int offset = sizeof(struct wrnc_proto_header) / 4;
	void *data;

	wrnc_message_header_get(msg, hdr);

	if (hdr->len < 3)
		return; /* there is nothing to read */

	tlv->index = msg->data[offset++];
	tlv->size = msg->data[offset++];
	if (tlv->size / 4 > hdr->len - 2)
		return; /* TLV greater than what is declared in header */

	data = &msg->data[offset];
	memcpy(tlv->structure, data, tlv->size);
	hdr->len = hdr->len - (tlv->size / 4) - 2; /* -2 because of index
						      and size in TLV */

	/* shift all data - +8 because of index and size which are uint32_t */
	memcpy(data, data + tlv->size + 8, hdr->len * 4);
}

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