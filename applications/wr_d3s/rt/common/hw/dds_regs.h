/*
  Register definitions for slave core: DDS RF distribution WB Slave

  * File           : dds_regs.h
  * Author         : auto-generated by wbgen2 from dds_wb_slave.wb
  * Created        : Tue Nov 15 14:13:49 2016
  * Standard       : ANSI C

    THIS FILE WAS GENERATED BY wbgen2 FROM SOURCE FILE dds_wb_slave.wb
    DO NOT HAND-EDIT UNLESS IT'S ABSOLUTELY NECESSARY!

*/

#ifndef __WBGEN2_REGDEFS_DDS_WB_SLAVE_WB
#define __WBGEN2_REGDEFS_DDS_WB_SLAVE_WB

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <inttypes.h>
#endif

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


/* definitions for register: Control Register */

/* definitions for field: Sampling Enable in reg: Control Register */
#define DDS_CR_SAMP_EN                        WBGEN2_GEN_MASK(0, 1)

/* definitions for field: Sample rate divider in reg: Control Register */
#define DDS_CR_SAMP_DIV_MASK                  WBGEN2_GEN_MASK(1, 16)
#define DDS_CR_SAMP_DIV_SHIFT                 1
#define DDS_CR_SAMP_DIV_W(value)              WBGEN2_GEN_WRITE(value, 1, 16)
#define DDS_CR_SAMP_DIV_R(reg)                WBGEN2_GEN_READ(reg, 1, 16)

/* definitions for field: Enable RF Counter in reg: Control Register */
#define DDS_CR_RF_CNT_ENABLE                  WBGEN2_GEN_MASK(17, 1)

/* definitions for register: Time Control Register */

/* definitions for field: WR Lock Enable in reg: Time Control Register */
#define DDS_TCR_WR_LOCK_EN                    WBGEN2_GEN_MASK(0, 1)

/* definitions for field: WR Locked in reg: Time Control Register */
#define DDS_TCR_WR_LOCKED                     WBGEN2_GEN_MASK(1, 1)

/* definitions for field: WR Link in reg: Time Control Register */
#define DDS_TCR_WR_LINK                       WBGEN2_GEN_MASK(2, 1)

/* definitions for field: WR Time Valid in reg: Time Control Register */
#define DDS_TCR_WR_TIME_VALID                 WBGEN2_GEN_MASK(3, 1)

/* definitions for register: GPIO register */

/* definitions for field: System PLL CS in reg: GPIO register */
#define DDS_GPIOR_PLL_SYS_CS_N                WBGEN2_GEN_MASK(0, 1)

/* definitions for field: System Reset in reg: GPIO register */
#define DDS_GPIOR_PLL_SYS_RESET_N             WBGEN2_GEN_MASK(1, 1)

/* definitions for field: PLL SCLK (shared) in reg: GPIO register */
#define DDS_GPIOR_PLL_SCLK                    WBGEN2_GEN_MASK(2, 1)

/* definitions for field: PLL SDIO (shared) in reg: GPIO register */
#define DDS_GPIOR_PLL_SDIO                    WBGEN2_GEN_MASK(3, 1)

/* definitions for field: PLL SDIO direction (shared) in reg: GPIO register */
#define DDS_GPIOR_PLL_SDIO_DIR                WBGEN2_GEN_MASK(4, 1)

/* definitions for field: VCXO PLL Reset in reg: GPIO register */
#define DDS_GPIOR_PLL_VCXO_RESET_N            WBGEN2_GEN_MASK(5, 1)

/* definitions for field: VCXO PLL Chip Select in reg: GPIO register */
#define DDS_GPIOR_PLL_VCXO_CS_N               WBGEN2_GEN_MASK(6, 1)

/* definitions for field: VCXO PLL SDO in reg: GPIO register */
#define DDS_GPIOR_PLL_VCXO_SDO                WBGEN2_GEN_MASK(7, 1)

/* definitions for field: ADF4002 Chip Enable in reg: GPIO register */
#define DDS_GPIOR_ADF_CE                      WBGEN2_GEN_MASK(8, 1)

/* definitions for field: ADF4002 Clock in reg: GPIO register */
#define DDS_GPIOR_ADF_CLK                     WBGEN2_GEN_MASK(9, 1)

/* definitions for field: ADF4002 Latch Enable in reg: GPIO register */
#define DDS_GPIOR_ADF_LE                      WBGEN2_GEN_MASK(10, 1)

