#ifndef __LIST_LIB_H
#define __LIST_LIB_H

#include <stdio.h>
#include <stdint.h>

#include "rt/common/list-common.h"

struct list_trigger_handle {
    uint32_t ptr_cond;
    uint32_t ptr_trig;
    int channel;
};
  
struct list_input_state {
    int input;

    uint32_t flags;           ///> enum list_io_flags
    uint32_t log_level;       ///> enum list_log_level
    int mode;
    
    uint32_t tagged_pulses;
    uint32_t sent_triggers;
    uint32_t sent_packets;

    struct list_trigger_entry last_sent;  
    struct list_id assigned_id;
    struct list_timestamp dead_time;
    struct list_timestamp delay;
    struct list_timestamp last;
};

struct list_trigger_handle handle;

struct list_output_trigger_state {
    int is_conditional;
    int enabled;
    struct list_id trigger;
    struct list_id condition;
    struct list_timestamp delay_trig;
    struct list_timestamp delay_cond;
    struct list_trigger_handle handle;
};

struct list_output_state {
    int output;

    uint32_t executed_pulses;
    uint32_t missed_pulses_late;
    uint32_t missed_pulses_deadtime;
    uint32_t missed_pulses_overflow;

    struct list_trigger_entry last_executed;
    struct list_trigger_entry last_programmed;
    struct list_trigger_entry last_enqueued;

    uint32_t flags;           ///> enum list_io_flags
    uint32_t log_level;       ///> enum list_log_level
    int mode;
    uint32_t dead_time;
    uint32_t pulse_width;
    struct list_timestamp worst_rx_delay;
    uint32_t total_rx_packets;
};

struct list_node;

//! Open a LIST node device.
/*!
  \param lun an integer argument to select the device or negative number to take the first one found.
  \return returns an anonymous list_node structure on success. On error, NULL is returned, and errno is set appropriately.
*/

struct list_node* list_open_node_by_lun(int lun);

//! Close a LIST node device.
/*!
  \param node pointer to open node device.
*/

void list_close_node ( struct list_node *node );


/* 3.3.1a Enable/disable the input. */

//! Hardware enable/disable a LIST trigger input.
/*!
  \param node pointer to open node device.
  \param input index of the trigger input to enable
  \param enable non-0 enables the input, 0 disables it.
*/
int list_in_enable(struct list_node *dev, int input, int enable);
int list_in_is_enabled(struct list_node *dev, int input);


/* 3.3.1b Get/set the Trigger/System/Source Port ID. Change the IDs during operation of the node. */

//! Assign/unassign a trigger ID to a given LIST input.
/*!
  \param node pointer to open node device.
  \param input index of the trigger input to assign trigger to.
  \param trig_id the trigger to be sent upon reception of a pulse on the given input.
  passing NULL un-assigns the current trigger (the input will be tagging pulses and logging them,
  but they will not be sent as triggers to the WR network).
*/
int list_in_assign_trigger ( struct list_node *, int input, struct list_id *trig_id );

/* 3.3.1d Get/set the mode: single shot or continuous. Single shot means the input will trigger on
the first incoming pulse and will ignore the subsequent pulses until re-armed. */

//! Set trigger mode for a given LIST input.
/*!
  \param node pointer to open node device.
  \param input index of the trigger input
  \param mode triggering mode (enum list_trigger_mode). The input must be
         armed by calling list_in_arm() at least once before it can send triggers.
*/
int list_in_set_trigger_mode ( struct list_node *, int input, int mode );

/* 3.3.1f Arm/re-arm the input. */

//! Arm a LIST input for triggering.
/*!
  \param node pointer to open node device.
  \param input index of the trigger input
  \param armed non-0 arms the input, making it ready to accept/send triggers. 0 disarms the input.
*/

int list_in_arm ( struct list_node *, int input, int armed );


/*
3.3.1h Log every trigger pulse sent out to the network. Each log message contains the input
       number, sequence ID, trigger ID, trigger counter (since arm) and origin timestamp.
3.3.1j Log pulses coming to each physical input (within the performance limits).
*/

int list_in_read_log ( struct list_node*, struct list_log_entry *log, int flags, int input_mask, int count );

/* 3.3.1k • Software-trigger the input at a given TAI value. */

int list_in_software_trigger ( struct list_node*, struct list_trigger_entry *trigger );

/* 3.3.1e Get/set the offset (for compensating cable delays), in 10 ps steps. */

int list_in_set_delay ( struct list_node *, int input, uint64_t delay_ps );

/* 3.3.1c Get/set the Sequence ID counter (counting up at every pulse). */

int list_in_set_seq_counter (struct list_node *dev, int input);

/* 3.3.1g Get/set the dead time (the minimum gap between input pulses, below which the TDC
ignores the subsequent pulses; limits maximum input pulse rate, 16 ns granularity) */

int list_in_set_dead_time ( struct list_node *dev, int input, uint64_t dead_time_ps );

int list_in_set_timebase_offset ( struct list_node *dev, int input, uint64_t offset );

/* 3.3.1i • Notify the host system that a trigger has been sent out to the network.
   + all "Get" commands */

int list_in_get_state ( struct list_node *, int input, struct list_input_state *state );

int list_in_reset_counters ( struct list_node *dev, int input );

int list_in_set_log_level ( struct list_node *, int input, uint32_t log_level);


int list_out_enable(struct list_node *dev, int output, int enable);
int list_out_set_dead_time ( struct list_node *dev, int output, uint64_t dead_time_ps );

int list_out_trig_assign ( struct list_node *dev, struct list_trigger_handle *handle, int output, struct list_id *trig, struct list_id *condition );
int list_out_trig_remove (struct list_node *dev, struct list_trigger_handle *handle);
int list_out_trig_get_all (struct list_node *, int output, struct list_output_trigger_state *triggers, int max_count);
int list_out_trig_set_delay ( struct list_node *, struct list_trigger_handle *handle, uint64_t delay_ps );
int list_out_trig_set_condition_delay ( struct list_node *, struct list_trigger_handle *handle, uint64_t delay_ps );
int list_out_trig_get_state ( struct list_node *, struct list_trigger_handle *handle, struct list_output_trigger_state *state );
int list_out_trig_enable ( struct list_node *, struct list_trigger_handle *handle, int enable );

int list_out_read_log ( struct list_node*, struct list_log_entry *log, int flags, int output_mask, int count );

int list_out_set_log_level ( struct list_node *, int output, uint32_t log_level);
int list_out_set_trigger_mode ( struct list_node *, int output, int mode );
int list_out_arm ( struct list_node *node, int input, int armed );
//int list_out_get_state ( struct list_node *node, int input, int armed );

int list_out_reset_counters ( struct list_node *dev, int output );
int list_out_check_triggered ( struct list_node *, int output );
//int list_out_wait_trigger ( struct list_node*, int output_mask, struct list_id *id );

#endif 
