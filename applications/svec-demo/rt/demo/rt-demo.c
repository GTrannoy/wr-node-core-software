/*
 * This work is part of the White Rabbit Node Core project.
 *
 * Copyright (C) 2013-2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *
 * Released according to the GNU GPL, version 2 or any later version.
 */

#include <string.h>
#include <rt.h>
#include <demo-common.h>

#define GPIO_CODR	0x0 /* Clear Data Register */
#define GPIO_SODR	0x4 /* Set Data Register */
#define GPIO_DDR	0x8 /* Direction Data Register */
#define GPIO_PSR	0xC /* Status Register */

void rt_get_time(uint32_t *seconds, uint32_t *cycles)
{
	*seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
	*cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
}

static uint32_t out_seq = 0;

/**
 * Set LEDs value
 */
static void demo_led_set(uint32_t val)
{
	val = (val << PIN_LED_OFFSET) & PIN_LED_MASK;
	dp_writel(val, GPIO_SODR);
	dp_writel((~val), GPIO_CODR);
}


/**
 * Get the current LED status
 */
static uint32_t demo_led_get(void)
{
	dp_readl(GPIO_PSR) & PIN_LED_MASK;
}

/**
 * Set LEMOs value
 */
static void demo_lemo_set(uint32_t val)
{
	dp_writel((val & PIN_LEMO_MASK),
		  GPIO_SODR);
        dp_writel(((~val) & PIN_LEMO_MASK),
		  GPIO_CODR);
}

/**
 * Get current LEMO status
 */
static uint32_t demo_lemo_get(void)
{
	return dp_readl(GPIO_PSR) & PIN_LEMO_MASK;
}


/**
 * Set direction for all LEMOs.
 */
static void demo_lemo_dir_set(uint32_t output)
{
	dp_writel(output & PIN_LEMO_MASK, GPIO_DDR);
}

/**
 * Get current LEMO's direction
 */
static uint32_t demo_lemo_dir_get(void)
{
	return dp_readl(GPIO_DDR) & PIN_LEMO_MASK;
}



/**
 * It sends the register status to the host.
 */
static void demo_status_send(uint32_t seq)
{
	struct wrnc_msg out_buf;

	out_buf = hmq_msg_claim_out (DEMO_HMQ_OUT, 128);
	out_buf.data[0] = DEMO_ID_STATE_GET_REP;
	out_buf.data[1] = seq;
	out_buf.data[2] = demo_led_get();
	out_buf.data[3] = demo_lemo_get();
	out_buf.data[4] = demo_lemo_dir_get();
	out_buf.datalen = 5;

	/* Send the message */
	hmq_msg_send (&out_buf);
}


/**
 * Send the status to inform that the program is alive. The use of this
 * function shows that we can autonomously send messages to the host
 * whenever we want.
 */
static inline void demo_alive(void)
{
	//pp_printf("Send Status.\n");
	demo_status_send(out_seq);
	out_seq++;
}


/**
 * Sends an acknowledgement reply
 */
static inline void demo_hmq_ack(uint32_t seq)
{
	struct wrnc_msg out_buf;

	out_buf = hmq_msg_claim_out(DEMO_HMQ_OUT, 128);
	/* For the time being there is not a standard for the acknowledge
	   message ID. The only constraint now is the fact that the sequence
	   number must be equal to the incoming sequence number */
	out_buf.data[0] = DEMO_ID_ACK;
	out_buf.data[1] = seq;
	out_buf.datalen = 2;

	/* Send the message */
	hmq_msg_send(&out_buf);
}


/**
 * It sends messages over the debug interface
 */
static void demo_debug_interface(void)
{
	pp_printf("Hello world.\n");
	pp_printf("We are messages over the debug interface.\n");
	pp_printf("Print here your messages.\n");
}


/**
 * It handles the incoming messages from an input HMQ.
 * This show you a way to handle incoming messages from the host.
 *
 * Here we handle both synchronous and asynchronous messages.
 */
static void demo_handle_hmq_in(void)
{
#ifdef RTPERFORMANCE
	uint32_t sec, cyc, sec_n, cyc_n;
#endif
	struct wrnc_msg in_buf;
	uint32_t id, seq, p, val;

	/* HMQ control slot empty? */
	p = mq_poll();
	if (!(p & ( 1<< DEMO_HMQ_IN)))
		return;

	/* We have something in the input HMQ */

	/* Get the message from the HMQ by claiming it */
	in_buf = hmq_msg_claim_in(DEMO_HMQ_IN, 8);

	/* Extract the ID and sequence number from the header */
	id = in_buf.data[0];
	seq = in_buf.data[1];

	pp_printf("Received message ID 0x%x SEQ 0x%x DATA[0] 0x%x.\n",
		  id, seq, in_buf.data[2]);
	/* According to the ID perform different actions */
#ifdef RTPERFORMANCE
	rt_get_time(&sec, &cyc);
#endif
	switch (id) {
	/* Set LEDs */
	case DEMO_ID_LED_SET:
		/* Extract the LEDs status to set from the message */
		val = in_buf.data[2];

		/* Set the new LEDs status */
		demo_led_set(val);

		/* This is *not* a synchronous message, so it does not need
		   any acknowledge message. There are not particular reasons
		   against synchronous message, this is just an example */
		break;

	/* Set LEMOs */
	case DEMO_ID_LEMO_SET:
		/* Extract from the messag the LEMOs status to sete */
		val = in_buf.data[2];

		/* Set the new LEDs status */
		demo_lemo_set(val);

		/* This is a synchronous message, so send an acknowledge message
		   by using the same sequence number. There are not particulr reasons
		   to be synchronous message, this is just an example */
		demo_hmq_ack(seq);
		break;

	/* Set LEMOs Direction */
	case DEMO_ID_LEMO_DIR_SET:
		/* Extract from the message the LEMO directions to set */
		val = in_buf.data[2];

		/* Set the new LEDs status */
		demo_lemo_dir_set(val);

		/* This is a synchronous message, so send an acknowledge message
		   by using the same sequence number. There are not particulr reasons
		   to be synchronous message, this is just an example */
		demo_hmq_ack(seq);
		break;

	/* Get state */
	case DEMO_ID_STATE_GET:
		/* This is a synchronous message. The user asks for data so we
		   must answer synchrounously */
		demo_status_send(seq);
		break;

	/* Other actions/cases here */

	/* Unknown actions */
	default:
		pp_printf("Unknown ID %d.\n", id);
		break;
	}
#ifdef RTPERFORMANCE
	rt_get_time(&sec_n, &cyc_n);
	pp_printf("Action ID: %d | SEQ: | %d TIME: %dns\n",
		  id, seq, (cyc_n - cyc) * 8);
#endif

	/* Drop the message once handled */
	mq_discard(0, DEMO_HMQ_IN);
}

/**
 * Well, the main :)
 */
int main()
{
	int i = 0;
	demo_debug_interface();

	while (1) {
		++i;
		demo_handle_hmq_in();

		/* Once in a while send an alive message */
		if (i % 500000 == 0)
			demo_alive();
	}

	return 0;
}
