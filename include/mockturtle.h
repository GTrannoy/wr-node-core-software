/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */
#ifndef __TRT_USER_H__
#define  __TRTL_USER_H__
/** @file mock-turtle.h */

#define TRTL_MAX_CARRIER 20 /**< Maximum number of WRNC components on a
			       single computer*/
#define TRTL_MAX_CPU 8 /**< Maximum number of CPU core in a WRNC bitstream */
#define TRTL_MAX_HMQ_SLOT 32 /**< Maximum number of HMQ slots in a
				WRNC bitstream */

#define TRTL_MAX_PAYLOAD_SIZE 128

/**
 * @enum trtl_smem_modifier
 * Shared memory operation modifier. This is a list of operation modifier
 * that you can use to access the shared memory.
 */
enum trtl_smem_modifier {
	TRTL_SMEM_DIRECT = 0, /**< direct read/write of the memory */
	TRTL_SMEM_ADD, /**< on write, atomic ADD to memory content */
	TRTL_SMEM_SUB, /**< on write, atomic SUB to memory content */
	TRTL_SMEM_OR, /**< on write, atomic OR with memory content */
	TRTL_SMEM_CLR_AND, /**< on write, atomic AND with complemented memory
			      content */
	TRTL_SMEM_XOR, /**< on write, atomic XOR with memory content */
};


/**
 * Messages descriptor
 */
struct trtl_msg {
	uint32_t datalen; /**< payload length*/
	uint32_t data[TRTL_MAX_PAYLOAD_SIZE]; /**< payload, free content
						 (no official protocol) */
};

/**
 * Message descriptor used to send synchronous messages
 */
struct trtl_msg_sync {
	struct trtl_msg *msg; /**< the message to send. It will be overwritten by
				the synchronous answer */
	uint16_t index_in; /**< where write the message */
	uint16_t index_out; /**< where we expect the synchronous answer */
	unsigned int timeout_ms; /**< time to wait for an answer in ms */
};

/**
 * @enum trtl_msg_filter_operation_type
 * List of available filter's operations
 */
enum trtl_msg_filter_operation_type {
	TRTL_MSG_FILTER_AND,
	TRTL_MSG_FILTER_OR,
	TRTL_MSG_FILTER_NOT,
	TRTL_MSG_FILTER_EQ,
};

/**
 * It describe a filter to apply to messages
 */
struct trtl_msg_filter {
	uint32_t operation; /**< kind of operation to perform */
	uint32_t word_offset; /**< offset of the word to check */
	uint32_t mask; /**< mask to apply before the operation */
	uint32_t value; /**< second operand of the operation */
};


/**
 * Descriptor of the IO operation on Shared Memory
 */
struct trtl_smem_io {
	uint32_t addr; /**< address to access */
	uint32_t value; /**< value to write. After ioctl it will be overwritte
			   with the new value in the shared memory */
	int is_input; /**< flag to determinte data direction */
	enum trtl_smem_modifier mod;  /**< the kind of operation to do */
};

/**
 * @enum trtl_ioctl_commands
 * Available ioctl() messages
 */
enum trtl_ioctl_commands {
        TRTL_MSG_SYNC, /**< send a synchronous message */
	TRTL_SMEM_IO, /**< access to shared memory */
	TRTL_MSG_FILTER_ADD, /**< add a message filter */
	TRTL_MSG_FILTER_CLEAN, /**< remove all filters */
};


#define TRTL_IOCTL_MAGIC 's'
#define TRTL_IOCTL_MSG_SYNC _IOWR(TRTL_IOCTL_MAGIC, TRTL_MSG_SYNC, \
				    struct trtl_msg_sync)
#define TRTL_IOCTL_SMEM_IO _IOWR(TRTL_IOCTL_MAGIC, TRTL_SMEM_IO, \
				    struct trtl_smem_io)

#define TRTL_IOCTL_MSG_FILTER_ADD _IOW(TRTL_IOCTL_MAGIC,	\
				       TRTL_MSG_FILTER_ADD,	\
				       struct trtl_msg_filter)
#define TRTL_IOCTL_MSG_FILTER_CLEAN _IOW(TRTL_IOCTL_MAGIC,		\
					 TRTL_MSG_FILTER_CLEAN,		\
					 struct trtl_msg_filter)
#endif
