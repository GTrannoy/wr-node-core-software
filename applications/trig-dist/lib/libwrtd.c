/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *         inspired by a draft of Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * License: GPL v3
 */

#include <stdlib.h>
#include <errno.h>
#include <libwrnc.h>
#include "libwrtd-internal.h"

static struct wr_timestamp picos_to_ts( uint64_t p )
{
	struct wr_timestamp t;

	t.seconds = p / (1000ULL * 1000ULL * 1000ULL * 1000ULL);
	p %= (1000ULL * 1000ULL * 1000ULL * 1000ULL);
	t.ticks = p / 8000;
	p %= 8000;
	t.bins = p * 4096 / 8000;

	return t;
}

static void unbag_ts(uint32_t *buf, int offset, struct wr_timestamp *ts)
{
    ts->seconds = buf[offset];
    ts->ticks = buf[offset + 1];
    ts->bins = buf[offset + 2];
}

static const uint32_t application_id[] = {
	0x115790de,
};

static const char *wrtd_errors[] = {
	"Received an invalid answer from white-rabbit-node-code CPU",
	"Cannot read channel state",
	"You are using an invalid binary",
	"Invalid dead time value",
	"Invalid trigger identifier",
	"Invalid channel number",
	"Function not yet implemented",
};


/**
 * It returns the error message associated to a given error code
 * @param[in] err the error code
 * @return an error message
 */
const char *wrtd_strerror(int err)
{
	if (err < EWRTD_INVALD_ANSWER_ACK || err >= __EWRTD_MAX_ERROR_NUMBER)
		return wrnc_strerror(err);

	return wrtd_errors[err - EWRTD_INVALD_ANSWER_ACK];
}


/**
 * It initializes this library. It must be called before doing anything else.
 * This library is based on the libwrnc, so internally, this function also
 * run wrnc_init() in order to initialize the WRNC library.
 */
int wrtd_init()
{
	int err;

	err = wrnc_init();
	if (err)
		return err;

	return 0;
}

/**
 * It release all the library resources. It must be called when
 * you stop to use this library.
 */
void wrtd_exit()
{
	wrnc_exit();
}


/**
 * Open a WRTD node device using LUN
 * @param[in] device_id FMC device identificator
 * @return It returns an anonymous wrtd_node structure on success.
 *         On error, NULL is returned, and errno is set appropriately.
 */
struct wrtd_node *wrtd_open_by_fmc(uint32_t device_id)
{
	struct wrtd_desc *wrtd;

	wrtd = malloc(sizeof(struct wrtd_desc));
	if (!wrtd)
		return NULL;

	wrtd->wrnc = wrnc_open_by_fmc(device_id);
	if (!wrtd->wrnc)
		goto out;

	return (struct wrtd_node *)wrtd;

out:
	free(wrtd);
	return NULL;
}

/**
 * Open a WRTD node device using LUN
 * @param[in] lun an integer argument to select the device or
 *            negative number to take the first one found.
 * @return It returns an anonymous wrtd_node structure on success.
 *         On error, NULL is returned, and errno is set appropriately.
 */
struct wrtd_node *wrtd_open_by_lun(int lun)
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
void wrtd_close(struct wrtd_node *dev)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;

	wrnc_close(wrtd->wrnc);
	free(wrtd);
	dev = NULL;
}


/**
 * It returns the white-rabbit node-core token
 * @param[in] dev trig-dist device to use
 * @return the wrnc token
 */
struct wrnc_dev *wrtd_get_wrnc_dev(struct wrtd_node *dev)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;

	return (struct wrnc_dev *)wrtd->wrnc;
}


/**
 * It load a set of real-time applications for TDC and FD
 * @param[in] dev wrtd device to use
 * @param[in] rt_tdc path to the TDC application
 * @param[in] rt_fd path to the Fine Delay application
 */
int wrtd_load_application(struct wrtd_node *dev, char *rt_tdc,
			  char *rt_fd)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	uint32_t reg_old;
	int err;

	if (!rt_tdc || !rt_fd) {
		errno = EWRTD_INVALD_BINARY;
		return -1;
	}
	err = wrnc_cpu_reset_get(wrtd->wrnc, &reg_old);
	if (err)
		return err;

	/* Keep the CPUs in reset state */
	err = wrnc_cpu_reset_set(wrtd->wrnc,
				 (1 << WRTD_CPU_TDC) | (1 << WRTD_CPU_FD));
	if (err)
		return err;

	/* Program CPUs application */
	err = wrnc_cpu_load_application_file(wrtd->wrnc, WRTD_CPU_TDC, rt_tdc);
	if (err)
		return err;
	err = wrnc_cpu_load_application_file(wrtd->wrnc, WRTD_CPU_FD, rt_tdc);
	if (err)
		return err;

	/* Re-enable the CPUs */
	err = wrnc_cpu_reset_set(wrtd->wrnc, reg_old);
	if (err)
		return err;

	return 0;
}

