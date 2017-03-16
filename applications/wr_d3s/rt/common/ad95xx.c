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
 * ad95xx.c: RT driver for AD9516/AD9510 PLL chips on the FMC DDS v2 mezzanine.
 */

#include <stdio.h>
#include <stdint.h>

#include "rt-d3s.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#endif

struct ad95xx_reg {
    uint16_t reg;
    uint8_t val;
};

#include "ad9516_config.h"
#include "ad9516_rfpll_master_config.h"
#include "ad9516_rfpll_slave_config.h"

#define CS_PLL_SYS 0
#define CS_PLL_VCXO 1

int bb_spi_init(void *base_addr)
{
	gpior_set(DDS_GPIOR_PLL_SYS_CS_N, 1);
	gpior_set(DDS_GPIOR_PLL_VCXO_CS_N, 1);
	gpior_set(DDS_GPIOR_PLL_SCLK, 1);

	return 0;
}

/* Trivial bit-banged SPI access. Used to program the PLLs once during
   initialization and not touch them again ;) */
int bb_spi_txrx(int ss, int nbits, uint32_t in, uint32_t *out)
{
    uint32_t cs_mask;
    switch(ss)
    {
	case CS_PLL_SYS: cs_mask = DDS_GPIOR_PLL_SYS_CS_N; break;
	case CS_PLL_VCXO: cs_mask = DDS_GPIOR_PLL_VCXO_CS_N; break;
	default: break;
    }

    gpior_set(cs_mask, 0);
    delay(10);    
    
    int i;
    uint32_t rv = 0;

    for(i=nbits-1; i>=0 ;i--)
    {
	gpior_set(DDS_GPIOR_PLL_SCLK, 0);
	delay(10);
	gpior_set(DDS_GPIOR_PLL_SDIO_DIR, 1);
	gpior_set(DDS_GPIOR_PLL_SDIO, in & (1 << i) ? 1 : 0);
	delay(10);
	gpior_set(DDS_GPIOR_PLL_SCLK, 1);
	delay(10);
	gpior_set(DDS_GPIOR_PLL_SDIO_DIR, 0);
	delay(10);
    	if (gpior_get(DDS_GPIOR_PLL_SDIO))
	    rv |= (1 << i);

    }

    gpior_set(cs_mask, 1);
    delay(10);    

    *out = rv;

    return 0;
}

void ad9516_reset()
{
    gpior_set(DDS_GPIOR_PLL_SYS_RESET_N, 0);
    delay(10);
    gpior_set(DDS_GPIOR_PLL_SYS_RESET_N, 1);
    delay(10);
}


/*
 * AD9516 stuff, using SPI, used by later code.
 * "reg" is 12 bits, "val" is 8 bits, but both are better used as int
 */

void ad95xx_write_reg(int cs, int reg, int val)
{
	bb_spi_txrx(cs, 24, (reg << 8) | val, NULL);
}

int ad95xx_read_reg(int cs, int reg)
{
	uint32_t rval;
	bb_spi_txrx(cs, 24, (reg << 8) | (1 << 23), &rval);
	return rval & 0xff;
}

static void ad95xx_load_regset(int cs, const struct ad95xx_reg *regs, int n_regs)
{
	int i;
	for(i=0; i<n_regs; i++)
		ad95xx_write_reg(cs, regs[i].reg, regs[i].val);
}

static void ad9516_rfpll_calibration()
{
  //set VCO cal bit to 0
  ad95xx_write_reg(CS_PLL_VCXO,0x18, 0x6);
  ad95xx_write_reg(CS_PLL_VCXO,0x232, 0x1); //update registers
  //set VCO cal bit to 1: initiate calibration
  ad95xx_write_reg(CS_PLL_VCXO,0x18, 0x7);
  ad95xx_write_reg(CS_PLL_VCXO,0x232, 0x1); //update registers
  //Wait for calib finishes
  int i;
  for(i=0;i<10;i++)
  {
    uint8_t r= ad95xx_read_reg(CS_PLL_VCXO,0x1f);
    if(r&0x40)
    {
      pp_printf("RF PLL VCO calibration finished...[%x]\n",r);
      break;
    }
    pp_printf("Waiting for RF PLL VCO calibration...[%x]\n",r);
    delay(10000000);
  }
}

static void ad9516_wait_lock(int cs)
{
    int i;
    for(i=0;i<3;i++)    
    {
	uint8_t r= ad95xx_read_reg(cs,0x1f);
	if(r&1)
	{
	  if(cs==CS_PLL_SYS)
	  {
	    pp_printf("WR PLL locked...[%x]\n",r);
	  }
	  else
	  {
	    pp_printf("RF PLL locked...[%x]\n",r);
	  }
	  break;
	}
	if(cs==CS_PLL_SYS)
	{
	  pp_printf("Waiting for WR PLL lock...[%x]\n",r);
	}
	else
	{
	  pp_printf("Waiting for RF PLL lock...[%x]\n",r);
	}
	delay(10000000);
    }
}

#define SECONDARY_DIVIDER 0x100

