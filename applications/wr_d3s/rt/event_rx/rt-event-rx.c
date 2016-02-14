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
 * rt-event-rx.c: Simple event receiver.
 */

#include <string.h>

#include "wrtd-serializers.h"
#include "wr-d3s-common.h"

#include "shared_vars.h"
#include "hw/dds_regs.h"

struct latency_tracker {
    uint32_t lmax;
    uint32_t avg_sum;
    uint32_t avg_count;
    uint32_t avg_n;
    uint32_t avg_current;
};

void ltracker_init(struct latency_tracker *lt, int n_avg)
{
    lt->lmax = 0;
    lt->avg_sum = 0;
    lt->avg_count = n_avg;
    lt->avg_n = 0;
}

void ltracker_update(struct latency_tracker *lt,uint32_t origin_tai, uint32_t origin_cycles)
{
    int32_t ctai = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
    int32_t ccyc = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
    
    ccyc -= origin_cycles;
    if(ccyc < 0)
    {
	ccyc += 125000000;
    }

    if(ccyc > lt->lmax)
	lt->lmax = ccyc;
	
    lt->avg_sum += ccyc;
    lt->avg_n ++;

    if(lt->avg_count == lt->avg_n)
    {
	lt->avg_current = lt->avg_sum / lt->avg_count;
	lt->avg_n = 0;
	lt->avg_sum = 0;
    }
}

struct latency_tracker ltrack;

void schedule_pulse(uint32_t rf_cycles)
{
    dp_writel(rf_cycles, DDS_REG_PULSE_OUT_CYCLES);
    dp_writel(DDS_PULSE_OUT_CSR_ARM, DDS_REG_PULSE_OUT_CSR);
}

#define DELAY 10000 /* aprox 220 us @ 44 MHz */

void do_rx()
{
    if (rmq_poll( WR_D3S_REMOTE_IN_EVENTS )) {
        struct wr_rf_event *msg = mq_map_in_buffer (1, WR_D3S_REMOTE_IN_EVENTS) - sizeof(struct rmq_message_addr);
    
	ltracker_update(&ltrack, msg->transmit_seconds, msg->transmit_cycles);



	if(smem_rf_ok)
	{
	    pp_printf("Got event [%x] seq %d rf %d latency max %d latency avg %d", msg->event_id, msg->seq, smem_rf_ok,ltrack.lmax, ltrack.avg_current);        
    	    schedule_pulse(msg->origin_rf_ticks + DELAY);
	}
	mq_discard(1, WR_D3S_REMOTE_IN_EVENTS ) ;
    }                    
}

void xdelay(int ticks)
{
    lr_writel(ticks, WRN_CPU_LR_REG_DELAY_CNT);
    
    while(lr_readl( WRN_CPU_LR_REG_DELAY_CNT));
}

int main()
{
	ltracker_init(&ltrack, 10);
	
	for(;;)
	{
	    do_rx();
	}

	
	return 0;
}


