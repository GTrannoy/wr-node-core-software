#ifndef __LIST_LIB_H
#define __LIST_LIB_H

#include <stdio.h>
#include <stdlib.h>

#include "rt/common/rt-common.h"

struct list_node;

struct list_node* list_open_node_by_lun(int lun);
void list_close_node ( struct list_node *n );

int list_in_enable(struct list_node *dev, int input, int enable);
int list_in_assign_trigger ( struct list_node *, int input, struct list_id *trig );
int list_in_set_trigger_mode ( struct list_node *, int input, int mode );
int list_in_arm ( struct list_node *, int input, int armed );
int list_in_check_triggered ( struct list_node *, int input );
int list_in_set_mode ( struct list_node *, int input, int mode );
int list_in_set_port_id ( struct list_node *, int input, uint32_t id);
int list_in_wait_trigger ( struct list_node*, int input_mask, struct list_id *id );
int list_in_read_raw ( struct list_node*, int input_mask, struct list_timestamp *ts, int count );
int list_in_read_log ( struct list_node*, int input_mask, struct list_input_log_entry *ts, int count );
int list_in_software_trigger ( struct list_node*, struct list_trigger_entry *trigger );
int list_in_set_delay (struct list_node *, int input, uint64_t delay_ps);
int list_in_get_state ( struct list_node *, int input, struct tdc_channel_state *state );
int list_in_reset_seq_counter (struct list_node *dev, int input);
int list_in_set_dead_time ( struct list_node *dev, int input, uint64_t dead_time_ps );

#endif 
