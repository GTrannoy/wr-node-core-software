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
 * It returns the error message associated to a given error code
 * @param[in] err the error code
 * @return an error message
 */
extern const char *wrtd_strerror(int err);

/**
 * It initializes this library. This library is based on the libwrnc,
 * so internally, this function also run wrnc_init() in order to initialize the
 * WRNC library.
 */
extern int wrtd_init();

/**
 * It release all the library resources
 */
extern void wrtd_exit();

/**
 * Open a WRTD node device using LUN
 * @param[in] device_id FMC device identificator
 * @return It returns an anonymous wrtd_node structure on success.
 *         On error, NULL is returned, and errno is set appropriately.
 */
extern struct wrtd_node *wrtd_open_by_fmc(uint32_t device_id);

/**
 * Open a WRTD node device using LUN
 * @param[in] lun an integer argument to select the device or
 *            negative number to take the first one found.
 * @return It returns an anonymous wrtd_node structure on success.
 *         On error, NULL is returned, and errno is set appropriately.
 */
static inline struct wrtd_node *wrtd_open_by_lun(int lun)
{
	char name[12];
	uint32_t dev_id;

	snprintf(name, 12, "wrnc.%d", lun);
	/*TODO convert to FMC */
	return wrtd_open_by_fmc(dev_id);
}

/**
 * Close a LIST node device.
 * @param[in] dev pointer to open node device.
 */

extern void wrtd_close(struct wrtd_node *dev);

/**
 * It returns the white-rabbit node-core token
 * @param[in] dev trig-dist device to use
 * @return the wrnc token
 */
extern struct wrnc_dev *wrtd_get_wrnc_dev(struct wrtd_node *dev);

/**
 * It load a set of real-time applications
 */
extern int wrtd_load_application(struct wrtd_node *dev, char *rt_tdc,
					 char *rt_fd);

/**
 * Hardware enable/disable a WRTD trigger input.
 * @param[in] dev pointer to open node device.
 * @param[in] input index of the trigger input to enable
 * @param[in] enable non-0 enables the input, 0 disables it.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_in_enable(struct wrtd_node *dev, unsigned int input, int enable);


/**
 * Check the enable status on a trigger input.
 * @param[in] dev pointer to open node device.
 * @param[in] input index of the trigger input to enable
 * @param[in] enable 1 enables the input, 0 disables it.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_in_is_enabled(struct wrtd_node *dev, unsigned int input);


/**
 * Assign (unassign) a trigger ID to a given WRTD input. Passing a NULL trig_id
 * un-assigns the current trigger (the input will be tagging pulses and
 * logging them, but they will not be sent as triggers to the WR network).
 * @param[in] dev pointer to open node device.
 * @param[in] input index of the trigger input to assign trigger to.
 * @param[in] trig_id the trigger to be sent upon reception of a pulse on the
 *            given input.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_in_trigger_assign(struct wrtd_node *dev, unsigned int input,
					  struct wrtd_trig_id *trig_id);

/**
 * It un-assign the trigger on an input channel. It is just an helper that
 * internally use wrtd_in_trigger_unassign()
 */
static inline int wrtd_in_trigger_unassign(struct wrtd_node *dev,
					   unsigned int input)
{
	return wrtd_in_trigger_assign(dev, input, NULL);
}

/**
 * Set trigger mode for a given WRTD input. Note that the input must be armed
 * by calling wrtd_in_arm() at least once before it can send triggers.
 *
 * The mode can be single shot or continuous. Single shot means the input will
 * trigger on the first incoming pulse and will ignore the subsequent pulses
 * until re-armed.
 *
 * @param[in] dev pointer to open node device.
 * @param[in] input index of the trigger input
 * @param[in] mode triggering mode.
 * @return 0 on success, -1 on error and errno is set appropriately
*/
extern int wrtd_in_trigger_mode_set(struct wrtd_node *dev, unsigned int input,
				    enum wrtd_trigger_mode mode);

/**
 * Arm (disarm) a WRTD input for triggering. By arming the input, you are making
 * it ready to accept/send triggers
 * @param[in] dev pointer to open node device.
 * @param[in] input index of the trigger input
 * @param[in] armed 1 arms the input, 0 disarms the input.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_in_arm(struct wrtd_node *dev, unsigned int input, int armed);

/**
 * Disarm the WRTD input. It is just an helper that internally use wrtd_in_arm()
 */