int ad9516_set_output_divider(int cs, int output, int ratio, int phase_offset)
{
	uint8_t lcycles = (ratio/2) - 1;
	uint8_t hcycles = (ratio - (ratio / 2)) - 1;
	int secondary = (output & SECONDARY_DIVIDER) ? 1 : 0;
	output &= 0xf;

	if(output >= 0 && output < 6) /* LVPECL outputs */
	{
		uint16_t base = (output / 2) * 0x3 + 0x190;

		if(ratio == 1)  /* bypass the divider */
		{
			uint8_t div_ctl = ad95xx_read_reg(cs,base + 1);

			ad95xx_write_reg(cs,base + 1, (1<<7) ); 
			ad95xx_write_reg(cs,base + 2, 1 ); 

		} else {
			uint8_t div_ctl = ad95xx_read_reg(cs,base + 1);

			ad95xx_write_reg(cs,base + 1, (div_ctl & (~(1<<7))) | (phase_offset & 0xf));  /* disable bypass bit */
			ad95xx_write_reg(cs,base, (lcycles << 4) | hcycles);
		}
	} else { /* LVDS/CMOS outputs */
			
		uint16_t base = ((output - 6) / 2) * 0x5 + 0x199;


		if(!secondary)
		{
			if(ratio == 1)  /* bypass the divider 1 */
				ad95xx_write_reg(cs,base + 3, ad95xx_read_reg(cs,base + 3) | 0x10); 
			else if(ratio == 50) {                         // added for WHIST OUT8 at 10 MHz
				ad95xx_write_reg(cs,base + 3, 0x00); // enable both dividers, necessary for ratio > 32
				ad95xx_write_reg(cs,base , (2 << 4) | 1); // div4.1=(2+1) + (1+1)=5
				ad95xx_write_reg(cs,base + 2, (4 << 4) | 4); // div4.2=10= (4+1)+(4+1), 50% duty cycle
			}
			else {
				ad95xx_write_reg(cs,base, (lcycles << 4) | hcycles); // ok for ratios<32
				ad95xx_write_reg(cs,base + 1, phase_offset & 0xf);
			}
		} else {
			if(ratio == 1)  /* bypass the divider 2 */
				ad95xx_write_reg(cs,base + 3, ad95xx_read_reg(cs,base + 3) | 0x20);
			else {
				ad95xx_write_reg(cs,base + 2, (lcycles << 4) | hcycles); // ok for ratios<32
//				ad95xx_write_reg(cs,base + 1, phase_offset & 0xf);
				
			}
		}		
	}

	/* update */
	ad95xx_write_reg(cs,0x232, 0x0);
	ad95xx_write_reg(cs,0x232, 0x1);
	ad95xx_write_reg(cs,0x232, 0x0);
	return 0;
}

/* Sets the VCO divider (2..6) or 0 to enable static output */
int ad9516_set_vco_divider(int cs, int ratio)
{
	if(ratio == 0)
		ad95xx_write_reg(cs,0x1e0, 0x5); /* static mode */
	else
		ad95xx_write_reg(cs,0x1e0, (ratio-2));
	ad95xx_write_reg(cs,0x232, 0x1); //update
	return 0;
}

void ad9516_sync_outputs(int cs)
{
	/* VCO divider: static mode */
	ad95xx_write_reg(cs,0x1E0, 0x7);
	ad95xx_write_reg(cs,0x232, 0x1);

	/* Sync the outputs when they're inactive to avoid +-1 cycle uncertainity */
	ad95xx_write_reg(cs,0x230, 1);
	ad95xx_write_reg(cs,0x232, 1);
	ad95xx_write_reg(cs,0x230, 0);
	ad95xx_write_reg(cs,0x232, 1);
}

int ad9516_wrpll_init()
{
	bb_spi_init(0);

	pp_printf("Initializing AD9516 WR PLL...\n");
	/* Use unidirectional SPI mode */
	ad95xx_write_reg(CS_PLL_SYS,0x000, 0x18);

	/* Check the presence of the chip */
	if (ad95xx_read_reg(CS_PLL_SYS,0x3) != 0xc3) {
		pp_printf("Error: AD9516 WR PLL not responding.\n");
		return -1;
	}


	ad95xx_load_regset(CS_PLL_SYS, ad9516_config, ARRAY_SIZE(ad9516_config));

	ad9516_set_vco_divider(CS_PLL_SYS,3); // vco div 3 = 500 MHz

	ad9516_set_output_divider(CS_PLL_SYS, 0, 1, 1);  	// OUT1. 500 MHz for the DDS
	ad9516_set_output_divider(CS_PLL_SYS, 6, 4, 0);  	// OUT6. 125 MHz for the FPGA
	ad9516_set_output_divider(CS_PLL_SYS, 8, 50, 0);  	// OUT8. 10 MHz for WHIST

	ad9516_wait_lock(CS_PLL_SYS);

	ad9516_sync_outputs(CS_PLL_SYS);
	ad9516_set_vco_divider(CS_PLL_SYS, 3); 

	return 0;
}

int ad9516_rfpll_init(int mode)
{
	bb_spi_init(0);

	pp_printf("Initializing AD9516 RF PLL...\n");
	/* Use unidirectional SPI mode */
	ad95xx_write_reg(CS_PLL_VCXO,0x000, 0x18);

	/* Check the presence of the chip */
	if (ad95xx_read_reg(CS_PLL_VCXO,0x3) != 0xc3) {
		pp_printf("Error: AD9516 RF PLL not responding.\n");
		return -1;
	}

	if(mode == D3S_STREAM_MASTER)
	{
	  ad95xx_load_regset(CS_PLL_VCXO, ad9516_rfpll_master_config, ARRAY_SIZE(ad9516_rfpll_master_config));
	}
	else
	{
	  ad95xx_load_regset(CS_PLL_VCXO, ad9516_rfpll_slave_config, ARRAY_SIZE(ad9516_rfpll_slave_config));
	}

	ad9516_rfpll_calibration();
	
	ad9516_wait_lock(CS_PLL_VCXO);

	ad9516_sync_outputs(CS_PLL_VCXO);
	ad9516_set_vco_divider(CS_PLL_VCXO, 5);

	return 0;
}
