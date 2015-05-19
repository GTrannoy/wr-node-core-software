/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *         inspired by a draft of Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * License: GPL v3
 */

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <libwrnc.h>
#include <libwrtd-internal.h>
#include <wrtd-serializers.h>

static const uint32_t application_id[] = {
	0x115790de,
};

const char *wrtd_errors[] = {
	"Received an invalid answer from white-rabbit-node-code CPU",
	"Cannot read channel/trigger state",
	"You are using an invalid binary",
	"Invalid dead time value",
	"Invalid delay value",
	"Invalid trigger identifier",
	"Invalid channel number",
	"Function not yet implemented",
	"Received an invalid trigger entry",
	"Received an invalid hash entry",
	"Received an invalid hash chain",
	"Received an invalid trigger handle",
	"Trigger not found",
	"No trigger condition",
	"Invalid pulse width",
};


/**
 * It returns a string messages corresponding to a given error code. If
 * it is not a libwrtd error code, it will run wrnc_strerror()
 * @param[in] err error code
 * @return a message error
 */
const char *wrtd_strerror(int err)
{
	if (err < EWRTD_INVALID_ANSWER_ACK || err >= __EWRTD_MAX_ERROR_NUMBER)
		return wrnc_strerror(err);

	return wrtd_errors[err - EWRTD_INVALID_ANSWER_ACK];
}


/**
 * It returns a string that describe a given log level
 * @param[in] lvl log level
 * @return a string if the log level is mapped, otherwise an empty string
 */
const char *wrtd_strlogging(enum wrtd_log_level lvl)
{
	switch (lvl) {
	case WRTD_LOG_NOTHING:
		return "No logging";
	case WRTD_LOG_RAW:
		return "incoming pulse";
	case WRTD_LOG_SENT:
		return "trigger message sent";
	case WRTD_LOG_PROMISC:
		return "received trigger message";
	case WRTD_LOG_FILTERED:
		return "trigger message assigned";
	case WRTD_LOG_EXECUTED:
		return "pulse generated";
	case WRTD_LOG_MISSED:
		return "pulse missed";
	case WRTD_LOG_ALL:
		return "all";
	}

	return "n/a";
}

/**
 * It initializes the WRTD library. It must be called before doing
 * anything else. If you are going to load/unload WRTD devices, then
 * you have to un-load (wrtd_exit()) e reload (wrtd_init()) the library.
 *
 * This library is based on the libwrnc, so internally, this function also
 * run wrnc_init() in order to initialize the WRNC library.
 * @return 0 on success, otherwise -1 and errno is appropriately set
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
 * It releases the resources allocated by wrtd_init(). It must be called when
 * you stop to use this library. Then, you cannot use functions from this
 * library.
 */
void wrtd_exit()
{
	wrnc_exit();
}


/**
 * Open a WRTD node device using FMC ID
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

	wrtd->dev_id = device_id;
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
	struct wrtd_desc *wrtd;

	wrtd = malloc(sizeof(struct wrtd_desc));
	if (!wrtd)
		return NULL;

	wrtd->wrnc = wrnc_open_by_lun(lun);
	if (!wrtd->wrnc)
		goto out;

	wrtd->dev_id = lun;

	return (struct wrtd_node *)wrtd;

out:
	free(wrtd);
	return NULL;
}


/**
 * It closes a WRTD device opened with one of the following function:
 * wrtd_open_by_lun(), wrtd_open_by_fmc()
 * @param[in] dev device token
 */
void wrtd_close(struct wrtd_node *dev)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;

	wrnc_close(wrtd->wrnc);
	free(wrtd);
	dev = NULL;
}


/**
 * It returns the WRNC token in order to allows users to run
 * functions from the WRNC library
 * @param[in] dev device token
 * @return the WRNC token
 */
struct wrnc_dev *wrtd_get_wrnc_dev(struct wrtd_node *dev)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;

	return (struct wrnc_dev *)wrtd->wrnc;
}




/**
 * It loads a set of real-time applications for TDC and FD
 * @param[in] dev device token
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
		errno = EWRTD_INVALID_BINARY;
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
	err = wrnc_cpu_load_application_file(wrtd->wrnc, WRTD_CPU_FD, rt_fd);
	if (err)
		return err;

	/* Re-enable the CPUs */
	err = wrnc_cpu_reset_set(wrtd->wrnc, reg_old);
	if (err)
		return err;

	return 0;
}


