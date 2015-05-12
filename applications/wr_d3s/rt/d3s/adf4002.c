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
 * adf4002.c: RT driver for the ADF4002 phase detector on the FMC DDS v2 mezzanine.
 */


#include "rt-d3s.h"

void adf4002_write(uint32_t value)
{
    gpior_set(DDS_GPIOR_ADF_CLK, 0);
    delay(10);
    gpior_set(DDS_GPIOR_ADF_LE, 0);
    delay(10);
    int i;
    for(i=0;i<24;i++)
    {
	value <<= 1;
	gpior_set(DDS_GPIOR_ADF_DATA, value & (1<<24) ? 1 : 0);
        delay(10);
        gpior_set(DDS_GPIOR_ADF_CLK, 1);
        delay(10);
        gpior_set(DDS_GPIOR_ADF_CLK, 0);
    }

    gpior_set(DDS_GPIOR_ADF_LE, 1);
    delay(10);
}

void adf4002_configure(int r_div, int n_div, int mon_output)
{
    gpior_set(DDS_GPIOR_ADF_CE, 1); /* enable the PD */
    delay(10);
 
    adf4002_write(0 | (r_div << 2));
    adf4002_write(1 | (n_div << 8));
    adf4002_write(2 | (7<<15) | (7<<18) | (0<<7) | ( mon_output << 4));
}
