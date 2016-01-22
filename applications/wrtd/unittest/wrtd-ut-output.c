/**
 * Copyright (C) 2016 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
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
	CuAssertIntEquals(tc, 0, wrtd_out_ping(wrtd));
	wrtd_close(wrtd);
}


static void test_channels_enable(CuTest *tc)
{
	struct wrtd_node *wrtd;
	unsigned int i, enable;

	wrtd = wrtd_open_by_lun(0);

	for (i = 0; i < FD_NUM_CHANNELS; i++) {
		CuAssertIntEquals(tc, 0, wrtd_out_enable(wrtd, i, 1));
		CuAssertIntEquals(tc, 0, wrtd_out_is_enabled(wrtd, i, &enable));
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
		CuAssertIntEquals(tc, 0, wrtd_out_enable(wrtd, i, 0));
		CuAssertIntEquals(tc, 0, wrtd_out_is_enabled(wrtd, i, &enable));
		CuAssertIntEquals(tc, 0, enable);
	}

	wrtd_close(wrtd);
}

static void test_channels_arm(CuTest *tc)
{
	struct wrtd_node *wrtd;
	unsigned int i, armed;

	wrtd = wrtd_open_by_lun(0);
	for (i = 0; i < FD_NUM_CHANNELS; i++) {
		CuAssertIntEquals(tc, 0, wrtd_out_arm(wrtd, i, 1));
		CuAssertIntEquals(tc, 0, wrtd_out_is_armed(wrtd, i, &armed));
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
		CuAssertIntEquals(tc, 0, wrtd_out_arm(wrtd, i, 0));
		CuAssertIntEquals(tc, 0, wrtd_out_is_armed(wrtd, i, &armed));
		CuAssertIntEquals(tc, 0, armed);
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

	/* printf("Channel %d Trigger ID %d:%d:%d\n", */
	/* 	chan, id->system, id->source_port, id->trigger); */

	CuAssertIntEquals(tc, 0, wrtd_out_has_trigger(wrtd, chan, id, &assigned));
	CuAssertIntEquals(tc, 0, assigned);

	ret = wrtd_out_trig_assign(wrtd, chan, &h, id, NULL);
	sprintf(msg, "Assert failed - Channel %d Trigger ID %d:%d:%d - %s",
		chan, id->system, id->source_port, id->trigger, wrtd_strerror(errno));
	CuAssertIntEquals_Msg(tc, msg, 0, ret);

	ret = wrtd_out_trig_state_get_by_handle(wrtd, &h, &trig);
	sprintf(msg, "Assert failed - Channel %d Trigger ID %d:%d:%d - %s",
		chan, id->system, id->source_port, id->trigger, wrtd_strerror(errno));
	CuAssertIntEquals_Msg(tc, msg, 0, ret);

 	CuAssertIntEquals(tc, 0, memcmp(id, &trig.trigger,
					sizeof(struct wrtd_trig_id)));
	CuAssertIntEquals(tc, 0, wrtd_out_has_trigger(wrtd, chan, id, &assigned));
	CuAssertTrue(tc, assigned);
}

static void test_trigger_assign_all(CuTest *tc, struct wrtd_node *wrtd,
				    unsigned int chan)
{
	struct wrtd_trig_id id;
	int k, w;

	id.source_port = chan;
	for (k = 4; k >= 0; k--) {
		for (w = 0; w < 3; w++) {
			id.system = k;
			id.trigger = w;
			test_trigger_assign_one(tc, wrtd, chan, &id);
		}
	}
}


static void test_trigger_assign(CuTest *tc)
{
	struct wrtd_output_trigger_state trig[15];
	struct wrtd_node *wrtd;
	unsigned int assigned;
        int i, ret;

	wrtd = wrtd_open_by_lun(0);

	for (i = 0; i < FD_NUM_CHANNELS; i++) {
		CuAssertIntEquals(tc, 0, wrtd_out_trig_get_all(wrtd, i,
							       trig, 15));
		test_trigger_assign_all(tc, wrtd, i);
		CuAssertIntEquals(tc, 0, wrtd_out_has_trigger(wrtd, i, NULL,
						       &assigned));
		CuAssertTrue(tc, assigned);
		memset(&trig, 0,
		       sizeof(struct wrtd_output_trigger_state) * (15));
		ret = wrtd_out_trig_get_all(wrtd, i, trig, 15);
		CuAssertIntEquals(tc, 15, ret);
	}

	wrtd_close(wrtd);
}


