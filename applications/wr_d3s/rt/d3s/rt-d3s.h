#ifndef __RT_D3S_H
#define __RT_D3S_H

#include "rt.h"
#include "hw/dds_regs.h"

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
