/*
 * Copyright (C) 2015 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */


/*
 * This is just a DEMO, the code is not optimized
 */
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <libwrnc.h>
#include <libdemo-internal.h>

const char *demo_errors[] = {
	"Received an invalid answer from white-rabbit-node-code CPU",
};


/**
 * It returns a string messages corresponding to a given error code. If
 * it is not a libwrtd error code, it will run wrnc_strerror()
 * @param[in] err error code
 * @return a message error
 */
const char *demo_strerror(unsigned int err)
{
	if (err < EDEMO_INVALID_ANSWER_ACK || err >= __EDEMO_MAX_ERROR_NUMBER)
		return wrnc_strerror(err);

	return demo_errors[err - EDEMO_INVALID_ANSWER_ACK];
}


/**
 * It initializes the DEMO library. It must be called before doing
 * anything else.
 * This library is based on the libwrnc, so internally, this function also
 * run demo_init() in order to initialize the WRNC library.
 * @return 0 on success, otherwise -1 and errno is appropriately set
 */
int demo_init()
{
	int err;

	err = wrnc_init();
	if (err)
		return err;

	return 0;
}


/**
 * It releases the resources allocated by demo_init(). It must be called when
 * you stop to use this library. Then, you cannot use functions from this
 * library.
 */
void demo_exit()
{
	wrnc_exit();
}


/**
 * Open a WRTD node device using FMC ID
 * @param[in] device_id FMC device identificator
 * @return It returns an anonymous demo_node structure on success.
 *         On error, NULL is returned, and errno is set appropriately.
 */
struct demo_node *demo_open_by_fmc(uint32_t device_id)
{
	struct demo_desc *demo;

	demo = malloc(sizeof(struct demo_desc));
	if (!demo)
		return NULL;

	demo->wrnc = wrnc_open_by_fmc(device_id);
	if (!demo->wrnc)
		goto out;

	demo->dev_id = device_id;
	return (struct demo_node *)demo;

out:
	free(demo);
	return NULL;
}


/**
 * Open a WRTD node device using LUN
 * @param[in] lun an integer argument to select the device or
 *            negative number to take the first one found.
 * @return It returns an anonymous demo_node structure on success.
 *         On error, NULL is returned, and errno is set appropriately.
 */
struct demo_node *demo_open_by_lun(int lun)
{
	struct demo_desc *demo;

	demo = malloc(sizeof(struct demo_desc));
	if (!demo)
		return NULL;

	demo->wrnc = wrnc_open_by_lun(lun);
	if (!demo->wrnc)
		goto out;

	demo->dev_id = lun;

	return (struct demo_node *)demo;

out:
	free(demo);
	return NULL;
}


/**
 * It closes a DEMO device opened with one of the following function:
 * demo_open_by_lun(), demo_open_by_fmc()
 * @param[in] dev device token
 */
void demo_close(struct demo_node *dev)
{
	struct demo_desc *demo = (struct demo_desc *)dev;

	wrnc_close(demo->wrnc);
	free(demo);
	dev = NULL;
}


/**
 * It returns the WRNC token in order to allows users to run
 * functions from the WRNC library
 * @param[in] dev device token
 * @return the WRNC token
 */
struct wrnc_dev *demo_get_wrnc_dev(struct demo_node *dev)
{
	struct demo_desc *demo = (struct demo_desc *)dev;

	return (struct wrnc_dev *)demo->wrnc;
}


int demo_lemo_dir_set(struct demo_node *dev, uint32_t value)
{
	struct demo_desc *demo = (struct demo_desc *)dev;
	struct wrnc_msg msg;
	struct wrnc_hmq *hmq;
	int err;

	/* Build the message */
	msg.datalen = 3;
	msg.data[0] = DEMO_ID_LEMO_DIR_SET;
	msg.data[1] = 0;
	msg.data[2] = value & PIN_LEMO_MASK;

	hmq = wrnc_hmq_open(demo->wrnc, DEMO_HMQ_IN, WRNC_HMQ_INCOMING);
	if (!hmq)
		return -1;

	/* Send synchronous message  */
	err = wrnc_hmq_send_and_receive_sync(hmq, DEMO_HMQ_OUT, &msg, 1000);

	wrnc_hmq_close(hmq);

	return err;
}