static void test_trigger_assign_cond_one(CuTest *tc, struct wrtd_node *wrtd,
				    unsigned int chan,
				    struct wrtd_trig_id *id)
{
	struct wrtd_output_trigger_state trig, cond;
	struct wrtd_trig_id condid = *id;
	struct wrtd_trigger_handle h;
	unsigned int assigned;
	char msg[128];
	int ret;

	condid.trigger += 100;
	CuAssertIntEquals(tc, 0, wrtd_out_has_trigger(wrtd, chan, id, &assigned));
	CuAssertIntEquals(tc, 0, assigned);

	ret = wrtd_out_trig_assign(wrtd, chan, &h, id, &condid);
	sprintf(msg, "Assert failed - Channel %d Trigger ID %d:%d:%d - %s",
		chan, id->system, id->source_port, id->trigger, wrtd_strerror(errno));
	CuAssertIntEquals_Msg(tc, msg, 0, ret);

	ret = wrtd_out_trig_state_get_by_handle(wrtd, &h, &trig);
	sprintf(msg, "Assert failed - Channel %d Trigger ID %d:%d:%d - %s",
		chan, id->system, id->source_port, id->trigger, wrtd_strerror(errno));
	CuAssertIntEquals_Msg(tc, msg, 0, ret);

	h.ptr_trig = h.ptr_cond;
	h.ptr_cond = NULL;
	ret = wrtd_out_trig_state_get_by_handle(wrtd, &h, &cond);
	sprintf(msg, "Assert failed - Channel %d Trigger ID %d:%d:%d - %s",
		chan, id->system, id->source_port, id->trigger, wrtd_strerror(errno));
	CuAssertIntEquals_Msg(tc, msg, 0, ret);

 	CuAssertIntEquals(tc, 0, memcmp(id, &trig.trigger,
					sizeof(struct wrtd_trig_id)));
	CuAssertIntEquals(tc, 0, memcmp(&condid, &cond.trigger,
					sizeof(struct wrtd_trig_id)));
	CuAssertIntEquals(tc, 0, wrtd_out_has_trigger(wrtd, chan, id, &assigned));
	CuAssertTrue(tc, assigned);
}

static void test_trigger_assign_cond_all(CuTest *tc, struct wrtd_node *wrtd,
					 unsigned int chan)
{
	struct wrtd_trig_id id;
	int k, w;

	id.source_port = chan;
	for (k = 0; k < 2; k++) {
		for (w = 0; w < 3; w++) {
			id.system = k;
			id.trigger = w;
			test_trigger_assign_cond_one(tc, wrtd, chan, &id);
		}
	}
}

