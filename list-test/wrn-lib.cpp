#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include <pthread.h>

#include <arpa/inet.h>

#include "fmc-lib.h"

#include "hw/wrn_cpu_csr.h"
#include "hw/mqueue.h"

#include "wrn-lib.h"
#include "wrn-lib-private.h"

#define MAX_MQUEUE_SLOTS 16

#define BASE_HMQ 	0x00000
#define BASE_CPU_CSR 	0x10000

static int fd_base = 1;

using namespace std;

void *update_thread_entry( void * );

int wrn_lib_init()
{
    return 0;
}

void wrn_lib_exit()
{
    
}

int wrn_get_node_count()
{
    return 1;
}

static uint32_t wrn_readl(struct wrn_dev *dev, uint32_t addr)
{
    return dev->fmc->readl(dev->fmc, dev->base + addr);
}

static void wrn_writel(struct wrn_dev *dev, uint32_t data, uint32_t addr)
{
    return dev->fmc->writel(dev->fmc, data, dev->base + addr);
}

#define HMQ_OUT(x) (0x8000 + 0x400 *(x))
#define HMQ_IN(x) (0x4000 + 0x400 *(x))
#define HMQ_GCR 0

static uint32_t hmq_readl(struct wrn_dev *dev, uint32_t what, uint32_t offset)
{
    return wrn_readl(dev, BASE_HMQ + what + offset);
}

static void hmq_writel(struct wrn_dev *dev, uint32_t what, uint32_t data, uint32_t offset)
{
    wrn_writel(dev, data, BASE_HMQ + what + offset);
}

static int init_hmq( struct wrn_dev *dev )
{
    uint32_t slot_count, slot_status;
    int i, entries, width;

    slot_count = hmq_readl (dev, HMQ_GCR, MQUEUE_GCR_SLOT_COUNT);

    int n_in = slot_count & 0xff;
    int n_out = (slot_count >> 8) & 0xff;

    dev->hmq.n_in = n_in;
    dev->hmq.n_out = n_out;

    printf("init_hmq: CPU->Host (outgoing) slots: %d, Host->CPU (incoming) slots: %d\n", n_out, n_in);

    for(i=0 ; i<n_out; i++)
    {
	slot_status = hmq_readl (dev, HMQ_OUT(i), MQUEUE_SLOT_STATUS);
        width = 1 << (( slot_status >> 28) & 0xf);
        entries = 1 << (( slot_status >> 2) & 0x3f);
        hmq_writel(dev, HMQ_OUT(i), MQUEUE_CMD_PURGE, MQUEUE_SLOT_COMMAND);
        printf(" - out%d: width=%d, entries=%d\n", i, width, entries);
    }
    for(i =0 ; i<n_in; i++)
    {
        slot_status =hmq_readl(dev, HMQ_IN(i), MQUEUE_SLOT_STATUS);
        width = 1 << (( slot_status >> 28) & 0xf);
        entries = 1 << (( slot_status >> 2) & 0x3f);
        hmq_writel(dev, HMQ_IN(i), MQUEUE_CMD_PURGE, MQUEUE_SLOT_COMMAND);
	   printf(" - in%d: width=%d, entries=%d\n", i, width, entries);
    }
    return 0;
}

int init_cpus(struct wrn_dev *dev)
{
 //   wrn_writel(dev, 0xffffffff, BASE_CPU_CSR + WRN_CPU_CSR_REG_RESET);

    dev->cpu_count = wrn_readl(dev, BASE_CPU_CSR + WRN_CPU_CSR_REG_CORE_COUNT) & 0xf;
    //printf("init_cpus: %d CPU CBs\n", dev->cpu_count);
    int i;
    for(i=0;i<dev->cpu_count;i++)
    {
         uint32_t memsize;
         wrn_writel(dev, i, BASE_CPU_CSR + WRN_CPU_CSR_REG_CORE_SEL);
         memsize = wrn_readl(dev, BASE_CPU_CSR + WRN_CPU_CSR_REG_CORE_MEMSIZE);
         //printf("- core %d: %d kB private memory\n", i, memsize/1024);
    }
    return 0;
}