static inline int wrtd_send_and_receive_sync(struct wrtd_desc *wrtd, struct wrnc_msg *msg)
{
	/* Send the message and get answer */
	return wrnc_slot_send_and_receive_sync(wrtd->wrnc,
					       WRTD_IN_TDC_CONTROL,
					       WRTD_OUT_TDC_CONTROL,
					       msg,
					       WRTD_DEFAULT_TIMEOUT);
}

static inline int wrtd_validate_acknowledge(struct wrnc_msg *msg)
{
	if (msg->datalen != 2 || msg->data[0] != WRTD_REP_ACK_ID) {
		errno = EWRTD_INVALD_ANSWER_ACK;
		return -1;
	}

	return 0;
}


/**
 * It retreives the current status of a given input channel
 * @param[in] dev pointer to open node device.
 * @param[in] input channel to use
 * @param[out] state the current state of a channel
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_state_get(struct wrtd_node *dev, unsigned int input,
		      struct wrtd_input_state *state)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg msg;
	int err;

  	if (input >= WRTD_IN_MAX) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	if (state == NULL) {
		errno = ENOMEM;
		return -1;
	}

	/* Build the message */
	msg.datalen = 3;
	msg.data[0] = WRTD_CMD_TDC_CHAN_GET_STATE;
	msg.data[1] = 0;
	msg.data[2] = input;

	/* Send the message and get answer */
	err = wrtd_send_and_receive_sync(wrtd, &msg);
        if (msg.datalen != 29 || msg.data[0] != WRTD_REP_STATE) {
		errno = EWRTD_INVALD_ANSWER_STATE;
		return -1;
	}

	/* Copy status information */
	state->input = input;
	state->flags = msg.data[15];
	state->log_level = msg.data[16];
	state->mode = msg.data[17];
	state->tagged_pulses = msg.data[18];
	state->sent_triggers = msg.data[19];
	state->dead_time.seconds = 0;
	state->dead_time.bins = 0;
	state->dead_time.ticks = msg.data[20] * 2;

	state->assigned_id.system = msg.data[3];
	state->assigned_id.source_port = msg.data[4];
	state->assigned_id.trigger = msg.data[5];

	unbag_ts(msg.data, 6, &state->delay);
	unbag_ts(msg.data, 12, &state->last);
	unbag_ts(msg.data, 21, &state->last_sent.ts);
	state->last_sent.id.system = msg.data[24];
	state->last_sent.id.source_port = msg.data[25];
	state->last_sent.id.trigger = msg.data[26];
	state->last_sent.seq = msg.data[27];
	state->sent_packets = msg.data[28];

	return 0;
}


/**
 * Hardware enable/disable a WRTD input channel.
 * @param[in] dev pointer to open node device.
 * @param[in] input index of the trigger input to enable
 * @param[in] enable non-0 enables the input, 0 disables it.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_enable(struct wrtd_node *dev, unsigned int input, int enable)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg msg;
	int err;

	if (input >= WRTD_IN_MAX) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	/* Build the message */
	msg.datalen = 4;
	msg.data[0] = WRTD_CMD_TDC_CHAN_ENABLE;
	msg.data[1] = 0;
	msg.data[2] = input;
	msg.data[3] = !!enable;

	/* Send the message and get answer */
	err = wrtd_send_and_receive_sync(wrtd, &msg);
	if (err)
		return err;

	return wrtd_validate_acknowledge(&msg);
}


/**
 * Set the dead time (the minimum gap between input pulses, below which
 * the TDC ignores the subsequent pulses; limits maximum input pulse rate,
 * 16 ns granularity)
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_dead_time_set(struct wrtd_node *dev, unsigned int input,
				 uint64_t dead_time_ps)
{
  	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg msg;
	int err, dead_time_cycles;

	if (input >= WRTD_IN_MAX) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	dead_time_cycles = dead_time_ps / 16000;
	if(dead_time_cycles < 5000 || dead_time_cycles > 10000000 ) {
		errno = EWRTD_INVALD_DEAD_TIME;
		return -1;
	}

	/* Build the message */
	msg.datalen = 4;
	msg.data[0] = WRTD_CMD_TDC_CHAN_SET_DEAD_TIME;
	msg.data[1] = 0;
	msg.data[2] = input;
	msg.data[3] = dead_time_cycles;

	/* Send the message and get answer */
	err = wrtd_send_and_receive_sync(wrtd, &msg);
	if (err)
		return err;

	return wrtd_validate_acknowledge(&msg);
}


