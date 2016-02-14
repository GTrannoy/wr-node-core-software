#include "rt-d3s.h"

// WR aux clock disciplining

#define WR_LINK_OFFLINE     1
#define WR_LINK_ONLINE      2
#define WR_LINK_SYNCING     3
#define WR_LINK_SYNCED      4

static int wr_state;

int wr_link_up()
{
    return dp_readl ( DDS_REG_TCR ) & DDS_TCR_WR_LINK;
}

int wr_time_locked()
{
    return dp_readl ( DDS_REG_TCR ) & DDS_TCR_WR_LOCKED;
}

int wr_time_ready()
{
   return dp_readl ( DDS_REG_TCR ) & DDS_TCR_WR_TIME_VALID;
}

int wr_enable_lock( int enable )
{
    if(enable)
        dp_writel ( DDS_TCR_WR_LOCK_EN, DDS_REG_TCR );
    else
        dp_writel ( 0, DDS_REG_TCR);
}

void wr_update_link()
{
    switch(wr_state)
    {
        case WR_LINK_OFFLINE:
            if ( wr_link_up() )
            {
                wr_state = WR_LINK_ONLINE;
                dbg_printf("WR link online!");

            }
            break;
        
        case WR_LINK_ONLINE:
            if (wr_time_ready())
            {
                wr_state = WR_LINK_SYNCING;
                dbg_printf("WR time ok [lock on]!");

                wr_enable_lock(1);
            }
            break;

        case WR_LINK_SYNCING:
            if (wr_time_locked())
            {
                dbg_printf("WR link locked!");
                wr_state = WR_LINK_SYNCED;
            }
            break;

        case WR_LINK_SYNCED:
            break;
    }

    if( wr_state != WR_LINK_OFFLINE && !wr_link_up() )
    {
        wr_state = WR_LINK_OFFLINE;
        wr_enable_lock(0);
    }
}

int wr_is_timing_ok()
{
    return (wr_state == WR_LINK_SYNCED);
}

int wr_init()
{
    wr_state = WR_LINK_OFFLINE;
    wr_enable_lock(0);
}