static void start_update_thread ( wrn_dev *dev )
{
    pthread_create( &dev->update_thread, NULL, update_thread_entry, (void *) dev );
}

struct wrn_dev* wrn_open_by_lun( int lun )
{
    struct wrn_dev * dev= new wrn_dev();
    
    dev->base = 0xc0000;
    dev->fmc = fmc_svec_create(lun);
    
    dev->app_id = wrn_readl( dev, BASE_CPU_CSR + WRN_CPU_CSR_REG_APP_ID );
    printf("wrn_open: application ID = 0x%08x\n", dev->app_id);

    init_cpus( dev );
    init_hmq ( dev );
    
    start_update_thread( dev );


    return dev;
}


uint32_t wrn_get_app_id( struct wrn_dev *dev )
{
    return dev->app_id;
}

int wrn_cpu_count( struct wrn_dev*  dev)
{
    return dev->cpu_count;
}

int wrn_cpu_stop ( struct wrn_dev* dev, uint32_t mask )
{
    uint32_t r = wrn_readl(dev, BASE_CPU_CSR + WRN_CPU_CSR_REG_RESET );
    //printf("cpu_stop: r %x mask %x csrbase %x\n", r, mask,  BASE_CPU_CSR +WRN_CPU_CSR_REG_RESET);
    r |= mask;
    //printf("cpu_stop: r %x \n", r);
    wrn_writel(dev, r, BASE_CPU_CSR + WRN_CPU_CSR_REG_RESET );
    return 0;
}

int wrn_cpu_start ( struct wrn_dev * dev, uint32_t mask )
{
    uint32_t r = wrn_readl(dev, BASE_CPU_CSR + WRN_CPU_CSR_REG_RESET );
    //printf("cpu_stop: r %x mask %x csrbase %x\n", r, mask,  BASE_CPU_CSR +WRN_CPU_CSR_REG_RESET);
    r &= ~mask;
    //printf("cpu_start: r %x \n", r);
    wrn_writel(dev, r, BASE_CPU_CSR + WRN_CPU_CSR_REG_RESET );
    return 0;
}

int wrn_cpu_reset ( struct wrn_dev *dev, uint32_t mask )
{
    wrn_cpu_stop(dev, mask);
    wrn_cpu_start(dev, mask);
}

int wrn_cpu_load_application ( struct wrn_dev *dev, int cpu, const void *code, size_t code_size )
{
    int i;
   
   wrn_cpu_stop(dev, (1<<cpu));

    wrn_writel(dev, cpu, BASE_CPU_CSR + WRN_CPU_CSR_REG_CORE_SEL );

    printf("CPU%d: loading %d bytes of LM32 code\n", cpu, code_size);

    uint32_t *code_c = (uint32_t *)code;


    for(i = 0; i < 8192; i++) // fixme: memsize
    {
        wrn_writel(dev, i,    BASE_CPU_CSR + WRN_CPU_CSR_REG_UADDR);
        wrn_writel(dev, 0, BASE_CPU_CSR + WRN_CPU_CSR_REG_UDATA);
    }
    for(i = 0; i < (code_size + 3) / 4; i ++)
    {
    	uint32_t word = htonl( code_c[i] );
    	wrn_writel(dev, i,    BASE_CPU_CSR + WRN_CPU_CSR_REG_UADDR);
    	wrn_writel(dev, word, BASE_CPU_CSR + WRN_CPU_CSR_REG_UDATA);
    }

    int errors = 0;

    for(i = 0; i < (code_size + 3) / 4; i ++)
    {
        uint32_t word = htonl( code_c[i] ), word_rdbk;
        wrn_writel(dev, i,    BASE_CPU_CSR + WRN_CPU_CSR_REG_UADDR);
        word_rdbk = wrn_readl(dev, BASE_CPU_CSR + WRN_CPU_CSR_REG_UDATA) ;
        if(word_rdbk != word)
        {
            printf("verify error: addr %x %x != %x\n", i, word_rdbk, word);
            errors++;
        }
    }

    if(errors)
    {
        printf("Encountered %d verification errors: errors\n");
        exit(-1);
    }
//    wrn_cpu_start(dev, (1<<cpu));
}

