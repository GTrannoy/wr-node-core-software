/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *         inspired by a draft of Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * License: GPL v3
 */

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <libmockturtle.h>
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
	"Invalid input real-time application version",
	"Invalid output real-time application version"
};


/**
 * It returns a string messages corresponding to a given error code. If
 * it is not a libwrtd error code, it will run trtl_strerror()
 * @param[in] err error code
 * @return a message error
 */
const char *wrtd_strerror(int err)
{
	if (err < EWRTD_INVALID_ANSWER_ACK || err >= __EWRTD_MAX_ERROR_NUMBER)
		return trtl_strerror(err);

	return wrtd_errors[err - EWRTD_INVALID_ANSWER_ACK];
}


/**
 * It initializes the WRTD library. It must be called before doing
 * anything else. If you are going to load/unload WRTD devices, then
 * you have to un-load (wrtd_exit()) e reload (wrtd_init()) the library.
 *
 * This library is based on the libmockturtle, so internally, this function also
 * run trtl_init() in order to initialize the WRNC library.
 * @return 0 on success, otherwise -1 and errno is appropriately set
 */
int wrtd_init()
{
	int err;

	err = trtl_init();
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
	trtl_exit();
}


/**
 * Check if the RT-app running version is compatible with the current
 * library
 * @param[in] dev device token
 * @return 1 if the version is correct, 0 otherwise and errno is
 *         appropriately set
 */
int wrtd_version_is_valid(struct wrtd_node *dev)
{
	struct trtl_rt_version version;
	int err;

	errno = 0;
	err = wrtd_in_version(dev, &version);
	if (err)
		return 0;

	if (version.rt_id != WRTD_IN_RT_ID) {
		errno = EWRTD_INVALID_IN_APP;
		return 0;
	}

	return 1;
}


/**
 * It opens and initialize the configuration for the given device
 * @param[in] device_id device identifier
 * @param[in] is_lun 1 if device_id is a LUN
 * @return It returns an anonymous wrtd_node structure on success.
 *         On error, NULL is returned, and errno is set appropriately.
 */
static struct wrtd_node *wrtd_open(uint32_t device_id, unsigned int is_lun)
{
	struct wrtd_desc *wrtd;
	int err;

	wrtd = malloc(sizeof(struct wrtd_desc));
	if (!wrtd)
		return NULL;

	if (is_lun)
		wrtd->trtl = trtl_open_by_lun(device_id);
	else
		wrtd->trtl = trtl_open_by_fmc(device_id);
	if (!wrtd->trtl)
		goto out;

	wrtd->dev_id = device_id;

	/* Logging interface is always in share mode */
	err = trtl_hmq_share_set(wrtd->trtl, TRTL_HMQ_OUTCOMING,
				 WRTD_OUT_FD_LOGGING, 1);
	if (err)
		goto out;

	err = trtl_hmq_share_set(wrtd->trtl, TRTL_HMQ_OUTCOMING,
				 WRTD_OUT_TDC_LOGGING, 1);
	if (err)
		goto out;

	return (struct wrtd_node *)wrtd;

out:
	free(wrtd);
	return NULL;
}

/**
 * Open a WRTD node device using FMC ID
 * @param[in] device_id FMC device identificator
 * @return It returns an anonymous wrtd_node structure on success.
 *         On error, NULL is returned, and errno is set appropriately.
 */
struct wrtd_node *wrtd_open_by_fmc(uint32_t device_id)
{
	return wrtd_open(device_id, 0);
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
	return wrtd_open(lun, 1);
}


/**
 * It closes a WRTD device opened with one of the following function:
 * wrtd_open_by_lun(), wrtd_open_by_fmc()
 * @param[in] dev device token
 */
void wrtd_close(struct wrtd_node *dev)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;

	trtl_close(wrtd->trtl);
	free(wrtd);
	dev = NULL;
}


/**
 * It returns the WRNC token in order to allows users to run
 * functions from the WRNC library
 * @param[in] dev device token
 * @return the WRNC token
 */
struct trtl_dev *wrtd_get_trtl_dev(struct wrtd_node *dev)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;

	return (struct trtl_dev *)wrtd->trtl;
}


/**
 * It restarts both real-time applications
 * @param[in] dev device token
 * @return 0 on success, -1 on error and errno is set appropriately
 */
int wrtd_cpu_restart(struct wrtd_node *dev)
{
	struct wrtd_desc *wrtd = (struct wrtd_desc *)dev;
	int err;

	err = trtl_cpu_disable(wrtd->trtl,WRTD_CPU_TDC);
	if (err)
		return err;
	err = trtl_cpu_disable(wrtd->trtl,WRTD_CPU_FD);
	if (err)
		return err;
	err = trtl_cpu_enable(wrtd->trtl,WRTD_CPU_TDC);
	if (err)
		return err;
	return trtl_cpu_enable(wrtd->trtl,WRTD_CPU_FD);
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
	err = trtl_cpu_reset_get(wrtd->trtl, &reg_old);
	if (err)
		return err;

	/* Keep the CPUs in reset state */
	err = trtl_cpu_reset_set(wrtd->trtl,
				 (1 << WRTD_CPU_TDC) | (1 << WRTD_CPU_FD));
	if (err)
		return err;

	/* Program CPUs application */
	err = trtl_cpu_load_application_file(wrtd->trtl, WRTD_CPU_TDC, rt_tdc);
	if (err)
		return err;
	err = trtl_cpu_load_application_file(wrtd->trtl, WRTD_CPU_FD, rt_fd);
	if (err)
		return err;

	/* Re-enable the CPUs */
	err = trtl_cpu_reset_set(wrtd->trtl, reg_old);
	if (err)
		return err;

	return 0;
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
