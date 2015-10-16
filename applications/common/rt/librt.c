/*
 * Copyright (C) 2015 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *
 * Released according to the GNU GPL, version 3
 */

#include <stdint.h>
#include <errno.h>
#include <librt.h>


const char *name = RT_APPLICATION_NAME;
const struct wrnc_rt_version version = {0, RT_APPLICATION_ID, 0, GIT_VERSION};
struct rt_application *_app;


/**
 * it sets a structure coming from the host
 */
int rt_structure_setter(struct wrnc_proto_header *hin, void *pin,
			struct wrnc_proto_header *hout, void *pout)
{
	unsigned int i, offset = 0, index, size;
	uint32_t *din = pin;

	while (offset < hin->len) {
#ifdef LIBRT_DEBUG
		pp_printf("%s: offset %d/%d\n", __func__, offset, hin->len);
#endif
		index = din[offset++];
		size = din[offset++];
#ifdef LIBRT_DEBUG
		pp_printf("%s Type %d Len %d Addr 0x%x\n", __func__,
			  index, size, _app->structures[index].struct_ptr);
		delay(100000);
#endif
		if (_app->structures[index].len == size) {
			memcpy(_app->structures[index].struct_ptr,
			       &din[offset], size);
		}
#ifdef LIBRT_ERROR
		else {
			pp_printf("%s:%d structure len not correct %d != %d\n",
				  __func__, __LINE__,
				  _app->structures[index].len, size);
		}
#endif
		offset += (size / 4); /* Next TLV record */
	}

	/* Return back new values. Host can compare with what it sent
	   to spot errors */
	if (hin->flags & WRNC_PROTO_FLAG_SYNC)
		return rt_structure_getter(hin, pin, hout, pout);

	return 0;
}


/**
 * it copies a structure to the host
 */
int rt_structure_getter(struct wrnc_proto_header *hin, void *pin,
			struct wrnc_proto_header *hout, void *pout)
{
	unsigned int i, offset = 0, index, size;
	uint32_t *din = pin;
	uint32_t *dout = pout;

	hout->msg_id = RT_ACTION_SEND_STRUCT_GET;

	while (offset < hin->len) {
#ifdef LIBRT_DEBUG
		pp_printf("%s: offset %d/%d\n", __func__, offset, hin->len);
#endif
		index = din[offset];
		dout[offset++] = index;
		size = din[offset];
		dout[offset++] = size;
#ifdef LIBRT_DEBUG
		pp_printf("%s Type %d Len %d Addr 0x%x\n", __func__,
			  index, size, _app->structures[index].struct_ptr);
		delay(100000);
#endif
		if (_app->structures[index].len == size) {
			memcpy(&dout[offset], _app->structures[index].struct_ptr,
			       size);
		}
#ifdef LIBRT_ERROR
		else {
			pp_printf("%s: structure len not correct %d != %d\n",
				  __func__, _app->structures[index].len, size);
		}
#endif
		offset += (size / 4); /* Next TLV record */
	}

	return 0;
}


/**
 * Get the version. It is a structure, but a special one, so it is not using
 * the generic function
 */
int rt_version_getter(struct wrnc_proto_header *hin, void *pin,
		      struct wrnc_proto_header *hout, void *pout)
{
	uint32_t *dout = pout;

	hout->msg_id = RT_ACTION_SEND_VERSION;
	hout->len = sizeof(struct wrnc_rt_version) / 4;
	memcpy(dout, &version, sizeof(struct wrnc_rt_version));

	return 0;
}


/**
 * This is a generic setter that an external system can invoke
 * to set a set of variable values.
 * We use directly pointers and not an index
 */
int rt_variable_setter(struct wrnc_proto_header *hin, void *pin,
		       struct wrnc_proto_header *hout, void *pout)
{
	struct rt_variable *var;
	uint32_t *din = pin, *mem, val;
	int i;

	/* we always have a pair of values  */
	if (hin->len % 2)
		rt_send_nack(hin, pin, hout, pout);

	/* Write all values in the proper place */
	for (i = 0; i < hin->len; i += 2) {
		if (din[i] >= _app->n_variables)
			continue;
		var = &_app->variables[din[i]];
		mem = (uint32_t *) var->addr;
		val = ((din[i + 1] & var->mask) << var->offset);
		if (var->flags & RT_VARIABLE_FLAG_WO)
			*mem = val;
		else
			*mem = (*mem & ~var->mask) | val;

#ifdef LIBRT_DEBUG
		pp_printf("%s index %d/%d | [0x%x] = 0x%08x <- 0x%08x (0x%08x) | index in msg (%d/%d)\n",
			  __func__,
			  din[i], _app->n_variables,
			  mem, *mem, val, din[i + 1],
			  i + 1, hin->len - 1);
		delay(100000);
#endif
	}

	/* Return back new values. Host can compare with what it sent
	   to spot errors */
	if (hin->flags & WRNC_PROTO_FLAG_SYNC)
		return rt_variable_getter(hin, pin, hout, pout);

	return 0;
}


/**
 * This is a generic getter that an external system can invoke
 * to get a set of variable values
 */
int rt_variable_getter(struct wrnc_proto_header *hin, void *pin,
		       struct wrnc_proto_header *hout, void *pout)
{
	struct rt_variable *var;
	uint32_t *dout = pout, *din = pin, *mem, val;
	int i;

	if (!hout || !pout)
		return -1;

	/* we always have a pair of values  */
	if (hin->len % 2)
		return -1;

	hout->msg_id = RT_ACTION_SEND_FIELD_GET;