static void *load_binary ( const char *filename, int *size )
{
    FILE *f=fopen(filename,"rb");
    if(!f)
	return NULL;
    fseek(f,0,SEEK_END);
    int s = ftell(f);
    rewind(f);
    char *buf = new char [s];
    if(!buf)
    {
	fclose(f);
	return NULL;
    }
    fread(buf,1,s,f);
    *size=s;
    fclose(f);
    return buf;
}


int wrn_cpu_load_file ( struct wrn_dev *dev, int cpu, const char *filename) 
{
    int size;
    void *p = load_binary(filename,&size);

    if(!p)
	return -1;

    int rv = wrn_cpu_load_application(dev, cpu, p, size);
    free(p);
    return rv;
}


#define SLOT_OUT_MASK 0xffff

static bool do_rx(struct wrn_dev *dev, wrn_message& msg, int& slot)
{
    uint32_t in_stat = hmq_readl(dev, HMQ_GCR, MQUEUE_GCR_SLOT_STATUS);
    //uint32_t cnt = hmq_readl(dev, HMQ_GCR, MQUEUE_GCR_SLOT_COUNT);


    /*printf("gcr %x\n", in_stat);
    if(in_stat & 1)
    {
        printf("Got!\n");
    }*/
    if(in_stat & SLOT_OUT_MASK)
    {
    	int i, j;
    	for(i = 0; i < dev->hmq.n_out; i++)
    	{
    	    if (in_stat & (1<<i))
    	    {
        		uint32_t slot_stat = hmq_readl (dev, HMQ_OUT(i), MQUEUE_SLOT_STATUS );
        		int size = (slot_stat >> 16) & 0xff;
                msg.clear();

      

        		for(j=0;j<size;j++)
                {
                    uint32_t rv = hmq_readl(dev, HMQ_OUT(i), MQUEUE_SLOT_DATA_START + j * 4);
                    msg.push_back(rv);
                    //printf("rv %x %x %c\n", MQUEUE_SLOT_DATA_START + j * 4, rv, (char) rv );
                }
        		slot = i;
                hmq_writel(dev, HMQ_OUT(i), MQUEUE_CMD_DISCARD, MQUEUE_SLOT_COMMAND);

                if (msg[0] == 0xdeadbeef)
                {
                    char str[128];
                    for(j=1;j<msg.size();j++)
                        str[j-1] = msg[j];
                    str[j-1] = 0;
                    fprintf(stderr,"* DBG%d: %s\n", slot, str );
                    return false;
                }
                return true;
    	    }
    	}	
    }

    return false;
}

//struct wrn_dev* wrn_open_by_fmc ( int device_id );
//struct wrn_dev* wrn_open( const char *device );

bool tx_ready(struct wrn_dev *dev, int slot)
{
    uint32_t slot_stat = hmq_readl (dev, HMQ_IN(slot), MQUEUE_SLOT_STATUS );
  
    return !(slot_stat & MQUEUE_SLOT_STATUS_FULL);
}

void do_tx(struct wrn_dev *dev, const wrn_message& msg, int slot )
{
    //printf("do_tx: slot %d size %d\n", slot, msg.size());

    hmq_writel(dev, HMQ_IN(slot),  MQUEUE_CMD_CLAIM, MQUEUE_SLOT_COMMAND);
    for(int i=0;i<msg.size();i++)
    {
        //printf("%d: %x\n", i, msg[i]);
        hmq_writel(dev, HMQ_IN(slot), msg[i], MQUEUE_SLOT_DATA_START + i * 4);
    }
    hmq_writel(dev, HMQ_IN(slot), MQUEUE_CMD_READY, MQUEUE_SLOT_COMMAND);
}



