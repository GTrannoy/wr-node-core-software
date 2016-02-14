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
 * rt-event-tx.c: Simple timing master, sending a single event after receving a pulse.
 */

#include <string.h>

#include "wrtd-serializers.h"
#include "wr-d3s-common.h"

#include "hw/dds_regs.h"
#include "shared_vars.h"

static int seq=0;

static inline void send_event( int id, uint32_t origin_rf_ticks )
{
    volatile struct wr_rf_event *msg = mq_map_out_buffer(1, WR_D3S_REMOTE_OUT_EVENTS );

    mq_claim(1, WR_D3S_REMOTE_OUT_EVENTS);

    msg->hdr.target_ip = 0xffffffff;    /* broadcast */
    msg->hdr.target_port = 0xebd0;      /* port */
    msg->hdr.target_offset = 0x4400;    /* target EB slot (1) */

    /* Embed transmission time for latency measyurement */
    msg->transmit_seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
    msg->transmit_cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
    msg->event_id = id;
    msg->seq = seq++;
    msg->origin_rf_ticks = origin_rf_ticks;

    mq_send(1, WR_D3S_REMOTE_OUT_EVENTS, 32);
}

void xdelay(int ticks)
{
    lr_writel(ticks, WRN_CPU_LR_REG_DELAY_CNT);
    
    while(lr_readl( WRN_CPU_LR_REG_DELAY_CNT));
}

int main()
{

	int armed = 0;
	
	for(;;)
	{
	    if(smem_rf_ok)
	    {	
		if(!armed)
		{
		    dp_writel(DDS_TRIG_IN_CSR_ARM, DDS_REG_TRIG_IN_CSR);
		    armed = 1;
		}
		else 
		{
		    if (dp_readl(DDS_REG_TRIG_IN_CSR) & DDS_TRIG_IN_CSR_DONE)
		    {
			uint32_t rf_cycles = dp_readl(DDS_REG_TRIG_IN_SNAPSHOT);

			send_event(0xdeadbeef, rf_cycles);

			pp_printf("Got a trigger pulse @ %d RF cycles, sending an event!", rf_cycles);
			
			armed = 0;
		    }
		}
	    } 
	}
	
	return 0;
}


