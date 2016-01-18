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
	CuAssertIntEquals(tc, 0, !wrtd);
	CuAssertIntEquals(tc, 0, wrtd_in_ping(wrtd));
	wrtd_close(wrtd);
}

static void test_channels_enable(CuTest *tc)
{
	struct wrtd_node *wrtd;
	unsigned int i, enable;

	wrtd = wrtd_open_by_lun(0);
	CuAssertIntEquals(tc, 0, !wrtd);
	for (i = 0; i < TDC_NUM_CHANNELS; i++) {
		CuAssertIntEquals(tc, 0, wrtd_in_enable(wrtd, i, 1));
		CuAssertIntEquals(tc, 0, wrtd_in_is_enabled(wrtd, i, &enable));
		CuAssertTrue(tc, enable);
	}

	wrtd_close(wrtd);
}

static void test_channels_disable(CuTest *tc)
{
	struct wrtd_node *wrtd;
	unsigned int i, enable;

	wrtd = wrtd_open_by_lun(0);
	CuAssertIntEquals(tc, 0, !wrtd);
	for (i = 0; i < TDC_NUM_CHANNELS; i++) {
		CuAssertIntEquals(tc, 0, wrtd_in_enable(wrtd, i, 0));
		CuAssertIntEquals(tc, 0, wrtd_in_is_enabled(wrtd, i, &enable));
		CuAssertTrue(tc, !enable);
	}

	wrtd_close(wrtd);
}

static void test_channels_arm(CuTest *tc)
{
	struct wrtd_node *wrtd;
	unsigned int i, armed;

	wrtd = wrtd_open_by_lun(0);
	CuAssertIntEquals(tc, 0, !wrtd);
	for (i = 0; i < TDC_NUM_CHANNELS; i++) {
		CuAssertIntEquals(tc, 0, wrtd_in_arm(wrtd, i, 1));
		CuAssertIntEquals(tc, 0, wrtd_in_is_armed(wrtd, i, &armed));
		CuAssertTrue(tc, armed);
	}
	wrtd_close(wrtd);
}

static void test_channels_disarm(CuTest *tc)
{
	struct wrtd_node *wrtd;
	unsigned int i, armed;

	wrtd = wrtd_open_by_lun(0);
	CuAssertIntEquals(tc, 0, !wrtd);
	for (i = 0; i < TDC_NUM_CHANNELS; i++) {
		CuAssertIntEquals(tc, 0, wrtd_in_arm(wrtd, i, 0));
		CuAssertIntEquals(tc, 0, wrtd_in_is_armed(wrtd, i, &armed));
		CuAssertTrue(tc, !armed);
	}
	wrtd_close(wrtd);
}

static void test_trigger_mode(CuTest *tc)
{
	struct wrtd_node *wrtd;
	struct wrtd_input_state st;
	int i;

	wrtd = wrtd_open_by_lun(0);
	CuAssertIntEquals(tc, 0, !wrtd);
	for (i = 0; i < TDC_NUM_CHANNELS; i++) {
		CuAssertIntEquals(tc, 0, wrtd_in_trigger_mode_set(wrtd, i, WRTD_TRIGGER_MODE_SINGLE));
		CuAssertIntEquals(tc, 0, wrtd_in_state_get(wrtd, i, &st));
		CuAssertIntEquals(tc, WRTD_TRIGGER_MODE_SINGLE, st.mode);

		CuAssertIntEquals(tc, 0, wrtd_in_trigger_mode_set(wrtd, i, WRTD_TRIGGER_MODE_AUTO));
		CuAssertIntEquals(tc, 0, wrtd_in_state_get(wrtd, i, &st));
		CuAssertIntEquals(tc, WRTD_TRIGGER_MODE_AUTO, st.mode);
	}
	wrtd_close(wrtd);
}

static void test_trigger_assign(CuTest *tc)
{
	struct wrtd_node *wrtd;
	struct wrtd_input_state st;
	struct wrtd_trig_id id = {1,2,3};
	unsigned int assigned;
	int i;

	wrtd = wrtd_open_by_lun(0);
	CuAssertIntEquals(tc, 0, !wrtd);
	for (i = 0; i < TDC_NUM_CHANNELS; i++) {
		id.trigger = i;
		CuAssertIntEquals(tc, 0, wrtd_in_trigger_assign(wrtd, i, &id));
		CuAssertIntEquals(tc, 0, wrtd_in_state_get(wrtd, i, &st));
		CuAssertIntEquals(tc, 0, memcmp(&id, &st.assigned_id,
					 sizeof(struct wrtd_trig_id)));
		CuAssertIntEquals(tc, 0, wrtd_in_has_trigger(wrtd, i, &assigned));
		CuAssertTrue(tc, assigned);
	}
	wrtd_close(wrtd);
}

static void test_trigger_unassign(CuTest *tc)
{
	struct wrtd_node *wrtd;
	struct wrtd_input_state st;
	struct wrtd_trig_id id = {0,0,0};
	unsigned int assigned;
	int i;

	wrtd = wrtd_open_by_lun(0);
	CuAssertIntEquals(tc, 0, !wrtd);
	for (i = 0; i < TDC_NUM_CHANNELS; i++) {
		CuAssertIntEquals(tc, 0, wrtd_in_trigger_unassign(wrtd, i));
		CuAssertIntEquals(tc, 0, wrtd_in_state_get(wrtd, i, &st));
		CuAssertIntEquals(tc, 0, memcmp(&id, &st.assigned_id,
					 sizeof(struct wrtd_trig_id)));
		CuAssertIntEquals(tc, 0, wrtd_in_has_trigger(wrtd, i, &assigned));
		CuAssertTrue(tc, !assigned);
	}
	wrtd_close(wrtd);
}