int update_mqueues( struct wrn_dev * dev )
{
    wrn_message msg;
    int slot;
    int rx_limit = 5;
    int got_sth = 0;
    // outgoing path (from CPUs)
    while(do_rx (dev, msg, slot) && rx_limit)
    {
    //    printf("RxLim %d\n", rx_limit);
        rx_limit--;
        got_sth = 1;
        //printf("slot %d rx %d words\n", slot, msg.size());
        for (wrn_connmap::iterator i = dev->fds.begin(); i != dev->fds.end(); ++i)
        {
            wrn_buffer *buf = i->second->out;

            if( buf  && buf->slot == slot)
            {
          //      printf("-> queue to fd %d\n", i->first);
                buf->lock( true );
                //printf("locked\n");
                buf->process(msg);
                buf->unlock();

            }
        }
    }

    // incoming path (to CPUs)
    for (wrn_connmap::iterator i = dev->fds.begin(); i != dev->fds.end(); ++i)
    {
        wrn_buffer *buf = i->second->in;
        if( buf && !buf->empty() )
        {
            if(tx_ready(dev, buf->slot))
            {
                buf->lock(true);
                got_sth = 1;
//                printf("DoTX\n");
                do_tx(dev, buf->pop(), buf->slot);
                buf->unlock();
            }

        }
    }
    return got_sth;

}

wrn_buffer::wrn_buffer (int slot_)
{
    slot = slot_;
    pthread_mutex_init(&mutex, NULL);
}


bool wrn_buffer::lock(bool blocking)
{
    if (blocking)
    {
        pthread_mutex_lock(&mutex);
        return true;
    } else {
        return pthread_mutex_trylock(&mutex) ? false : true;
    }
}

void wrn_buffer::unlock()
{
    pthread_mutex_unlock(&mutex);
}

wrn_buffer::~wrn_buffer()
{
    pthread_mutex_destroy(&mutex);
}


 

int wrn_open_slot ( struct wrn_dev *dev, int slot, int flags )
{
    int fd = fd_base;
    fd_base++;
    wrn_connection *conn = new wrn_connection();
    conn->flags = flags;

    conn-> in = NULL;
    conn->out = NULL;
    if(flags & WRN_SLOT_OUTGOING)
        conn->out = new wrn_buffer(slot);
    
    if(flags & WRN_SLOT_INCOMING)
        conn->in = new wrn_buffer(slot);
    
    dev->fds[fd] = conn;
    printf("open slot %d, fd = %d\n", slot, fd);
    return fd;
}

void wrn_close_slot ( struct wrn_dev *dev, int fd )
{
    wrn_connection *conn = dev->fds[ fd ];
    if(conn)
        delete conn;

    dev->fds.erase(fd);
}

int64_t get_tics()
{
    struct timezone tz = {0,0};
    struct timeval tv;
    return tv.tv_sec * 1000000LL + tv.tv_usec;
}

int wrn_recv ( struct wrn_dev *dev, int fd, uint32_t *buffer, size_t buf_size, int timeout_us )
{
    wrn_connection *conn = dev->fds[ fd ];

    int64_t t_start = get_tics();

    if(!timeout_us && conn->out->empty())
        return 0;

    while(conn->out->empty())
    {

        if(timeout_us >= 0 && (get_tics() - t_start) >= timeout_us )
            return 0;

        //fprintf(stderr,"w");
        usleep(1);

    }
    
    conn->out->lock(true);    
    wrn_message msg = conn->out->pop();
    conn->out->unlock();
    
    int n = std::min(buf_size, msg.size());
    for(int i = 0; i<n; i++)
        buffer[i] = msg[i];

    return n;
}

int wrn_send ( struct wrn_dev *dev, int fd, uint32_t *buffer, size_t buf_size, int timeout_us )
{
    wrn_connection *conn = dev->fds[ fd ];

    wrn_message msg;


    for(int i = 0; i < buf_size; i++)
        msg.push_back(buffer[i]);

    conn->in->lock(true);    
    conn->in->push( msg );
    conn->in->unlock();
    return buf_size;
}



void *update_thread_entry( void *data )
{
    wrn_dev *dev = (wrn_dev *) data;

//    fprintf(stderr,"readout thread started\n");
    for(;;)
    {
        if( !update_mqueues(dev) );
            usleep(1000);
    }
}
