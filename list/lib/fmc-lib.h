#ifndef __FMC_LIB_H
#define __FMC_LIB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// use DMA when reading/writing the block
#define FMC_BLK_DMA_ENABLE  (1<<0)
// don't increment DMA device address (not all plaftorms)
#define FMC_BLK_DMA_NO_INCREMENT (1<<1)

//
#define FMC_IRQ_USE_VIC (1<<0)

#define FMC_PGM_PROGRAMMED 1
#define FMC_PGM_BOOTLOADER_MODE 2
#define FMC_PGM_UNPROGRAMMED -1

#define FMC_MEM_MEZZANINE_EEPROM(x)
#define FMC_MEM_CARRIER_EEPROM(x)
#define FMC_MEM_CARRIER_FLASH(x)

struct fmc_dev {
    void (*writeb) ( struct fmc_dev *, uint8_t, uint64_t addr );
	void (*writeh) ( struct fmc_dev *, uint16_t, uint64_t addr );
	void (*writel) ( struct fmc_dev *, uint32_t, uint64_t addr );
    void (*writeq) ( struct fmc_dev *, uint64_t, uint64_t addr );

    uint8_t (*readb) ( struct fmc_dev *, uint64_t addr );
    uint16_t (*readh) ( struct fmc_dev *, uint64_t addr );
	uint32_t (*readl) ( struct fmc_dev *, uint64_t addr );
    uint64_t (*readq) ( struct fmc_dev *, uint64_t addr );

    int (*read_block) ( struct fmc_dev *, uint64_t addr, void *buf, size_t size, int flags );
	int (*write_block) ( struct fmc_dev *, uint64_t addr, void *buf, size_t size, int flags );

    int (*reprogram) ( struct fmc_dev*, const char *filename );
	int (*reprogram_raw) ( struct fmc_dev*, int device, const void *buf, size_t size );
    int (*is_programmed) ( struct fmc_dev * );
    int (*enter_bootloader) (struct fmc_dev * );

	int (*read_nv) (struct fmc_dev *, int mem, uint64_t address, void *data, size_t size );
	int (*write_nv) (struct fmc_dev *, int mem, uint64_t address, void *data, size_t size );
	int (*flush_nv) ( struct fmc_dev *, int mem );

    uint32_t (*sdb_find_device) ( struct fmc_dev *, uint64_t vendor, uint32_t dev_id, int ordinal );

    int (*enable_irq) ( struct fmc_dev *, int irq_id, int enable, int flags );
	int (*wait_irq) ( struct fmc_dev *, int irq_id, int flags, int timeout_us );

    void * priv;
};

struct fmc_dev *fmc_svec_create(int lun);

// path format:

// svec:slot=NUM,lun=NUM
// spec:bus=NUM,dev=NUM
// etherbone:ip=IP_ADDR,port=PORT,interface=NETWORK_CARD
// raw:base=ADDR,size=SIZE
struct fmc_dev *fmc_open( const char *path );

#ifdef __cplusplus
};
#endif

#endif
