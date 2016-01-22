/**
 * Copyright (C) 2016 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v3
 *
 * @TODO software trigger test
 */
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <time.h>

#include "wrtd-ut.h"
#include "CuTest.h"

#include <libwrnc.h>
#include <libwrtd.h>

/**
 * For this test there is no special configuration because it uses the
 * internal loopback
 */
static void test_msg_loop(CuTest *tc)
{
	struct wrtd_node *wrtd;
	struct wrtd_trig_id tid = {9, 9, 9};
	struct wrtd_trigger_handle h;
	struct wrtd_input_state sti;
	struct wrtd_output_state sto;

	wrtd = wrtd_open_by_lun(1);
	CuAssertPtrNotNull(tc, wrtd);

	CuAssertIntEquals(tc, 0, wrtd_cpu_restart(wrtd));
	sleep(10); /* Wait the White Rabbit link */

	/* Output */
	CuAssertIntEquals(tc, 0, wrtd_out_trig_assign(wrtd, 0, &h, &tid, NULL));
	CuAssertIntEquals(tc, 0, wrtd_out_trigger_mode_set(wrtd, 0, WRTD_TRIGGER_MODE_AUTO));
	CuAssertIntEquals(tc, 0, wrtd_out_arm(wrtd, 0, 1));
	CuAssertIntEquals(tc, 0, wrtd_out_enable(wrtd, 0, 1));
	CuAssertIntEquals(tc, 0, wrtd_out_trig_enable(wrtd, &h, 1));

	/* Input */
	CuAssertIntEquals(tc, 0, wrtd_in_trigger_assign(wrtd, 0, &tid));
	CuAssertIntEquals(tc, 0, wrtd_in_trigger_mode_set(wrtd, 0, WRTD_TRIGGER_MODE_AUTO));
	CuAssertIntEquals(tc, 0, wrtd_in_arm(wrtd, 0, 1));
	CuAssertIntEquals(tc, 0, wrtd_in_enable(wrtd, 0, 1));
	sleep(5);

	CuAssertIntEquals(tc, 0, wrtd_in_enable(wrtd, 0, 0));
	sleep(1);
	CuAssertIntEquals(tc, 0, wrtd_out_enable(wrtd, 0, 0));

	CuAssertIntEquals(tc, 0, wrtd_in_state_get(wrtd, 0, &sti));
	CuAssertIntEquals(tc, 0, wrtd_out_state_get(wrtd, 0, &sto));

	CuAssertIntEquals(tc, sti.tagged_pulses, sti.sent_triggers);
	CuAssertIntEquals(tc, sti.sent_triggers, sto.received_loopback);
	CuAssertIntEquals(tc, 0, sto.received_messages);
	CuAssertIntEquals(tc, 0, memcmp(&sti.last_sent, &sto.last_received,
					sizeof(struct wrtd_trigger_entry)));

	/* Clear */
	CuAssertIntEquals(tc, 0, wrtd_in_trigger_unassign(wrtd, 0));
	CuAssertIntEquals(tc, 0, wrtd_out_trig_unassign(wrtd, &h));
	wrtd_close(wrtd);
}

/**
 * For this test you need 2 WRTD boards (LUN 0 LUN 1) and 1 white rabbit switch.
 * You have to connect a pulse generator to the channel 0 of the TDC
 * mezzanine on board with LUN 1
 */
