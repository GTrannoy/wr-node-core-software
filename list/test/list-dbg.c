#include <stdio.h>

#include "list-lib.h"

void list_boot_node(struct list_node *dev);

int main(int argc, char *argv[])
{
	struct list_node *dev;
	int  lun;

	if(argc < 2)
	{
		printf("usage: %s <lun>\n", argv[0]);
		return 0;
	}

	lun = atoi(argv[1]);

	dev = list_open_node_by_lun(lun);

	sleep(20000);

	return 0;

}
