/*
  Register definitions for slave core: WR Node CPU Local Registers

  * File           : wrn_cpu_lr.h
  * Author         : auto-generated by wbgen2 from wrn_cpu_lr.wb
  * Created        : Mon May  2 17:06:48 2016
  * Standard       : ANSI C

    THIS FILE WAS GENERATED BY wbgen2 FROM SOURCE FILE wrn_cpu_lr.wb
    DO NOT HAND-EDIT UNLESS IT'S ABSOLUTELY NECESSARY!

*/

#ifndef __WBGEN2_REGDEFS_WRN_CPU_LR_WB
#define __WBGEN2_REGDEFS_WRN_CPU_LR_WB

#include <inttypes.h>

#if defined( __GNUC__)
#define PACKED __attribute__ ((packed))
#else
#error "Unsupported compiler?"
#endif

#ifndef __WBGEN2_MACROS_DEFINED__
#define __WBGEN2_MACROS_DEFINED__
#define WBGEN2_GEN_MASK(offset, size) (((1<<(size))-1) << (offset))
#define WBGEN2_GEN_WRITE(value, offset, size) (((value) & ((1<<(size))-1)) << (offset))
#define WBGEN2_GEN_READ(reg, offset, size) (((reg) >> (offset)) & ((1<<(size))-1))
#define WBGEN2_SIGN_EXTEND(value, bits) (((value) & (1<<bits) ? ~((1<<(bits))-1): 0 ) | (value))
#endif


/* definitions for register: CPU Polling Register */

/* definitions for field: HMQ Slot Status in reg: CPU Polling Register */
#define WRN_CPU_LR_POLL_HMQ_MASK              WBGEN2_GEN_MASK(0, 16)
#define WRN_CPU_LR_POLL_HMQ_SHIFT             0
#define WRN_CPU_LR_POLL_HMQ_W(value)          WBGEN2_GEN_WRITE(value, 0, 16)
#define WRN_CPU_LR_POLL_HMQ_R(reg)            WBGEN2_GEN_READ(reg, 0, 16)

/* definitions for field: RMQ Slot Status in reg: CPU Polling Register */
#define WRN_CPU_LR_POLL_RMQ_MASK              WBGEN2_GEN_MASK(16, 16)
#define WRN_CPU_LR_POLL_RMQ_SHIFT             16
#define WRN_CPU_LR_POLL_RMQ_W(value)          WBGEN2_GEN_WRITE(value, 16, 16)
#define WRN_CPU_LR_POLL_RMQ_R(reg)            WBGEN2_GEN_READ(reg, 16, 16)

/* definitions for register: CPU Status Register */

/* definitions for field: WR Link Up in reg: CPU Status Register */
#define WRN_CPU_LR_STAT_WR_LINK               WBGEN2_GEN_MASK(0, 1)

/* definitions for field: WR Time OK in reg: CPU Status Register */
#define WRN_CPU_LR_STAT_WR_TIME_OK            WBGEN2_GEN_MASK(1, 1)

/* definitions for field: WR Aux Clock OK in reg: CPU Status Register */
#define WRN_CPU_LR_STAT_WR_AUX_CLOCK_OK_MASK  WBGEN2_GEN_MASK(2, 8)
#define WRN_CPU_LR_STAT_WR_AUX_CLOCK_OK_SHIFT 2
#define WRN_CPU_LR_STAT_WR_AUX_CLOCK_OK_W(value) WBGEN2_GEN_WRITE(value, 2, 8)
#define WRN_CPU_LR_STAT_WR_AUX_CLOCK_OK_R(reg) WBGEN2_GEN_READ(reg, 2, 8)

/* definitions for field: Core ID in reg: CPU Status Register */
#define WRN_CPU_LR_STAT_CORE_ID_MASK          WBGEN2_GEN_MASK(28, 4)
#define WRN_CPU_LR_STAT_CORE_ID_SHIFT         28
#define WRN_CPU_LR_STAT_CORE_ID_W(value)      WBGEN2_GEN_WRITE(value, 28, 4)
#define WRN_CPU_LR_STAT_CORE_ID_R(reg)        WBGEN2_GEN_READ(reg, 28, 4)

/* definitions for register: GPIO Set/Readback Register */

/* definitions for field: FMC I2C bitbanged SCL in reg: GPIO Set/Readback Register */
#define WRN_CPU_LR_GPSR_FMC_SCL               WBGEN2_GEN_MASK(0, 1)

/* definitions for field: FMC I2C bitbanged SDA in reg: GPIO Set/Readback Register */
#define WRN_CPU_LR_GPSR_FMC_SDA               WBGEN2_GEN_MASK(1, 1)

/* definitions for field: Select I2C bus in reg: GPIO Set/Readback Register */
#define WRN_CPU_LR_GPSR_FMC_SEL               WBGEN2_GEN_MASK(2, 1)

/* definitions for field: I2C bus locked in reg: GPIO Set/Readback Register */
#define WRN_CPU_LR_GPSR_FMC_LCK               WBGEN2_GEN_MASK(3, 1)

/* definitions for register: GPIO Clear Register */

/* definitions for field: FMC I2C bitbanged SCL in reg: GPIO Clear Register */
#define WRN_CPU_LR_GPCR_FMC_SCL               WBGEN2_GEN_MASK(0, 1)

/* definitions for field: FMC I2C bitbanged SDA in reg: GPIO Clear Register */
#define WRN_CPU_LR_GPCR_FMC_SDA               WBGEN2_GEN_MASK(1, 1)

/* definitions for field: Select FMC I2C bus in reg: GPIO Clear Register */
#define WRN_CPU_LR_GPCR_FMC_SEL               WBGEN2_GEN_MASK(2, 1)

/* definitions for register: TAI Cycles */

/* definitions for register: TAI Seconds */

/* definitions for register: GPIO Input */

/* definitions for register: GPIO Set */

/* definitions for register: GPIO Clear */

/* definitions for register: Debug Message Output */

/* definitions for register: Delay Counter Register */

/* definitions for register: Application ID */
/* [0x0]: REG CPU Polling Register */
#define WRN_CPU_LR_REG_POLL 0x00000000
/* [0x4]: REG CPU Status Register */
#define WRN_CPU_LR_REG_STAT 0x00000004
/* [0x8]: REG GPIO Set/Readback Register */
#define WRN_CPU_LR_REG_GPSR 0x00000008
/* [0xc]: REG GPIO Clear Register */
#define WRN_CPU_LR_REG_GPCR 0x0000000c
/* [0x10]: REG TAI Cycles */
#define WRN_CPU_LR_REG_TAI_CYCLES 0x00000010
/* [0x14]: REG TAI Seconds */
#define WRN_CPU_LR_REG_TAI_SEC 0x00000014
/* [0x18]: REG GPIO Input */
#define WRN_CPU_LR_REG_GPIO_IN 0x00000018
/* [0x1c]: REG GPIO Set */
#define WRN_CPU_LR_REG_GPIO_SET 0x0000001c
/* [0x20]: REG GPIO Clear */
#define WRN_CPU_LR_REG_GPIO_CLEAR 0x00000020
/* [0x24]: REG Debug Message Output */
#define WRN_CPU_LR_REG_DBG_CHR 0x00000024
/* [0x28]: REG Delay Counter Register */
#define WRN_CPU_LR_REG_DELAY_CNT 0x00000028
/* [0x2c]: REG Application ID */
#define WRN_CPU_LR_REG_APP_ID 0x0000002c
#endif
