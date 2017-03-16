#ifndef __RT_D3S_H
#define __RT_D3S_H

#include "rt.h"
#include "hw/dds_regs.h"
#include "wr-d3s-common.h"
#include "gqueue.h"

#define DEBUG

#ifdef DEBUG
    #define dbg_printf pp_printf
#else
    #define dbg_printf(...)
#endif


// number of DDS samples by which the phase snapshot is delayed
#define DDS_SNAP_LAG_SAMPLES 3

// DDS accumulator bits
#define DDS_ACC_BITS 43

// Mask for the above bits
#define DDS_ACC_MASK ((1ULL << (DDS_ACC_BITS) ) - 1)

// Maximum reconstruction delay allowed by the slave (in sampling periods)
#define DDS_MAX_SLAVE_DELAY 16

/* Sets/clears the GPIO bits selected by the mask */
static inline void gpior_set(uint32_t mask, int value)
{
    uint32_t gpior = dp_readl(DDS_REG_GPIOR);
    if(value)
        gpior |= mask;
    else
	gpior &= ~mask;

    dp_writel(gpior, DDS_REG_GPIOR);
}

/* Gets the GPIO bit selected by the mask */
static inline int gpior_get(uint32_t mask)
{
    uint32_t gpior = dp_readl(DDS_REG_GPIOR);
    return (gpior & mask) ? 1 : 0;
}

void ad9516_reset();
int ad9516_wrpll_init();
int ad9516_rfpll_init(int mode);
void adf4002_configure(int r_div, int n_div, int mon_output);


int wr_link_up();
int wr_time_locked();
int wr_time_ready();
int wr_enable_lock( int enable );
void wr_update_link();
int wr_is_timing_ok();
int wr_init();

#endif