static void test_msg_wr(CuTest *tc)
{
	struct wrtd_node *wrtd_0, *wrtd_1;
	struct wrtd_trig_id tid = {9, 9, 9};
	struct wrtd_trigger_handle h;
	struct wrtd_input_state sti;
	struct wrtd_output_state sto;
	unsigned int prev_lb, prev_msg;

	wrtd_0 = wrtd_open_by_lun(1);
	CuAssertPtrNotNull(tc, wrtd_0);
	wrtd_1 = wrtd_open_by_lun(0);
	CuAssertPtrNotNull(tc, wrtd_1);

	CuAssertIntEquals(tc, 0, wrtd_cpu_restart(wrtd_0));
	CuAssertIntEquals(tc, 0, wrtd_cpu_restart(wrtd_1));
	sleep(10); /* Wait the White Rabbit link */

	CuAssertIntEquals(tc, 0, wrtd_out_state_get(wrtd_1, 0, &sto));
	prev_lb = sto.received_loopback;
	prev_msg = sto.received_messages;

	CuAssertIntEquals(tc, 0, wrtd_in_enable(wrtd_0, 0, 0));
	CuAssertIntEquals(tc, 0, wrtd_out_enable(wrtd_1, 0, 0));

	CuAssertIntEquals(tc, 0, wrtd_in_counters_reset(wrtd_0, 0));
	CuAssertIntEquals(tc, 0, wrtd_out_counters_reset(wrtd_1, 0));

	CuAssertIntEquals(tc, 0, wrtd_out_state_get(wrtd_1, 0, &sto));
	prev_lb = sto.received_loopback;
	prev_msg = sto.received_messages;

	/* Input */
	CuAssertIntEquals(tc, 0, wrtd_in_trigger_assign(wrtd_0, 0, &tid));
	CuAssertIntEquals(tc, 0, wrtd_in_trigger_mode_set(wrtd_0, 0, WRTD_TRIGGER_MODE_AUTO));
	CuAssertIntEquals(tc, 0, wrtd_in_arm(wrtd_0, 0, 1));
	CuAssertIntEquals(tc, 0, wrtd_in_enable(wrtd_0, 0, 1));

	/* Output */
	CuAssertIntEquals(tc, 0, wrtd_out_trig_assign(wrtd_1, 0, &h, &tid, NULL));
	CuAssertIntEquals(tc, 0, wrtd_out_trigger_mode_set(wrtd_1, 0, WRTD_TRIGGER_MODE_AUTO));
	CuAssertIntEquals(tc, 0, wrtd_out_arm(wrtd_1, 0, 1));
	CuAssertIntEquals(tc, 0, wrtd_out_enable(wrtd_1, 0, 1));
	CuAssertIntEquals(tc, 0, wrtd_out_trig_enable(wrtd_1, &h, 1));

	sleep(5);
	CuAssertIntEquals(tc, 0, wrtd_in_enable(wrtd_0, 0, 0));
	sleep(1);
	CuAssertIntEquals(tc, 0, wrtd_out_enable(wrtd_1, 0, 0));

	CuAssertIntEquals(tc, 0, wrtd_in_state_get(wrtd_0, 0, &sti));
	CuAssertIntEquals(tc, 0, wrtd_out_state_get(wrtd_1, 0, &sto));

	CuAssertIntEquals(tc, sti.sent_triggers,
			  sto.received_messages - prev_msg);
	CuAssertIntEquals(tc, prev_lb, sto.received_loopback);
	CuAssertIntEquals(tc, 0, memcmp(&sti.last_sent, &sto.last_received,
					sizeof(struct wrtd_trigger_entry)));

	/* Clear */
	CuAssertIntEquals(tc, 0, wrtd_in_arm(wrtd_0, 0, 0));
	CuAssertIntEquals(tc, 0, wrtd_in_trigger_unassign(wrtd_0, 0));
	CuAssertIntEquals(tc, 0, wrtd_out_arm(wrtd_1, 0, 0));
	CuAssertIntEquals(tc, 0, wrtd_out_trig_unassign(wrtd_1, &h));
	wrtd_close(wrtd_0);
	wrtd_close(wrtd_1);
}


/**
 * For this test you need 2 WRTD boards (LUN 0 LUN 1) and 1 white rabbit switch.
 * You have to connect a pulse generator to the channel 0 of the TDC
 * mezzanine on board with LUN 1.
 * You have to put the following lemo cables:
 * - LUN 0, FineDelay channel 0 -----> LUN 1 TDC channel 1
 * - LUN 0, FineDelay channel 1 -----> LUN 0 TDC channel 1
 * - LUN 1, FineDelay channel 0 -----> LUN 0 TDC channel 0
 */
