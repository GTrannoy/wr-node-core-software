#include <stdarg.h>
#include <errno.h>

#include "wrn-lib.h"
#include "list-lib.h"


#define TDC_APPLICATION "rt/tdc/rt-tdc.bin"
#define FD_APPLICATION "rt/fd/rt-fd.bin"

#define WRN_APPID_LIST_TDC_FD 0x115790de
#define WRN_APPID_LIST_DUAL_FD 0x115790df

struct list_node {
	uint32_t appid;

	int n_inputs;
	int n_outputs;

	int fd_cpu_control[2];
	int fd_cpu_logging[2];

	struct wrn_dev *wrn;
};


struct list_timestamp picos_to_ts( uint64_t p )
{
	struct list_timestamp t;
	t.seconds = p / (1000ULL * 1000ULL * 1000ULL * 1000ULL);
	p %= (1000ULL * 1000ULL * 1000ULL * 1000ULL);
	t.cycles = p / 8000;
	p %= 8000;
	t.frac = p * 4096 / 8000;
	return t;
}

static int rt_request ( struct list_node *node, int cpu, int command, uint32_t *req, int req_size, uint32_t *rsp )
{
	uint32_t buf[128];

	buf[0] = command;
	buf[1] = 0;
	if(req_size)
		memcpy(buf + 2, req, req_size * 4);

	wrn_send ( node->wrn, node->fd_cpu_control[cpu], buf, req_size + 2, 0);
	int n = wrn_recv ( node->wrn, node->fd_cpu_control[cpu], buf, 128, -1);
	if(rsp)
		memcpy(rsp, buf, n * 4);
	return n;
}

static int rt_requestf ( struct list_node *node, int cpu, int command, int n_args, ... )
{
	va_list ap;
	va_start( ap, n_args);
	uint32_t req[128], rsp[128];
	int i;

	for(i = 0; i < n_args; i++ )
		req[i] = va_arg(ap, uint32_t);

	va_end(ap);

	int n = rt_request (node, cpu, command, req, n_args, rsp);

	if(n == 2 && rsp[0] == ID_REP_ACK)
		return 0;
	else
		return -1;
}

#if 0
static int check_node_running( struct list_node *dev )
{
	uint32_t rsp[2];

	if ( cpu_cmd( n, 0, ID_TDC_CMD_PING, NULL, 0, rsp) == 2);
	

	return rsp[0] == ID_TDC	
}

#endif


// private function used by list-init, move to the driver
void list_boot_node(struct list_node *dev)
{
	wrn_cpu_stop(dev->wrn, 0xff);
	wrn_cpu_load_file(dev->wrn, 0, TDC_APPLICATION);
	wrn_cpu_load_file(dev->wrn, 1, FD_APPLICATION);
	wrn_cpu_start(dev->wrn, 0x3);
}

struct list_node* list_open_node_by_lun(int lun)
{
	if ( wrn_lib_init() < 0)
		return NULL;

	struct wrn_dev *wrn = wrn_open_by_lun( lun );

	if(!wrn)
		return NULL;

	uint32_t app_id = wrn_get_app_id(wrn);

	struct list_node *n = malloc(sizeof(struct list_node));

	n->wrn = wrn;

	switch(app_id)
	{
		case WRN_APPID_LIST_DUAL_FD:
			break;
		case WRN_APPID_LIST_TDC_FD:
			printf("Found TDC-FD node\n");
			n->n_inputs = 5;
			n->n_outputs = 4;			
			n->fd_cpu_control[0] =  wrn_open_slot ( wrn, 0, WRN_SLOT_OUTGOING | WRN_SLOT_INCOMING );
			n->fd_cpu_control[1] =  wrn_open_slot ( wrn, 2, WRN_SLOT_OUTGOING | WRN_SLOT_INCOMING );
			n->fd_cpu_logging[0] =  wrn_open_slot ( wrn, 1, WRN_SLOT_OUTGOING );
			n->fd_cpu_logging[1] =  wrn_open_slot ( wrn, 3, WRN_SLOT_OUTGOING );
			
			break;
	}

	return n;
}


int list_in_enable(struct list_node *dev, int input, int enable)
{
	if(input < 1 || input > 5)
		return -EINVAL;
	
	return rt_requestf (dev, 0, ID_TDC_CMD_CHAN_ENABLE, 2, input - 1, enable ? 1 : 0 );
}

int list_in_set_dead_time ( struct list_node *dev, int input, uint64_t dead_time_ps )
{
	int dead_time_cycles = dead_time_ps / 8000;

	if(dead_time_cycles < 10000 || dead_time_cycles > 10000000 )
		return -EINVAL;

	if(input < 1 || input > 5)
		return -EINVAL;

	return rt_requestf(dev, 0, ID_TDC_CMD_CHAN_SET_DEAD_TIME, 2, input-1, dead_time_cycles);
}