/* definitions for field: ADF4002 Data in reg: GPIO register */
#define DDS_GPIOR_ADF_DATA                    WBGEN2_GEN_MASK(11, 1)

/* definitions for field: Serdes PLL locked in reg: GPIO register */
#define DDS_GPIOR_SERDES_PLL_LOCKED           WBGEN2_GEN_MASK(12, 1)

/* definitions for register: PD ADC Data register */

/* definitions for field: ADC data in reg: PD ADC Data register */
#define DDS_PD_DATA_DATA_MASK                 WBGEN2_GEN_MASK(0, 16)
#define DDS_PD_DATA_DATA_SHIFT                0
#define DDS_PD_DATA_DATA_W(value)             WBGEN2_GEN_WRITE(value, 0, 16)
#define DDS_PD_DATA_DATA_R(reg)               WBGEN2_GEN_READ(reg, 0, 16)

/* definitions for field: ADC data valid in reg: PD ADC Data register */
#define DDS_PD_DATA_VALID                     WBGEN2_GEN_MASK(16, 1)

/* definitions for register: DDS Tune Value */

/* definitions for field: DDS tune word in reg: DDS Tune Value */
#define DDS_TUNE_VAL_TUNE_MASK                WBGEN2_GEN_MASK(0, 28)
#define DDS_TUNE_VAL_TUNE_SHIFT               0
#define DDS_TUNE_VAL_TUNE_W(value)            WBGEN2_GEN_WRITE(value, 0, 28)
#define DDS_TUNE_VAL_TUNE_R(reg)              WBGEN2_GEN_READ(reg, 0, 28)

/* definitions for field: Load DDS Accumulator along with next tune sample. in reg: DDS Tune Value */
#define DDS_TUNE_VAL_LOAD_ACC                 WBGEN2_GEN_MASK(28, 1)

/* definitions for register: DDS Center frequency hi */

/* definitions for register: DDS Center frequency lo */

/* definitions for register: DDS Accumulator Snapshot HI */

/* definitions for register: DDS Accumulator Snapshot LO */

/* definitions for register: DDS Accumulator Load Value HI */

/* definitions for register: DDS Accumulator Load Value LO */

/* definitions for register: DDS Gain */

/* definitions for register: Reset Register */

/* definitions for field: FPGA REF/Serdes PLL Reset in reg: Reset Register */
#define DDS_RSTR_PLL_RST                      WBGEN2_GEN_MASK(0, 1)

/* definitions for field: FPGA DDS Logic software reset in reg: Reset Register */
#define DDS_RSTR_SW_RST                       WBGEN2_GEN_MASK(1, 1)

/* definitions for register: I2C bitbanged IO register */

/* definitions for field: SCL Line out in reg: I2C bitbanged IO register */
#define DDS_I2CR_SCL_OUT                      WBGEN2_GEN_MASK(0, 1)

/* definitions for field: SDA Line out in reg: I2C bitbanged IO register */
#define DDS_I2CR_SDA_OUT                      WBGEN2_GEN_MASK(1, 1)

/* definitions for field: SCL Line in in reg: I2C bitbanged IO register */
#define DDS_I2CR_SCL_IN                       WBGEN2_GEN_MASK(2, 1)

/* definitions for field: SDA Line in in reg: I2C bitbanged IO register */
#define DDS_I2CR_SDA_IN                       WBGEN2_GEN_MASK(3, 1)

/* definitions for register: VCXO/Reference Clock Frequency */

/* definitions for register: DDS Output Frequency */

/* definitions for register: Frequency Measurement Gating */

/* definitions for register: Current Sample Index */

/* definitions for register: RF Counter reset safe phase value */

/* definitions for field: Lower bound in reg: RF Counter reset safe phase value */
#define DDS_RF_RST_PHASE_LO_MASK              WBGEN2_GEN_MASK(0, 8)
#define DDS_RF_RST_PHASE_LO_SHIFT             0
#define DDS_RF_RST_PHASE_LO_W(value)          WBGEN2_GEN_WRITE(value, 0, 8)
#define DDS_RF_RST_PHASE_LO_R(reg)            WBGEN2_GEN_READ(reg, 0, 8)

