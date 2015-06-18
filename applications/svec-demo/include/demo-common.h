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

/* HMQ slots used for input */
#define DEMO_HMQ_IN 0

/* HMQ slots used for output */
#define DEMO_HMQ_OUT 0

/* Command and log message IDs */
#define DEMO_ID_LED_SET		0x0001
#define DEMO_ID_LEMO_SET	0x0002
#define DEMO_ID_LEMO_DIR_SET    0x0003
#define DEMO_ID_STATE_GET	0x0004

#define DEMO_ID_ACK		0x0100
#define DEMO_ID_STATE_GET_REP	0x0200

#define PIN_LEMO_COUNT 4
#define PIN_LEMO_MASK (0x0000000F)
#define PIN_LED_COUNT 8
#define PIN_LED_OFFSET 8
#define PIN_LED_MASK (0xFFFFFF00)

#define PIN_LED_RED(ledno)	(8 + (ledno * 2))
#define PIN_LED_GREEN(ledno)	(9 + (ledno * 2))

#endif
