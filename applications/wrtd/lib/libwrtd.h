/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *         inspired by a draft of Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * License: GPL v3
 */

#ifndef __WRTD_LIB_H__
#define __WRTD_LIB_H__
/** @file libwrtd.h */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include "libwrnc.h"
/**
 * @file wrtd-common.h
 */
#include "wrtd-common.h"

struct wrtd_node;

#define WRTD_DEFAULT_TIMEOUT	1000

/**
 * @enum wrtd_error_list
 * White Rabbit Trigger Distribution errors
 */
enum wrtd_error_list {
	EWRTD_INVALID_ANSWER_ACK = 3276,
	EWRTD_INVALID_ANSWER_STATE,
	EWRTD_INVALID_BINARY,
	EWRTD_INVALID_DEAD_TIME,
	EWRTD_INVALID_DELAY,
	EWRTD_INVALID_TRIG_ID,
	EWRTD_INVALID_CHANNEL,
	EWRTD_NO_IMPLEMENTATION,
	EWRTD_INVALID_ANSWER_TRIG,
	EWRTD_INVALID_ANSWER_HASH,
	EWRTD_INVALID_ANSWER_HASH_CONT,
	EWRTD_INVALID_ANSWER_HANDLE,
	EWRTD_NOFOUND_TRIGGER,
	EWRTD_NO_TRIGGER_CONDITION,
	EWRTD_INVALID_PULSE_WIDTH,
	EWRTD_INVALID_IN_APP,
	EWRTD_INVALID_OUT_APP,
	__EWRTD_MAX_ERROR_NUMBER,
};

/**
 * @enum wrtd_core
 * Core indexes according to their main purpose
 */
enum wrtd_core {
	WRTD_CORE_IN = 0, /**< Input core that manages incoming pulses */
	WRTD_CORE_OUT = 1, /**< Output core that manages pulses generation */
};


/**
 * Trigger token
 */
struct wrtd_trigger_handle {
	uint32_t ptr_cond; /**< trigger condition pointer */
	uint32_t ptr_trig; /**< trigger pointer */
	int channel;  /**< channel assigned to the trigger */
};


/**
 * Status description for an input channel
 */
struct wrtd_input_state {
	int input; /**< Input channel index */

	uint32_t flags;           /**< enum list_io_flags */
	uint32_t log_level;       /**< enum list_log_level */
	enum wrtd_trigger_mode mode; /**< Trigger mode in use */

	uint32_t tagged_pulses; /**< Number of incoming pulses detected */
	uint32_t sent_triggers; /**< Number of triggers sent over the
				   white-rabbit network  */
	uint32_t sent_packets; /**< Number of packets sent over the
				   white-rabbit network  */

	struct wrtd_trigger_entry last_sent; /**< Description of the last
						trigger sent */
	struct wrtd_trig_id assigned_id; /**< trigger assigned to this channel */
	struct wr_timestamp dead_time; /**< Configured dead time  */
	struct wr_timestamp delay; /**< Time delay assined */
	struct wr_timestamp last_tagged_pulse; /** Time stamp of the last
						   detected pulse*/
	struct wr_timestamp tdc_timebase_offset; /**< TDC time base offset */
};

/**
 * Status description for a trigger on output
 */
struct wrtd_output_trigger_state {
	int is_conditional; /**< tell if the trigger is under condition or not */
	int enabled; /**< tell if the trigger is enable, so it may be
			generater as output */
	struct wrtd_trig_id trigger; /**< Trigger identifier */
	struct wrtd_trig_id condition; /**< Trigger identifier for the trigger
					  condition */
	struct wr_timestamp delay_trig; /**< Configured trigger delay */
	struct wr_timestamp delay_cond; /**< Configured trigger condition delay */
	struct wrtd_trigger_handle handle;
	int latency_worst_us; /**< Worst latency in micro-seconds */
	int latency_average_us; /**< Average latency in micro-seconds */
	uint32_t executed_pulses; /**< Number of executed pulses */
	uint32_t missed_pulses; /**< Number of missed pulses */
	void *private_data; /**< private pointer used by the library */
};

/**
 * Status description for an output channel
 */
struct wrtd_output_state {
	int output; /**< Output channel index */

	uint32_t executed_pulses; /**< Number of generated pulses */
	uint32_t missed_pulses_late; /**< Number of missed pulses due to: */
	uint32_t missed_pulses_deadtime; /**< Number of missed pulses due to: */
	uint32_t missed_pulses_overflow; /**< Number of missed pulses due to: */
	uint32_t missed_pulses_no_timing; /**< Number of missed pulses due to: */

