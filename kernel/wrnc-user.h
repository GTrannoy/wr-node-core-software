/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */


#define WRNC_MAX_PAYLOAD_SIZE 128

/**
 * Messages descriptor
 */
struct wrnc_msg {
	uint32_t datalen; /**< payload length*/
	uint32_t data[WRNC_MAX_PAYLOAD_SIZE]; /**< payload */
};

/**
 * Message descriptor used to send synchronous messages
 */
struct wrnc_msg_sync {
	struct wrnc_msg msg; /**< the message to send. It will be overwritten by
				the synchronous answer */
	uint16_t index_in; /**< where write the message */
	uint16_t index_out; /**< where we expect the synchronous answer */
	unsigned int timeout_ms; /**< time to wait for an answer in ms */
};

/**
 * Available ioctl() messages
 */
enum ual_ioctl_commands {
        WRNC_MSG_SYNC, /**< send a synchronous message */
};


#define WRNC_IOCTL_MAGIC 's'
#define WRNC_IOCTL_MSG_SYNC _IOWR(WRNC_IOCTL_MAGIC, WRNC_MSG_SYNC, \
				    struct wrnc_msg_sync)