/**
 * Get the dead time (the minimum gap between input pulses, below which
 * the TDC ignores the subsequent pulses; limits maximum input pulse rate,
 * 16 ns granularity)
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_dead_time_get(struct wrtd_node *dev, unsigned int input,
				 uint64_t *dead_time_ps)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}


/**
 * Set the offset (for compensating cable delays), in 10 ps steps.
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_delay_set(struct wrtd_node *dev, unsigned int input,
				 uint64_t delay_ps)
{
  	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wr_timestamp t;
	struct wrnc_msg msg;
	int err;

	if (input >= WRTD_IN_MAX) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

        t = picos_to_ts(delay_ps);

	/* Build the message */
	msg.datalen = 6;
	msg.data[0] = WRTD_CMD_TDC_CHAN_SET_DELAY;
	msg.data[1] = 0;
	msg.data[2] = input;
	msg.data[3] = t.seconds;
	msg.data[4] = t.ticks;
	msg.data[5] = t.bins;

	/* Send the message and get answer */
	err = wrtd_send_and_receive_sync(wrtd, &msg);
	if (err)
		return err;

	return wrtd_validate_acknowledge(&msg);
}


/**
 * Get the offset (for compensating cable delays), in 10 ps steps.
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_delay_get(struct wrtd_node *dev, unsigned int input,
			     uint64_t *delay_ps)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
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
int wrtd_in_trigger_mode_set(struct wrtd_node *dev, unsigned int input,
				    enum wrtd_trigger_mode mode)
{
    	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg msg;
	int err;

	if (input >= WRTD_IN_MAX) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	/* Build the message */
	msg.datalen = 4;
	msg.data[0] = WRTD_CMD_TDC_CHAN_SET_MODE;
	msg.data[1] = 0;
	msg.data[2] = input;
	msg.data[3] = mode;

	/* Send the message and get answer */
	err = wrtd_send_and_receive_sync(wrtd, &msg);
	if (err)
		return err;

	return wrtd_validate_acknowledge(&msg);
}


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
int wrtd_in_trigger_assign(struct wrtd_node *dev, unsigned int input,
					  struct wrtd_trig_id *trig_id)
{
   	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg msg;
	int err;

	if (input >= WRTD_IN_MAX) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	/* Build the message */
	msg.datalen = 7;
	msg.data[0] = WRTD_CMD_TDC_CHAN_ASSIGN_TRIGGER;
	msg.data[1] = 0;
	msg.data[2] = input;
	/* '? :' should be optimized by the compiler */
	msg.data[3] = trig_id ? 1 : 0;
	msg.data[4] = trig_id ? trig_id->system : 0;
	msg.data[5] = trig_id ? trig_id->source_port : 0;
	msg.data[6] = trig_id ? trig_id->trigger : 0;

	/* Send the message and get answer */
	err = wrtd_send_and_receive_sync(wrtd, &msg);
	if (err)
		return err;

	return wrtd_validate_acknowledge(&msg);
}


/**
 * It un-assign the trigger on an input channel. It is just an helper that
 * internally use wrtd_in_trigger_unassign()
 */
int wrtd_in_trigger_unassign(struct wrtd_node *dev,
					   unsigned int input)
{
	return wrtd_in_trigger_assign(dev, input, NULL);
}


/**
 * Arm (disarm) a WRTD input for triggering. By arming the input, you are making
 * it ready to accept/send triggers
 * @param[in] dev pointer to open node device.
 * @param[in] input index of the trigger input
 * @param[in] armed 1 arms the input, 0 disarms the input.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_arm(struct wrtd_node *dev, unsigned int input, int armed)
{
   	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg msg;
	int err;

	if (input >= WRTD_IN_MAX) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	/* Build the message */
	msg.datalen = 4;
	msg.data[0] = WRTD_CMD_TDC_CHAN_ARM;
	msg.data[1] = 0;
	msg.data[2] = input;
	msg.data[3] = !!armed;

	/* Send the message and get answer */
	err = wrtd_send_and_receive_sync(wrtd, &msg);
	if (err)
		return err;

	return wrtd_validate_acknowledge(&msg);
}


/**
 * Disarm the WRTD input. It is just an helper that internally use wrtd_in_arm()
 */
int wrtd_in_disarm(struct wrtd_node *dev, unsigned int input, int armed)
{
	return wrtd_in_arm(dev, input, 0);
}



/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_counters_reset(struct wrtd_node *dev, unsigned int input)
{
   	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg msg;
	int err;

	if (input >= WRTD_IN_MAX) {
		errno = EWRTD_INVALID_CHANNEL;
		return -1;
	}

	/* Build the message */
	msg.datalen = 3;
	msg.data[0] = WRTD_CMD_TDC_CHAN_RESET_COUNTERS;
	msg.data[1] = 0;
	msg.data[2] = input;

	/* Send the message and get answer */
	err = wrtd_send_and_receive_sync(wrtd, &msg);
	if (err)
		return err;

	return wrtd_validate_acknowledge(&msg);
}


