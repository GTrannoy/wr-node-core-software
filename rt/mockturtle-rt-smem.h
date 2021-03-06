/*
 * This work is part of the White Rabbit Node Core project.
 *
 * Copyright (C) 2013-2014 CERN (www.cern.ch)
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * Released according to the GNU GPL, version 2 or any later version.
 */

/*.
 * White Rabbit Node Core
 *
 * rt-smem.h: Shared Memory definitions & API
 */

#ifndef __WRNODE_SMEM_H
#define __WRNODE_SMEM_H

#define SMEM_RANGE_ADD		0x10000
#define SMEM_RANGE_SUB		0x20000
#define SMEM_RANGE_SET		0x30000
#define SMEM_RANGE_CLEAR	0x40000
#define SMEM_RANGE_FLIP		0x50000

#define SMEM volatile __attribute__((section(".smem")))

/**
 * Perform an operation on a given pointer. Operation can be performed only
 * on integer variables
 */
static inline void __smem_atomic_op(volatile int *p, int x, unsigned int range)
{
	*(volatile int *)(p + (range >> 2)) = x;
}


/**
 * Add x to the value in the shared memory pointed by p
 */
static inline void smem_atomic_add(volatile int *p, int x)
{
	__smem_atomic_op(p, x, SMEM_RANGE_ADD);
}


/**
 * Subtract x to the value in the shared memory pointed by p
 */
static inline void smem_atomic_sub(volatile int *p, int x)
{
	__smem_atomic_op(p, x, SMEM_RANGE_SUB);
}


/**
 * Do "OR" between x and the value in the shared memory pointed by p
 */
static inline void smem_atomic_or(volatile int *p, int x)
{
	__smem_atomic_op(p, x, SMEM_RANGE_SET);
}


/**
 * Do "AND ~" between x and the value in the shared memory pointed by p
 */
static inline void smem_atomic_and_not(volatile int *p, int x)
{
	__smem_atomic_op(p, x, SMEM_RANGE_CLEAR);
}


/**
 * Do "XOR" between x and the value in the shared memory pointed by p
 */
static inline void smem_atomic_xor(int *p, int x)
{
	__smem_atomic_op(p, x, SMEM_RANGE_FLIP);
}

#endif
