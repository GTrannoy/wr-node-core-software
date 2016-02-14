#ifndef __RT_D3S_H
#define __RT_D3S_H

#include "rt.h"
#include "hw/dds_regs.h"
#include "wr-d3s-common.h"
#include "gqueue.h"

#define DEBUG

#ifdef DEBUG
    #define dbg_printf pp_printf
#else
    #define dbg_printf(...)
#endif


// number of DDS samples by which the phase snapshot is delayed
#define DDS_SNAP_LAG_SAMPLES 3

// DDS accumulator bits
#define DDS_ACC_BITS 43

// Mask for the above bits
#define DDS_ACC_MASK ((1ULL << (DDS_ACC_BITS) ) - 1)

// DDS tuning gain (value written to GAIN register of the DDS core)
#define DDS_TUNE_GAIN 1024

// Maximum reconstruction delay allowed by the slave (in sampling periods)
#define DDS_MAX_SLAVE_DELAY 16

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


// Slave state machine states
#define SLAVE_WAIT_CONFIG 0
#define SLAVE_RUN 1

// Slave's RF counter sync FSM states
#define RF_CNT_SLAVE_WAIT_FIXUP 0
#define RF_CNT_SLAVE_RESYNC 1
#define RF_CNT_SLAVE_READY 2

// DDS slave node statistics
struct dds_slave_stats {
// number of received fixup messages
    int rx_fixups;
// number of received tune value messages
    int rx_tune_updates;
// number of messages dropped from the tune queue due to overflow
    int queue_drops;
// number of samples not passed to the slave's DDS due to too much
// processing latency
    int missed_samples;
};

struct dds_slave_state {
    int enabled;
    int lock_id;
    int stream_id;

    int slave_state;

    int64_t base_freq;
    int vco_gain;

// last tune value written into DDS core
    int last_tune;

// number of received fixup messages
    int fixup_count;
// number of phase fixups written to HW
    int fixups_applied;
// reconstruction delay, in sampling periods
    int delay_samples;

// timestamp of the last DDS accumulator/phase fixup
    struct wr_timestamp fixup_time;
// value of the DDS accumulator for the last fixup
    int64_t fixup_phase;
// phase correction (to compensate for phase drift introduced by delay)
    int64_t phase_correction;

// see dds_master_state
    int samples_per_second;
    int sampling_divider;
    int current_sample_idx;

// FIFO with recently received tune values, waiting to be applied to HW
    struct generic_queue tune_queue;
// see dds_master_state
    struct dds_slave_stats stats;

// state of the RF counter sync FSM (RF_CNT_xxx)
    int rf_cnt_state;
// period of the RF counter (Hz)
    uint32_t rf_cnt_period;
// snapshot of the master's RF counter taken at (rf_cnt_snap_timestamp)
    uint32_t rf_cnt_snap_count;

// Exact tiemstamp the snapshot was taken at (TAI time)
    struct wr_timestamp rf_cnt_snap_timestamp;
};


/* Sets/clears the GPIO bits selected by the mask */
static inline void gpior_set(uint32_t mask, int value)
{
    uint32_t gpior = dp_readl(DDS_REG_GPIOR);
    if(value)
        gpior |= mask;
    else
	gpior &= ~mask;

    dp_writel(gpior, DDS_REG_GPIOR);
}

/* Gets the GPIO bit selected by the mask */
static inline int gpior_get(uint32_t mask)
{
    uint32_t gpior = dp_readl(DDS_REG_GPIOR);
    return (gpior & mask) ? 1 : 0;
}

int ad9516_init();
int ad9510_init();
void adf4002_configure(int r_div, int n_div, int mon_output);


int wr_link_up();
int wr_time_locked();
int wr_time_ready();
int wr_enable_lock( int enable );
void wr_update_link();
int wr_is_timing_ok();
int wr_init();

#endif