/* definitions for field: Upper bound in reg: RF Counter reset safe phase value */
#define DDS_RF_RST_PHASE_HI_MASK              WBGEN2_GEN_MASK(8, 8)
#define DDS_RF_RST_PHASE_HI_SHIFT             8
#define DDS_RF_RST_PHASE_HI_W(value)          WBGEN2_GEN_WRITE(value, 8, 8)
#define DDS_RF_RST_PHASE_HI_R(reg)            WBGEN2_GEN_READ(reg, 8, 8)

/* definitions for register: RF Load TAI Trigger cycles */

/* definitions for field: TAI cycles in reg: RF Load TAI Trigger cycles */
#define DDS_RF_CNT_TRIGGER_CYCLES_MASK        WBGEN2_GEN_MASK(0, 28)
#define DDS_RF_CNT_TRIGGER_CYCLES_SHIFT       0
#define DDS_RF_CNT_TRIGGER_CYCLES_W(value)    WBGEN2_GEN_WRITE(value, 0, 28)
#define DDS_RF_CNT_TRIGGER_CYCLES_R(reg)      WBGEN2_GEN_READ(reg, 0, 28)

/* definitions for field: ARM RF Counter Load in reg: RF Load TAI Trigger cycles */
#define DDS_RF_CNT_TRIGGER_ARM_LOAD           WBGEN2_GEN_MASK(28, 1)

/* definitions for field: DONE in reg: RF Load TAI Trigger cycles */
#define DDS_RF_CNT_TRIGGER_DONE               WBGEN2_GEN_MASK(29, 1)

/* definitions for register: RF Counter Sync Value */

/* definitions for register: RF Counter Period */

/* definitions for register: RF Counter Snapshot (RF ticks) */

/* definitions for register: RF Counter Raw Value (RF ticks) */

/* definitions for register: RF Counter TAI Resync Snapshot */

/* definitions for register: Trigger In Snapshot */

/* definitions for register: Trigger In Control */

/* definitions for field: Trigger In Arm (Snapshot) in reg: Trigger In Control */
#define DDS_TRIG_IN_CSR_ARM                   WBGEN2_GEN_MASK(0, 1)

/* definitions for field: DONE in reg: Trigger In Control */
#define DDS_TRIG_IN_CSR_DONE                  WBGEN2_GEN_MASK(1, 1)

/* definitions for register: Pulse Out Cycles */

/* definitions for register: Pulse Out Control */

/* definitions for field: Arm (Snapshot) in reg: Pulse Out Control */
#define DDS_PULSE_OUT_CSR_ARM                 WBGEN2_GEN_MASK(0, 1)

/* definitions for field: DONE in reg: Pulse Out Control */
#define DDS_PULSE_OUT_CSR_DONE                WBGEN2_GEN_MASK(1, 1)

/* definitions for field: Coarse Adjust in reg: Pulse Out Control */
#define DDS_PULSE_OUT_CSR_ADJ_COARSE_MASK     WBGEN2_GEN_MASK(2, 3)
#define DDS_PULSE_OUT_CSR_ADJ_COARSE_SHIFT    2
#define DDS_PULSE_OUT_CSR_ADJ_COARSE_W(value) WBGEN2_GEN_WRITE(value, 2, 3)
#define DDS_PULSE_OUT_CSR_ADJ_COARSE_R(reg)   WBGEN2_GEN_READ(reg, 2, 3)

/* definitions for field: Fine Adjust in reg: Pulse Out Control */
#define DDS_PULSE_OUT_CSR_ADJ_FINE_MASK       WBGEN2_GEN_MASK(5, 10)
#define DDS_PULSE_OUT_CSR_ADJ_FINE_SHIFT      5
#define DDS_PULSE_OUT_CSR_ADJ_FINE_W(value)   WBGEN2_GEN_WRITE(value, 5, 10)
#define DDS_PULSE_OUT_CSR_ADJ_FINE_R(reg)     WBGEN2_GEN_READ(reg, 5, 10)

/* definitions for register: DDS Phase Adjust HI */

/* definitions for field: Phase MSB in reg: DDS Phase Adjust HI */
#define DDS_PHASE_HI_PHASE_HI_MASK            WBGEN2_GEN_MASK(0, 11)
#define DDS_PHASE_HI_PHASE_HI_SHIFT           0
#define DDS_PHASE_HI_PHASE_HI_W(value)        WBGEN2_GEN_WRITE(value, 0, 11)
#define DDS_PHASE_HI_PHASE_HI_R(reg)          WBGEN2_GEN_READ(reg, 0, 11)

