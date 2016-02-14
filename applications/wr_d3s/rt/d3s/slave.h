#ifndef __D3S_SLAVE_H
#define __D3S_SLAVE_H

#include "rt.h"
#include "hw/dds_regs.h"
#include "wr-d3s-common.h"
#include "gqueue.h"

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

// state of the slave node.
struct dds_slave_state {
    int enabled;
    int lock_id;
    int stream_id;

// main FSM state (SLAVE_xxx)
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

void dds_slave_init(struct dds_slave_state *state);
void dds_slave_start(struct dds_slave_state *state);
void dds_slave_update(struct dds_slave_state *state);
void dds_slave_stop(struct dds_slave_state *state);

#endif
