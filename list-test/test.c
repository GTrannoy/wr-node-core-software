#include <stdio.h>
#include <unistd.h>

#include "fmc-lib.h"
#include "wrn-lib.h"

//uint32_t sdb_traverse (struct fmc_dev *dev, uint32_t base, uint32_t sdb_addr, uint32_t vendor, uint32_t device);

void monitor_hmq()
{

}

main()
{

    struct wrn_dev *node = wrn_open_by_lun(0);

    wrn_cpu_load_file(node, 0, "rt/tdc/rt-tdc.bin");
    wrn_cpu_start(node, (1<<0));

    for(;;)
    {
	wrn_update_mqueues(node);
	usleep(10000);
    } 

}