static void test_trigger_assign_cond(CuTest *tc)
{
	struct wrtd_output_trigger_state trig[12];
	struct wrtd_node *wrtd;
	unsigned int assigned;
        int i, ret;

	wrtd = wrtd_open_by_lun(0);

	for (i = 0; i < FD_NUM_CHANNELS; i++) {
		CuAssertIntEquals(tc, 0, wrtd_out_trig_get_all(wrtd, i,
							       trig, 12));
		test_trigger_assign_cond_all(tc, wrtd, i);
		CuAssertIntEquals(tc, 0, wrtd_out_has_trigger(wrtd, i, NULL,
						       &assigned));
		CuAssertTrue(tc, assigned);
		memset(&trig, 0,
		       sizeof(struct wrtd_output_trigger_state) * (12));
		ret = wrtd_out_trig_get_all(wrtd, i, trig, 12);
		CuAssertIntEquals(tc, 12, ret);
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

	CuAssertIntEquals(tc, 0, wrtd_out_has_trigger(wrtd, chan, NULL, &assigned));
	CuAssertTrue(tc, assigned);

	ret = wrtd_out_trig_state_get_by_id(wrtd, chan, id, &trig);
	sprintf(msg, "Assert failed - Chan %d Trigger ID %d:%d:%d - %s",
		chan, id->system, id->source_port, id->trigger, wrtd_strerror(errno));
	CuAssertIntEquals_Msg(tc, msg, 0, ret);

	ret = wrtd_out_trig_unassign(wrtd, &trig.handle);
	CuAssertIntEquals_Msg(tc, msg, 0, ret);

	ret = wrtd_out_trig_state_get_by_handle(wrtd, &trig.handle, &trig);
	CuAssertIntEquals_Msg(tc, msg, -1, ret);
	CuAssertIntEquals(tc, errno, EWRTD_NOFOUND_TRIGGER);

	CuAssertIntEquals(tc, 0, wrtd_out_has_trigger(wrtd, chan, id, &assigned));
	CuAssertIntEquals(tc, 0, assigned);
}

static void test_trigger_unassign_all(CuTest *tc, struct wrtd_node *wrtd,
				      unsigned int chan)
{
	struct wrtd_trig_id id;
	int k, w;

	id.source_port = chan;
	for (k = 4; k >= 0; k--) {
		for (w = 0; w < 3; w++) {
			id.system = k;
			id.trigger = w;
			test_trigger_unassign_one(tc, wrtd, chan, &id);
		}

	}
}

static void test_trigger_unassign(CuTest *tc)
{
	struct wrtd_output_trigger_state trig[15];
	struct wrtd_node *wrtd;
	unsigned int assigned;
	unsigned int i;

	wrtd = wrtd_open_by_lun(0);

	for (i = 0; i < FD_NUM_CHANNELS; i++) {
		memset(&trig, 0,
		       sizeof(struct wrtd_output_trigger_state) * (15));
		CuAssertIntEquals(tc, 15, wrtd_out_trig_get_all(wrtd, i,
								trig, 15));
		test_trigger_unassign_all(tc, wrtd, i);
		CuAssertIntEquals(tc, 0, wrtd_out_has_trigger(wrtd, i, NULL,
						       &assigned));
		CuAssertIntEquals(tc, 0, assigned);

		memset(&trig, 0,
		       sizeof(struct wrtd_output_trigger_state) * (15));
		CuAssertIntEquals(tc, 0, wrtd_out_trig_get_all(wrtd, i, trig, 15));
	}

	wrtd_close(wrtd);
}

static void test_trigger_unassign_cond_all(CuTest *tc, struct wrtd_node *wrtd,
					   unsigned int chan)
{
	struct wrtd_trig_id id;
	int k, w;

	id.source_port = chan;
	for (k = 0; k < 2; k++) {
		for (w = 0; w < 3; w++) {
			id.system = k;
			id.trigger = w;
			test_trigger_unassign_one(tc, wrtd, chan, &id);
		}
	}
}

static void test_trigger_unassign_cond(CuTest *tc)
{
	struct wrtd_output_trigger_state trig[12];
	struct wrtd_node *wrtd;
	unsigned int assigned;
	unsigned int i;

	wrtd = wrtd_open_by_lun(0);

	for (i = 0; i < FD_NUM_CHANNELS; i++) {
		memset(&trig, 0,
		       sizeof(struct wrtd_output_trigger_state) * (12));
		CuAssertIntEquals(tc, 12, wrtd_out_trig_get_all(wrtd, i,
								trig, 12));
		test_trigger_unassign_cond_all(tc, wrtd, i);
		CuAssertIntEquals(tc, 0, wrtd_out_has_trigger(wrtd, i, NULL,
						       &assigned));
		CuAssertIntEquals(tc, 0, assigned);

		memset(&trig, 0,
		       sizeof(struct wrtd_output_trigger_state) * (12));
		CuAssertIntEquals(tc, 0, wrtd_out_trig_get_all(wrtd, i, trig, 12));
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
		CuAssertIntEquals(tc, -1, wrtd_out_pulse_width_set(wrtd, i, 0));
		CuAssertIntEquals(tc, errno, EWRTD_INVALID_PULSE_WIDTH);
		CuAssertIntEquals(tc, -1, wrtd_out_pulse_width_set(wrtd, i, 1000000000001ULL));
		CuAssertIntEquals(tc, errno, EWRTD_INVALID_PULSE_WIDTH);
		for (k = 250000; k < FD_NUM_CHANNELS; k+=10000) {
			CuAssertIntEquals(tc, 0, wrtd_out_pulse_width_set(wrtd, i, k));
			CuAssertIntEquals(tc, 0, wrtd_out_state_get(wrtd, i, &st));
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
			CuAssertIntEquals(tc, 0, wrtd_out_dead_time_set(wrtd, i, k));
			CuAssertIntEquals(tc, 0, wrtd_out_state_get(wrtd, i, &st));
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
	struct wrtd_trigger_handle h[FD_NUM_CHANNELS];
	struct wrtd_trig_id id = {9,9,9};
	char msg[128];
	uint64_t ps, min, max;
	int i, k;

	wrtd = wrtd_open_by_lun(0);
	for (i = 0; i < FD_NUM_CHANNELS; i++) {
		id.source_port = i;
		CuAssertIntEquals(tc, 0, wrtd_out_trig_assign(wrtd, i, &h[i],
							      &id, NULL));
		CuAssertIntEquals(tc, -1,
				  wrtd_out_trig_delay_set(wrtd, &h[i],
							  999999999999));
		CuAssertIntEquals(tc, EWRTD_INVALID_DELAY, errno);
		for (k = 1; k < 0x8000000; k <<= 1) {
			CuAssertIntEquals(tc, 0,
					  wrtd_out_trig_delay_set(wrtd, &h[i],
								  k));
			CuAssertIntEquals(tc, 0,
					  wrtd_out_trig_state_get_by_handle(wrtd,
									    &h[i],
									    &trig));
			wrtd_ts_to_pico(&trig.delay_trig, &ps);
			/* Conversion is not precise, some approximation may
			   happen */
			min = k < RANGE ? 0 : k - RANGE;
			max = k + RANGE;
			sprintf(msg,
				"Assert failed - chan %d - %"PRIu64" not in range [%"PRIu64", %"PRIu64"]",
				i, ps, min, max);
			CuAssertTrue_Msg(tc, msg, ps >= min && ps <= max);
		}

	}

	for (i = 0; i < FD_NUM_CHANNELS; i++)
		wrtd_out_trig_unassign(wrtd, &h[i]);

	wrtd_close(wrtd);
}

static void test_trigger_enable(CuTest *tc)
{
	struct wrtd_node *wrtd;
	struct wrtd_output_trigger_state trig;
	struct wrtd_trigger_handle h;
	struct wrtd_trig_id id = {9,9,9};
	int i, k;

	wrtd = wrtd_open_by_lun(0);
	for (i = 0; i < FD_NUM_CHANNELS; i++) {
		CuAssertIntEquals(tc, 0, wrtd_out_trig_assign(wrtd, i, &h, &id, NULL));
		for (k = 0; k < 10; k++) {
			/* Enable */
			CuAssertIntEquals(tc, 0, wrtd_out_trig_enable(wrtd, &h, 1));
			CuAssertIntEquals(tc, 0, wrtd_out_trig_state_get_by_handle(wrtd, &h,
									    &trig));
			CuAssertTrue(tc, trig.enabled);

			/* Disable */
			CuAssertIntEquals(tc, 0, wrtd_out_trig_enable(wrtd, &h, 0));
			CuAssertIntEquals(tc, 0, wrtd_out_trig_state_get_by_handle(wrtd, &h,
									    &trig));
			CuAssertIntEquals(tc, 0, trig.enabled);
		}
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
		CuAssertIntEquals(tc, 0, wrtd_out_trigger_mode_set(wrtd, i,
							    WRTD_TRIGGER_MODE_SINGLE));
		CuAssertIntEquals(tc, 0, wrtd_out_state_get(wrtd, i, &st));
		CuAssertIntEquals(tc, WRTD_TRIGGER_MODE_SINGLE, st.mode);

		CuAssertIntEquals(tc, 0, wrtd_out_trigger_mode_set(wrtd, i,
							    WRTD_TRIGGER_MODE_AUTO));
		CuAssertIntEquals(tc, 0, wrtd_out_state_get(wrtd, i, &st));
		CuAssertIntEquals(tc, WRTD_TRIGGER_MODE_AUTO, st.mode);

		CuAssertIntEquals(tc, 0, wrtd_out_trigger_mode_set(wrtd, i,
							    WRTD_TRIGGER_MODE_SINGLE));
		CuAssertIntEquals(tc, 0, wrtd_out_state_get(wrtd, i, &st));
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
		CuAssertIntEquals(tc, 0, wrtd_out_counters_reset(wrtd, i));
		CuAssertIntEquals(tc, 0, wrtd_out_state_get(wrtd, i, &st));
		CuAssertIntEquals(tc, 0, st.missed_pulses_late);
		CuAssertIntEquals(tc, 0, st.missed_pulses_deadtime);
		CuAssertIntEquals(tc, 0, st.missed_pulses_overflow);
		CuAssertIntEquals(tc, 0, st.missed_pulses_no_timing);
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
	SUITE_ADD_TEST(suite, test_trigger_mode);
	SUITE_ADD_TEST(suite, test_reset_counters);
	SUITE_ADD_TEST(suite, test_trigger_assign_cond);
	SUITE_ADD_TEST(suite, test_trigger_unassign_cond);

	return suite;
}
