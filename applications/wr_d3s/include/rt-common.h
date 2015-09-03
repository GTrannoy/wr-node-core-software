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
 * rt-common.h: common WRN CPU definitions and routines
 */

#ifndef __RT_COMMON_H
#define __RT_COMMON_H

#include <stdint.h>
#include <stdio.h>

#include "hw/wrn_cpu_lr.h"

/* Dedicated Peripheral base */
#define CPU_DP_BASE 0x200000

/* CPU Local Registers base */
#define CPU_LR_BASE 0x100000

void rt_set_debug_slot(int slot);

static inline uint32_t dp_readl ( uint32_t reg )
{
  return *(volatile uint32_t *) ( reg + CPU_DP_BASE );
}

static inline void dp_writel ( uint32_t value, uint32_t reg )
{
     *(volatile uint32_t *) ( reg + CPU_DP_BASE ) = value;
}

static inline uint32_t lr_readl ( uint32_t reg )
{
  return *(volatile uint32_t *) ( reg + CPU_LR_BASE );
}

static inline uint32_t lr_writel ( uint32_t value, uint32_t reg )
{
  *(volatile uint32_t *) ( reg + CPU_LR_BASE ) = value;
}

static inline void gpio_set ( int pin )
{
    lr_writel ( (1<<pin), WRN_CPU_LR_REG_GPIO_SET );
}

static inline void gpio_clear ( int pin )
{
    lr_writel ( (1<<pin), WRN_CPU_LR_REG_GPIO_CLEAR );
}

/* fixme: use Timing Unit */
static inline void delay(int n)
{
    int i;
    for(i=0;i<n;i++) asm volatile("nop");
}

static uint32_t _get_ticks()
{
    volatile uint32_t seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
    /* Need to read the seconds to latch the cycles value */
    volatile uint32_t ticks = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
    return ticks;
}

static inline void delay_ticks(uint32_t ticks)
{
    uint32_t start = _get_ticks(); 

    while(1)
    {
	uint32_t v = _get_ticks() - start;
	if (v < 0)
	    v += 125 * 1000 * 1000;
	if (v >= ticks)
	    return;
    }
}

static inline void delay_us(uint32_t usecs)
{
    return delay_ticks(usecs * 125);
}

static inline uint32_t rt_set_delay_counter(uint32_t delay_cnt)
{
    lr_writel(delay_cnt, WRN_CPU_LR_REG_DELAY_CNT);
    return lr_readl( WRN_CPU_LR_REG_DELAY_CNT);
}

static inline uint32_t rt_get_delay_counter()
{
    return lr_readl( WRN_CPU_LR_REG_DELAY_CNT);
}

#endif