static inline int wrtd_in_disarm(struct wrtd_node *dev, unsigned int input, int armed)
{
	return wrtd_in_arm(dev, input, 0);
}

/**
 * Log every trigger pulse sent out to the network. Each log message contains
 * the input number, sequence ID, trigger ID, trigger counter (since arm) and
 * origin timestamp.
 * @param[in] dev pointer to open node device.
 * @param[out] log
 * @param[in] flags
 * @param[in] input_mask
 * @param[in] count
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_in_read_log(struct wrtd_node *dev, struct wrtd_log_entry *log,
			      int flags, int input_mask, int count);

/**
 * Software-trigger the input at a given TAI value
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_in_trigger_software(struct wrtd_node *dev,
			     struct wrtd_trigger_entry *trigger);

/**
 * Set the offset (for compensating cable delays), in 10 ps steps.
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_in_delay_set(struct wrtd_node *dev, unsigned int input,
			     uint64_t delay_ps);

/**
 * Get the offset (for compensating cable delays), in 10 ps steps.
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_in_delay_get(struct wrtd_node *dev, unsigned int input,
			     uint64_t *delay_ps);

/**
 * Get/set the Sequence ID counter (counting up at every pulse)
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_in_seq_counter_set (struct wrtd_node *dev, unsigned int input);


/**
 * Set the dead time (the minimum gap between input pulses, below which
 * the TDC ignores the subsequent pulses; limits maximum input pulse rate,
 * 16 ns granularity)
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_in_dead_time_set(struct wrtd_node *dev, unsigned int input,
				 uint64_t dead_time_ps);

/**
 * Get the dead time (the minimum gap between input pulses, below which
 * the TDC ignores the subsequent pulses; limits maximum input pulse rate,
 * 16 ns granularity)
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_in_dead_time_get(struct wrtd_node *dev, unsigned int input,
				 uint64_t *dead_time_ps);

/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_in_timebase_offset_set(struct wrtd_node *dev,
				       unsigned int input, uint64_t offset);

/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_in_state_get(struct wrtd_node *dev, unsigned int input,
			     struct wrtd_input_state *state);

/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_in_counters_reset(struct wrtd_node *dev, unsigned int input);

/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_in_log_level_set(struct wrtd_node *dev, unsigned int input,
				 uint32_t log_level);


/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_enable(struct wrtd_node *dev, unsigned int output,
			   int enable);

/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_dead_time_set(struct wrtd_node *dev, unsigned int output,
				  uint64_t dead_time_ps);

/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_trig_assign(struct wrtd_node *dev,
				struct wrtd_trigger_handle *handle,
				int output, struct wrtd_trig_id *trig,
				struct wrtd_trig_id *condition);

/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_trig_remove (struct wrtd_node *dev,
				 struct wrtd_trigger_handle *handle);

/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_trig_get_all (struct wrtd_node *dev, unsigned int output,
				  struct wrtd_output_trigger_state *triggers,
				  int max_count);

/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_trig_set_delay(struct wrtd_node *dev,
				   struct wrtd_trigger_handle *handle,
				   uint64_t delay_ps);

/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_trig_set_condition_delay(struct wrtd_node *dev,
					     struct wrtd_trigger_handle *handle,
					     uint64_t delay_ps);

/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_trig_get_state(struct wrtd_node *dev,
				   struct wrtd_trigger_handle *handle,
				   struct wrtd_output_trigger_state *state);

/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_trig_enable(struct wrtd_node *dev,
				struct wrtd_trigger_handle *handle, int enable);

/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_read_log(struct wrtd_node*, struct wrtd_log_entry *log,
			     int flags, unsigned int output_mask, int count);

/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_set_log_level(struct wrtd_node *dev, unsigned int output,
				  uint32_t log_level);

/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_set_trigger_mode(struct wrtd_node *dev,
				     unsigned int output, int mode);

/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_arm(struct wrtd_node *node, unsigned int input, int armed);
//int wrtd_out_get_state(struct wrtd_node *node, unsigned int input, int armed);

/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_reset_counters(struct wrtd_node *dev, unsigned int output);

/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_check_triggered(struct wrtd_node *dev, unsigned int output);
//int wrtd_out_wait_trigger(struct wrtd_node*, int output_mask, struct wrtd_trig_id *id);

#endif
