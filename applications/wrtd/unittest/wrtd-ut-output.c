/**
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *         inspired by a draft of Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * License: GPL v3
 *
 * @TODO software trigger test
 */

#include <string.h>
#include <errno.h>
#include <inttypes.h>

#include "wrtd-ut.h"
#include "CuTest.h"

#include <libwrnc.h>
#include <libwrtd.h>



static void test_ping(CuTest *tc)
{
	struct wrtd_node *wrtd;

	wrtd = wrtd_open_by_lun(0);
	CuAssertTrue(tc, !wrtd_out_ping(wrtd));
	wrtd_close(wrtd);
}


static void test_channels_enable(CuTest *tc)
{
	struct wrtd_node *wrtd;
	unsigned int i, enable;

	wrtd = wrtd_open_by_lun(0);

	for (i = 0; i < FD_NUM_CHANNELS; i++) {
		CuAssertTrue(tc, !wrtd_out_enable(wrtd, i, 1));
		CuAssertTrue(tc, !wrtd_out_is_enabled(wrtd, i, &enable));
		CuAssertTrue(tc, enable);
	}

	wrtd_close(wrtd);
}

static void test_channels_disable(CuTest *tc)
{
	struct wrtd_node *wrtd;
	unsigned int i, enable;

	wrtd = wrtd_open_by_lun(0);

	for (i = 0; i < FD_NUM_CHANNELS; i++) {
		CuAssertTrue(tc, !wrtd_out_enable(wrtd, i, 0));
		CuAssertTrue(tc, !wrtd_out_is_enabled(wrtd, i, &enable));
		CuAssertTrue(tc, !enable);
	}

	wrtd_close(wrtd);
}

static void test_channels_arm(CuTest *tc)
{
	struct wrtd_node *wrtd;
	unsigned int i, armed;

	wrtd = wrtd_open_by_lun(0);
	for (i = 0; i < FD_NUM_CHANNELS; i++) {
		CuAssertTrue(tc, !wrtd_out_arm(wrtd, i, 1));
		CuAssertTrue(tc, !wrtd_out_is_armed(wrtd, i, &armed));
		CuAssertTrue(tc, armed);
	}
	wrtd_close(wrtd);
}

static void test_channels_disarm(CuTest *tc)
{
	struct wrtd_node *wrtd;
	unsigned int i, armed;

	wrtd = wrtd_open_by_lun(0);
	for (i = 0; i < FD_NUM_CHANNELS; i++) {
		CuAssertTrue(tc, !wrtd_out_arm(wrtd, i, 0));
		CuAssertTrue(tc, !wrtd_out_is_armed(wrtd, i, &armed));
		CuAssertTrue(tc, !armed);
	}
	wrtd_close(wrtd);
}

static void test_trigger_assign_one(CuTest *tc, struct wrtd_node *wrtd,
				    unsigned int chan,
				    struct wrtd_trig_id *id)
{
	struct wrtd_output_trigger_state trig;
	struct wrtd_trigger_handle h;
	unsigned int assigned;
	char msg[128];
	int ret;

	sprintf(msg, "Assert failed - Channel %d Trigger ID %d:%d:%d",
		chan, id->system, id->source_port, id->trigger);


	ret = wrtd_out_trig_assign(wrtd, chan, &h, id, NULL);
	CuAssertTrue_Msg(tc, msg, !ret);

	ret = wrtd_out_trig_state_get_by_handle(wrtd, &h, &trig);
	CuAssertTrue_Msg(tc, msg, !ret);

 	CuAssertTrue(tc,
		     !memcmp(id, &trig.trigger, sizeof(struct wrtd_trig_id)));
	CuAssertTrue(tc, !wrtd_out_has_trigger(wrtd, chan, id, &assigned));
	CuAssertTrue(tc, assigned);
}


