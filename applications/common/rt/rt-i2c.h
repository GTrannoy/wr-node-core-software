/*
 * This work is part of the White Rabbit project
 *
 * Copyright (C) 2011,2012 CERN (www.cern.ch)
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * Author: Grzegorz Daniluk <grzegorz.daniluk@cern.ch>
 *
 * Released according to the GNU GPL, version 2 or any later version.
 */
#ifndef __I2C_H
#define __I2C_H

#include <stdint.h>
#include "hw/wrn_cpu_lr.h"

#define WRPC_FMC_I2C  0
#define WRPC_SFP_I2C  1
#define I2C_DELAY 300

struct s_i2c_if {
	uint32_t scl;
	uint32_t sda;
};

extern struct s_i2c_if i2c_if[2];

/****************************
 *        GPIO
 ***************************/
static inline void gpio_out(int pin, int val)
{
	if (val)
	{
	  lr_writel(pin,WRN_CPU_LR_REG_GPSR);
	}
	else
	{
	  lr_writel(pin,WRN_CPU_LR_REG_GPCR);
	}
}

static inline int gpio_in(int pin)
{
  return lr_readl(WRN_CPU_LR_REG_GPSR) & pin ? 1 : 0;
}

void mi2c_delay(void)
{
	int i;
	for (i = 0; i < I2C_DELAY; i++)
		asm volatile ("nop");
}

//there is one i2c for WRNC. So set i to FMC I2C directly
#define M_SDA_OUT(i, x) { gpio_out(i2c_if[i].sda, x); mi2c_delay(); }
#define M_SCL_OUT(i, x) { gpio_out(i2c_if[i].scl, x); mi2c_delay(); }
#define M_SDA_IN(i) gpio_in(i2c_if[i].sda)

struct s_i2c_if i2c_if[2] = {
	{WRN_CPU_LR_GPSR_FMC_SCL, WRN_CPU_LR_GPSR_FMC_SDA},
	{WRN_CPU_LR_GPSR_FMC_SCL, WRN_CPU_LR_GPSR_FMC_SDA}
};


uint8_t mi2c_poll(uint32_t time_out_us)
{
  //mi2c_delay ~= 5us if CPU frequency is 62.5MHz
  uint32_t us = (uint32_t)(time_out_us / 5);
  uint32_t i;
  for(i = 0; i < us; i++)
  {
    //if FMC I2C bus is locked
    if(gpio_in(WRN_CPU_LR_GPSR_FMC_LCK))
    {
      mi2c_delay();
    }
    else
    {
      return 1;
    }
  }
  return 0;
}

void mi2c_lock(void)
{
  gpio_out(WRN_CPU_LR_GPSR_FMC_SEL,1);
  pp_printf("init:%d %d %d\n",WRN_CPU_LR_GPSR_FMC_SEL,i2c_if[0].scl,i2c_if[0].sda);
}

void mi2c_unlock(void)
{
  gpio_out(WRN_CPU_LR_GPSR_FMC_SEL,0);
}

void mi2c_start(uint8_t i2cif)
{
	M_SDA_OUT(i2cif, 0);
	M_SCL_OUT(i2cif, 0);
}

void mi2c_repeat_start(uint8_t i2cif)
{
	M_SDA_OUT(i2cif, 1);
	M_SCL_OUT(i2cif, 1);
	M_SDA_OUT(i2cif, 0);
	M_SCL_OUT(i2cif, 0);
}

void mi2c_stop(uint8_t i2cif)
{
	M_SDA_OUT(i2cif, 0);
	M_SCL_OUT(i2cif, 1);
	M_SDA_OUT(i2cif, 1);
}

unsigned char mi2c_put_byte(uint8_t i2cif, unsigned char data)
{
	char i;
	unsigned char ack;

	for (i = 0; i < 8; i++, data <<= 1) {
		M_SDA_OUT(i2cif, data & 0x80);
		M_SCL_OUT(i2cif, 1);
		M_SCL_OUT(i2cif, 0);
	}

	M_SDA_OUT(i2cif, 1);
	M_SCL_OUT(i2cif, 1);

	ack = M_SDA_IN(i2cif);	/* ack: sda is pulled low ->success.     */

	M_SCL_OUT(i2cif, 0);
	M_SDA_OUT(i2cif, 0);

	return ack != 0;
}

void mi2c_get_byte(uint8_t i2cif, unsigned char *data, uint8_t last)
{

	int i;
	unsigned char indata = 0;

	M_SDA_OUT(i2cif, 1);
	/* assert: scl is low */
	M_SCL_OUT(i2cif, 0);

	for (i = 0; i < 8; i++) {
		M_SCL_OUT(i2cif, 1);
		indata <<= 1;
		if (M_SDA_IN(i2cif))
			indata |= 0x01;
		M_SCL_OUT(i2cif, 0);
	}

	if (last) {
		M_SDA_OUT(i2cif, 1);	//noack
		M_SCL_OUT(i2cif, 1);
		M_SCL_OUT(i2cif, 0);
	} else {
		M_SDA_OUT(i2cif, 0);	//ack
		M_SCL_OUT(i2cif, 1);
		M_SCL_OUT(i2cif, 0);
	}

	*data = indata;
}

void mi2c_init(uint8_t i2cif)
{
	M_SCL_OUT(i2cif, 1);
	M_SDA_OUT(i2cif, 1);
}

uint8_t mi2c_devprobe(uint8_t i2cif, uint8_t i2c_addr)
{
	uint8_t ret;
	mi2c_start(i2cif);
	ret = !mi2c_put_byte(i2cif, i2c_addr << 1);
	mi2c_stop(i2cif);

	return ret;
}

#endif