int list_in_software_trigger ( struct list_node* dev, struct list_trigger_entry *trigger )
{
	uint32_t rsp[128];

	int n = rt_request(dev, 0, ID_TDC_CMD_SOFTWARE_TRIGGER, (uint32_t *) trigger, sizeof(struct list_trigger_entry) / 4, rsp);
	
	if(n == 2 && rsp[0] == ID_REP_ACK)
		return 0;
	else
		return -1;
}

int list_in_assign_trigger ( struct list_node *dev, int input, struct list_id *trig )
{
	if(trig == NULL) // un-assign
		return rt_requestf(dev, 0, ID_TDC_CMD_CHAN_ASSIGN_TRIGGER, 5,  input - 1, 0, 0, 0, 0);	
	else
		return rt_requestf(dev, 0, ID_TDC_CMD_CHAN_ASSIGN_TRIGGER, 5,  input - 1, 1, trig->system, trig->source_port, trig->trigger);
}

int list_in_set_trigger_mode ( struct list_node *dev, int input, int mode )
{
	return rt_requestf(dev, 0, ID_TDC_CMD_CHAN_SET_MODE, 2, input - 1 , mode );
}

int list_out_enable ( struct list_node *dev, int output, int enable )
{
	return rt_requestf(dev, 1, ID_TDC_CMD_CHAN_SET_MODE, 2, output - 1 , enable );
}

int list_in_arm ( struct list_node *dev, int input, int armed )
{
	return rt_requestf(dev, 0, ID_TDC_CMD_CHAN_ARM, 2, input - 1, armed ? 1 : 0 );		
}

int list_in_set_timebase_offset ( struct list_node *dev, int input, uint64_t offset )
{
	struct list_timestamp t = picos_to_ts( offset );
	return rt_requestf(dev, 0, ID_TDC_CMD_CHAN_SET_TIMEBASE_OFFSET, 4, input - 1, t.seconds, t.cycles, t.frac );		
}

int list_in_get_state ( struct list_node *dev, int input, struct list_input_state *state )
{
	uint32_t rsp[128], req[2];

	req[0] = input - 1;

	int n = rt_request(dev, 0, ID_TDC_CMD_CHAN_GET_STATE, req, 1, rsp);
	
	if(n == 29 && rsp[0] == ID_REP_STATE)
	{
		state->input = input;
		state->flags = rsp[15];
		state->log_level = rsp[16];
		state->mode = rsp[17];
		state->tagged_pulses = rsp[18];
		state->sent_triggers = rsp[19];
		state->dead_time.seconds = 0;
		state->dead_time.frac = 0;
		state->dead_time.cycles = rsp[20];

		state->assigned_id.system = rsp[3];
	    state->assigned_id.source_port = rsp[4];
	    state->assigned_id.trigger = rsp[5];
	    
	    state->delay.seconds = rsp[6];
	    state->delay.cycles = rsp[7];
	    state->delay.frac = rsp[8];

	 	state->last.seconds = rsp[12];
	    state->last.cycles = rsp[13];
	    state->last.frac = rsp[14];

	 	state->last_sent.ts.seconds = rsp[21];
	    state->last_sent.ts.cycles = rsp[22];
	    state->last_sent.ts.frac = rsp[23];
	    state->last_sent.id.system = rsp[24];
	    state->last_sent.id.source_port = rsp[25];
	    state->last_sent.id.trigger = rsp[26];
	    state->last_sent.seq = rsp[27];
	    state->sent_packets = rsp[28];

		return 0;
	} else
		return -1;


}

int list_in_set_delay ( struct list_node *dev, int input, uint64_t delay_ps )
{
	struct list_timestamp t = picos_to_ts(delay_ps);

	return rt_requestf(dev, 0, ID_TDC_CMD_CHAN_SET_DELAY, 4, input - 1, t.seconds, t.cycles, t.frac );		
}

int list_in_set_log_level ( struct list_node *dev, int input, uint32_t log_level)
{
	return rt_requestf(dev, 0, ID_TDC_CMD_CHAN_SET_LOG_LEVEL, 2, input - 1, log_level );		
}

int list_in_read_log ( struct list_node *dev, struct list_log_entry *log, int flags, int input_mask, int count )
{
	int remaining = count;
	int n_read = 0;
	uint32_t buf[128];
	struct list_log_entry *cur = log;

	while(remaining)
	{
		int n = wrn_recv ( dev->wrn, dev->fd_cpu_logging[0], buf, 128, 0);

		cur->type = buf[0];
		cur->channel = buf[2];
		if(n > 0)
		{
			if ((cur->type & flags) && (cur->channel & input_mask))
			{
				cur->seq=buf[1];
    			cur->id.system =buf[3];
    			cur->id.source_port =buf[4];
    			cur->id.trigger =buf[5];
    			cur->ts.seconds =buf[6];
    			cur->ts.cycles =buf[7];
    			cur->ts.frac =buf[8];
    			remaining--;
				n_read++;
				cur++;
			}
		} else 
			break;
	}

	return n_read;


}