static void test_trigger_assign(CuTest *tc)
{
	struct wrtd_output_trigger_state trig[3*3*3];
	struct wrtd_node *wrtd;
	struct wrtd_trig_id id;
	unsigned int assigned;
        int i, k, y, w, ret;

	wrtd = wrtd_open_by_lun(0);

	for (i = 0; i < FD_NUM_CHANNELS; i++) {
		ret = wrtd_out_trig_get_all(wrtd, i, trig, 3*3*3);
		CuAssertTrue(tc, ret == 0);
		for (k = 2; k >= 0; k--) { /* so we force the real-time app to
					    re-order triggers */
			for (y = 0; y < 3; y++) {
				for (w = 0; w < 3; w++) {
					id.system = k;
					id.source_port = y;
					id.trigger = w;
					test_trigger_assign_one(tc, wrtd,
								i, &id);
				}
			}
		}
		CuAssertTrue(tc, !wrtd_out_has_trigger(wrtd, i, NULL,
						       &assigned));
		CuAssertTrue(tc, assigned);
		memset(&trig, 0,
		       sizeof(struct wrtd_output_trigger_state) * (3*3*3));
		ret = wrtd_out_trig_get_all(wrtd, i, trig, 3*3*3);
		CuAssertTrue(tc, ret == 3*3*3);
	}

	wrtd_close(wrtd);
}


static void test_trigger_unassign_one(CuTest *tc, struct wrtd_node *wrtd,
				      unsigned int chan,
				      struct wrtd_trig_id *id)
{
	struct wrtd_output_trigger_state trig;
	unsigned int assigned;
	char msg[128];
	int ret;

	sprintf(msg, "Assert failed - Channel %d Trigger ID %d:%d:%d",
		chan, id->system, id->source_port, id->trigger);

	ret = wrtd_out_trig_state_get_by_id(wrtd, chan, id, &trig);
	CuAssertTrue_Msg(tc, msg, !ret);

	ret = wrtd_out_trig_unassign(wrtd, &trig.handle);
	CuAssertTrue_Msg(tc, msg, !ret);

	ret = wrtd_out_trig_state_get_by_handle(wrtd, &trig.handle, &trig);
	CuAssertTrue_Msg(tc, msg, ret);
	// FIXME only with librt
	//CuAssertIntEquals(tc, errno, EWRTD_NOFOUND_TRIGGER);
	CuAssertTrue(tc, !wrtd_out_has_trigger(wrtd, chan, id, &assigned));
	CuAssertTrue(tc, !assigned);
}

static void test_trigger_unassign(CuTest *tc)
{
	struct wrtd_output_trigger_state trig[3*3*3];
	struct wrtd_node *wrtd;
	struct wrtd_trig_id id;
	unsigned int assigned;
	unsigned int i, k, y, w;
	int ret;

	wrtd = wrtd_open_by_lun(0);

	for (i = 0; i < FD_NUM_CHANNELS; i++) {
		memset(&trig, 0,
		       sizeof(struct wrtd_output_trigger_state) * (3*3*3));
		ret = wrtd_out_trig_get_all(wrtd, i, trig, 3*3*3);
		CuAssertTrue(tc, ret == 3*3*3);
		for (k = 0; k < 3; k++) {
			for (y = 0; y < 3; y++) {
				for (w = 0; w < 3; w++) {
					id.system = k;
					id.source_port = y;
					id.trigger = w;
					test_trigger_unassign_one(tc, wrtd,
								  i, &id);
				}
			}
		}
		CuAssertTrue(tc, !wrtd_out_has_trigger(wrtd, i, NULL,
						       &assigned));
		CuAssertTrue(tc, !assigned);

		memset(&trig, 0,
		       sizeof(struct wrtd_output_trigger_state) * (3*3*3));
		ret = wrtd_out_trig_get_all(wrtd, i, trig, 3*3*3);
		CuAssertTrue(tc, ret == 0);
	}

	wrtd_close(wrtd);
}

