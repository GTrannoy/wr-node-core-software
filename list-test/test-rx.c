#include <stdio.h>
#include <unistd.h>

#include "wrn-lib.h"
#include "list-lib.h"
main()
{
    struct list_node *n = list_open_node_by_lun ( 0 );
    struct list_id trig = { 10, 20, 30 };
    //list_in_set_dead_time(n, 1, 90 * 1000 * 1000);
    list_in_enable(n, 1, 1);
    list_in_assign_trigger(n, 1, &trig);
    list_in_set_trigger_mode(n, 1, TDC_CHAN_MODE_CONTINUOUS);
    list_in_arm(n, 1, 1);

#if 0
    struct list_trigger_entry t;

    t.id.system = 0x1;
    t.id.source_port = 0x1;
    t.id.trigger = 0x3;
    t.ts.seconds = 0x4;
    t.ts.cycles = 0x5;
    t.ts.frac = 0x6;
    t.seq = 0;
    for(;;)
    {
        list_in_software_trigger(n, &t);
        t.seq++;
        usleep(10000);
        printf("swT\n");
    }
#endif

    sleep(1000);
    return 0;
}
