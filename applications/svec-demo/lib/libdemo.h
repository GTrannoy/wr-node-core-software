/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 */


#ifndef __LIBWRTD_INTERNAL__H__
#define __LIBWRTD_INTERNAL__H__

#include <demo-common.h>

struct demo_node;

struct demo_status {
	uint32_t led;
	uint32_t lemo;
	uint32_t lemo_dir;
	uint32_t autodemo;
};

enum demo_error_list {
	EDEMO_INVALID_ANSWER_ACK = 3200,
	EDEMO_ANSWER_NACK,
	__EDEMO_MAX_ERROR_NUMBER,
};

enum demo_color {
	DEMO_GREEN = 0,
	DEMO_RED,
	DEMO_ORANGE,
};

extern const char *demo_strerror(unsigned int error);
extern int demo_init();
extern void demo_exit();
extern struct demo_node *demo_open_by_fmc(uint32_t device_id);
extern struct demo_node *demo_open_by_lun(int lun);
extern void demo_close(struct demo_node *dev);
extern struct wrnc_dev *demo_get_wrnc_dev(struct demo_node *dev);

extern int demo_led_set(struct demo_node *dev, uint32_t value,
			enum demo_color color);
extern int demo_lemo_set(struct demo_node *dev, uint32_t value);
extern int demo_lemo_dir_set(struct demo_node *dev, uint32_t value);
extern int demo_status_get(struct demo_node *dev, struct demo_status *status);
extern int demo_run_autodemo(struct demo_node *dev, uint32_t run);
extern int demo_version(struct demo_node *dev, struct wrnc_rt_version *version);
extern int demo_test_struct_set(struct demo_node *dev,
				struct demo_structure *test);
extern int demo_test_struct_get(struct demo_node *dev,
				struct demo_structure *test);
#endif