	struct wrtd_trigger_entry last_executed; /**< Trigger description of the
						    last generated trigger */
	struct wrtd_trigger_entry last_received; /**< Trigger description of the
						    last received trigger from
						    the white-rabbit network */
	struct wrtd_trigger_entry last_enqueued;  /**< Trigger description of
						     the last enqueued trigger
						     in the execution queue */
	struct wrtd_trigger_entry last_lost; /**< Trigger description of the
						last lost trigger */

	uint32_t flags;           /**< enum list_io_flags */
	uint32_t log_level;       /**< enum list_log_level */
	enum wrtd_trigger_mode mode; /**< Trigger mode in use */
	struct wr_timestamp dead_time; /**< Configured dead time */
	struct wr_timestamp pulse_width; /**< Pulse width */

	uint32_t received_messages; /**< Number of received packets from
				       the network */
	uint32_t received_loopback;
};

/**
 * @file libwrtd-common.c
 */
/**
 * @defgroup dev
 * Set of functions to manage the basic device and library configuration.
 * @{
 */
extern int wrtd_init();
extern void wrtd_exit();
extern struct wrtd_node *wrtd_open_by_fmc(uint32_t device_id);
extern struct wrtd_node *wrtd_open_by_lun(int lun);
extern void wrtd_close(struct wrtd_node *dev);
extern struct wrnc_dev *wrtd_get_wrnc_dev(struct wrtd_node *dev);
extern int wrtd_load_application(struct wrtd_node *dev, char *rt_tdc,
					 char *rt_fd);
extern int wrtd_white_rabbit_sync(struct wrtd_node *dev,
				  unsigned long timeout_s);
extern int wrtd_cpu_restart(struct wrtd_node *dev);
extern int wrtd_time_get(struct wrtd_node *dev, unsigned int input,
			struct wr_timestamp *current_time);
/**@}*/

/**
 * @defgroup util Utilities
 * Set of utilities
 * @{
 */
extern int wrtd_version_is_valid(struct wrtd_node *dev);
extern const char *wrtd_strerror(int err);
extern void wrtd_ts_to_pico(struct wr_timestamp *ts, uint64_t *pico);
extern void wrtd_pico_to_ts(uint64_t *pico, struct wr_timestamp *ts);
extern void wrtd_ts_to_sec_pico(struct wr_timestamp *ts,
				uint64_t *sec, uint64_t *pico);
extern void wrtd_sec_pico_to_ts(uint64_t sec, uint64_t pico,
				struct wr_timestamp *ts);
/**@}*/


/**
 * @defgroup log Logging
 * Set of logging functions for input and output channels
 * @{
 */
extern const char *wrtd_strlogging(enum wrtd_log_level lvl);
enum wrtd_log_level wrtd_strlogging_to_level(char *log);
extern void wrtd_strlogging_full(char *buf, uint32_t log_level);
extern int wrtd_log_read(struct wrnc_hmq *hmq_log, struct wrtd_log_entry *log,
			 int count, int poll_timeout);
extern void wrtd_log_close(struct wrnc_hmq *hmq);
extern struct wrnc_hmq *wrtd_in_log_open(struct wrtd_node *dev, int input);
extern int wrtd_in_log_level_set(struct wrtd_node *dev, unsigned int input,
				 uint32_t log_level);
extern int wrtd_in_log_level_get(struct wrtd_node *dev, unsigned int input,
				 uint32_t *log_level);
extern int wrtd_out_log_level_set(struct wrtd_node *dev, unsigned int output,
				  uint32_t log_level);
extern int wrtd_out_log_level_get(struct wrtd_node *dev, unsigned int input,
				  uint32_t *log_level);
extern struct wrnc_hmq *wrtd_out_log_open(struct wrtd_node *dev, int output);
/**@}*/



/**
 * @file libwrtd-input.c
 */
/**
 * @defgroup input Input Management
 * Set of functions to handle input channels
 * @{
 */
extern int wrtd_in_state_get(struct wrtd_node *dev, unsigned int input,
			     struct wrtd_input_state *state);
extern int wrtd_in_enable(struct wrtd_node *dev, unsigned int input, int enable);
extern int wrtd_in_trigger_assign(struct wrtd_node *dev, unsigned int input,
					  struct wrtd_trig_id *trig_id);
extern int wrtd_in_trigger_unassign(struct wrtd_node *dev, unsigned int input);
extern int wrtd_in_trigger_mode_set(struct wrtd_node *dev, unsigned int input,
				    enum wrtd_trigger_mode mode);
extern int wrtd_in_trigger_software(struct wrtd_node *dev,
			     struct wrtd_trigger_entry *trigger);
extern int wrtd_in_arm(struct wrtd_node *dev, unsigned int input, int armed);
extern int wrtd_in_dead_time_set(struct wrtd_node *dev, unsigned int input,
				 uint64_t dead_time_ps);
