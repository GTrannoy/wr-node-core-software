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
 * ad9516_config.h: Config registers for AD9516 (main WR clock PLL/fanout)
 * on the FMC DDS v2 mezzanine.
 */


const struct ad95xx_reg ad9516_rfpll_master_config[] = {
//Serial Port Configuration
{0x0000, 0x18},
{0x0003, 0xC3}, // Read only : 9516-4 version
{0x0004, 0x01}, // Readback active registers
//PLL
{0x0010, 0x7D}, //Polarity POS; CP currrent default, CP mode default, PLL inactive (OFF)
{0x0011, 0x01}, //R divider LSB
{0x0012, 0x00}, //R divider MSB
{0x0013, 0x00}, //A counter
{0x0014, 0x05}, //B counter LSB
{0x0015, 0x00}, //B counter MSB
{0x0016, 0x04}, //Prescaler Dual Modulus divide-by-8 (8-9 mode) N=PxB+A=40
{0x0017, 0xAC}, //status pin = Status of VCO frequency (active high)
{0x0018, 0x06}, //lock detect enebled, vco cal divider default 16
{0x0019, 0x00}, //default
{0x001A, 0x00}, //LD pin = digital lock detect
{0x001B, 0x00}, //default
{0x001C, 0x01}, //Differential mode
{0x001D, 0x00}, //default
{0x001E, 0x00},
//Fine Delay Adjust
{0x00A0, 0x01}, //OUT6 delay bypassed
{0x00A1, 0x00},
{0x00A2, 0x00},
{0x00A3, 0x01}, //OUT7 delay bypassed
{0x00A4, 0x00},
{0x00A5, 0x00},
{0x00A6, 0x01}, //OUT8 delay bypassed
{0x00A7, 0x00},
{0x00A8, 0x00},
{0x00A9, 0x01}, //OUT9 delay bypassed
{0x00AA, 0x00},
{0x00AB, 0x00},
//LVPECL Outputs
{0x00F0, 0x04}, //OUT0 LVPECL, 600mV RF_OUT
{0x00F1, 0x0A}, //OUT1 unused, default
{0x00F2, 0x0A}, //OUT2 unused
{0x00F3, 0x04}, //OUT3 LVPECL, 600mV CKRF_OUT
{0x00F4, 0x08}, //OUT4 LVPECL, 780mV, to Phase Detector
{0x00F5, 0x0A}, //OUT5 unused, default
//LVDS/CMOS Outputs
{0x0140, 0x43}, //OUT6 unused, Power Down
{0x0141, 0x42}, //OUT7 LVDS, to FPGA frequency meter
{0x0142, 0x43}, //OUT8 unused, Power Down
{0x0143, 0x43}, //OUT9 unused, Power Down
//LVPECL Channel Dividers
{0x0190, 0x00},
{0x0191, 0x80}, //bypass divider (VCO divider unused): OUT0/OUT1=CLK=RF
{0x0192, 0x00},
{0x0193, 0x00},
{0x0194, 0x80}, //bypass divider (VCO divider unused): OUT2/OUT3=CLK=RF
{0x0195, 0x00},
{0x0196, 0x33}, //CLK/8 OUT4=RF/8 to Phase Detector
{0x0197, 0x00},
{0x0198, 0x00},
//LVDS/CMOS Channel Dividers
{0x0199, 0x33}, //CLK/8 OUT7=RF/8 to FPGA frequency meter
{0x019A, 0x00},
{0x019B, 0x00},
{0x019C, 0x20}, //Bypass (power down) Divider 3.2
{0x019D, 0x00},
{0x019E, 0x00},
{0x019F, 0x00},
{0x01A0, 0x00},
{0x01A1, 0x30}, //Bypass (power down) Divider 4.1 and 4.2
{0x01A2, 0x00},
//VCO Divider and CLK Input
{0x01E0, 0x02}, //Not used, default (VCO divider = 4)
{0x01E1, 0x01}, //Select external CLK, bypass VCO divider
//System
{0x0230, 0x00},
//Update All Registers
{0x0231, 0x01}, //Update after all previous write
};
