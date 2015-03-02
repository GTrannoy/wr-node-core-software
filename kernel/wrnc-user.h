/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */
#ifndef __WRNC_USER_H__
#define  __WRNC_USER_H__

#define WRNC_MAX_CARRIER 20 /**< Maximum number of WRNC components on a
			       single computer*/
#define WRNC_MAX_CPU 8 /**< Maximum number of CPU core in a WRNC bitstream */
#define WRNC_MAX_HMQ_SLOT 32 /**< Maximum number of HMQ slots in a
				WRNC bitstream */

#define WRNC_MAX_PAYLOAD_SIZE 128

enum wrnc_smem_modifier {
	WRNC_SMEM_DIRECT = 0, /**< direct read/write of the memory */
	WRNC_SMEM_OR, /**< on write, atomic OR with memory content */
	WRNC_SMEM_CLR_AND, /**< on write, atomic AND with complemented memory
			      content */
	WRNC_SMEM_XOR, /**< on write, atomic XOR with memory content */
	WRNC_SMEM_ADD, /**< on write, atomic ADD to memory content */
};

/**
 * Messages directions
 */
enum wrnc_msg_direction {
    WRNC_MSG_DIR_SEND = 0,
    WRNC_MSG_DIR_RECEIVE = 1
};

/**
 * Messages descriptor
 */
struct wrnc_msg {
	uint32_t datalen; /**< payload length*/
	uint32_t data[WRNC_MAX_PAYLOAD_SIZE]; /**< payload */
	uint32_t max_size; /**< maximum message size for chosen slot */
	uint32_t offset; /**< serialization/deserialization offset */
	enum wrnc_msg_direction direction; /**< serialization direction (used by wrnc_msg_x functions) */
	int error; /** serialization error status */
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
 * List of available filter's operations
 */
enum wrnc_msg_filter_operation_type {
	WRNC_MSG_FILTER_AND,
	WRNC_MSG_FILTER_OR,
	WRNC_MSG_FILTER_NOT,
	WRNC_MSG_FILTER_EQ,
};

/**
 * It describe a filter to apply to messages
 */
struct wrnc_msg_filter {
	uint32_t operation; /**< kind of operation
							  to perform */
	uint32_t word_offset; /**< offset of the word to check */
	uint32_t mask; /**< mask to apply before the operation */
	uint32_t value; /**< second operand of the operation */
};


/**
 * Descriptor of the IO operation on Shared Memory
 */
struct wrnc_smem_io {
	uint32_t addr; /**< address to access */
	uint32_t value; /**< value to write. After ioctl it will be overwritte
			   with the new value in the shared memory */
	int is_input; /**< flag to determinte data direction */
	enum wrnc_smem_modifier mod;  /**< the kind of operation to do */
};

/**
 * Available ioctl() messages
 */
enum wrnc_ioctl_commands {
        WRNC_MSG_SYNC, /**< send a synchronous message */
	WRNC_SMEM_IO, /**< access to shared memory */
	WRNC_MSG_FILTER_ADD, /**< add a message filter */
	WRNC_MSG_FILTER_CLEAN, /**< remove all filters */
};


#define WRNC_IOCTL_MAGIC 's'
#define WRNC_IOCTL_MSG_SYNC _IOWR(WRNC_IOCTL_MAGIC, WRNC_MSG_SYNC, \
				    struct wrnc_msg_sync)
#define WRNC_IOCTL_SMEM_IO _IOWR(WRNC_IOCTL_MAGIC, WRNC_SMEM_IO, \
				    struct wrnc_smem_io)

#define WRNC_IOCTL_MSG_FILTER_ADD _IOW(WRNC_IOCTL_MAGIC,	\
				       WRNC_MSG_FILTER_ADD,	\
				       struct wrnc_msg_filter)
#define WRNC_IOCTL_MSG_FILTER_CLEAN _IOW(WRNC_IOCTL_MAGIC,		\
					 WRNC_MSG_FILTER_CLEAN,		\
					 struct wrnc_msg_filter)
#endif