extern int wrtd_in_delay_set(struct wrtd_node *dev, unsigned int input,
			     uint64_t delay_ps);
extern int wrtd_in_timebase_offset_set(struct wrtd_node *dev,
				       unsigned int input, uint64_t offset);
extern int wrtd_in_counters_reset(struct wrtd_node *dev, unsigned int input);
extern int wrtd_in_seq_counter_set (struct wrtd_node *dev, unsigned int input,
				    unsigned int value);
extern int wrtd_in_is_enabled(struct wrtd_node *dev, unsigned int input,
			      unsigned int *enable);
extern int wrtd_in_is_armed(struct wrtd_node *dev, unsigned int input,
			    unsigned int *armed);
extern int wrtd_in_has_trigger(struct wrtd_node *dev, unsigned int input,
			       unsigned int *assigned);
extern int wrtd_in_ping(struct wrtd_node *dev);
extern int wrtd_in_base_time(struct wrtd_node *dev, struct wr_timestamp *ts);
extern int wrtd_in_version(struct wrtd_node *dev,
			   struct wrnc_rt_version *version);
extern int wrtd_in_dead_time_get(struct wrtd_node *dev, unsigned int input,
				 uint64_t *dead_time_ps);
extern int wrtd_in_delay_get(struct wrtd_node *dev, unsigned int input,
			     uint64_t *delay_ps);


/**@}*/

/**
 * @file libwrtd-output.c
 */
/**
 * @defgroup output Output Management
 * Set of functions to handle output channels
 * @{
 */
extern int wrtd_out_state_get(struct wrtd_node *dev, unsigned int output,
			     struct wrtd_output_state *state);
extern int wrtd_out_enable(struct wrtd_node *dev, unsigned int output,
			   int enable);
extern int wrtd_out_trig_assign(struct wrtd_node *dev, unsigned int output,
				struct wrtd_trigger_handle *handle,
				struct wrtd_trig_id *trig,
				struct wrtd_trig_id *condition);
extern int wrtd_out_trig_unassign(struct wrtd_node *dev,
				  struct wrtd_trigger_handle *handle);
extern int wrtd_out_trig_get_all (struct wrtd_node *dev, unsigned int output,
				  struct wrtd_output_trigger_state *triggers,
				  int max_count);
extern int wrtd_out_trig_state_get_by_index(struct wrtd_node *dev,
					    unsigned int index,
					    unsigned int output,
					    struct wrtd_output_trigger_state *trigger);
extern int wrtd_out_trig_state_get_by_id(struct wrtd_node *dev,
					 unsigned int output,
					 struct wrtd_trig_id *id,
					 struct wrtd_output_trigger_state *trigger);
extern int wrtd_out_trig_state_get_by_handle(struct wrtd_node *dev,
					     struct wrtd_trigger_handle *handle,
					     struct wrtd_output_trigger_state *state);
extern int wrtd_out_trig_delay_set(struct wrtd_node *dev,
				   struct wrtd_trigger_handle *handle,
				   uint64_t delay_ps);
extern int wrtd_out_dead_time_set(struct wrtd_node *dev, unsigned int output,
				  uint64_t dead_time_ps);
extern int wrtd_out_pulse_width_set(struct wrtd_node *dev, unsigned int output,
				  uint64_t pulse_width_ps);
extern int wrtd_out_trig_enable(struct wrtd_node *dev,
				struct wrtd_trigger_handle *handle, int enable);
extern int wrtd_out_ping(struct wrtd_node *dev);
extern int wrtd_out_base_time(struct wrtd_node *dev, struct wr_timestamp *ts);
extern int wrtd_out_version(struct wrtd_node *dev, uint32_t *gitsha1);
extern int wrtd_out_trigger_mode_set(struct wrtd_node *dev,
				     unsigned int output,
				     enum wrtd_trigger_mode mode);
extern int wrtd_out_trig_condition_delay_set(struct wrtd_node *dev,
					     struct wrtd_trigger_handle *handle,
					     uint64_t delay_ps);
extern int wrtd_out_arm(struct wrtd_node *dev, unsigned int input, int armed);
extern int wrtd_out_counters_reset(struct wrtd_node *dev, unsigned int output);
extern int wrtd_out_check_triggered(struct wrtd_node *dev, unsigned int output);
extern int wrtd_out_is_enabled(struct wrtd_node *dev, unsigned int output,
			       unsigned int *enable);
extern int wrtd_out_is_armed(struct wrtd_node *dev, unsigned int output,
			     unsigned int *armed);
extern int wrtd_out_has_trigger(struct wrtd_node *dev, unsigned int output,
				struct wrtd_trig_id *id, unsigned int *assigned);
/**@}*/

#ifdef __cplusplus
};
#endif

#endif
