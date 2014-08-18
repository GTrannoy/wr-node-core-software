#include <stdio.h>

#include "list-lib.h"
void list_boot_node(struct list_node *dev);

main(int argc, char *argv[])
{
	if(argc < 2)
	{
		printf("usage: %s <lun>\n", argv[0]);
		return 0;
	}

	int lun = atoi(argv[1]);

	printf("Booting LIST Node @ Lun %d\n", lun);

	struct list_node *dev = list_open_node_by_lun(lun);
	if(!dev)
	{
		printf("Open failed.\n");
		return -1;
	}

	list_boot_node(dev);

	return 0;

}