static void test_msg_mix(CuTest *tc)
{
	struct wrtd_node *wrtd_0, *wrtd_1;
	struct wrtd_trig_id tid_1 = {0, 0, 1};
	struct wrtd_trig_id tid_2 = {0, 1, 1};
	struct wrtd_trig_id tid_3 = {1, 0, 1};
	struct wrtd_trig_id tid_4 = {1, 1, 1};
	struct wrtd_trigger_handle h_2, h_3, h_4;
	struct wrtd_input_state sti1, sti2;
	struct wrtd_output_state sto1, sto2;
	unsigned int prev_msg_2, prev_msg_3, prev_msg_4;

	wrtd_0 = wrtd_open_by_lun(0);
	CuAssertPtrNotNull(tc, wrtd_0);
	wrtd_1 = wrtd_open_by_lun(1);
	CuAssertPtrNotNull(tc, wrtd_1);

	CuAssertIntEquals(tc, 0, wrtd_cpu_restart(wrtd_0));
	CuAssertIntEquals(tc, 0, wrtd_cpu_restart(wrtd_1));
	sleep(10); /* Wait the White Rabbit link */

	CuAssertIntEquals(tc, 0, wrtd_out_state_get(wrtd_1, 0, &sto1));
	prev_msg_2 = sto1.received_messages;
	CuAssertIntEquals(tc, 0, wrtd_out_state_get(wrtd_0, 0, &sto1));
	prev_msg_3 = sto1.received_messages;
	CuAssertIntEquals(tc, 0, wrtd_out_state_get(wrtd_0, 1, &sto1));
	prev_msg_4 = sto1.received_messages;

	/* Assign trigger input */
	CuAssertIntEquals(tc, 0, wrtd_in_trigger_assign(wrtd_0, 0, &tid_1));
	CuAssertIntEquals(tc, 0, wrtd_in_trigger_assign(wrtd_0, 1, &tid_2));
	CuAssertIntEquals(tc, 0, wrtd_in_trigger_assign(wrtd_1, 0, &tid_3));
	CuAssertIntEquals(tc, 0, wrtd_in_trigger_assign(wrtd_1, 1, &tid_4));
	CuAssertIntEquals(tc, 0, wrtd_in_trigger_mode_set(wrtd_0, 0, WRTD_TRIGGER_MODE_AUTO));
	CuAssertIntEquals(tc, 0, wrtd_in_trigger_mode_set(wrtd_0, 1, WRTD_TRIGGER_MODE_AUTO));
	CuAssertIntEquals(tc, 0, wrtd_in_trigger_mode_set(wrtd_1, 0, WRTD_TRIGGER_MODE_AUTO));
	CuAssertIntEquals(tc, 0, wrtd_in_trigger_mode_set(wrtd_1, 1, WRTD_TRIGGER_MODE_AUTO));
	/* Assign trigger output */
	CuAssertIntEquals(tc, 0, wrtd_out_trig_assign(wrtd_1, 0, &h_2, &tid_2, NULL));
	CuAssertIntEquals(tc, 0, wrtd_out_trig_assign(wrtd_0, 0, &h_3, &tid_3, NULL));
	CuAssertIntEquals(tc, 0, wrtd_out_trig_assign(wrtd_0, 1, &h_4, &tid_4, NULL));

	CuAssertIntEquals(tc, 0, wrtd_out_trigger_mode_set(wrtd_1, 0, WRTD_TRIGGER_MODE_AUTO));
	CuAssertIntEquals(tc, 0, wrtd_out_trigger_mode_set(wrtd_0, 1, WRTD_TRIGGER_MODE_AUTO));

	CuAssertIntEquals(tc, 0, wrtd_in_dead_time_set(wrtd_0, 0, 80000000));
	CuAssertIntEquals(tc, 0, wrtd_in_dead_time_set(wrtd_1, 0, 80000000));
	CuAssertIntEquals(tc, 0, wrtd_in_dead_time_set(wrtd_0, 1, 80000000));
	CuAssertIntEquals(tc, 0, wrtd_in_dead_time_set(wrtd_1, 1, 80000000));
	/* Enable and Arm */
	CuAssertIntEquals(tc, 0, wrtd_out_arm(wrtd_0, 0, 1));
	CuAssertIntEquals(tc, 0, wrtd_out_arm(wrtd_0, 1, 1));
	CuAssertIntEquals(tc, 0, wrtd_out_arm(wrtd_1, 0, 1));
	CuAssertIntEquals(tc, 0, wrtd_out_enable(wrtd_0, 0, 1));
	CuAssertIntEquals(tc, 0, wrtd_out_enable(wrtd_0, 1, 1));
	CuAssertIntEquals(tc, 0, wrtd_out_enable(wrtd_1, 0, 1));
	CuAssertIntEquals(tc, 0, wrtd_out_trig_enable(wrtd_1, &h_2, 1));
	CuAssertIntEquals(tc, 0, wrtd_out_trig_enable(wrtd_0, &h_3, 1));
	CuAssertIntEquals(tc, 0, wrtd_out_trig_enable(wrtd_0, &h_4, 1));

	CuAssertIntEquals(tc, 0, wrtd_in_arm(wrtd_0, 0, 1));
	CuAssertIntEquals(tc, 0, wrtd_in_arm(wrtd_0, 1, 1));
	CuAssertIntEquals(tc, 0, wrtd_in_arm(wrtd_1, 1, 1));
	/* Last because connected to pulse generator */
	CuAssertIntEquals(tc, 0, wrtd_in_arm(wrtd_1, 0, 1));
	CuAssertIntEquals(tc, 0, wrtd_in_enable(wrtd_0, 0, 1));
	CuAssertIntEquals(tc, 0, wrtd_in_enable(wrtd_0, 1, 1));
	CuAssertIntEquals(tc, 0, wrtd_in_enable(wrtd_1, 1, 1));
	/* Last because connected to pulse generator */
	CuAssertIntEquals(tc, 0, wrtd_in_enable(wrtd_1, 0, 1));

	sleep(5);

	/* Stop getting pulses from pulse-generator */
	CuAssertIntEquals(tc, 0, wrtd_in_enable(wrtd_1, 0, 0));
	sleep(2);
	CuAssertIntEquals(tc, 0, wrtd_in_enable(wrtd_1, 1, 0));
	CuAssertIntEquals(tc, 0, wrtd_in_enable(wrtd_0, 0, 0));
	CuAssertIntEquals(tc, 0, wrtd_in_enable(wrtd_0, 1, 0));
	CuAssertIntEquals(tc, 0, wrtd_out_enable(wrtd_0, 0, 0));
	CuAssertIntEquals(tc, 0, wrtd_out_enable(wrtd_0, 1, 0));
	CuAssertIntEquals(tc, 0, wrtd_out_enable(wrtd_1, 0, 0));


	CuAssertIntEquals(tc, 0, wrtd_in_state_get(wrtd_1, 0, &sti1));
	CuAssertIntEquals(tc, 0, wrtd_out_state_get(wrtd_0, 0, &sto1));
	CuAssertIntEquals(tc, 0, wrtd_in_state_get(wrtd_1, 1, &sti2));
	CuAssertIntEquals(tc, 0, wrtd_out_state_get(wrtd_0, 1, &sto2));

	/* Sent and received messages are the same */
	CuAssertIntEquals(tc, sti1.sent_triggers, sto1.executed_pulses);
	CuAssertIntEquals(tc, sti2.sent_triggers, sto2.executed_pulses);
	CuAssertIntEquals(tc, sti1.sent_triggers, sti2.sent_triggers);

	/* Last sent trigger is the last executed */
	CuAssertIntEquals(tc, sti1.last_sent.seq, sto1.last_executed.seq);
	CuAssertIntEquals(tc, sti2.last_sent.seq, sto2.last_executed.seq);
	CuAssertIntEquals(tc, 0, memcmp(&sti1.last_sent.id,
					&sto1.last_executed.id,
					sizeof(struct wrtd_trig_id)));
	CuAssertIntEquals(tc, 0, memcmp(&sti2.last_sent.id,
					&sto2.last_executed.id,
					sizeof(struct wrtd_trig_id)));

	CuAssertIntEquals(tc, 0, wrtd_out_trig_unassign(wrtd_1, &h_2));
	CuAssertIntEquals(tc, 0, wrtd_out_trig_unassign(wrtd_0, &h_3));
	CuAssertIntEquals(tc, 0, wrtd_out_trig_unassign(wrtd_0, &h_4));

	/* Close */
	wrtd_close(wrtd_0);
	wrtd_close(wrtd_1);
}

CuSuite *wrtd_ut_op_suite_get(void)
{
	CuSuite *suite = CuSuiteNew();

	SUITE_ADD_TEST(suite, test_msg_loop);
	SUITE_ADD_TEST(suite, test_msg_wr);
	SUITE_ADD_TEST(suite, test_msg_mix);

	return suite;
}
