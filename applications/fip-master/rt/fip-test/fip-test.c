/*
 * This work is part of the White Rabbit Node Core project.
 *
 * Copyright (C) 2013-2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *
 * Released according to the GNU GPL, version 2 or any later version.
 */

#include <string.h>
#include <rt.h>

#define FIP_BASE 0x10000

void rt_get_time(uint32_t *seconds, uint32_t *cycles)
{
	*seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
	*cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
}

/* magic constants below taken from Eva's Python test program */

void fip_reset()
{
    dp_writel(0xCAFE0003, FIP_BASE + 0x00); // reset inactive
    dp_writel(0xCAFE0000, FIP_BASE + 0x00); // reset active
    dp_writel(0xCAFE0003, FIP_BASE + 0x00); // reset inactive
}

void fip_send_id_dat( uint32_t varid )
{
        dp_writel(0x01, FIP_BASE + 0x2C);  // rst tx
        dp_writel(0x00, FIP_BASE + 0x2C);  // release rst tx
        dp_writel(0x3, FIP_BASE + 0xC4);   // ctrl byte
        dp_writel(varid, FIP_BASE + 0xC8); // varid 2 bytes
        dp_writel(0x202, FIP_BASE + 0x2C); // tx start
}

int fip_rx()
{
	int rx_status = dp_readl( FIP_BASE + 0x3c );
	
	// Poll for a frame
	if(rx_status & (1<<1) )
	{
	    int n_bytes = (rx_status >> 8) & 0x1ff;
	    uint32_t ctrl = dp_readl( FIP_BASE + 0x40 );
	    uint32_t b1 = dp_readl( FIP_BASE + 0x44 );
	    uint32_t b2 = dp_readl( FIP_BASE + 0x48 );
	    uint32_t b3 = dp_readl( FIP_BASE + 0x4c );
	    uint32_t b4 = dp_readl( FIP_BASE + 0x40 );

	    pp_printf("RX bytes %d ctrl %x data %x %x %x %x\n", n_bytes, ctrl, b1, b2, b3, b4);

    	    dp_writel(0x01, FIP_BASE + 0x3C);  // rst rx
    	    dp_writel(0x00, FIP_BASE + 0x3C);  // release rst rx

	}
}

main()
{
    int i = 0;

    fip_reset();
    for(;;)
    {

	if( i == 1000000 )
	{
	    // send a presence request
	    fip_send_id_dat(0x310);
	    i = 0;
	}

	fip_rx();

	i++;
    }
}