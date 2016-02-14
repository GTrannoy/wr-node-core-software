#ifndef __RT_D3S_H
#define __RT_D3S_H

#include "rt.h"
#include "hw/dds_regs.h"

// number of DDS samples by which the phase snapshot is delayed
#define DDS_SNAP_LAG_SAMPLES 3

// DDS accumulator bits
#define DDS_ACC_BITS 43

// Mask for the above bits
#define DDS_ACC_MASK ((1ULL << (DDS_ACC_BITS) ) - 1)

// DDS tuning gain (value written to GAIN register of the DDS core)
#define DDS_TUNE_GAIN 1024

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

int ad9516_init();
int ad9510_init();
void adf4002_configure(int r_div, int n_div, int mon_output);

#endif
