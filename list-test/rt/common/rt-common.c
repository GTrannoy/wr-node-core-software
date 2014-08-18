#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "rt-mqueue.h"
#include "rt-common.h"

int puts(const char *p)
{
    int i;
    volatile uint32_t *buf = mq_map_out_buffer(0, 1);
    
    mq_claim(0, 1);
    
    buf[0] = 0xdeadbeef;
    for(i=0;i<127;i++,p++)
      {
  	   if(*p)
	       buf[i+1] = *p;
	     else
	       break;
      }

    mq_send(0, 1, i + 1);
    return i;
}

/*

void ts_add(struct list_timestamp *a, struct list_timestamp *b)
{
  a->frac += b->frac;
  if(a->frac >= 4096)
    a->cycles += 1 + b->cycles;
  else
    a->cycles += b->cycles;

  if(a->cycle > 125 * 1000 * 1000)
    a->seconds += 1 + b->seconds;
  else
    a->seconds += b->seconds;
}

*/