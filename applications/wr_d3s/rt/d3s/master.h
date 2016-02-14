/*
 * This work is part of the White Rabbit Node Core project.
 *
 * Copyright (C) 2013-2016 CERN (www.cern.ch)
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * Released according to the GNU GPL, version 2 or any later version.
 */


/*.
 * WR Distributed DDS Realtime Firmware.
 *
 * master.h - master side PLL
 */

#ifndef __D3S_MASTER_H
#define __D3S_MASTER_H

#include "rt-d3s.h"
#include "hw/dds_regs.h"
#include "wr-d3s-common.h"
#include "gqueue.h"

// Size of the data buffer in the PLL response log packet
#define RESP_LOG_BUF_SIZE 128

struct resp_log_state
{
    int seq;
    int remaining_samples;
    int block_index;
    int block_samples;
    int block_offset;
    int req_id;
    int active;
    uint32_t buf[RESP_LOG_BUF_SIZE];
};

struct dds_master_stats
{
    int sent_fixups;
    int sent_updates;
};

// State of the master DDS loop (phase detector-based)
struct dds_master_state {
// loop enable
    int enabled;
// loop locked
    int locked;
// Identifier of the RF stream broadcast to all slaves
    int stream_id;
// PI integrator
    int integ;
// PI coefficients
    int kp, ki;
// Sample counter for lock detection
    int lock_counter;
// Lock phase error threshold: if abs(error) stays below lock_threshold for
// lock_samples, we assume the PLL is locked.
    int lock_threshold;
// Sample counte for lock detection (see above)
    int lock_samples;
// Identifier of the "current lock" : every time the PLL locks, the value is incremented.
// It's broadcast to the slaves to let them restart their clock recovery logic when the master
// loses the lock.
    int lock_id;
// Opposite of the lock_samples. If abs(error) stays above lock_threshold for
// delock_samples, the PLL loses the lock.
    int delock_samples;
// Current DDS tune adjustment value (output of the PI regulator)
    int current_tune;
// Center DDS frequency (expressed as the DDS phase step). The equation assumes floating point numbers.
// base_freq  = (1<<42) * (base_freq[Hz] / sample_rate[Hz]) * 8;
// The sample_rate is 500 MHz by default.
    int64_t base_freq;
// Gain coefficient of the DDS frequency tuning
    int vco_gain;
// Total number of samples processed by the PLL.
    int sample_count;
// PLL sampling rate, in Hz
    int samples_per_second;
// PLL sample rate divider:
// samples_per_second = 125 MHz / (125 * sampling_divider)
    int sampling_divider;

// Index of the current sample wrs to the PPS, e.g (assuming samples_per_second = 10000 -> period = 100us)
// current_sample_idx = 0 : sample taken at x + 0us
// current_sample_idx = 1 : sample taken at x + 100us
    int current_sample_idx;

// Last value read from the phase detector ADC
    int last_adc_value;

// Snapshot of the DDS phase accumulator taken at the PPS (beginning of current TAI second)
    int64_t fixup_phase;
// Timestamp of the last phase fixup (usually just TAI seconds)
    struct wr_timestamp fixup_time;
// Fixup ready flag (non-0: value fixup_phase/fixup_time contain correct values)
    int fixup_ready;

// Period of the RF counter (in Hz)
    uint32_t rf_cnt_period;
// Value of the RF counter snapshot (in RF ticks)
    uint32_t rf_cnt_snap_count;
// Exact tiemstamp the snapshot was taken at (TAI time)
    struct wr_timestamp rf_cnt_snap_timestamp;

// PLL response logging buffer
    struct resp_log_state rsp_log;
// statistics counters
    struct dds_master_stats stats;
};


void dds_master_init (struct dds_master_state *state);
void dds_master_start (struct dds_master_state *state);
void dds_master_stop (struct dds_master_state *state);
void dds_master_update (struct dds_master_state *state);

#endif
