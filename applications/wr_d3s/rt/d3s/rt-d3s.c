/*
 * This work is part of the White Rabbit Node Core project.
 *
 * Copyright (C) 2013-2015 CERN (www.cern.ch)
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * Released according to the GNU GPL, version 2 or any later version.
 */


/*.
 * WR Distributed DDS Realtime Firmware.
 *
 * rt-d3s.c: Main RT firnmware for the Distributed DDS system.
 */

#include <string.h>

#include "rt-d3s.h"

/* Forces a software reset of the DDS core. May be only run
   AFTER the AD9516 has been programmed and locked */
void d3s_reset()
{

/* First, reset the DDS DAC serdes PLL */
    dp_writel(DDS_RSTR_PLL_RST, DDS_REG_RSTR);
    delay(1000);
    dp_writel(0, DDS_REG_RSTR);
    delay(1000);
    while(! gpior_get(DDS_GPIOR_SERDES_PLL_LOCKED) );

    dp_writel(DDS_RSTR_SW_RST, DDS_REG_RSTR); // trigger a software reset
    delay(1000);
    dp_writel(0, DDS_REG_RSTR);
    delay(1000);

    pp_printf("Reset done\n");

    delay(1000000);

    /* set DDS center frequency to 44 MHz (use scripts/calc_tune.py) */
    dp_writel(0x2d0, DDS_REG_FREQ_HI);
    dp_writel(0xe5604189, DDS_REG_FREQ_LO);
    /* Tuning gain = 1 */
    dp_writel(1<<12, DDS_REG_GAIN);

    pp_printf("FreqHi %x\n", dp_readl(DDS_REG_FREQ_HI));
    pp_printf("FreqLo %x\n", dp_readl(DDS_REG_FREQ_LO));
}

void d3s_set_tune(uint64_t tune)
{

}

void init() 
{
	ad9516_init();

	/* Setup for 352 MHz RF reference: divider = 8 (44 MHz internal/DDS frequency) */

	ad9510_init();


	d3s_reset();

	/* Set up the phase detector to work at ~15 MHz (44 MHz / 3) */
        adf4002_configure(2,2,4);

	pp_printf("RT_D3S firmware initialized.\n");

}

int integ = 0;
int kp = 30000;
int ki = 150;

int control_update(int phase_error)
{
    int err = -(phase_error - 2048);

    integ += err;

    int y = (kp * err + ki * integ) >> 10;

    return y;
}

void main_loop()
{

    /* enable sampling, set divider */

    dp_writel(DDS_CR_SAMP_EN | DDS_CR_SAMP_DIV_W(100), DDS_REG_CR);

    for(;;)
    {
	uint32_t pd_data;
	do {
	    pd_data = dp_readl(DDS_REG_PD_DATA);
	} while ( !( pd_data & DDS_PD_DATA_VALID));

	/* clear valid flag */
	dp_writel(0, DDS_REG_PD_DATA);

	int y = control_update(pd_data & 0xffff);

	dp_writel(y, DDS_REG_TUNE_VAL);

    }

}	


int main()
{
	init();

	main_loop();



#if 0
        dp_writel(1000000, DDS_REG_FREQ_MEAS_GATE);

    

	for(;;)
	{
	    int locked = dp_readl(DDS_REG_GPIOR) & DDS_GPIOR_SERDES_PLL_LOCKED ? 1 : 0;
	    int ref_freq = dp_readl(DDS_REG_FREQ_MEAS_VCXO_REF);
	    pp_printf("l: %d  f:%d [%x]\n", locked,ref_freq,ad95xx_read_reg(0,0x1f));

    	    delay_us(500000);
    	    delay_us(500000);
	    adf4002_configure(3,3,4);
	}

#endif

	
	return 0;
}