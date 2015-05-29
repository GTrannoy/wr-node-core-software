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
};

enum demo_error_list {
	EDEMO_INVALID_ANSWER_ACK = 3200,
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
extern int demo_status_get(struct demo_node *dev, struct demo_status *status);
#endif
