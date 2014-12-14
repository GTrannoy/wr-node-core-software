/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *         inspired by a draft of Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * License: GPL v3
 */

#include <stdlib.h>
#include <errno.h>
#include <libwrnc.h>
#include <libwrtd-internal.h>

static const uint32_t application_id[] = {
	0x115790de,
};

const char *wrtd_errors[] = {
	"Received an invalid answer from white-rabbit-node-code CPU",
	"Cannot read channel state",
	"You are using an invalid binary",
	"Invalid dead time value",
	"Invalid trigger identifier",
	"Invalid channel number",
	"Function not yet implemented",
	"Received an invalid answer from trigger",
	"Received an invalid answer from HASH",
	"Received an invalid HASH content",
};


/**
 * It returns a string messages corresponding to a given error code. If
 * it is not a libwrtd error code, it will run wrnc_strerror()
 * @param[in] err error code
 * @return a message error
 */
const char *wrtd_strerror(int err)
{
	if (err < EWRTD_INVALD_ANSWER_ACK || err >= __EWRTD_MAX_ERROR_NUMBER)
		return wrnc_strerror(err);

	return wrtd_errors[err - EWRTD_INVALD_ANSWER_ACK];
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

	return (struct wrtd_node *)wrtd;

out:
	free(wrtd);
	return wrtd;
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
	err = wrnc_cpu_load_application_file(wrtd->wrnc, WRTD_CPU_FD, rt_fd);
	if (err)
		return err;

	/* Re-enable the CPUs */
	err = wrnc_cpu_reset_set(wrtd->wrnc, reg_old);
	if (err)
		return err;

	return 0;
}
