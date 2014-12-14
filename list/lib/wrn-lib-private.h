#ifndef __WRN_LIB_PRIVATE_H
#define __WRN_LIB_PRIVATE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <pthread.h>

#include "fmc-lib.h"

#define MAX_MQUEUE_SLOTS 16

#include <vector>
#include <deque>
#include <map>

typedef std::vector<uint32_t> wrn_message;

struct wrn_mqueue_slot {
    int size;
    int width;
};

struct wrn_mqueue {
    int n_out;
    int n_in;
    std::vector<wrn_mqueue_slot> in;
    std::vector<wrn_mqueue_slot> out;
};

struct wrn_buffer {
    int slot;
    int total;

    wrn_buffer(int slot);
    ~wrn_buffer();

    bool lock(bool blocking);
    void unlock();

    bool process ( const wrn_message& msg )
    {
        msgs.push_back(msg);
    }
    bool push ( const wrn_message& msg)
    {
        msgs.push_back(msg);
    }
    bool empty()
    {
        return msgs.empty();
    }
    wrn_message pop()
    {
        wrn_message rv = msgs.back();
        msgs.pop_back();
        return rv;
    }

    int count()
    {
        return msgs.size();
    }

    pthread_mutex_t mutex;
    std::deque<wrn_message> msgs;
};

struct wrn_connection {
    wrn_buffer *in;
    wrn_buffer *out;
    int flags;
    int fd;
};

typedef std::map<int, wrn_connection *> wrn_connmap;

struct wrn_dev {
    int lun;
    const char *path;
    uint32_t base;
    uint32_t app_id;
    int cpu_count;
    struct wrn_mqueue hmq;
    struct fmc_dev *fmc;
    std::map<int, wrn_connection *> fds;
    pthread_t update_thread;
};

#endif