static void test_pulse_width(CuTest *tc)
{
	struct wrtd_output_state st;
	struct wrtd_node *wrtd;
	unsigned int i, k;
	uint64_t ps;

	wrtd = wrtd_open_by_lun(0);
	for (i = 0; i < FD_NUM_CHANNELS; i++) {
		CuAssertTrue(tc, wrtd_out_pulse_width_set(wrtd, i, 0));
		CuAssertIntEquals(tc, errno, EWRTD_INVALID_PULSE_WIDTH);
		CuAssertTrue(tc, wrtd_out_pulse_width_set(wrtd, i, 1000000000001ULL));
		CuAssertIntEquals(tc, errno, EWRTD_INVALID_PULSE_WIDTH);
		for (k = 250000; k < FD_NUM_CHANNELS; k+=10000) {
			CuAssertTrue(tc, !wrtd_out_pulse_width_set(wrtd, i, k));
			CuAssertTrue(tc, !wrtd_out_state_get(wrtd, i, &st));
			wrtd_ts_to_pico(&st.pulse_width, &ps);
			CuAssertIntEquals(tc, k, ps);
		}
	}
	wrtd_close(wrtd);
}

static void test_dead_time(CuTest *tc)
{
	struct wrtd_output_state st;
	struct wrtd_node *wrtd;
	unsigned int i, k;
	uint64_t ps;

	wrtd = wrtd_open_by_lun(0);
	for (i = 0; i < FD_NUM_CHANNELS; i++) {
		for (k = 0; k < FD_NUM_CHANNELS; k+=10000) {
			CuAssertTrue(tc, !wrtd_out_dead_time_set(wrtd, i, k));
			CuAssertTrue(tc, !wrtd_out_state_get(wrtd, i, &st));
			wrtd_ts_to_pico(&st.dead_time, &ps);
			CuAssertIntEquals(tc, k, ps);
		}
	}
	wrtd_close(wrtd);
}

#define RANGE 10
static void test_trigger_delay(CuTest *tc)
{
	struct wrtd_node *wrtd;
	struct wrtd_output_trigger_state trig;
	struct wrtd_trigger_handle h;
	struct wrtd_trig_id id = {1,2,3};
	char msg[128];
	uint64_t ps, min, max;
	int i, k;

	wrtd = wrtd_open_by_lun(0);
	for (i = 0; i < FD_NUM_CHANNELS; i++) {
		wrtd_out_trig_assign(wrtd, i, &h, &id, NULL);
		for (k = 1; k < 8; k <<= 1) {
			CuAssertTrue(tc, !wrtd_out_trig_delay_set(wrtd, &h, k));
			CuAssertTrue(tc, !wrtd_out_trig_state_get_by_handle(wrtd,
									    &h,
									    &trig));
			wrtd_ts_to_pico(&trig.delay_trig, &ps);
			/* Conversion is not precise, some approximation may
			   happen */
			min = k < RANGE ? 0 : k - RANGE;
			max = k + RANGE;
			sprintf(msg,
				"Assert failed - %"PRIu64" not in range [%"PRIu64", %"PRIu64"]",
				ps, min, max);
			CuAssertTrue_Msg(tc, msg, ps >= min && ps <= max);
		}
	}
	wrtd_out_trig_unassign(wrtd, &h);
	wrtd_close(wrtd);
}

static void test_trigger_enable(CuTest *tc)
{
	struct wrtd_node *wrtd;
	struct wrtd_output_trigger_state trig;
	struct wrtd_trigger_handle h;
	struct wrtd_trig_id id = {1,2,3};
	int i;

	wrtd = wrtd_open_by_lun(0);
	for (i = 0; i < FD_NUM_CHANNELS; i++) {
		wrtd_out_trig_assign(wrtd, i, &h, &id, NULL);
		CuAssertTrue(tc, !wrtd_out_trig_enable(wrtd, &h, 1));
		CuAssertTrue(tc, !wrtd_out_trig_state_get_by_handle(wrtd, &h,
								    &trig));
		CuAssertTrue(tc, trig.enabled);
	}
	wrtd_out_trig_unassign(wrtd, &h);
	wrtd_close(wrtd);
}