	/* Write all values in the proper place */
	for (i = 0; i < hout->len; i += 2) {
		if (din[i] >= _app->n_variables) {
			dout[i] = ~0; /* Report invalid index */
			continue;
		}
		dout[i] = din[i];
		var = &_app->variables[dout[i]];
		mem = (uint32_t *) var->addr;
		val = (*mem >> var->offset) & var->mask;
		dout[i + 1] = val;
#ifdef LIBRT_DEBUG
		pp_printf("%s index %d/%d | [0x%x] = 0x%08x -> 0x%08x | index in msg (%d/%d)\n",
			  __func__,
			  dout[i], _app->n_variables,
			  mem, *mem,  dout[i + 1],
			  i + 1, hin->len - 1);
		delay(100000);
#endif

	}

	return 0;
}


/**
 * This is a default action that answer on ping messages
 */
int rt_recv_ping(struct wrnc_proto_header *hin, void *pin,
		 struct wrnc_proto_header *hout, void *pout)
{
	rt_send_ack(hin, pin, hout, pout);
	return 0;
}


/**
 * It runs the action associated with the given identifier
 * @param[in] id action identifier
 * @param[in] msg input message for the action
 * @return 0 on success. A negative value on error
 */
static inline int rt_action_run(struct wrnc_proto_header *hin, void *pin)
{
	action_t *action;
	struct wrnc_msg out_buf;
	struct wrnc_proto_header hout;
	void *pout;
	int err = 0;

	if (hin->msg_id >= _app->n_actions || !_app->actions[hin->msg_id]) {
		pp_printf("Cannot dispatch ID 0x%x\n", hin->msg_id);
		return -EINVAL;
	}

	action = _app->actions[hin->msg_id];

	if (!(hin->flags & WRNC_PROTO_FLAG_SYNC)) {
		/* Asynchronous message, then no output */
		return action(hin, pin, NULL, NULL);
	}

#ifdef LIBRT_DEBUG
	pp_printf("Message Input\n");
	rt_print_header(hin);
	rt_print_data(pin, 8);
#endif
	/* Synchronous message */
	out_buf = rt_mq_claim_out(hin);
	pout = rt_proto_payload_get((void *) out_buf.data);
	memcpy(&hout, hin, sizeof(struct wrnc_proto_header));

	err = action(hin, pin, &hout, pout);
	if (err)
		rt_send_nack(hin, pin, &hout, NULL);
	rt_proto_header_set((void *) out_buf.data, &hout);

#ifdef LIBRT_DEBUG
	pp_printf("Message Output\n");
	rt_print_header(&hout);
	rt_print_data(pout, 8);
#endif
	mq_msg_send(&hout);

	return err;
}


/**
 * It dispatch messages coming from a given message queue.
 * @param[in] mq_in message queue index within mq declared in rt_application
 * @todo provide support for remote queue
 */
int rt_mq_action_dispatch(unsigned int mq_in)
{
#ifdef RTPERFORMANCE
	uint32_t sec, cyc, sec_n, cyc_n;
#endif
	struct wrnc_proto_header *header;
	unsigned int mq_in_slot = _app->mq[mq_in].index;
	unsigned int is_remote = _app->mq[mq_in].flags & RT_MQ_FLAGS_REMOTE;
	struct rt_action *action;
	uint32_t *msg;
	void *pin;
	int err = 0;

	/* HMQ control slot empty? */
	if (!(mq_poll() & ( 1 << mq_in_slot)))
		return -EAGAIN;

	/* Get the message from the HMQ */
	msg = mq_map_in_buffer(0, mq_in_slot);
#ifdef LIBRT_DEBUG
	pp_printf("Incoming message\n");
	rt_print_data(msg, 8);
#endif
	header = rt_proto_header_get((void *) msg);
	pin = rt_proto_payload_get((void *) msg);

	if (header->rt_app_id && header->rt_app_id != version.rt_id) {
		pp_printf("Not for this application 0x%x\n", header->rt_app_id);
		err = -EINVAL;
		goto out;
	}

#ifdef RTPERFORMANCE
	rt_get_time(&sec, &cyc);
#endif
	/* Run the correspondent action */
	err = rt_action_run(header, pin);
	if (err)
		pp_printf("%s: action failure err: %d\n", __func__, err);

#ifdef RTPERFORMANCE
	rt_get_time(&sec_n, &cyc_n);
	pp_printf("%s: time %d", __func__, (cyc_n - cyc) * 8);
#endif

out:
	mq_discard(0, mq_in_slot);
	return err;
}




/**
 * It get the current time from the internal WRNC timer
 * @param[out] seconds
 * @param[out] cycles
 */
void rt_get_time(uint32_t *seconds, uint32_t *cycles)
{
	*seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
	*cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
}



/**
 * Initialize the rt library for an optimal usage
 */
void rt_init(struct rt_application *app)
{
	int i;

	_app = app;

	pp_printf("Running application '%s'\n", name);
	if (version.fpga_id) {
		pp_printf("  compatible only with FPGA '0x%x'\n",
			  version.fpga_id);
		/* TODO get app id from FPGA and compare */
	}
	pp_printf("  application id            '0x%x'\n", version.rt_id);
	pp_printf("  application version       '%d'\n", version.rt_version);
	pp_printf("  source code id            '0x%x'\n", version.git_version);
	/* Purge all slots */
	for (i = 0; i < _app->n_mq; ++i) {
		mq_writel(!!(_app->mq[i].flags & RT_MQ_FLAGS_REMOTE),
			  MQ_CMD_PURGE,
			  MQ_OUT(_app->mq[i].index) + MQ_SLOT_COMMAND);
	}
}
