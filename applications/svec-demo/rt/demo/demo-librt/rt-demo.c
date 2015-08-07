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
#include <demo-common-rt.h>
#include <librt.h>

#define GPIO_CODR	0x0 /* Clear Data Register */
#define GPIO_SODR	0x4 /* Set Data Register */
#define GPIO_DDR	0x8 /* Direction Data Register */
#define GPIO_PSR	0xC /* Status Register */

static uint32_t out_seq = 0;

struct rt_variable demo_variables[] = {
	[DEMO_VAR_LEMO_STA] = {
		.addr = CPU_DP_BASE + GPIO_PSR,
		.mask = PIN_LEMO_MASK,
		.offset = 0,
	},
	[DEMO_VAR_LEMO_DIR] = {
		.addr = CPU_DP_BASE + GPIO_DDR,
		.mask = PIN_LEMO_MASK,
		.offset = 0,
	},
	[DEMO_VAR_LEMO_SET] = {
		.addr = CPU_DP_BASE + GPIO_SODR,
		.mask = PIN_LEMO_MASK,
		.offset = 0,
	},
	[DEMO_VAR_LEMO_CLR] = {
		.addr = CPU_DP_BASE + GPIO_CODR,
		.mask = PIN_LEMO_MASK,
		.offset = 0,
	},
	[DEMO_VAR_LED_STA] = {
		.addr = CPU_DP_BASE + GPIO_PSR,
		.mask = PIN_LED_MASK,
		.offset = 0,
	},
	[DEMO_VAR_LED_SET] = {
		.addr = CPU_DP_BASE + GPIO_SODR,
		.mask = PIN_LED_MASK,
		.offset = 0,
	},
	[DEMO_VAR_LED_CLR] = {
		.addr = CPU_DP_BASE + GPIO_CODR,
		.mask = PIN_LED_MASK,
		.offset = 0,
	},
};


/**
 * It sends messages over the debug interface
 */
static void demo_debug_interface(void)
{
	pp_printf("Hello world.\n");
	pp_printf("We are messages over the debug interface.\n");
	pp_printf("Print here your messages.\n");
}


/**
 * Well, the main :)
 */
int main()
{
	demo_debug_interface();

	rt_variable_export(demo_variables, __DEMO_VAR_MAX);

	/* Register all the action that the RT application can take */
	rt_mq_action_recv_register(RT_ACTION_RECV_PING, DEMO_HMQ_OUT,
				   rt_recv_ping);
	rt_mq_action_recv_register(RT_ACTION_RECV_DATA_SET, DEMO_HMQ_OUT,
				   rt_setter);
	rt_mq_action_recv_register(RT_ACTION_RECV_DATA_GET, DEMO_HMQ_OUT,
				   rt_getter);

	while (1) {
		/* Handle all messages incoming from slot DEMO_HMQ_IN
		   as actions */
		rt_mq_action_dispatch(DEMO_HMQ_IN, 0);
	}

	return 0;
}