static void test_trigger_disable(CuTest *tc)
{
	struct wrtd_node *wrtd;
	struct wrtd_output_trigger_state trig;
	struct wrtd_trigger_handle h;
	struct wrtd_trig_id id = {1,2,3};
	int i;

	wrtd = wrtd_open_by_lun(0);
	for (i = 0; i < FD_NUM_CHANNELS; i++) {
		wrtd_out_trig_assign(wrtd, i, &h, &id, NULL);
		CuAssertTrue(tc, !wrtd_out_trig_enable(wrtd, &h, 0));
		CuAssertTrue(tc, !wrtd_out_trig_state_get_by_handle(wrtd, &h,
								    &trig));
		CuAssertTrue(tc, !trig.enabled);
		wrtd_out_trig_unassign(wrtd, &h);
	}
	wrtd_close(wrtd);
}

static void test_trigger_mode(CuTest *tc)
{
	struct wrtd_node *wrtd;
	struct wrtd_output_state st;
	int i;

	wrtd = wrtd_open_by_lun(0);
	for (i = 0; i < FD_NUM_CHANNELS; i++) {
		CuAssertTrue(tc, !wrtd_out_trigger_mode_set(wrtd, i,
							    WRTD_TRIGGER_MODE_SINGLE));
		CuAssertTrue(tc, !wrtd_out_state_get(wrtd, i, &st));
		CuAssertIntEquals(tc, WRTD_TRIGGER_MODE_SINGLE, st.mode);

		CuAssertTrue(tc, !wrtd_out_trigger_mode_set(wrtd, i,
							    WRTD_TRIGGER_MODE_AUTO));
		CuAssertTrue(tc, !wrtd_out_state_get(wrtd, i, &st));
		CuAssertIntEquals(tc, WRTD_TRIGGER_MODE_AUTO, st.mode);

		CuAssertTrue(tc, !wrtd_out_trigger_mode_set(wrtd, i,
							    WRTD_TRIGGER_MODE_SINGLE));
		CuAssertTrue(tc, !wrtd_out_state_get(wrtd, i, &st));
		CuAssertIntEquals(tc, WRTD_TRIGGER_MODE_SINGLE, st.mode);

	}
	wrtd_close(wrtd);
}

static void test_reset_counters(CuTest *tc)
{
	struct wrtd_node *wrtd;
	struct wrtd_output_state st;
	int i;

	wrtd = wrtd_open_by_lun(0);
	for (i = 0; i < FD_NUM_CHANNELS; i++) {
		CuAssertTrue(tc, !wrtd_out_counters_reset(wrtd, i));
		CuAssertTrue(tc, !wrtd_out_state_get(wrtd, i, &st));
		CuAssertIntEquals(tc, st.executed_pulses, 0);
		CuAssertIntEquals(tc, st.missed_pulses_late, 0);
		CuAssertIntEquals(tc, st.missed_pulses_deadtime, 0);
		CuAssertIntEquals(tc, st.missed_pulses_overflow, 0);
		CuAssertIntEquals(tc, st.missed_pulses_no_timing, 0);
		CuAssertTrue(tc, !(st.flags & WRTD_LAST_VALID));
	}
	wrtd_close(wrtd);
}

CuSuite *wrtd_ut_out_suite_get(void)
{
	CuSuite *suite = CuSuiteNew();

	SUITE_ADD_TEST(suite, test_ping);
	SUITE_ADD_TEST(suite, test_channels_enable);
	SUITE_ADD_TEST(suite, test_channels_disable);
	SUITE_ADD_TEST(suite, test_channels_arm);
	SUITE_ADD_TEST(suite, test_channels_disarm);
	SUITE_ADD_TEST(suite, test_trigger_assign);
	SUITE_ADD_TEST(suite, test_trigger_unassign);
	SUITE_ADD_TEST(suite, test_pulse_width);
	SUITE_ADD_TEST(suite, test_dead_time);
	SUITE_ADD_TEST(suite, test_trigger_delay);
	SUITE_ADD_TEST(suite, test_trigger_enable);
	SUITE_ADD_TEST(suite, test_trigger_disable);
	SUITE_ADD_TEST(suite, test_trigger_mode);
	SUITE_ADD_TEST(suite, test_reset_counters);

	return suite;
}
