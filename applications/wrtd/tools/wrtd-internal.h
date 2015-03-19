/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * License: GPL v3
 */

#ifndef __WRTD_TOOLS_INTERNAL_H__
#define __WRTD_TOOLS_INTERNAL_H__

#include <libwrnc.h>
#include <libwrtd.h>
#include <wrtd-internal.h>

struct wrtd_commands {
	const char *name;
	const char *desc;
	int (*handler)(struct wrtd_node *wrtd, int line,
		       int argc, char **argv);
};


/**
 * @file wrtd-inout-common.c
 */
extern void decode_flags(char *buf, uint32_t flags);
extern void decode_mode(char *buf, int mode);
extern void decode_log_level(char *buf, uint32_t flags);
extern void format_ts(char *buf, struct wr_timestamp ts, int with_seconds);
extern void format_id(char *buf, struct wrtd_trig_id id);
extern uint64_t ts_to_picos(struct wr_timestamp ts);
extern int parse_delay(char *dly, uint64_t *delay_ps);
extern int parse_trigger_id(const char *str, struct wrtd_trig_id *id);
extern int parse_mode (char *mode_str, enum wrtd_trigger_mode *mode);
extern int parse_log_level (char *list[], int count, int *log_level);

#endif
