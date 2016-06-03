/*
 * This work is part of the White Rabbit Node Core project.
 *
 * Copyright (C) 2015 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *
 * Released according to the GNU GPL, version 2 or any later version.
 */

#ifndef __DEMO_COMMON_H
#define __DEMO_COMMON_H
#include <mockturtle-common.h>

/* HMQ slots used for input */
#define DEMO_HMQ_IN 0

/* HMQ slots used for output */
#define DEMO_HMQ_OUT 0

/* Variable index - used only by demo-librt */
enum rt_variable_index {
	DEMO_VAR_LEMO_STA = 0,
	DEMO_VAR_LEMO_DIR,
	DEMO_VAR_LEMO_SET,
	DEMO_VAR_LEMO_CLR,
	DEMO_VAR_LED_STA,
	DEMO_VAR_LED_SET,
	DEMO_VAR_LED_CLR,
	__DEMO_VAR_MAX,
};

enum rt_structure_index {
	DEMO_STRUCT_TEST = 0,
	__DEMO_STRUCT_MAX,
};

/* Command and log message IDs */
enum rt_action_recv_demo {
	DEMO_ID_LED_SET = __RT_ACTION_RECV_STANDARD_NUMBER,
	DEMO_ID_LEMO_SET,
	DEMO_ID_LEMO_DIR_SET,
	DEMO_ID_STATE_GET,
	DEMO_ID_RUN_AUTO,
	DEMO_ID_STATE_GET_REP,
};

/* For the time being all fields must be 32bit because of HW limits */
#define DEMO_STRUCT_MAX_ARRAY 5
struct demo_structure {
	uint32_t field1;
	uint32_t field2;
	uint32_t array[DEMO_STRUCT_MAX_ARRAY];
};

#define PIN_LEMO_COUNT	4
#define PIN_LEMO_MASK	(0x0000000F)
#define PIN_LED_COUNT	8
#define PIN_LED_OFFSET	8
#define PIN_LED_MASK	(0x0000FFFF)

#define PIN_LED_RED(ledno)	(8 + (ledno * 2))
#define PIN_LED_GREEN(ledno)	(9 + (ledno * 2))

#endif