/**
 * It reads one or more log entry from a given hmq_log. The user of this function
 * must check that the hmq_log used correspond to a logging interface
 * @param[in] hmq_log logging HMQ.
 * @param[out] log log message
 * @param[in] count number of messages to read
 * @return number of read messages on success (check errno if it returns less
 *         messages than expected), -1 on error and errno is set appropriately
 */
int wrtd_log_read(struct wrnc_hmq *hmq_log, struct wrtd_log_entry *log,
		  int count)
{
	struct wrtd_log_entry *cur = log;
	struct wrnc_msg *msg;
	int remaining = count;
	int n_read = 0;
	uint32_t id = 0, seq = 0;

	/* Clean up errno to be able to distinguish between error cases and
	   normal behaviour when the function return less messages
	   than expected */
	errno = 0;
	while (remaining) {
		struct wrtd_trigger_entry ent;
		msg = wrnc_hmq_receive(hmq_log);
		if (!msg)
			break;

		wrnc_msg_header (msg, &id, &seq);

		if (id != WRTD_REP_LOG_MESSAGE)
		{
			free(msg);
			errno = EWRTD_INVALID_ANSWER_STATE;
			break;
		}

		wrnc_msg_uint32 (msg, &cur->type);
		wrnc_msg_int32 (msg, &cur->channel);
		wrnc_msg_uint32 (msg, &cur->miss_reason);
		wrtd_msg_trigger_entry(msg, &ent);

		cur->ts = ent.ts;
		cur->seq = ent.seq;
		cur->id = ent.id;

		if ( wrnc_msg_check_error(msg) ) {
			free(msg);
			errno = EWRTD_INVALID_ANSWER_STATE;
			break;
		}

		remaining--;
		n_read++;
		cur++;
		free(msg);
	}

	return n_read ? n_read : -1;
}

/**
 * It closes the logging interface
 */
void wrtd_log_close(struct wrnc_hmq *hmq)
{
	wrnc_hmq_close(hmq);
}


/**
 * It converts the white rabbit time stamp to a pico seconds
 * @param[in] ts time-stamp
 * @param[out] pico pico-seconds
 */
void wrtd_ts_to_pico(struct wr_timestamp *ts, uint64_t *pico)
{
	uint64_t p;

	p = ts->frac * 8000 / 4096;
	p += (uint64_t) ts->ticks * 8000LL;
	p += ts->seconds * (1000ULL * 1000ULL * 1000ULL * 1000ULL);
	*pico = p;
}


/**
 * It converts a pico seconds integer into a white rabbit time stamp
 * @param[in] pico pico-seconds
 * @param[out] ts time-stamp
 */
void wrtd_pico_to_ts(uint64_t *pico, struct wr_timestamp *ts)
{
	uint64_t p = *pico;

	ts->seconds = p / (1000ULL * 1000ULL * 1000ULL * 1000ULL);
	p %= (1000ULL * 1000ULL * 1000ULL * 1000ULL);
	ts->ticks = p / 8000;
	p %= 8000;
	ts->frac = p * 4096 / 8000;
}


/**
 * It converts a white rabbit time stamp to seconds and pico-seconds
 * @param[in] ts time-stamp
 * @param[out] sec seconds
 * @param[out] pico pico-seconds
 */
void wrtd_ts_to_sec_pico(struct wr_timestamp *ts, uint64_t *sec, uint64_t *pico)
{
	*sec = ts->seconds;
	*pico = ts->frac * 8000 / 4096;
	*pico += (uint64_t) ts->ticks * 8000LL;
}


/**
 * It converts a white rabbit time stamp to seconds and pico-seconds
 * @param[in] sec seconds
 * @param[in] pico pico-seconds
 * @param[out] ts time-stamp
 */
void wrtd_sec_pico_to_ts(uint64_t sec, uint64_t pico, struct wr_timestamp *ts)
{
	ts->seconds = sec;
	ts->ticks = pico / 8000;
	ts->frac = (pico % 8000) * 4096 / 8000;
}
