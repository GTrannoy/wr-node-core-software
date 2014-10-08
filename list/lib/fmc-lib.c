#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <getopt.h>
#include <errno.h>

#include "fmc-lib.h"

#include <pthread.h>

struct svec_dev {
    int lun;
    pthread_mutex_t lock;
};


static void svec_lock( struct svec_dev *dev )
{
    pthread_mutex_lock(&dev->lock);
}

static void svec_unlock( struct svec_dev *dev )
{
    pthread_mutex_unlock(&dev->lock);
}

static uint8_t generic_readb(struct fmc_dev *dev, uint64_t addr)
{
    uint32_t rv = dev->readl(dev, addr & ~3);
    return (rv >> (( 3 - addr & 3) * 8 )) & 0xff;
}

static uint16_t generic_readh(struct fmc_dev *dev, uint64_t addr)
{
    uint16_t l = generic_readb(dev, addr + 1);
    uint16_t h = generic_readb(dev, addr + 0);

    return (h << 8) | l;
}

static uint64_t generic_readq(struct fmc_dev *dev, uint64_t addr)
{
    uint64_t l = dev->readl(dev, addr + 4);
    uint64_t h = dev->readl(dev, addr + 0);

    return (h << 32) | l;
}

static uint32_t svec_readl(struct fmc_dev *dev, uint64_t addr)
{
    int addr_fd, data_fd;
    struct svec_dev *svec = (struct svec_dev *) dev->priv;   
    char str[1024];
    svec_lock(svec);
    sprintf(str,"/sys/bus/vme/devices/svec.%d/vme_addr", svec->lun);
    addr_fd = open(str, O_RDWR);
    sprintf(str,"/sys/bus/vme/devices/svec.%d/vme_data", svec->lun);
    data_fd = open(str, O_RDWR);
    if(addr_fd < 0 || data_fd < 0)
    {
	fprintf(stderr, "can't open SVEC at LUN %d: ", svec->lun);
	perror("open()");
	exit(-1);
    }

    sprintf(str,"0x%x\n", addr);
    write(addr_fd, str, strlen(str) + 1);
    int n = read(data_fd, str, sizeof(str));
    uint32_t data;
    close(addr_fd);
    close(data_fd);
    svec_unlock(svec);
    if(n <= 0)
    {
	perror("read()");
	exit(-1);
    }
    str[n] = 0;
    sscanf(str, "%x", &data);
    return data;
}


static void svec_writel(struct fmc_dev *dev, uint32_t data, uint64_t addr)
{
    struct svec_dev *svec = (struct svec_dev *) dev->priv;
    int addr_fd;
    int data_fd;
    char str[256];
    svec_lock(svec);
    sprintf(str,"/sys/bus/vme/devices/svec.%d/vme_addr", svec->lun);
    addr_fd = open(str, O_RDWR);
    sprintf(str,"/sys/bus/vme/devices/svec.%d/vme_data", svec->lun);
    data_fd = open(str, O_RDWR);
    if(addr_fd < 0 || data_fd < 0)
    {
	fprintf(stderr, "can't open SVEC at LUN %d: ", svec->lun);
	perror("open()");
	exit(-1);
    }

    sprintf(str,"0x%x\n", addr);
    write(addr_fd, str, strlen(str) + 1);
    sprintf(str,"0x%x\n", data);
    write(data_fd, str, strlen(str) + 1);
    close(addr_fd);
    close(data_fd);
    svec_unlock(svec);
}

static void *load_binary ( const char *filename, int *size )
{
    FILE *f=fopen(filename,"rb");
    if(!f)
	return NULL;    
    fseek(f,0,SEEK_END);
    int s = ftell(f);
    rewind(f);
    char *buf = malloc(s);
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

static int svec_sysfs_write(struct svec_dev *svec, const char *file, char *buf, int size)
{
    int fd;
    char str[256];
    sprintf(str,"/sys/bus/vme/devices/svec.%d/%s", svec->lun, file);
    fd = open(str, O_RDWR);
    int rv = write(fd, buf, size);
    close(fd);
    return rv;
}

static int svec_reprogram ( struct fmc_dev *dev, const char *filename )
{
    struct svec_dev *svec = (struct svec_dev *) dev->priv;
    int remaining;
    char *buf = load_binary (filename, &remaining );
    const int chunk_size = 2048;
    char *p = buf;

    if(!buf)
	return -1;

    svec_sysfs_write(svec, "firmware_cmd", "0\n", 2);
    while(remaining > 0)
    {
	int n = remaining > chunk_size ? chunk_size : remaining;
	svec_sysfs_write(svec, "firmware_blob", p, n);
	remaining -= n;
	p += n;
    }
    svec_sysfs_write(svec, "firmware_cmd", "1\n", 2);
    free(buf);
    return 0;
}


#define INTERCONNECT 0
#define DEVICE 1
#define BRIDGE 2

static uint32_t sdb_traverse (struct fmc_dev *dev, uint32_t base, uint32_t sdb_addr, uint64_t vendor, uint32_t device)
{
    uint32_t addr = sdb_addr;
    if(dev->readl(dev, addr) != 0x5344422d)
	return -1;

    if(dev->readb(dev, addr + 0x3f) != INTERCONNECT)
	return -1;

    int rec_count = dev->readh(dev, addr + 4);
    int i;

    for (i=0;i<rec_count;i++)
    {
	int t = dev->readb(dev, addr + 0x3f);
	switch(t)
	{
	    case BRIDGE:
	    {
	        uint64_t child_sdb  = dev->readq(dev, addr);
	        uint64_t child_addr = dev->readq(dev, addr + 8);
	        uint32_t rv = sdb_traverse (dev, base + child_addr, base + child_sdb, vendor, device);
	        if(rv != -1)
		    return rv;
		break;
	    }
	    case DEVICE: 
	    {
		uint64_t dev_addr = base + dev->readq(dev, addr + 8);
		uint64_t dev_vendor = dev->readq(dev, addr + 24);
                uint32_t dev_id_sdb  = dev->readl(dev, addr + 32);
		if(dev_vendor == vendor && dev_id_sdb == device)
		    return dev_addr;
		break;
	    }
	}
	
	addr += 0x40;
    }
    return -1;
}

static uint32_t generic_sdb_find_device ( struct fmc_dev * dev, uint64_t vendor, uint32_t dev_id, int ordinal )
{
    return sdb_traverse(dev, 0, 0, vendor, dev_id);
}

struct fmc_dev *fmc_svec_create(int lun)
{
    int fd;
    char str[256];

    sprintf(str,"/sys/bus/vme/devices/svec.%d/vme_addr", lun);
    fd = open(str, O_RDWR);

    if(fd < 0)
	return NULL;

    struct fmc_dev *dev =malloc(sizeof(struct fmc_dev));
    struct svec_dev *svec =malloc(sizeof(struct svec_dev));


    dev->priv =svec;
    dev->readl = svec_readl;
    dev->writel = svec_writel;
    dev->readb = generic_readb;
    dev->readh = generic_readh;
    dev->readq = generic_readq;
    dev->reprogram = svec_reprogram;
    dev->sdb_find_device = generic_sdb_find_device;
    svec->lun = lun;
    pthread_mutex_init(&svec->lock, NULL);
    return dev;
        
}