/* definitions for field: Update phase in reg: DDS Phase Adjust HI */
#define DDS_PHASE_HI_UPDATE                   WBGEN2_GEN_MASK(11, 1)

/* definitions for register: DDS Phase Adjust LO */

/* definitions for register: Status register for D3S RT running */

/* definitions for field: RT D3S running in reg: Status register for D3S RT running */
#define DDS_SR_RT_D3S_RUNNING                 WBGEN2_GEN_MASK(0, 1)
/* [0x0]: REG Control Register */
#define DDS_REG_CR 0x00000000
/* [0x4]: REG Time Control Register */
#define DDS_REG_TCR 0x00000004
/* [0x8]: REG GPIO register */
#define DDS_REG_GPIOR 0x00000008
/* [0xc]: REG PD ADC Data register */
#define DDS_REG_PD_DATA 0x0000000c
/* [0x10]: REG DDS Tune Value */
#define DDS_REG_TUNE_VAL 0x00000010
/* [0x14]: REG DDS Center frequency hi */
#define DDS_REG_FREQ_HI 0x00000014
/* [0x18]: REG DDS Center frequency lo */
#define DDS_REG_FREQ_LO 0x00000018
/* [0x1c]: REG DDS Accumulator Snapshot HI */
#define DDS_REG_ACC_SNAP_HI 0x0000001c
/* [0x20]: REG DDS Accumulator Snapshot LO */
#define DDS_REG_ACC_SNAP_LO 0x00000020
/* [0x24]: REG DDS Accumulator Load Value HI */
#define DDS_REG_ACC_LOAD_HI 0x00000024
/* [0x28]: REG DDS Accumulator Load Value LO */
#define DDS_REG_ACC_LOAD_LO 0x00000028
/* [0x2c]: REG DDS Gain */
#define DDS_REG_GAIN 0x0000002c
/* [0x30]: REG Reset Register */
#define DDS_REG_RSTR 0x00000030
/* [0x34]: REG I2C bitbanged IO register */
#define DDS_REG_I2CR 0x00000034
/* [0x38]: REG VCXO/Reference Clock Frequency */
#define DDS_REG_FREQ_MEAS_RF_IN 0x00000038
/* [0x3c]: REG DDS Output Frequency */
#define DDS_REG_FREQ_MEAS_DDS 0x0000003c
/* [0x40]: REG Frequency Measurement Gating */
#define DDS_REG_FREQ_MEAS_GATE 0x00000040
/* [0x44]: REG Current Sample Index */
#define DDS_REG_SAMPLE_IDX 0x00000044
/* [0x48]: REG RF Counter reset safe phase value */
#define DDS_REG_RF_RST_PHASE 0x00000048
/* [0x4c]: REG RF Load TAI Trigger cycles */
#define DDS_REG_RF_CNT_TRIGGER 0x0000004c
/* [0x50]: REG RF Counter Sync Value */
#define DDS_REG_RF_CNT_SYNC_VALUE 0x00000050
/* [0x54]: REG RF Counter Period */
#define DDS_REG_RF_CNT_PERIOD 0x00000054
/* [0x58]: REG RF Counter Snapshot (RF ticks) */
#define DDS_REG_RF_CNT_RF_SNAPSHOT 0x00000058
/* [0x5c]: REG RF Counter Raw Value (RF ticks) */
#define DDS_REG_RF_CNT_RAW 0x0000005c
/* [0x60]: REG RF Counter TAI Resync Snapshot */
#define DDS_REG_RF_CNT_CYCLES_SNAPSHOT 0x00000060
/* [0x64]: REG Trigger In Snapshot */
#define DDS_REG_TRIG_IN_SNAPSHOT 0x00000064
/* [0x68]: REG Trigger In Control */
#define DDS_REG_TRIG_IN_CSR 0x00000068
/* [0x6c]: REG Pulse Out Cycles */
#define DDS_REG_PULSE_OUT_CYCLES 0x0000006c
/* [0x70]: REG Pulse Out Control */
#define DDS_REG_PULSE_OUT_CSR 0x00000070
/* [0x74]: REG DDS Phase Adjust HI */
#define DDS_REG_PHASE_HI 0x00000074
/* [0x78]: REG DDS Phase Adjust LO */
#define DDS_REG_PHASE_LO 0x00000078
/* [0x7c]: REG Status register for D3S RT running */
#define DDS_REG_SR 0x0000007c
#endif
