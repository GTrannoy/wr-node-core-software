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

enum rt_slot_name {
	DEMO_CMD_IN = 0,
	DEMO_CMD_OUT,
};

struct rt_mq mq[] = {
	[DEMO_CMD_IN] = {
		.index = 0,
		.flags = RT_MQ_FLAGS_LOCAL | RT_MQ_FLAGS_INPUT,
	},
	[DEMO_CMD_OUT] = {
		.index = 0,
		.flags = RT_MQ_FLAGS_LOCAL | RT_MQ_FLAGS_OUTPUT,
	},
};


static uint32_t out_seq = 0;

static struct demo_structure demo_struct;

struct rt_structure demo_structures[] = {
	[DEMO_STRUCT_TEST] = {
		.struct_ptr = &demo_struct,
		.len = sizeof(struct demo_structure),
	}
};

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
		.flags = RT_VARIABLE_FLAG_WO,
	},
	[DEMO_VAR_LEMO_CLR] = {
		.addr = CPU_DP_BASE + GPIO_CODR,
		.mask = PIN_LEMO_MASK,
		.offset = 0,
		.flags = RT_VARIABLE_FLAG_WO,
	},
	[DEMO_VAR_LED_STA] = {
		.addr = CPU_DP_BASE + GPIO_PSR,
		.mask = PIN_LED_MASK,
		.offset = PIN_LED_OFFSET,
	},
	[DEMO_VAR_LED_SET] = {
		.addr = CPU_DP_BASE + GPIO_SODR,
		.mask = PIN_LED_MASK,
		.offset = PIN_LED_OFFSET,
		.flags = RT_VARIABLE_FLAG_WO,
	},
	[DEMO_VAR_LED_CLR] = {
		.addr = CPU_DP_BASE + GPIO_CODR,
		.mask = PIN_LED_MASK,
		.offset = PIN_LED_OFFSET,
		.flags = RT_VARIABLE_FLAG_WO,
	},
};

static action_t *demo_actions[] = {
	[RT_ACTION_RECV_PING] = rt_recv_ping,
	[RT_ACTION_RECV_VERSION] = rt_version_getter,
	[RT_ACTION_RECV_FIELD_SET] = rt_variable_setter,
	[RT_ACTION_RECV_FIELD_GET] = rt_variable_getter,
	[RT_ACTION_RECV_STRUCT_SET] = rt_structure_setter,
	[RT_ACTION_RECV_STRUCT_GET] = rt_structure_getter,
};


struct rt_application app = {
	.mq = mq,
	.n_mq = 2,

	.structures = demo_structures,
	.n_structures = __DEMO_STRUCT_MAX,

	.variables = demo_variables,
	.n_variables = __DEMO_VAR_MAX,

	.actions = demo_actions,
	.n_actions = 128, /* FIXME replace 128 with correct number */
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
	rt_init(&app);

	demo_debug_interface();
	while (1) {
		/* Handle all messages incoming from slot DEMO_HMQ_IN
		   as actions */
		rt_mq_action_dispatch(DEMO_CMD_IN);
	}

	return 0;
}
