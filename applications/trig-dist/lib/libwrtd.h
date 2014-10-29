/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *         inspired by a draft of Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * License: GPL v3
 */

#ifndef __WRTD_LIB_H__
#define __WRTD_LIB_H__

#include <stdint.h>
#include <stdio.h>
#include <wrtd-common.h>

struct wrtd_node;

#define WRTD_DEFAULT_TIMEOUT	1000

enum wrtd_error_list {
	EWRTD_INVALD_ANSWER_ACK = 3276,
	EWRTD_INVALD_ANSWER_STATE,
	EWRTD_INVALD_BINARY,
	EWRTD_INVALD_DEAD_TIME,
	EWRTD_INVALID_TRIG_ID,
	EWRTD_INVALID_CHANNEL,
	EWRTD_NO_IMPLEMENTATION,
	__EWRTD_MAX_ERROR_NUMBER,
};

struct wrtd_trigger_handle {
    uint32_t ptr_cond;
    uint32_t ptr_trig;
    int channel;
};

struct wrtd_input_state {
    int input;

    uint32_t flags;           ///> enum list_io_flags
    uint32_t log_level;       ///> enum list_log_level
    int mode;

    uint32_t tagged_pulses;
    uint32_t sent_triggers;
    uint32_t sent_packets;

    struct wrtd_trigger_entry last_sent;
    struct wrtd_trig_id assigned_id;
    struct wr_timestamp dead_time;
    struct wr_timestamp delay;
    struct wr_timestamp last;
};

struct wrtd_output_trigger_state {
    int is_conditional;
    int enabled;
    struct wrtd_trig_id trigger;
    struct wrtd_trig_id condition;
    struct wr_timestamp delay_trig;
    struct wr_timestamp delay_cond;
    struct wrtd_trigger_handle handle;
    int worst_latency_us;
};

struct wrtd_output_state {
    int output;

    uint32_t executed_pulses;
    uint32_t missed_pulses_late;
    uint32_t missed_pulses_deadtime;
    uint32_t missed_pulses_overflow;

    struct wrtd_trigger_entry last_executed;
    struct wrtd_trigger_entry last_programmed;
    struct wrtd_trigger_entry last_enqueued;

    uint32_t flags;           ///> enum list_io_flags
    uint32_t log_level;       ///> enum list_log_level
    int mode;
    uint32_t dead_time;
    uint32_t pulse_width;
    struct wr_timestamp worst_rx_delay;
    uint32_t rx_packets;
    uint32_t rx_loopback;
};

/**
 * @file lbwrtd.c
 */
extern const char *wrtd_strerror(int err);
extern int wrtd_init();
extern void wrtd_exit();
extern struct wrtd_node *wrtd_open_by_fmc(uint32_t device_id);
extern struct wrtd_node *wrtd_open_by_lun(int lun);
extern void wrtd_close(struct wrtd_node *dev);
extern struct wrnc_dev *wrtd_get_wrnc_dev(struct wrtd_node *dev);
extern int wrtd_load_application(struct wrtd_node *dev, char *rt_tdc,
					 char *rt_fd);
extern int wrtd_in_enable(struct wrtd_node *dev, unsigned int input, int enable);
extern int wrtd_in_is_enabled(struct wrtd_node *dev, unsigned int input);

extern int wrtd_in_trigger_assign(struct wrtd_node *dev, unsigned int input,
					  struct wrtd_trig_id *trig_id);
extern int wrtd_in_trigger_unassign(struct wrtd_node *dev, unsigned int input);
extern int wrtd_in_trigger_mode_set(struct wrtd_node *dev, unsigned int input,
				    enum wrtd_trigger_mode mode);
extern int wrtd_in_arm(struct wrtd_node *dev, unsigned int input, int armed);
extern int wrtd_in_disarm(struct wrtd_node *dev, unsigned int input, int armed);

extern int wrtd_in_read_log(struct wrtd_node *dev, struct wrtd_log_entry *log,
			      int flags, int input_mask, int count);
extern int wrtd_in_trigger_software(struct wrtd_node *dev,
			     struct wrtd_trigger_entry *trigger);

extern int wrtd_in_delay_set(struct wrtd_node *dev, unsigned int input,
			     uint64_t delay_ps);
extern int wrtd_in_delay_get(struct wrtd_node *dev, unsigned int input,
			     uint64_t *delay_ps);
extern int wrtd_in_seq_counter_set (struct wrtd_node *dev, unsigned int input);
extern int wrtd_in_dead_time_set(struct wrtd_node *dev, unsigned int input,
				 uint64_t dead_time_ps);
extern int wrtd_in_dead_time_get(struct wrtd_node *dev, unsigned int input,
				 uint64_t *dead_time_ps);
extern int wrtd_in_state_get(struct wrtd_node *dev, unsigned int input,
			     struct wrtd_input_state *state);
extern int wrtd_in_counters_reset(struct wrtd_node *dev, unsigned int input);
extern int wrtd_in_timebase_offset_set(struct wrtd_node *dev,
				       unsigned int input, uint64_t offset);
extern int wrtd_in_log_level_set(struct wrtd_node *dev, unsigned int input,
				 uint32_t log_level);

extern int wrtd_out_enable(struct wrtd_node *dev, unsigned int output,
			   int enable);
extern int wrtd_out_dead_time_set(struct wrtd_node *dev, unsigned int output,
				  uint64_t dead_time_ps);
extern int wrtd_out_trig_assign(struct wrtd_node *dev,
				struct wrtd_trigger_handle *handle,
				int output, struct wrtd_trig_id *trig,
				struct wrtd_trig_id *condition);
extern int wrtd_out_trig_remove (struct wrtd_node *dev,
				 struct wrtd_trigger_handle *handle);
extern int wrtd_out_trig_get_all (struct wrtd_node *dev, unsigned int output,
				  struct wrtd_output_trigger_state *triggers,
				  int max_count);
extern int wrtd_out_trig_set_delay(struct wrtd_node *dev,
				   struct wrtd_trigger_handle *handle,
				   uint64_t delay_ps);
extern int wrtd_out_trig_set_condition_delay(struct wrtd_node *dev,
					     struct wrtd_trigger_handle *handle,
					     uint64_t delay_ps);
extern int wrtd_out_trig_get_state(struct wrtd_node *dev,
				   struct wrtd_trigger_handle *handle,
				   struct wrtd_output_trigger_state *state);
extern int wrtd_out_trig_enable(struct wrtd_node *dev,
				struct wrtd_trigger_handle *handle, int enable);
extern int wrtd_out_read_log(struct wrtd_node *dev, struct wrtd_log_entry *log,
			     int flags, unsigned int output_mask, int count);
extern int wrtd_out_set_log_level(struct wrtd_node *dev, unsigned int output,
				  uint32_t log_level);
extern int wrtd_out_set_trigger_mode(struct wrtd_node *dev,
				     unsigned int output, int mode);
extern int wrtd_out_arm(struct wrtd_node *dev, unsigned int input, int armed);
//int wrtd_out_get_state(struct wrtd_node *dev, unsigned int input, int armed);
extern int wrtd_out_reset_counters(struct wrtd_node *dev, unsigned int output);
extern int wrtd_out_check_triggered(struct wrtd_node *dev, unsigned int output);
//int wrtd_out_wait_trigger(struct wrtd_node*, int output_mask, struct wrtd_trig_id *id);

#endif
