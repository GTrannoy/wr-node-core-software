/*
 * This work is part of the White Rabbit Node Core project.
 *
 * Copyright (C) 2015 CERN (www.cern.ch)
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *
 * Released according to the GNU GPL, version 2 or any later version.
 */

#include <string.h>
#include "rt.h"
#include <demo-common-rt.h>

#define GPIO_CODR 0x0
#define GPIO_SODR 0x4
#define GPIO_DDR 0x8
#define GPIO_PSR 0xc
#define PIN_LEMO_L1 1
#define PIN_LEMO_L2 0
#define PIN_LEMO_L3 3
#define PIN_LEMO_L4 2

void gpio_set_dir(int pin, int out)
{
	uint32_t ddr = dp_readl(GPIO_DDR);
	if(out)
		ddr |= (1<<pin);
	else
		ddr &= ~(1<<pin);
	dp_writel(ddr, GPIO_DDR);
}

void gpio_set_state(int pin, int state)
{
	if(state)
		dp_writel(1<<pin, GPIO_SODR);
	else
		dp_writel(1<<pin, GPIO_CODR);
}

int gpio_get_state(int pin)
{
	return dp_readl(GPIO_PSR) & (1 << pin) ? 1 : 0;
}

void autodemo()
{
	int state, on = 1;
	uint32_t i, j = 0;

	/* Print something on the debug interface */
	pp_printf("Running autodemo\n");

	gpio_set_dir(PIN_LEMO_L1, 0); // Lemo L1 = input
	gpio_set_dir(PIN_LEMO_L2, 1); // Lemo L2 = output
	gpio_set_dir(PIN_LEMO_L3, 1); // Lemo L3 = output
	gpio_set_dir(PIN_LEMO_L4, 1); // Lemo L4 = output

	/* Clear all GPIOs (LEDs and LEMOs) */
	dp_writel(~0, GPIO_CODR);

	for (i = 0;; i++) {
		/* Lemo 2 follows Lemo 1 */
		state = gpio_get_state(PIN_LEMO_L1);
		gpio_set_state(PIN_LEMO_L2, state);

		/* Turn on/off leds one by one */
		if (i % 7500 == 0) {
			dp_writel(1 << (j + 8), on ? GPIO_SODR : GPIO_CODR);
			j = (j >= 16 ? 0 : j + 1);
			on = j ? on : !on;
		}

		/* Square signal on Lemo 3 */
		if (i % 10000 == 0) {
			state = gpio_get_state(PIN_LEMO_L3);
			gpio_set_state(PIN_LEMO_L3, !state);
		}

		/* Square signal on Lemo 4 */
		if (i % 40000 == 0) {
			state = gpio_get_state(PIN_LEMO_L4);
			gpio_set_state(PIN_LEMO_L4, !state);
		}

		/* GPIO status on debug interface */
		if (i % 1000000 == 0) {
			/* This output is not reliable for debugging purpose,
			   it's just to show that you can do it */
			pp_printf("GPIO direction 0x%x\nGPIO 0x%x\n",
				  dp_readl(GPIO_DDR),
				  dp_readl(GPIO_PSR));
		}

		/* Check if someone else (HOST or other RT application) want
		   to stop this execution */
		if (!autodemo_run) {
			pp_printf("Stopping autodemo\n");
			break;
		}
	}

	/* Clear all GPIOs (LEDs and LEMOs) */
	dp_writel(~0, GPIO_CODR);
	/* Set all GPIO as output */
	dp_writel(~0, GPIO_DDR);
}


int main()
{
	/* By default allow this program to run */
	smem_atomic_or(&autodemo_run, 1);

	while (1) {
		/* Wait that someone (HOST or other RT application) allows
		   this program to run */
		if (!autodemo_run)
			continue;

		autodemo();
	}
}