static void test_trigger_software(CuTest *tc)
{
	struct wrtd_node *wrtd;
	struct wrtd_input_state st;
	struct wrtd_trigger_entry trig = {
		.ts = {0, 100, 0},
		.id = {1,2,3},
	};
	int i, sent;

	wrtd = wrtd_open_by_lun(0);
	CuAssertIntEquals(tc, 0, !wrtd);
	/* It does not metter which channel we query, the sent triggers
	 value is global */
	CuAssertIntEquals(tc, 0, wrtd_in_state_get(wrtd, 0, &st));
	sent = st.sent_packets;
	for (i = 0; i < 100; i++) {
		trig.seq = i;
		CuAssertIntEquals(tc, 0, wrtd_in_trigger_software(wrtd, &trig));
	}
	CuAssertIntEquals(tc, 0, wrtd_in_state_get(wrtd, 0, &st));
	CuAssertIntEquals(tc, st.sent_packets, sent + 100);
	wrtd_close(wrtd);
}

static void test_reset_counters(CuTest *tc)
{
	struct wrtd_node *wrtd;
	struct wrtd_input_state st;
	int i;

	wrtd = wrtd_open_by_lun(0);
	CuAssertIntEquals(tc, 0, !wrtd);
	for (i = 0; i < FD_NUM_CHANNELS; i++) {
		CuAssertIntEquals(tc, 0, wrtd_in_counters_reset(wrtd, i));
		CuAssertIntEquals(tc, 0, wrtd_in_state_get(wrtd, i, &st));
		CuAssertIntEquals(tc, 0, st.sent_triggers);
		CuAssertIntEquals(tc, 0, st.tagged_pulses);
		CuAssertIntEquals(tc, 0, (st.flags & WRTD_LAST_VALID));
	}
	wrtd_close(wrtd);
}

#define RANGE_DEAD 16000
static void test_dead_time(CuTest *tc)
{
	struct wrtd_node *wrtd;
	uint64_t ps, min, max;
	char msg[128];
	int i, k;

	wrtd = wrtd_open_by_lun(0);
	CuAssertIntEquals(tc, 0, !wrtd);
	for (i = 0; i < TDC_NUM_CHANNELS; i++) {
		CuAssertIntEquals(tc, -1, wrtd_in_dead_time_set(wrtd, i, 0));
		CuAssertIntEquals(tc, EWRTD_INVALID_DEAD_TIME, errno);
		CuAssertIntEquals(tc, -1, wrtd_in_dead_time_set(wrtd, i,
								170000000000ULL));
		CuAssertIntEquals(tc, EWRTD_INVALID_DEAD_TIME, errno);
		for (k = 0x8000000; k < 0x80000000; k <<= 1) {
			CuAssertIntEquals(tc, 0, wrtd_in_dead_time_set(wrtd, i, k));
			CuAssertIntEquals(tc, 0, wrtd_in_dead_time_get(wrtd, i, &ps));
			min = k < RANGE_DEAD ? 0 : k - RANGE_DEAD;
			max = k + RANGE_DEAD;
			sprintf(msg,
				"Assert failed - %"PRIu64" not in range [%"PRIu64", %"PRIu64"]",
				ps, min, max);
			CuAssertTrue_Msg(tc, msg, ps >= min && ps <= max);
		}
	}
	wrtd_close(wrtd);
}

#define RANGE 10
static void test_delay(CuTest *tc)
{
	struct wrtd_node *wrtd;
	uint64_t ps, min, max;
	char msg[128];
	int i, k;

	wrtd = wrtd_open_by_lun(0);
	CuAssertIntEquals(tc, 0, !wrtd);
	for (i = 0; i < TDC_NUM_CHANNELS; i++) {
		for (k = 1; k < 8; k <<= 1) {
			CuAssertIntEquals(tc, 0, wrtd_in_delay_set(wrtd, i, k));
			CuAssertIntEquals(tc, 0, wrtd_in_delay_get(wrtd, i, &ps));
			min = k < RANGE ? 0 : k - RANGE;
			max = k + RANGE;
			sprintf(msg,
				"Assert failed - %"PRIu64" not in range [%"PRIu64", %"PRIu64"]",
				ps, min, max);
			CuAssertTrue_Msg(tc, msg, ps >= min && ps <= max);
		}
	}
	wrtd_close(wrtd);
}

CuSuite *wrtd_ut_in_suite_get(void)
{
	CuSuite *suite = CuSuiteNew();

	SUITE_ADD_TEST(suite, test_ping);
	SUITE_ADD_TEST(suite, test_channels_enable);
	SUITE_ADD_TEST(suite, test_channels_disable);
	SUITE_ADD_TEST(suite, test_channels_arm);
	SUITE_ADD_TEST(suite, test_channels_disarm);
	SUITE_ADD_TEST(suite, test_trigger_mode);
	SUITE_ADD_TEST(suite, test_trigger_assign);
	SUITE_ADD_TEST(suite, test_trigger_unassign);
	SUITE_ADD_TEST(suite, test_trigger_software);
	SUITE_ADD_TEST(suite, test_reset_counters);
	SUITE_ADD_TEST(suite, test_delay);
	SUITE_ADD_TEST(suite, test_dead_time);

	return suite;
}
