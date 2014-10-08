#ifndef __WRN_LIB_H
#define __WRN_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

struct wrn_dev;

int wrn_lib_init();
void wrn_lib_exit();
int wrn_get_node_count();

struct wrn_dev* wrn_open_by_lun( int lun );
uint32_t wrn_get_app_id( struct wrn_dev *device );

int wrn_cpu_count( struct wrn_dev*  dev);
int wrn_cpu_stop ( struct wrn_dev* dev, uint32_t mask );
int wrn_cpu_start ( struct wrn_dev * dev, uint32_t mask );
int wrn_cpu_reset ( struct wrn_dev *dev, uint32_t mask );
int wrn_cpu_load_application ( struct wrn_dev *dev, int cpu, const void *code, size_t code_size );
int wrn_cpu_load_file ( struct wrn_dev *dev, int cpu, const char *filename);

#define WRN_SLOT_INCOMING (1<<0) // sending messages to the CPUs
#define WRN_SLOT_OUTGOING (1<<1) // receiving messages from the CPUs
#define WRN_SLOT_EXCLUSIVE (1<<2)

int wrn_open_slot ( struct wrn_dev *, int slot, int flags );
void wrn_close_slot ( struct wrn_dev *, int slot );

#define WRN_FILTER_OR 0
#define WRN_FILTER_AND 1
#define WRN_FILTER_COMPARE 2
#define WRN_FILTER_NOT 3

struct wrn_message_filter {
  
    struct rule {
        int op;
        int word_offset;
        uint32_t mask;
        uint32_t value;
        struct rule *next;
    } *rules;

    int n_rules;
};

int wrn_bind ( struct wrn_dev *, int fd, struct wrn_message_filter *flt );
int wrn_wait ( struct wrn_dev *, int fd, int timeout_us );
int wrn_recv ( struct wrn_dev *, int fd, uint32_t *buffer, size_t buf_size, int timeout_us );
int wrn_send ( struct wrn_dev *, int fd, uint32_t *buffer, size_t buf_size, int timeout_us );

//void wrn_update_mqueues( struct wrn_dev * dev );


#ifdef __cplusplus
};
#endif

#endif