/**
 * Software-trigger the input at a given TAI value
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_trigger_software(struct wrtd_node *dev,
			     struct wrtd_trigger_entry *trigger)
{
   	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	struct wrnc_msg msg;
	uint32_t *buf = (uint32_t *)trigger;
	int err, i;

	if (trigger == NULL) {
		errno = EWRTD_INVALID_TRIG_ID;
		return -1;
	}

	/* Build the message */
	msg.datalen = 2 + (sizeof(struct wrtd_trigger_entry) / 4);
	msg.data[0] = WRTD_CMD_TDC_SOFTWARE_TRIGGER;
	msg.data[1] = 0;
	for (i = 2; i < msg.datalen; ++i)
		msg.data[i] = buf[i - 2];

	/* Send the message and get answer */
	err = wrtd_send_and_receive_sync(wrtd, &msg);
	if (err)
		return err;

	return wrtd_validate_acknowledge(&msg);
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
int wrtd_in_read_log(struct wrtd_node *dev, struct wrtd_log_entry *log,
			      int flags, int input_mask, int count)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}


/**
 * Check the enable status on a trigger input.
 * @param[in] dev pointer to open node device.
 * @param[in] input index of the trigger input to enable
 * @param[in] enable 1 enables the input, 0 disables it.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_is_enabled(struct wrtd_node *dev, unsigned int input)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}


/**
 * Get/set the Sequence ID counter (counting up at every pulse)
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_seq_counter_set(struct wrtd_node *dev, unsigned int input)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}


/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_timebase_offset_set(struct wrtd_node *dev, unsigned int input,
				uint64_t offset)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}


/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_in_log_level_set(struct wrtd_node *dev, unsigned int input,
			  uint32_t log_level)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}







/**
 * @param[in] dev pointer to open node device.
 * @param[in] output channel to use
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_enable(struct wrtd_node *dev, unsigned int output,
			   int enable)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}


/**
 * @param[in] dev pointer to open node device.
 * @param[in] output channel to use
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_dead_time_set(struct wrtd_node *dev, unsigned int output,
				  uint64_t dead_time_ps)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}


/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_trig_assign(struct wrtd_node *dev,
				struct wrtd_trigger_handle *handle,
				int output, struct wrtd_trig_id *trig,
				struct wrtd_trig_id *condition)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}


/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_trig_remove (struct wrtd_node *dev,
				 struct wrtd_trigger_handle *handle)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}


/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_trig_get_all (struct wrtd_node *dev, unsigned int output,
				  struct wrtd_output_trigger_state *triggers,
				  int max_count)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}


/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_trig_set_delay(struct wrtd_node *dev,
				   struct wrtd_trigger_handle *handle,
				   uint64_t delay_ps)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}


/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_trig_set_condition_delay(struct wrtd_node *dev,
					     struct wrtd_trigger_handle *handle,
					     uint64_t delay_ps)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}


/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_trig_get_state(struct wrtd_node *dev,
				   struct wrtd_trigger_handle *handle,
				   struct wrtd_output_trigger_state *state)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}


/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_trig_enable(struct wrtd_node *dev,
				struct wrtd_trigger_handle *handle, int enable)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}


/**
 * @param[in] dev pointer to open node device.
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_read_log(struct wrtd_node *dev, struct wrtd_log_entry *log,
			     int flags, unsigned int output_mask, int count)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}

/**
 * @param[in] dev pointer to open node device.
 * @param[in] output channel to use
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_set_log_level(struct wrtd_node *dev, unsigned int output,
				  uint32_t log_level)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}

/**
 * @param[in] dev pointer to open node device.
 * @param[in] output channel to use
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_set_trigger_mode(struct wrtd_node *dev,
				     unsigned int output, int mode)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}


/**
 * @param[in] dev pointer to open node device.
 * @param[in] output channel to use
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_arm(struct wrtd_node *dev, unsigned int ouput, int armed)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}
//int wrtd_out_get_state(struct wrtd_node *dev, unsigned int input, int armed);


/**
 * @param[in] dev pointer to open node device.
 * @param[in] output channel to use
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_reset_counters(struct wrtd_node *dev, unsigned int output)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}


/**
 * @param[in] dev pointer to open node device.
 * @param[in] output channel to use
 * @return 0 on success, -1 on error and errno is set appropriately
 */
extern int wrtd_out_check_triggered(struct wrtd_node *dev, unsigned int output)
{
	errno = EWRTD_NO_IMPLEMENTATION;
	return -1;
}
//int wrtd_out_wait_trigger(struct wrtd_node*, int output_mask, struct wrtd_trig_id *id);
