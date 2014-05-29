#include <stdarg.h>
#include <errno.h>

#include "wrn-lib.h"
#include "list-lib.h"


#define TDC_APPLICATION "rt/tdc/rt-tdc.bin"
#define FD_APPLICATION "rt/tdc/rt-fd.bin"

#define WRN_APPID_LIST_TDC_FD 0x115790de
#define WRN_APPID_LIST_DUAL_FD 0x115790df

struct list_node {
	uint32_t appid;

	int n_inputs;
	int n_outputs;

	int fd_cpu_control[2];

	struct wrn_dev *wrn;
};

static int rt_request ( struct list_node *node, int cpu, int command, uint32_t *req, int req_size, uint32_t *rsp )
{
	uint32_t buf[128];

	buf[0] = command;
	buf[1] = 0;
	if(req_size)
		memcpy(buf + 2, req, req_size * 4);

	wrn_send ( node->wrn, node->fd_cpu_control[cpu], buf, req_size + 2, 0);
	int n = wrn_recv ( node->wrn, node->fd_cpu_control[cpu], buf, 128, 0);
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


struct list_node* list_open_node_by_lun(int lun)
{
	if ( wrn_lib_init() < 0)
		return NULL;

	struct wrn_dev *wrn = wrn_open_by_lun( lun );

	printf("open-node : %d %p ",lun, wrn);
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
			n->n_inputs = 5;
			n->n_outputs = 4;			
			wrn_cpu_stop(wrn, 0xff);
			wrn_cpu_load_file(wrn, 0, TDC_APPLICATION);
			wrn_cpu_start(wrn, 0x1);

			n->fd_cpu_control[0] =  wrn_open_slot ( wrn, 0, WRN_SLOT_OUTGOING | WRN_SLOT_INCOMING );
			
			printf("open_slot: cpu 0 control fd %d\n", n->fd_cpu_control[0]);

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
	return rt_requestf(dev, 0, ID_TDC_CMD_CHAN_ASSIGN_TRIGGER, 4, input - 1, trig->system, trig->source_port, trig->trigger);
}

int list_in_set_trigger_mode ( struct list_node *dev, int input, int mode )
{
	return rt_requestf(dev, 0, ID_TDC_CMD_CHAN_SET_MODE, 2, input - 1 , mode );
}

int list_in_arm ( struct list_node *dev, int input, int armed )
{
	return rt_requestf(dev, 0, ID_TDC_CMD_CHAN_ARM, 2, input - 1, armed ? 1 : 0 );		
}