int demo_lemo_set(struct demo_node *dev, uint32_t value)
{
	struct demo_desc *demo = (struct demo_desc *)dev;
	struct wrnc_msg msg;
	struct wrnc_hmq *hmq;
	int err;

	/* Build the message */
	msg.datalen = 3;
	msg.data[0] = DEMO_ID_LEMO_SET;
	msg.data[1] = 0;
	msg.data[2] = value & PIN_LEMO_MASK;

	hmq = wrnc_hmq_open(demo->wrnc, DEMO_HMQ_IN, WRNC_HMQ_INCOMING);
	if (!hmq)
		return -1;

	/* Send synchronous message  */
	err = wrnc_hmq_send_and_receive_sync(hmq, DEMO_HMQ_OUT, &msg, 1000);

	wrnc_hmq_close(hmq);

	return err;
}


/**
 * Convert the given LEDs value wiht color codification
 */
static uint32_t demo_apply_color(uint32_t value, enum demo_color color)
{
	uint32_t val = 0;
	int i;

	for (i = 0; i < PIN_LED_COUNT; ++i) {
		if (!((value >> i) & 0x1))
			continue;
		switch (color) {
		case DEMO_GREEN:
		case DEMO_RED:
			val |= (0x1 << (color + (i * 2)));
			break;
		case DEMO_ORANGE:
			val |= (0x3 << ((i * 2)));
			break;
		}
	}

	return val;
}


/**
 * Set LED's register
 */
int demo_led_set(struct demo_node *dev, uint32_t value, enum demo_color color)
{
	struct demo_desc *demo = (struct demo_desc *)dev;
	struct wrnc_msg msg;
	struct wrnc_hmq *hmq;
	int err;

	/* Build the message */
	msg.datalen = 3;
	msg.data[0] = DEMO_ID_LED_SET;
	msg.data[1] = 0;
	msg.data[2] = demo_apply_color(value, color);

	hmq = wrnc_hmq_open(demo->wrnc, DEMO_HMQ_IN, WRNC_HMQ_INCOMING);
	if (!hmq)
		return -1;

	/* Send asynchronous message, we do not wait for answers  */
	err = wrnc_hmq_send(hmq, &msg);
	wrnc_hmq_close(hmq);

	return err;
}


/**
 * It gets the status of the DEMO program
 */
int demo_status_get(struct demo_node *dev, struct demo_status *status)
{
	struct demo_desc *demo = (struct demo_desc *)dev;
	struct wrnc_hmq *hmq;
	struct wrnc_msg msg;
	int err;

	/* Build the message */
	msg.datalen = 2;
	msg.data[0] = DEMO_ID_STATE_GET;
	msg.data[1] = 0;

	hmq = wrnc_hmq_open(demo->wrnc, DEMO_HMQ_IN, WRNC_HMQ_INCOMING);
	if (!hmq)
		return -1;

	/* Send synchronous message. The answer will overwrite our message */
	err = wrnc_hmq_send_and_receive_sync(hmq, DEMO_HMQ_OUT, &msg, 1000);

	wrnc_hmq_close(hmq);


	/* Check if it is the answer that we are expecting */
	if (msg.data[0] != DEMO_ID_STATE_GET_REP) {
		return -1;
	}

	/* unpack data */
	status->led = msg.data[2];
	status->lemo = msg.data[3];
	status->lemo_dir = msg.data[4];

	/* Of course you can use memcpy() to unpack, but this is a demo so,
	   this way is more explicit */

	return err;
}
