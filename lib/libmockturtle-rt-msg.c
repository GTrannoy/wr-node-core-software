/*
 * Copyright (C) 2015 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *
 * Released according to the GNU GPL, version 3
 */

#include <libmockturtle.h>
#include <errno.h>

/**
 * It embeds the header into the message
 * @param[in] msg the trtl message
 * @param[in] hdr the header you want to embed into the message
 */
void trtl_message_header_set(struct trtl_msg *msg,
			     struct trtl_proto_header *hdr)
{
	uint32_t *hdr32 = (uint32_t *) hdr;

	msg->data[0] = htobe32((hdr32[0] & 0xFFFF0000) |
			       ((hdr32[0] >> 8) & 0x00FF) |
			       ((hdr32[0] << 8) & 0xFF00));
	msg->data[1] = hdr32[1];
	msg->data[2] = htobe32(hdr32[2]);
	msg->data[3] = hdr32[3];
	msg->datalen = sizeof(struct trtl_proto_header) / 4;
	msg->max_size = msg->datalen;
}

/**
 * It retrieves the header from the message
 * @param[in] msg the trtl message
 * @param[out] hdr the header from the message
 */
void trtl_message_header_get(struct trtl_msg *msg,
			     struct trtl_proto_header *hdr)
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
void trtl_message_pack(struct trtl_msg *msg,
		       struct trtl_proto_header *hdr,
		       void *payload)
{
	void *data = msg->data;

	trtl_message_header_set(msg, hdr);
	if (!payload)
		return;
	memcpy(data + sizeof(struct trtl_proto_header), payload,
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
void trtl_message_unpack(struct trtl_msg *msg,
			 struct trtl_proto_header *hdr,
			 void *payload)
{
	void *data = msg->data;

	trtl_message_header_get(msg, hdr);
	if (!payload)
		return;
	memcpy(payload, data + sizeof(struct trtl_proto_header),
	       hdr->len * 4);
}



void trtl_message_structure_push(struct trtl_msg *msg,
				 struct trtl_proto_header *hdr,
				 struct trtl_structure_tlv *tlv)
{
	unsigned int offset = sizeof(struct trtl_proto_header) / 4 + hdr->len;
	void *data;

	msg->data[offset++] = tlv->index;
	msg->data[offset++] = tlv->size;
	data = &msg->data[offset];

	memcpy(data, tlv->structure, tlv->size);

	hdr->len += 2 + (tlv->size / 4);

	trtl_message_header_set(msg, hdr);
	msg->datalen = hdr->len + sizeof(struct trtl_proto_header) / 4;
}

/**
 * A TLV record containing a structure will be take from the message head.
 * The function will update the message lenght in the header by removing
 * the size occupied by the last record.
 * @param[in] msg raw message
 * @param[in] hdr message header
 * @param[out] tlv TLV record containing a structure
 */
void trtl_message_structure_pop(struct trtl_msg *msg,
				struct trtl_proto_header *hdr,
				struct trtl_structure_tlv *tlv)
{
	unsigned int offset = sizeof(struct trtl_proto_header) / 4;
	void *data;

	trtl_message_header_get(msg, hdr);

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
 * @param[in] trtl device token
 * @param[out] version FPGA, RT and GIT version
 * @param[in] hmq_in hmq slot index where send the message
 * @param[in] hmq_out hmq slot index where you expect the answer
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int trtl_rt_version_get(struct trtl_dev *trtl, struct trtl_rt_version *version,
			unsigned int hmq_in, unsigned int hmq_out)
{
	struct trtl_proto_header hdr;
	struct trtl_hmq *hmq;
	struct trtl_msg msg;
	int err;

	memset(&msg, 0, sizeof(struct trtl_msg));
	memset(&hdr, 0, sizeof(struct trtl_proto_header));
	hdr.msg_id = RT_ACTION_RECV_VERSION;
	hdr.slot_io = (hmq_in << 4) | hmq_out;
	hdr.flags = TRTL_PROTO_FLAG_SYNC;
	hdr.len = 0;
	trtl_message_pack(&msg, &hdr, NULL);

	hmq = trtl_hmq_open(trtl, hmq_in, TRTL_HMQ_INCOMING);
	if (!hmq)
		return -1;

	/* Send the message and get answer */
        err = trtl_hmq_send_and_receive_sync(hmq, hmq_out, &msg,
					     trtl_default_timeout_ms);
	trtl_hmq_close(hmq);
	if (err <= 0)
		return -1;

	trtl_message_unpack(&msg, &hdr, version);
	if (hdr.msg_id != RT_ACTION_SEND_VERSION) {
		errno = ETRTL_INVALID_MESSAGE;
		return -1;
	}

	return 0;
}


/**
 * It checks if a Real-Time application is running and answering to messages
 * @param[in] trtl device token
 * @param[in] hmq_in hmq slot index where send the message
 * @param[in] hmq_out hmq slot index where you expect the answer
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int trtl_rt_ping(struct trtl_dev *trtl,
		 unsigned int hmq_in, unsigned int hmq_out)
{
	struct trtl_proto_header hdr;
	struct trtl_hmq *hmq;
	struct trtl_msg msg;
	int err;

	memset(&hdr, 0, sizeof(struct trtl_proto_header));
	hdr.msg_id = RT_ACTION_RECV_PING;
	hdr.slot_io = (hmq_in << 4) | hmq_out;
	hdr.flags = TRTL_PROTO_FLAG_SYNC;
	hdr.len = 0;
	trtl_message_pack(&msg, &hdr, NULL);

	hmq = trtl_hmq_open(trtl, hmq_in, TRTL_HMQ_INCOMING);
	if (!hmq)
		return -1;

	/* Send the message and get answer */
        err = trtl_hmq_send_and_receive_sync(hmq, hmq_out, &msg,
					     trtl_default_timeout_ms);
	if (err <= 0)
		return -1;
	trtl_hmq_close(hmq);
	trtl_message_unpack(&msg, &hdr, NULL);
	if (hdr.msg_id != RT_ACTION_SEND_ACK) {
		errno = ETRTL_INVALID_MESSAGE;
		return -1;
	}

	return 0;
}


/**
 * Real implementation to read/write variables
 */
static inline int trtl_rt_variable(struct trtl_dev *trtl,
				   struct trtl_proto_header *hdr,
				   uint32_t *variables,
				   unsigned int n_variables)
{
	struct trtl_msg msg;
	struct trtl_hmq *hmq;
	int err;

	hmq = trtl_hmq_open(trtl, (hdr->slot_io >> 4 & 0xF), TRTL_HMQ_INCOMING);
	if (!hmq)
		return -1;

	memset(&msg, 0, sizeof(struct trtl_msg));

	/* Send asynchronous message, we do not wait for answers  */
	trtl_message_pack(&msg, hdr, variables);
	if (hdr->flags & TRTL_PROTO_FLAG_SYNC) {
		err = trtl_hmq_send_and_receive_sync(hmq, (hdr->slot_io & 0xF),
						     &msg, 1000);
		if (err > 0)
			trtl_message_unpack(&msg, hdr, variables);
	} else {
		err = trtl_hmq_send(hmq, &msg);
	}
	trtl_hmq_close(hmq);

	return err <= 0 ? -1 : 0;
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
 * This function will change the header content, in particular it will change
 * the following fields: msg_id, len
 * @param[in] trtl device token
 * @param[in|out] hdr header to send on input, header received on output
 * @param[in|out] var variables to get on input, variables values on output
 * @param[in] n_var number of variables to set. In other words,
 *            the number of indexes you have in the 'variables' fields.
 * @param[in] sync set if you want a synchronous message
 */
int trtl_rt_variable_set(struct trtl_dev *trtl,
			 struct trtl_proto_header *hdr,
			 uint32_t *var,
			 unsigned int n_var)
{
	int err;

	hdr->msg_id = RT_ACTION_RECV_FIELD_SET;
	hdr->len = n_var * 2;

	err = trtl_rt_variable(trtl, hdr, var, n_var);
	if (err)
		return err;
	if ((hdr->flags & TRTL_PROTO_FLAG_SYNC) &&
	    hdr->msg_id != RT_ACTION_SEND_FIELD_GET) {
		errno = ETRTL_INVALID_MESSAGE;
		return -1;
	}

	return 0;
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
 * This function will change the header content, in particular it will change
 * the following fields: msg_id, flags, len
 * @param[in] trtl device token
 * @param[in|out] hdr header to send on input, header received on output
 * @param[in|out] var variables to get on input, variables values on output
 * @param[in] n_var number of variables to set. In other words,
 *            the number of indexes you have in the 'variables' fields
 */
int trtl_rt_variable_get(struct trtl_dev *trtl,
			 struct trtl_proto_header *hdr,
			 uint32_t *var,
			 unsigned int n_var)
{
	int err;

	hdr->msg_id = RT_ACTION_RECV_FIELD_GET;
	/* Getting variables is always synchronous */
	hdr->flags |= TRTL_PROTO_FLAG_SYNC;
	hdr->len = n_var * 2;

        err = trtl_rt_variable(trtl, hdr, var, n_var);
	if (err)
		return err;
	if (hdr->msg_id != RT_ACTION_SEND_FIELD_GET) {
		errno = ETRTL_INVALID_MESSAGE;
		return -1;
	}

	return 0;
}


/**
 * Real implementation to read/write structures
 */
static int trtl_rt_structure(struct trtl_dev *trtl,
			     struct trtl_proto_header *hdr,
			     struct trtl_structure_tlv *tlv,
			     unsigned int n_tlv)
{
	struct trtl_msg msg;
	struct trtl_hmq *hmq;
	int err, i;

	hmq = trtl_hmq_open(trtl, (hdr->slot_io >> 4 & 0xF), TRTL_HMQ_INCOMING);
	if (!hmq)
		return -1;

	memset(&msg, 0, sizeof(struct trtl_msg));

	/* Send asynchronous message, we do not wait for answers  */
	for (i = 0; i < n_tlv; ++i)
		trtl_message_structure_push(&msg, hdr, &tlv[i]);

	if (hdr->flags & TRTL_PROTO_FLAG_SYNC) {
		err = trtl_hmq_send_and_receive_sync(hmq, (hdr->slot_io & 0xF),
						     &msg, 1000);
		if (err > 0)
			for (i = 0; i < n_tlv; ++i)
				trtl_message_structure_pop(&msg, hdr, &tlv[i]);
	} else {
		err = trtl_hmq_send(hmq, &msg);
	}
	trtl_hmq_close(hmq);

	return err <= 0 ? -1 : 0;
}


/**
 * It sends/receives a set of structures within TLV records.
 * This function will change the header content, in particular it will change
 * the following fields: msg_id, len
 * @param[in] trtl device token
 * @param[in|out] hdr header to send on input, header received on output
 * @param[in|out] tlv structures to get on input, structures values on output
 * @param[in] n_tlv number of tlv structures
 */
int trtl_rt_structure_set(struct trtl_dev *trtl,
			  struct trtl_proto_header *hdr,
			  struct trtl_structure_tlv *tlv,
			  unsigned int n_tlv)
{
	int err;

	hdr->len = 0;
	hdr->msg_id = RT_ACTION_RECV_STRUCT_SET;

	err = trtl_rt_structure(trtl, hdr, tlv, n_tlv);
	if (err)
		return err;
	if ((hdr->flags & TRTL_PROTO_FLAG_SYNC) &&
	    hdr->msg_id != RT_ACTION_SEND_STRUCT_GET) {
		errno = ETRTL_INVALID_MESSAGE;
		return -1;
	}

	return 0;
}

/**
 * It receives a set of structures within TLV records.
 * This function will change the header content, in particular it will change
 * the following fields: msg_id, flags, len
 * @param[in] trtl device token
 * @param[in|out] hdr header to send on input, header received on output
 * @param[in|out] tlv structures to get on input, structures values on output
 * @param[in] n_tlv number of tlv structures
 */
int trtl_rt_structure_get(struct trtl_dev *trtl,
			  struct trtl_proto_header *hdr,
			  struct trtl_structure_tlv *tlv,
			  unsigned int n_tlv)
{
	int err;

	hdr->len = 0;
	hdr->msg_id = RT_ACTION_RECV_STRUCT_GET;
	/* Getting variables is always synchronous */
	hdr->flags |= TRTL_PROTO_FLAG_SYNC;

        err = trtl_rt_structure(trtl, hdr, tlv, n_tlv);
	if (err)
		return err;
	if (hdr->msg_id != RT_ACTION_SEND_STRUCT_GET) {
		errno = ETRTL_INVALID_MESSAGE;
		return -1;
	}

	return 0;
}
