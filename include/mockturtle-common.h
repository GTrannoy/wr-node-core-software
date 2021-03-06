/*
 * Copyright (C) 2015 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *
 * Released according to the GNU GPL, version 3
 */

#ifndef __TRTL_COMMON_H__
#define __TRTL_COMMON_H__

#include <string.h>

#define ARRAY_SIZE(_a) (sizeof(_a) / sizeof(_a[0]))

/**
 * It describes the version running on the embedded CPU
 */
struct trtl_rt_version {
	uint32_t fpga_id; /**< FPGA identifier expected to run the RT app */
	uint32_t rt_id; /**< RT application identifier */
	uint32_t rt_version; /**< RT application version*/
	uint32_t git_version; /**< git commit SHA1 of the compilation time */
};

enum rt_action_standard {
	RT_ACTION_RECV_PING = 0,
	RT_ACTION_RECV_FIELD_SET,
	RT_ACTION_RECV_FIELD_GET,
	RT_ACTION_RECV_STRUCT_SET,
	RT_ACTION_RECV_STRUCT_GET,
	RT_ACTION_RECV_VERSION,
	RT_ACTION_SEND_ACK,
	RT_ACTION_SEND_NACK,
	RT_ACTION_SEND_FIELD_GET,
	RT_ACTION_SEND_STRUCT_GET,
	RT_ACTION_SEND_VERSION,
	__RT_ACTION_RECV_STANDARD_NUMBER,
};

/**< __MAX_ACTION_RECV coming from GCC on compilation */
#define MAX_ACTION_RECV (__MAX_ACTION_RECV + __RT_ACTION_RECV_STANDARD_NUMBER)
/**< __MAX_ACTION_SEND coming from GCC on compilation */
#define MAX_ACTION_SEND (__MAX_ACTION_SEND + __RT_ACTION_SEND_STANDARD_NUMBER)

/* Protocol Definition */

#define TRTL_PROTO_FLAG_REMOTE		(1 << 0)
#define TRTL_PROTO_FLAG_SYNC		(1 << 1)
#define TRTL_PROTO_FLAG_RPC		(1 << 2)
#define TRTL_PROTO_FLAG_PERIODICAL	(1 << 3)

/**
 * Protocol header definition
 */
struct trtl_proto_header {
	uint16_t rt_app_id; /**< Real-Time application unique identifier */
	uint8_t msg_id; /**< Message identifier */
	uint8_t slot_io; /**< Message Queue IO to use
			    (4bit Input, 4 bit output) */
	uint32_t seq; /**< sequence number */
	uint8_t len; /**< message data lenght */
	uint8_t flags; /**< protocol flags */
	uint8_t unused; /**< not used, future use */
	uint8_t trans; /**< transaction descriptor - flag and seq number  */
	uint32_t time;
};


/**
 * Data structure representing a MockTurtle packet
 */
struct mturtle_packet {
	/* offset 0x00 */
	volatile struct trtl_proto_header header; /**< packet header */
	/* offset 0x10 */
	volatile uint32_t payload[]; /**< packet payload  */
};

/**
 * It returns the pointer to the MockTurtle packet
 * @param[in] buf buffer containing the packet
 * @return pointer to the packet
 */
static inline struct mturtle_packet *rt_proto_packet_get(void *buf)
{
	return (struct mturtle_packet *)buf;
}


/**
 * It extracts the header from a raw message
 * @param[in] raw_msg raw message
 * @param[out] header the header from the message
 */
static inline struct trtl_proto_header *rt_proto_header_get(void *raw_msg)
{
	return (struct trtl_proto_header *) raw_msg;
}


/**
 * It embeds the header from a raw message
 * @param[in] raw_msg raw message
 * @param[out] header the header from the message
 */
static inline void rt_proto_header_set(void *raw_msg,
				       struct trtl_proto_header *header)
{
	memcpy(raw_msg, header, sizeof(struct trtl_proto_header));
}


/**
 * It returns the pointer where it starts the message payload
 * @param[in] raw_msg raw message
 * @param[out] header the header from the message
 */
static inline void *rt_proto_payload_get(void *raw_msg)
{
	return ((char *)raw_msg + sizeof(struct trtl_proto_header));
}


#endif
