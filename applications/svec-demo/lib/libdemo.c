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
#include <libmockturtle.h>
#include <libdemo-internal.h>

const char *demo_errors[] = {
	"Received an invalid answer from white-rabbit-node-code CPU",
	"Real-Time application does not acknowledge",
};


/**
 * It returns a string messages corresponding to a given error code. If
 * it is not a libwrtd error code, it will run trtl_strerror()
 * @param[in] err error code
 * @return a message error
 */
const char *demo_strerror(unsigned int err)
{
	if (err < EDEMO_INVALID_ANSWER_ACK || err >= __EDEMO_MAX_ERROR_NUMBER)
		return trtl_strerror(err);

	return demo_errors[err - EDEMO_INVALID_ANSWER_ACK];
}


/**
 * It initializes the DEMO library. It must be called before doing
 * anything else.
 * This library is based on the libmockturtle, so internally, this function also
 * run demo_init() in order to initialize the WRNC library.
 * @return 0 on success, otherwise -1 and errno is appropriately set
 */
int demo_init()
{
	int err;

	err = trtl_init();
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
	trtl_exit();
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

	demo->trtl = trtl_open_by_fmc(device_id);
	if (!demo->trtl)
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

	demo->trtl = trtl_open_by_lun(lun);
	if (!demo->trtl)
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

	trtl_close(demo->trtl);
	free(demo);
	dev = NULL;
}


/**
 * It returns the WRNC token in order to allows users to run
 * functions from the WRNC library
 * @param[in] dev device token
 * @return the WRNC token
 */
struct trtl_dev *demo_get_trtl_dev(struct demo_node *dev)
{
	struct demo_desc *demo = (struct demo_desc *)dev;

	return (struct trtl_dev *)demo->trtl;
}

int demo_lemo_dir_set(struct demo_node *dev, uint32_t value)
{
	struct demo_desc *demo = (struct demo_desc *)dev;
	uint32_t fields[] = {DEMO_VAR_LEMO_DIR, value};
	struct trtl_proto_header hdr = {
		.slot_io = (DEMO_HMQ_IN << 4) |
			   (DEMO_HMQ_OUT & 0xF),
		.len = 2,
	};

	return trtl_rt_variable_set(demo->trtl, &hdr, fields, 1);
}

int demo_lemo_set(struct demo_node *dev, uint32_t value)
{
	struct demo_desc *demo = (struct demo_desc *)dev;
	uint32_t fields[] = {DEMO_VAR_LEMO_SET, value,
			     DEMO_VAR_LEMO_CLR, ~value};
	struct trtl_proto_header hdr = {
		.slot_io = (DEMO_HMQ_IN << 4) |
			   (DEMO_HMQ_OUT & 0xF),
		.len = 4,
	};

	return trtl_rt_variable_set(demo->trtl, &hdr, fields, 2);
}


/**
 * Convert the given LEDs value with color codification
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
	uint32_t real_value = demo_apply_color(value, color);
	uint32_t fields[] = {DEMO_VAR_LED_SET, real_value,
			     DEMO_VAR_LED_CLR, ~real_value};
	struct trtl_proto_header hdr = {
		.slot_io = (DEMO_HMQ_IN << 4) |
			   (DEMO_HMQ_OUT & 0xF),
		.len = 6,
	};
	return trtl_rt_variable_set(demo->trtl, &hdr, fields, 2);
}


/**
 * It gets the status of the DEMO program
 */
int demo_status_get(struct demo_node *dev, struct demo_status *status)
{
	struct demo_desc *demo = (struct demo_desc *)dev;
	uint32_t fields[] = {DEMO_VAR_LEMO_STA, 0,
			     DEMO_VAR_LED_STA, 0,
			     DEMO_VAR_LEMO_DIR, 0};
	struct trtl_proto_header hdr = {
		.slot_io = (DEMO_HMQ_IN << 4) |
			   (DEMO_HMQ_OUT & 0xF),
		.flags = TRTL_PROTO_FLAG_SYNC,
		.len = 6,
	};
	int err;

	err = trtl_rt_variable_get(demo->trtl, &hdr, fields, 3);
	if (err)
		return err;

	status->lemo = fields[1];
	status->led = fields[3];
	status->lemo_dir = fields[5];

	return 0;
}

int demo_run_autodemo(struct demo_node *dev, uint32_t run){ return 0; }

int demo_version(struct demo_node *dev, struct trtl_rt_version *version)
{
	struct demo_desc *demo = (struct demo_desc *)dev;

	return trtl_rt_version_get(demo->trtl, version,
				   DEMO_HMQ_IN, DEMO_HMQ_OUT);
}

int demo_test_struct_get(struct demo_node *dev, struct demo_structure *test)
{
	struct demo_desc *demo = (struct demo_desc *)dev;
	struct trtl_structure_tlv tlv = {
		.index = DEMO_STRUCT_TEST,
		.size = sizeof(struct demo_structure),
		.structure = test,
	};
	struct trtl_proto_header hdr = {
		.slot_io = (DEMO_HMQ_IN << 4) |
			   (DEMO_HMQ_OUT & 0xF),
		.flags = TRTL_PROTO_FLAG_SYNC,
	};

	return trtl_rt_structure_get(demo->trtl, &hdr, &tlv, 1);
}

int demo_test_struct_set(struct demo_node *dev, struct demo_structure *test)
{
	struct demo_desc *demo = (struct demo_desc *)dev;
	struct trtl_structure_tlv tlv = {
		.index = DEMO_STRUCT_TEST,
		.size = sizeof(struct demo_structure),
		.structure = test,
	};
	struct trtl_proto_header hdr = {
		.slot_io = (DEMO_HMQ_IN << 4) |
			   (DEMO_HMQ_OUT & 0xF),
	};

	return trtl_rt_structure_set(demo->trtl, &hdr, &tlv, 1);
}
