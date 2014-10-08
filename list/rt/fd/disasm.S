
rt-fd.elf:     file format elf32-lm32


Disassembly of section .boot:

00000000 <_reset_handler>:
   0:	98 00 00 00 	xor r0,r0,r0
   4:	d0 00 00 00 	wcsr IE,r0
   8:	d0 20 00 00 	wcsr IM,r0
   c:	78 01 00 00 	mvhi r1,0x0
  10:	38 21 00 00 	ori r1,r1,0x0
  14:	d0 e1 00 00 	wcsr EBA,r1
  18:	f8 00 00 3a 	calli 100 <_crt0>
  1c:	34 00 00 00 	nop
	...

000000c0 <_interrupt_handler>:
  c0:	34 00 00 00 	nop
	...

00000100 <_crt0>:
 100:	98 00 00 00 	xor r0,r0,r0
 104:	98 21 08 00 	xor r1,r1,r1
 108:	98 42 10 00 	xor r2,r2,r2
 10c:	98 63 18 00 	xor r3,r3,r3
 110:	78 1c 00 00 	mvhi sp,0x0
 114:	3b 9c 7f fc 	ori sp,sp,0x7ffc
 118:	78 1a 00 00 	mvhi gp,0x0
 11c:	3b 5a a5 e0 	ori gp,gp,0xa5e0
 120:	78 01 00 00 	mvhi r1,0x0
 124:	38 21 25 ec 	ori r1,r1,0x25ec
 128:	34 02 00 00 	mvi r2,0
 12c:	78 03 00 00 	mvhi r3,0x0
 130:	38 63 4f 20 	ori r3,r3,0x4f20
 134:	c8 61 18 00 	sub r3,r3,r1
 138:	34 01 00 00 	mvi r1,0
 13c:	34 02 00 00 	mvi r2,0
 140:	34 03 00 00 	mvi r3,0
 144:	f8 00 02 80 	calli b44 <main>

00000148 <loopf>:
 148:	e0 00 00 00 	bi 148 <loopf>

Disassembly of section .text:

0000014c <pulse_queue_init>:
    }
}

void pulse_queue_init(struct lrt_pulse_queue *p)
{
    p->head = 0;
     14c:	58 20 00 c0 	sw (r1+192),r0
    p->tail = 0;
     150:	58 20 00 c4 	sw (r1+196),r0
    p->count = 0;
     154:	58 20 00 c8 	sw (r1+200),r0
}
     158:	c3 a0 00 00 	ret

0000015c <pulse_queue_push>:

int pulse_queue_push(struct lrt_pulse_queue *p, struct list_timestamp *ts)
{
    if(p->count == FD_MAX_QUEUE_PULSES)
     15c:	28 23 00 c8 	lw r3,(r1+200)
     160:	34 05 00 10 	mvi r5,16
	   return -1;
     164:	34 04 ff ff 	mvi r4,-1
    p->count = 0;
}

int pulse_queue_push(struct lrt_pulse_queue *p, struct list_timestamp *ts)
{
    if(p->count == FD_MAX_QUEUE_PULSES)
     168:	44 65 00 12 	be r3,r5,1b0 <pulse_queue_push+0x54>
	   return -1;

    p->count++;
    p->data[p->head] = *ts;
     16c:	28 24 00 c0 	lw r4,(r1+192)
int pulse_queue_push(struct lrt_pulse_queue *p, struct list_timestamp *ts)
{
    if(p->count == FD_MAX_QUEUE_PULSES)
	   return -1;

    p->count++;
     170:	34 67 00 01 	addi r7,r3,1
    p->data[p->head] = *ts;
     174:	28 49 00 00 	lw r9,(r2+0)
     178:	3c 88 00 04 	sli r8,r4,4
     17c:	3c 83 00 02 	sli r3,r4,2
    p->head++;
     180:	34 86 00 01 	addi r6,r4,1
{
    if(p->count == FD_MAX_QUEUE_PULSES)
	   return -1;

    p->count++;
    p->data[p->head] = *ts;
     184:	c9 03 18 00 	sub r3,r8,r3
     188:	b4 23 18 00 	add r3,r1,r3
     18c:	58 69 00 00 	sw (r3+0),r9
     190:	28 48 00 04 	lw r8,(r2+4)
int pulse_queue_push(struct lrt_pulse_queue *p, struct list_timestamp *ts)
{
    if(p->count == FD_MAX_QUEUE_PULSES)
	   return -1;

    p->count++;
     194:	58 27 00 c8 	sw (r1+200),r7
    p->data[p->head] = *ts;
    p->head++;
    if(p->head == FD_MAX_QUEUE_PULSES)
	   p->head = 0;

    return 0;
     198:	34 04 00 00 	mvi r4,0
{
    if(p->count == FD_MAX_QUEUE_PULSES)
	   return -1;

    p->count++;
    p->data[p->head] = *ts;
     19c:	58 68 00 04 	sw (r3+4),r8
     1a0:	28 42 00 08 	lw r2,(r2+8)
    p->head++;
     1a4:	58 26 00 c0 	sw (r1+192),r6
{
    if(p->count == FD_MAX_QUEUE_PULSES)
	   return -1;

    p->count++;
    p->data[p->head] = *ts;
     1a8:	58 62 00 08 	sw (r3+8),r2
    p->head++;
    if(p->head == FD_MAX_QUEUE_PULSES)
     1ac:	44 c5 00 03 	be r6,r5,1b8 <pulse_queue_push+0x5c>
	   p->head = 0;

    return 0;
}
     1b0:	b8 80 08 00 	mv r1,r4
     1b4:	c3 a0 00 00 	ret

    p->count++;
    p->data[p->head] = *ts;
    p->head++;
    if(p->head == FD_MAX_QUEUE_PULSES)
	   p->head = 0;
     1b8:	58 20 00 c0 	sw (r1+192),r0

    return 0;
}
     1bc:	b8 80 08 00 	mv r1,r4
     1c0:	c3 a0 00 00 	ret

000001c4 <pulse_queue_empty>:

int pulse_queue_empty(struct lrt_pulse_queue *p)
{
    return p->count == 0;
     1c4:	28 21 00 c8 	lw r1,(r1+200)
}
     1c8:	64 21 00 00 	cmpei r1,r1,0
     1cc:	c3 a0 00 00 	ret

000001d0 <pulse_queue_front>:

struct list_timestamp* pulse_queue_front(struct lrt_pulse_queue *p)
{
    if(!p->count)
     1d0:	28 23 00 c8 	lw r3,(r1+200)
{
    return p->count == 0;
}

struct list_timestamp* pulse_queue_front(struct lrt_pulse_queue *p)
{
     1d4:	b8 20 10 00 	mv r2,r1
    if(!p->count)
	   return NULL;
     1d8:	34 01 00 00 	mvi r1,0
    return p->count == 0;
}

struct list_timestamp* pulse_queue_front(struct lrt_pulse_queue *p)
{
    if(!p->count)
     1dc:	44 60 00 06 	be r3,r0,1f4 <pulse_queue_front+0x24>
	   return NULL;
    return &p->data[p->tail];
     1e0:	28 41 00 c4 	lw r1,(r2+196)
     1e4:	3c 23 00 02 	sli r3,r1,2
     1e8:	3c 21 00 04 	sli r1,r1,4
     1ec:	c8 23 08 00 	sub r1,r1,r3
     1f0:	b4 41 08 00 	add r1,r2,r1
}
     1f4:	c3 a0 00 00 	ret

000001f8 <pulse_queue_pop>:

void pulse_queue_pop(struct lrt_pulse_queue *p)
{
    p->tail++;
     1f8:	28 22 00 c4 	lw r2,(r1+196)
    if(p->tail == FD_MAX_QUEUE_PULSES)
     1fc:	34 03 00 10 	mvi r3,16
    return &p->data[p->tail];
}

void pulse_queue_pop(struct lrt_pulse_queue *p)
{
    p->tail++;
     200:	34 42 00 01 	addi r2,r2,1
     204:	58 22 00 c4 	sw (r1+196),r2
    if(p->tail == FD_MAX_QUEUE_PULSES)
     208:	5c 43 00 02 	bne r2,r3,210 <pulse_queue_pop+0x18>
	   p->tail = 0;
     20c:	58 20 00 c4 	sw (r1+196),r0
    p->count--;
     210:	28 22 00 c8 	lw r2,(r1+200)
     214:	34 42 ff ff 	addi r2,r2,-1
     218:	58 22 00 c8 	sw (r1+200),r2
}
     21c:	c3 a0 00 00 	ret

00000220 <init_outputs>:
{
    int i;

    for(i=0; i<FD_NUM_CHANNELS;i++)
    {
        outputs[i].base_addr = 0x100 + i * 0x100;
     220:	78 01 00 00 	mvhi r1,0x0
     224:	38 21 47 b0 	ori r1,r1,0x47b0
     228:	34 05 01 00 	mvi r5,256
     22c:	58 25 00 00 	sw (r1+0),r5
     230:	34 05 02 00 	mvi r5,512
     234:	58 25 01 5c 	sw (r1+348),r5
     238:	34 05 03 00 	mvi r5,768
     23c:	58 25 02 b8 	sw (r1+696),r5
        outputs[i].index = i;
     240:	34 05 00 02 	mvi r5,2
     244:	58 25 02 bc 	sw (r1+700),r5
{
    int i;

    for(i=0; i<FD_NUM_CHANNELS;i++)
    {
        outputs[i].base_addr = 0x100 + i * 0x100;
     248:	34 05 04 00 	mvi r5,1024
        outputs[i].index = i;
        outputs[i].idle = 1;
     24c:	34 02 00 01 	mvi r2,1
        outputs[i].dead_time = 80000 / 8; // 80 us
     250:	34 04 27 10 	mvi r4,10000
        outputs[i].width_cycles = 1250; // 1us
     254:	34 03 04 e2 	mvi r3,1250
{
    int i;

    for(i=0; i<FD_NUM_CHANNELS;i++)
    {
        outputs[i].base_addr = 0x100 + i * 0x100;
     258:	58 25 04 14 	sw (r1+1044),r5
        outputs[i].index = i;
     25c:	34 05 00 03 	mvi r5,3
     260:	58 20 00 04 	sw (r1+4),r0
        outputs[i].idle = 1;
     264:	58 22 00 6c 	sw (r1+108),r2
        outputs[i].dead_time = 80000 / 8; // 80 us
     268:	58 24 00 80 	sw (r1+128),r4
        outputs[i].width_cycles = 1250; // 1us
     26c:	58 23 00 84 	sw (r1+132),r3
    int i;

    for(i=0; i<FD_NUM_CHANNELS;i++)
    {
        outputs[i].base_addr = 0x100 + i * 0x100;
        outputs[i].index = i;
     270:	58 22 01 60 	sw (r1+352),r2
        outputs[i].idle = 1;
     274:	58 22 01 c8 	sw (r1+456),r2
        outputs[i].dead_time = 80000 / 8; // 80 us
     278:	58 24 01 dc 	sw (r1+476),r4
        outputs[i].width_cycles = 1250; // 1us
     27c:	58 23 01 e0 	sw (r1+480),r3

    for(i=0; i<FD_NUM_CHANNELS;i++)
    {
        outputs[i].base_addr = 0x100 + i * 0x100;
        outputs[i].index = i;
        outputs[i].idle = 1;
     280:	58 22 03 24 	sw (r1+804),r2
        outputs[i].dead_time = 80000 / 8; // 80 us
     284:	58 24 03 38 	sw (r1+824),r4
        outputs[i].width_cycles = 1250; // 1us
     288:	58 23 03 3c 	sw (r1+828),r3
    int i;

    for(i=0; i<FD_NUM_CHANNELS;i++)
    {
        outputs[i].base_addr = 0x100 + i * 0x100;
        outputs[i].index = i;
     28c:	58 25 04 18 	sw (r1+1048),r5
        outputs[i].idle = 1;
     290:	58 22 04 80 	sw (r1+1152),r2
        outputs[i].dead_time = 80000 / 8; // 80 us
     294:	58 24 04 94 	sw (r1+1172),r4
        outputs[i].width_cycles = 1250; // 1us
     298:	58 23 04 98 	sw (r1+1176),r3
    }
}
     29c:	c3 a0 00 00 	ret

000002a0 <do_output>:
static int last_cyc=0;

static struct list_timestamp prev_ts,prev_tc;

void do_output( struct lrt_output *out )
{
     2a0:	37 9c ff ec 	addi sp,sp,-20
     2a4:	5b 8b 00 14 	sw (sp+20),r11
     2a8:	5b 8c 00 10 	sw (sp+16),r12
     2ac:	5b 8d 00 0c 	sw (sp+12),r13
     2b0:	5b 8e 00 08 	sw (sp+8),r14
     2b4:	5b 9d 00 04 	sw (sp+4),ra

void rt_set_debug_slot(int slot);

static inline uint32_t dp_readl ( uint32_t reg )
{
  return *(volatile uint32_t *) ( reg + CPU_DP_BASE );
     2b8:	28 23 00 00 	lw r3,(r1+0)
     2bc:	78 02 00 20 	mvhi r2,0x20
     2c0:	b8 20 58 00 	mv r11,r1
     2c4:	b4 62 10 00 	add r2,r3,r2
        
    struct lrt_pulse_queue *q = &out->queue;
    uint32_t dcr = fd_ch_readl(out, FD_REG_DCR);

    if(!out->idle) {
     2c8:	28 21 00 6c 	lw r1,(r1+108)
     2cc:	28 42 00 00 	lw r2,(r2+0)
     2d0:	5c 20 00 4c 	bne r1,r0,400 <do_output+0x160>
        if (!(dcr & FD_DCR_PG_TRIG)) // still waiting for trigger
     2d4:	20 42 00 08 	andi r2,r2,0x8
     2d8:	5c 41 00 d7 	bne r2,r1,634 <do_output+0x394>
     *(volatile uint32_t *) ( reg + CPU_DP_BASE ) = value;
}

static inline uint32_t lr_readl ( uint32_t reg )
{
  return *(volatile uint32_t *) ( reg + CPU_LR_BASE );
     2dc:	78 03 00 00 	mvhi r3,0x0
     2e0:	78 05 00 00 	mvhi r5,0x0
     2e4:	38 63 22 04 	ori r3,r3,0x2204
     2e8:	38 a5 22 08 	ori r5,r5,0x2208
     2ec:	28 61 00 00 	lw r1,(r3+0)
     2f0:	28 a4 00 00 	lw r4,(r5+0)

            tc.cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
            tc.seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);


     int delta = tc.seconds - prev_tc.seconds;
     2f4:	78 0c 00 00 	mvhi r12,0x0
     2f8:	39 8c 25 f0 	ori r12,r12,0x25f0
    delta *= 125000000;
     2fc:	78 06 00 00 	mvhi r6,0x0
     300:	28 2d 00 00 	lw r13,(r1+0)

            tc.cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
            tc.seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);


     int delta = tc.seconds - prev_tc.seconds;
     304:	29 83 00 00 	lw r3,(r12+0)
     308:	28 8e 00 00 	lw r14,(r4+0)
    delta *= 125000000;
     30c:	38 c6 22 0c 	ori r6,r6,0x220c
     310:	28 c4 00 00 	lw r4,(r6+0)
    delta += tc.cycles - prev_tc.cycles;
     314:	29 81 00 04 	lw r1,(r12+4)

            tc.cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
            tc.seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);


     int delta = tc.seconds - prev_tc.seconds;
     318:	c9 c3 18 00 	sub r3,r14,r3
    delta *= 125000000;
     31c:	88 64 18 00 	mul r3,r3,r4
    delta += tc.cycles - prev_tc.cycles;
     320:	c9 a1 08 00 	sub r1,r13,r1
     324:	b4 23 08 00 	add r1,r1,r3

    if(delta < 0)
     328:	4c 22 00 04 	bge r1,r2,338 <do_output+0x98>
	pp_printf("FUCK-fd!");
     32c:	78 01 00 00 	mvhi r1,0x0
     330:	38 21 21 dc 	ori r1,r1,0x21dc
     334:	f8 00 06 74 	calli 1d04 <pp_printf>

	    prev_tc = tc;
	    delta = tc.seconds - out->last_programmed.seconds;
	    
	    delta *= 125000000;
     338:	78 07 00 00 	mvhi r7,0x0

    if(delta < 0)
	pp_printf("FUCK-fd!");

	    prev_tc = tc;
	    delta = tc.seconds - out->last_programmed.seconds;
     33c:	29 62 00 3c 	lw r2,(r11+60)
	    
	    delta *= 125000000;
     340:	38 e7 22 0c 	ori r7,r7,0x220c
     344:	28 e3 00 00 	lw r3,(r7+0)
	    delta += tc.cycles - out->last_programmed.cycles;
     348:	29 61 00 40 	lw r1,(r11+64)

    if(delta < 0)
	pp_printf("FUCK-fd!");

	    prev_tc = tc;
	    delta = tc.seconds - out->last_programmed.seconds;
     34c:	c9 c2 10 00 	sub r2,r14,r2
	    
	    delta *= 125000000;
     350:	88 43 10 00 	mul r2,r2,r3
	    delta += tc.cycles - out->last_programmed.cycles;
     354:	c9 a1 08 00 	sub r1,r13,r1
     int delta = tc.seconds - prev_tc.seconds;
    delta *= 125000000;
    delta += tc.cycles - prev_tc.cycles;

    if(delta < 0)
	pp_printf("FUCK-fd!");
     358:	59 8e 00 00 	sw (r12+0),r14
     35c:	59 8d 00 04 	sw (r12+4),r13
     360:	59 80 00 08 	sw (r12+8),r0

	    prev_tc = tc;
	    delta = tc.seconds - out->last_programmed.seconds;
	    
	    delta *= 125000000;
	    delta += tc.cycles - out->last_programmed.cycles;
     364:	b4 22 08 00 	add r1,r1,r2

	    if(delta > 0)
     368:	4c 01 00 1c 	bge r0,r1,3d8 <do_output+0x138>
	    {
                out->miss_timeout ++;
     36c:	29 61 00 0c 	lw r1,(r11+12)
  return *(volatile uint32_t *) ( reg + CPU_DP_BASE );
}

static inline void dp_writel ( uint32_t value, uint32_t reg )
{
     *(volatile uint32_t *) ( reg + CPU_DP_BASE ) = value;
     370:	29 62 00 00 	lw r2,(r11+0)
     374:	34 03 00 02 	mvi r3,2
     378:	34 21 00 01 	addi r1,r1,1
     37c:	59 61 00 0c 	sw (r11+12),r1
		out->idle = 1;
     380:	34 01 00 01 	mvi r1,1
     384:	59 61 00 6c 	sw (r11+108),r1
     388:	78 01 00 20 	mvhi r1,0x20
     38c:	b4 41 10 00 	add r2,r2,r1
	        fd_ch_writel(out, FD_DCR_MODE, FD_REG_DCR);

    		out->l1 = out->last_programmed;//tc.seconds; //delta;
		out->l2 = tc;
		out->l3 = prev_ts;
     390:	78 01 00 00 	mvhi r1,0x0
     394:	58 43 00 00 	sw (r2+0),r3
     398:	38 21 25 fc 	ori r1,r1,0x25fc
     39c:	28 23 00 00 	lw r3,(r1+0)
     3a0:	28 22 00 04 	lw r2,(r1+4)
	    {
                out->miss_timeout ++;
		out->idle = 1;
	        fd_ch_writel(out, FD_DCR_MODE, FD_REG_DCR);

    		out->l1 = out->last_programmed;//tc.seconds; //delta;
     3a4:	29 66 00 3c 	lw r6,(r11+60)
     3a8:	29 65 00 40 	lw r5,(r11+64)
     3ac:	29 64 00 44 	lw r4,(r11+68)
		out->l2 = tc;
		out->l3 = prev_ts;
     3b0:	28 21 00 08 	lw r1,(r1+8)
	    {
                out->miss_timeout ++;
		out->idle = 1;
	        fd_ch_writel(out, FD_DCR_MODE, FD_REG_DCR);

    		out->l1 = out->last_programmed;//tc.seconds; //delta;
     3b4:	59 66 00 48 	sw (r11+72),r6
     3b8:	59 65 00 4c 	sw (r11+76),r5
     3bc:	59 64 00 50 	sw (r11+80),r4
     3c0:	59 6e 00 54 	sw (r11+84),r14
     3c4:	59 6d 00 58 	sw (r11+88),r13
     3c8:	59 60 00 5c 	sw (r11+92),r0
		out->l2 = tc;
		out->l3 = prev_ts;
     3cc:	59 63 00 60 	sw (r11+96),r3
     3d0:	59 62 00 64 	sw (r11+100),r2
     3d4:	59 61 00 68 	sw (r11+104),r1

	    }

	    last_cyc = tc.cycles;
     3d8:	78 01 00 00 	mvhi r1,0x0
     3dc:	38 21 26 08 	ori r1,r1,0x2608
     3e0:	58 2d 00 00 	sw (r1+0),r13
    out->last_programmed = *ts;
    out->idle = 0;

    gpio_set(0);
    gpio_clear(0);
}
     3e4:	2b 9d 00 04 	lw ra,(sp+4)
     3e8:	2b 8b 00 14 	lw r11,(sp+20)
     3ec:	2b 8c 00 10 	lw r12,(sp+16)
     3f0:	2b 8d 00 0c 	lw r13,(sp+12)
     3f4:	2b 8e 00 08 	lw r14,(sp+8)
     3f8:	37 9c 00 14 	addi sp,sp,20
     3fc:	c3 a0 00 00 	ret
    return 0;
}

int pulse_queue_empty(struct lrt_pulse_queue *p)
{
    return p->count == 0;
     400:	29 62 01 50 	lw r2,(r11+336)

	return;
    } 
    

    if(pulse_queue_empty(q))
     404:	44 40 ff f8 	be r2,r0,3e4 <do_output+0x144>

struct list_timestamp* pulse_queue_front(struct lrt_pulse_queue *p)
{
    if(!p->count)
	   return NULL;
    return &p->data[p->tail];
     408:	29 61 01 4c 	lw r1,(r11+332)
}

void pulse_queue_pop(struct lrt_pulse_queue *p)
{
    p->tail++;
    if(p->tail == FD_MAX_QUEUE_PULSES)
     40c:	34 04 00 10 	mvi r4,16
    return &p->data[p->tail];
}

void pulse_queue_pop(struct lrt_pulse_queue *p)
{
    p->tail++;
     410:	34 23 00 01 	addi r3,r1,1
     414:	59 63 01 4c 	sw (r11+332),r3
    if(p->tail == FD_MAX_QUEUE_PULSES)
     418:	5c 64 00 02 	bne r3,r4,420 <do_output+0x180>
	   p->tail = 0;
     41c:	59 60 01 4c 	sw (r11+332),r0
  return *(volatile uint32_t *) ( reg + CPU_LR_BASE );
}

static inline uint32_t lr_writel ( uint32_t value, uint32_t reg )
{
  *(volatile uint32_t *) ( reg + CPU_LR_BASE ) = value;
     420:	78 05 00 00 	mvhi r5,0x0
    p->count--;
     424:	34 42 ff ff 	addi r2,r2,-1
     428:	38 a5 22 10 	ori r5,r5,0x2210
     42c:	59 62 01 50 	sw (r11+336),r2
    gpio_set(0);
    gpio_clear(0);

//    out->pgms++;
        
    fd_ch_writel(out, ts->seconds, FD_REG_U_STARTL);
     430:	3c 24 00 02 	sli r4,r1,2
     434:	3c 22 00 04 	sli r2,r1,4
     438:	28 a1 00 00 	lw r1,(r5+0)
     43c:	78 06 00 00 	mvhi r6,0x0
     440:	34 03 00 01 	mvi r3,1
     444:	38 c6 22 14 	ori r6,r6,0x2214
     448:	58 23 00 00 	sw (r1+0),r3
     44c:	28 c1 00 00 	lw r1,(r6+0)
  return *(volatile uint32_t *) ( reg + CPU_DP_BASE );
}

static inline void dp_writel ( uint32_t value, uint32_t reg )
{
     *(volatile uint32_t *) ( reg + CPU_DP_BASE ) = value;
     450:	78 07 00 00 	mvhi r7,0x0
     454:	c8 44 10 00 	sub r2,r2,r4
  return *(volatile uint32_t *) ( reg + CPU_LR_BASE );
}

static inline uint32_t lr_writel ( uint32_t value, uint32_t reg )
{
  *(volatile uint32_t *) ( reg + CPU_LR_BASE ) = value;
     458:	58 23 00 00 	sw (r1+0),r3
  return *(volatile uint32_t *) ( reg + CPU_DP_BASE );
}

static inline void dp_writel ( uint32_t value, uint32_t reg )
{
     *(volatile uint32_t *) ( reg + CPU_DP_BASE ) = value;
     45c:	38 e7 22 18 	ori r7,r7,0x2218
     460:	b5 62 08 00 	add r1,r11,r2
     464:	29 65 00 00 	lw r5,(r11+0)
     468:	28 e3 00 00 	lw r3,(r7+0)
     46c:	28 24 00 88 	lw r4,(r1+136)
     470:	78 06 00 00 	mvhi r6,0x0
     474:	b4 a3 18 00 	add r3,r5,r3
     478:	58 64 00 00 	sw (r3+0),r4
     47c:	38 c6 22 1c 	ori r6,r6,0x221c
     480:	29 65 00 00 	lw r5,(r11+0)
     484:	28 c3 00 00 	lw r3,(r6+0)
    fd_ch_writel(out, ts->cycles, FD_REG_C_START);
     488:	28 24 00 8c 	lw r4,(r1+140)
     48c:	78 07 00 00 	mvhi r7,0x0
     490:	b4 a3 18 00 	add r3,r5,r3
     494:	58 64 00 00 	sw (r3+0),r4
     498:	38 e7 22 20 	ori r7,r7,0x2220
     49c:	29 66 00 00 	lw r6,(r11+0)
     4a0:	28 e4 00 00 	lw r4,(r7+0)
    fd_ch_writel(out, ts->frac, FD_REG_F_START);
     4a4:	28 23 00 90 	lw r3,(r1+144)
    
    ts->cycles += out->width_cycles;
     4a8:	29 65 00 84 	lw r5,(r11+132)
     4ac:	b4 c4 20 00 	add r4,r6,r4
     4b0:	58 83 00 00 	sw (r4+0),r3
     4b4:	28 23 00 8c 	lw r3,(r1+140)
     4b8:	b4 a3 18 00 	add r3,r5,r3
    if(ts->cycles >= 125000000)
     4bc:	78 05 00 00 	mvhi r5,0x0
     4c0:	38 a5 22 24 	ori r5,r5,0x2224
     4c4:	28 a4 00 00 	lw r4,(r5+0)
        
    fd_ch_writel(out, ts->seconds, FD_REG_U_STARTL);
    fd_ch_writel(out, ts->cycles, FD_REG_C_START);
    fd_ch_writel(out, ts->frac, FD_REG_F_START);
    
    ts->cycles += out->width_cycles;
     4c8:	58 23 00 8c 	sw (r1+140),r3
    if(ts->cycles >= 125000000)
     4cc:	48 64 00 66 	bg r3,r4,664 <do_output+0x3c4>
     4d0:	28 24 00 88 	lw r4,(r1+136)
     4d4:	78 07 00 00 	mvhi r7,0x0
     4d8:	38 e7 22 2c 	ori r7,r7,0x222c
     4dc:	29 65 00 00 	lw r5,(r11+0)
     4e0:	28 e3 00 00 	lw r3,(r7+0)
     4e4:	78 06 00 00 	mvhi r6,0x0
     4e8:	38 c6 22 30 	ori r6,r6,0x2230
     4ec:	b4 a3 18 00 	add r3,r5,r3
     4f0:	58 64 00 00 	sw (r3+0),r4
     4f4:	29 65 00 00 	lw r5,(r11+0)
        ts->cycles -= 125000000;
        ts->seconds++;
    }

    fd_ch_writel(out, ts->seconds, FD_REG_U_ENDL);
    fd_ch_writel(out, ts->cycles, FD_REG_C_END);
     4f8:	b5 62 08 00 	add r1,r11,r2
     4fc:	28 c3 00 00 	lw r3,(r6+0)
     500:	28 24 00 8c 	lw r4,(r1+140)
     504:	78 07 00 00 	mvhi r7,0x0
     508:	b4 a3 18 00 	add r3,r5,r3
     50c:	58 64 00 00 	sw (r3+0),r4
     510:	38 e7 22 34 	ori r7,r7,0x2234
     514:	29 65 00 00 	lw r5,(r11+0)
     518:	28 e3 00 00 	lw r3,(r7+0)
    fd_ch_writel(out, ts->frac, FD_REG_F_END);
     51c:	28 24 00 90 	lw r4,(r1+144)
     520:	78 06 00 00 	mvhi r6,0x0
     524:	b4 a3 18 00 	add r3,r5,r3
     528:	58 64 00 00 	sw (r3+0),r4
     52c:	38 c6 22 38 	ori r6,r6,0x2238
     530:	29 65 00 00 	lw r5,(r11+0)
     534:	28 c4 00 00 	lw r4,(r6+0)
     538:	78 03 00 20 	mvhi r3,0x20
    fd_ch_writel(out, FD_DCR_MODE, FD_REG_DCR);
    fd_ch_writel(out, FD_DCR_MODE | FD_DCR_UPDATE, FD_REG_DCR);
    fd_ch_writel(out, FD_DCR_MODE | FD_DCR_PG_ARM | FD_DCR_ENABLE, FD_REG_DCR);

    ts->cycles += 1000;
    if(ts->cycles >= 125000000)
     53c:	78 07 00 00 	mvhi r7,0x0
     540:	b4 a4 20 00 	add r4,r5,r4
     544:	58 80 00 00 	sw (r4+0),r0
     548:	29 64 00 00 	lw r4,(r11+0)
     54c:	34 05 00 02 	mvi r5,2
     550:	38 e7 22 24 	ori r7,r7,0x2224
     554:	b4 83 20 00 	add r4,r4,r3
     558:	58 85 00 00 	sw (r4+0),r5
     55c:	29 64 00 00 	lw r4,(r11+0)
     560:	34 05 00 12 	mvi r5,18
     564:	b4 83 20 00 	add r4,r4,r3
     568:	58 85 00 00 	sw (r4+0),r5
     56c:	29 64 00 00 	lw r4,(r11+0)
     570:	b4 83 18 00 	add r3,r4,r3
     574:	34 04 00 07 	mvi r4,7
     578:	58 64 00 00 	sw (r3+0),r4
    fd_ch_writel(out, 0, FD_REG_RCR);
    fd_ch_writel(out, FD_DCR_MODE, FD_REG_DCR);
    fd_ch_writel(out, FD_DCR_MODE | FD_DCR_UPDATE, FD_REG_DCR);
    fd_ch_writel(out, FD_DCR_MODE | FD_DCR_PG_ARM | FD_DCR_ENABLE, FD_REG_DCR);

    ts->cycles += 1000;
     57c:	28 23 00 8c 	lw r3,(r1+140)
    if(ts->cycles >= 125000000)
     580:	28 e4 00 00 	lw r4,(r7+0)
    fd_ch_writel(out, 0, FD_REG_RCR);
    fd_ch_writel(out, FD_DCR_MODE, FD_REG_DCR);
    fd_ch_writel(out, FD_DCR_MODE | FD_DCR_UPDATE, FD_REG_DCR);
    fd_ch_writel(out, FD_DCR_MODE | FD_DCR_PG_ARM | FD_DCR_ENABLE, FD_REG_DCR);

    ts->cycles += 1000;
     584:	34 63 03 e8 	addi r3,r3,1000
     588:	58 23 00 8c 	sw (r1+140),r3
    if(ts->cycles >= 125000000)
     58c:	4c 83 00 09 	bge r4,r3,5b0 <do_output+0x310>
    {
        ts->cycles -= 125000000;
     590:	78 06 00 00 	mvhi r6,0x0
     594:	38 c6 22 28 	ori r6,r6,0x2228
        ts->seconds++;
     598:	28 24 00 88 	lw r4,(r1+136)
    fd_ch_writel(out, FD_DCR_MODE | FD_DCR_PG_ARM | FD_DCR_ENABLE, FD_REG_DCR);

    ts->cycles += 1000;
    if(ts->cycles >= 125000000)
    {
        ts->cycles -= 125000000;
     59c:	28 c5 00 00 	lw r5,(r6+0)
        ts->seconds++;
     5a0:	34 84 00 01 	addi r4,r4,1
    fd_ch_writel(out, FD_DCR_MODE | FD_DCR_PG_ARM | FD_DCR_ENABLE, FD_REG_DCR);

    ts->cycles += 1000;
    if(ts->cycles >= 125000000)
    {
        ts->cycles -= 125000000;
     5a4:	b4 65 18 00 	add r3,r3,r5
     5a8:	58 23 00 8c 	sw (r1+140),r3
        ts->seconds++;
     5ac:	58 24 00 88 	sw (r1+136),r4
    }


    // fixme: hardware miss detection?
    prev_ts = out->last_programmed;
     5b0:	29 63 00 44 	lw r3,(r11+68)
     5b4:	29 65 00 3c 	lw r5,(r11+60)
     5b8:	29 64 00 40 	lw r4,(r11+64)
     5bc:	78 01 00 00 	mvhi r1,0x0
     5c0:	38 21 25 fc 	ori r1,r1,0x25fc

    out->last_programmed = *ts;
     5c4:	b5 62 10 00 	add r2,r11,r2
        ts->seconds++;
    }


    // fixme: hardware miss detection?
    prev_ts = out->last_programmed;
     5c8:	58 24 00 04 	sw (r1+4),r4
     5cc:	58 23 00 08 	sw (r1+8),r3
     5d0:	58 25 00 00 	sw (r1+0),r5

    out->last_programmed = *ts;
     5d4:	34 41 00 88 	addi r1,r2,136
     5d8:	28 43 00 88 	lw r3,(r2+136)
     5dc:	28 22 00 04 	lw r2,(r1+4)
  return *(volatile uint32_t *) ( reg + CPU_LR_BASE );
}

static inline uint32_t lr_writel ( uint32_t value, uint32_t reg )
{
  *(volatile uint32_t *) ( reg + CPU_LR_BASE ) = value;
     5e0:	78 07 00 00 	mvhi r7,0x0
     5e4:	28 21 00 08 	lw r1,(r1+8)
     5e8:	38 e7 22 10 	ori r7,r7,0x2210
     5ec:	59 62 00 40 	sw (r11+64),r2
     5f0:	28 e2 00 00 	lw r2,(r7+0)
     5f4:	59 63 00 3c 	sw (r11+60),r3
     5f8:	78 03 00 00 	mvhi r3,0x0
     5fc:	59 61 00 44 	sw (r11+68),r1
    out->idle = 0;
     600:	59 60 00 6c 	sw (r11+108),r0
     604:	34 01 00 01 	mvi r1,1
     608:	38 63 22 14 	ori r3,r3,0x2214
     60c:	58 41 00 00 	sw (r2+0),r1
     610:	28 62 00 00 	lw r2,(r3+0)
     614:	58 41 00 00 	sw (r2+0),r1

    gpio_set(0);
    gpio_clear(0);
}
     618:	2b 9d 00 04 	lw ra,(sp+4)
     61c:	2b 8b 00 14 	lw r11,(sp+20)
     620:	2b 8c 00 10 	lw r12,(sp+16)
     624:	2b 8d 00 0c 	lw r13,(sp+12)
     628:	2b 8e 00 08 	lw r14,(sp+8)
     62c:	37 9c 00 14 	addi sp,sp,20
     630:	c3 a0 00 00 	ret

	    last_cyc = tc.cycles;


        } else {
	    out->hits++;
     634:	29 61 00 08 	lw r1,(r11+8)
	    out->idle = 1;
     638:	34 02 00 01 	mvi r2,1
     63c:	59 62 00 6c 	sw (r11+108),r2

	    last_cyc = tc.cycles;


        } else {
	    out->hits++;
     640:	34 21 00 01 	addi r1,r1,1
     644:	59 61 00 08 	sw (r11+8),r1
    out->last_programmed = *ts;
    out->idle = 0;

    gpio_set(0);
    gpio_clear(0);
}
     648:	2b 9d 00 04 	lw ra,(sp+4)
     64c:	2b 8b 00 14 	lw r11,(sp+20)
     650:	2b 8c 00 10 	lw r12,(sp+16)
     654:	2b 8d 00 0c 	lw r13,(sp+12)
     658:	2b 8e 00 08 	lw r14,(sp+8)
     65c:	37 9c 00 14 	addi sp,sp,20
     660:	c3 a0 00 00 	ret
    fd_ch_writel(out, ts->frac, FD_REG_F_START);
    
    ts->cycles += out->width_cycles;
    if(ts->cycles >= 125000000)
    {
        ts->cycles -= 125000000;
     664:	78 06 00 00 	mvhi r6,0x0
     668:	38 c6 22 28 	ori r6,r6,0x2228
        ts->seconds++;
     66c:	28 24 00 88 	lw r4,(r1+136)
    fd_ch_writel(out, ts->frac, FD_REG_F_START);
    
    ts->cycles += out->width_cycles;
    if(ts->cycles >= 125000000)
    {
        ts->cycles -= 125000000;
     670:	28 c5 00 00 	lw r5,(r6+0)
        ts->seconds++;
     674:	34 84 00 01 	addi r4,r4,1
    fd_ch_writel(out, ts->frac, FD_REG_F_START);
    
    ts->cycles += out->width_cycles;
    if(ts->cycles >= 125000000)
    {
        ts->cycles -= 125000000;
     678:	b4 65 18 00 	add r3,r3,r5
     67c:	58 23 00 8c 	sw (r1+140),r3
        ts->seconds++;
     680:	58 24 00 88 	sw (r1+136),r4
     684:	e3 ff ff 94 	bi 4d4 <do_output+0x234>

00000688 <enqueue_trigger>:




void enqueue_trigger ( int output, struct lrt_output_rule *rule, struct list_id *id, struct list_timestamp *ts, int seq)
{
     688:	37 9c ff fc 	addi sp,sp,-4
     68c:	5b 8b 00 04 	sw (sp+4),r11
    struct lrt_output *out = &outputs[output];
    struct list_timestamp adjusted = *ts;

    ts_adjust_delay (&adjusted, rule->delay_cycles, rule->delay_frac);
     690:	2c 46 00 04 	lhu r6,(r2+4)
    int channel;
};

static inline void ts_adjust_delay(struct list_timestamp *ts, uint32_t cycles, uint32_t frac)
{
    ts->frac += frac;
     694:	28 88 00 08 	lw r8,(r4+8)
void enqueue_trigger ( int output, struct lrt_output_rule *rule, struct list_id *id, struct list_timestamp *ts, int seq)
{
    struct lrt_output *out = &outputs[output];
    struct list_timestamp adjusted = *ts;

    ts_adjust_delay (&adjusted, rule->delay_cycles, rule->delay_frac);
     698:	28 45 00 00 	lw r5,(r2+0)



void enqueue_trigger ( int output, struct lrt_output_rule *rule, struct list_id *id, struct list_timestamp *ts, int seq)
{
    struct lrt_output *out = &outputs[output];
     69c:	28 83 00 04 	lw r3,(r4+4)
    int channel;
};

static inline void ts_adjust_delay(struct list_timestamp *ts, uint32_t cycles, uint32_t frac)
{
    ts->frac += frac;
     6a0:	b4 c8 40 00 	add r8,r6,r8
    
    if(ts->frac & 0x1000)
     6a4:	21 02 10 00 	andi r2,r8,0x1000
	ts->cycles++;
     6a8:	7c 42 00 00 	cmpnei r2,r2,0



void enqueue_trigger ( int output, struct lrt_output_rule *rule, struct list_id *id, struct list_timestamp *ts, int seq)
{
    struct lrt_output *out = &outputs[output];
     6ac:	28 86 00 00 	lw r6,(r4+0)
static inline void ts_adjust_delay(struct list_timestamp *ts, uint32_t cycles, uint32_t frac)
{
    ts->frac += frac;
    
    if(ts->frac & 0x1000)
	ts->cycles++;
     6b0:	b4 62 18 00 	add r3,r3,r2

    ts->frac &= 0xfff;
    ts->cycles += cycles;
     6b4:	b4 65 28 00 	add r5,r3,r5

    if(ts->cycles >= 125000000)
     6b8:	78 03 00 00 	mvhi r3,0x0
     6bc:	38 63 22 24 	ori r3,r3,0x2224
     6c0:	28 62 00 00 	lw r2,(r3+0)
     6c4:	4c 45 00 06 	bge r2,r5,6dc <enqueue_trigger+0x54>
    {
	ts->cycles -= 125000000;
     6c8:	78 0a 00 00 	mvhi r10,0x0
     6cc:	39 4a 22 28 	ori r10,r10,0x2228
     6d0:	29 42 00 00 	lw r2,(r10+0)
	ts->seconds++;
     6d4:	34 c6 00 01 	addi r6,r6,1
    ts->frac &= 0xfff;
    ts->cycles += cycles;

    if(ts->cycles >= 125000000)
    {
	ts->cycles -= 125000000;
     6d8:	b4 a2 28 00 	add r5,r5,r2



static inline int check_dead_time( struct lrt_output *out, struct list_timestamp *ts )
{
    if(out->idle)
     6dc:	08 21 01 5c 	muli r1,r1,348
     6e0:	78 02 00 00 	mvhi r2,0x0
     6e4:	38 42 47 b0 	ori r2,r2,0x47b0
     6e8:	b4 41 18 00 	add r3,r2,r1
     6ec:	28 67 00 6c 	lw r7,(r3+108)
     6f0:	5c e0 00 0f 	bne r7,r0,72c <enqueue_trigger+0xa4>
        return 1;

    int delta_s = ts->seconds - out->last_enqueued.seconds;
    int delta_c = ts->cycles - out->last_enqueued.cycles;
     6f4:	28 6a 00 34 	lw r10,(r3+52)
static inline int check_dead_time( struct lrt_output *out, struct list_timestamp *ts )
{
    if(out->idle)
        return 1;

    int delta_s = ts->seconds - out->last_enqueued.seconds;
     6f8:	28 69 00 30 	lw r9,(r3+48)
    int delta_c = ts->cycles - out->last_enqueued.cycles;
     6fc:	c8 aa 18 00 	sub r3,r5,r10
static inline int check_dead_time( struct lrt_output *out, struct list_timestamp *ts )
{
    if(out->idle)
        return 1;

    int delta_s = ts->seconds - out->last_enqueued.seconds;
     700:	c8 c9 48 00 	sub r9,r6,r9
    int delta_c = ts->cycles - out->last_enqueued.cycles;

    if(delta_c < 0)
     704:	4c 67 00 06 	bge r3,r7,71c <enqueue_trigger+0x94>
    {
        delta_c += 125 * 1000 * 1000;
     708:	78 0a 00 00 	mvhi r10,0x0
     70c:	39 4a 22 0c 	ori r10,r10,0x220c
     710:	29 47 00 00 	lw r7,(r10+0)
        delta_s--;
     714:	35 29 ff ff 	addi r9,r9,-1
    int delta_s = ts->seconds - out->last_enqueued.seconds;
    int delta_c = ts->cycles - out->last_enqueued.cycles;

    if(delta_c < 0)
    {
        delta_c += 125 * 1000 * 1000;
     718:	b4 67 18 00 	add r3,r3,r7
        delta_s--;
    }

    if(delta_s < 0 || delta_c < out->dead_time)
     71c:	48 09 00 2b 	bg r0,r9,7c8 <enqueue_trigger+0x140>
     720:	b4 41 38 00 	add r7,r2,r1
     724:	28 e7 00 80 	lw r7,(r7+128)
     728:	48 e3 00 28 	bg r7,r3,7c8 <enqueue_trigger+0x140>
    p->count = 0;
}

int pulse_queue_push(struct lrt_pulse_queue *p, struct list_timestamp *ts)
{
    if(p->count == FD_MAX_QUEUE_PULSES)
     72c:	b4 41 18 00 	add r3,r2,r1
     730:	28 67 01 50 	lw r7,(r3+336)
     734:	34 0a 00 10 	mvi r10,16
     738:	44 ea 00 2b 	be r7,r10,7e4 <enqueue_trigger+0x15c>
	   return -1;

    p->count++;
    p->data[p->head] = *ts;
     73c:	28 69 01 48 	lw r9,(r3+328)
int pulse_queue_push(struct lrt_pulse_queue *p, struct list_timestamp *ts)
{
    if(p->count == FD_MAX_QUEUE_PULSES)
	   return -1;

    p->count++;
     740:	34 e7 00 01 	addi r7,r7,1
     744:	58 67 01 50 	sw (r3+336),r7
    p->data[p->head] = *ts;
     748:	3d 2b 00 04 	sli r11,r9,4
     74c:	3d 27 00 02 	sli r7,r9,2
    ts->frac += frac;
    
    if(ts->frac & 0x1000)
	ts->cycles++;

    ts->frac &= 0xfff;
     750:	21 08 0f ff 	andi r8,r8,0xfff
{
    if(p->count == FD_MAX_QUEUE_PULSES)
	   return -1;

    p->count++;
    p->data[p->head] = *ts;
     754:	c9 67 38 00 	sub r7,r11,r7
     758:	b4 e1 38 00 	add r7,r7,r1
     75c:	b4 47 38 00 	add r7,r2,r7
     760:	58 e6 00 88 	sw (r7+136),r6
     764:	58 e5 00 8c 	sw (r7+140),r5
     768:	58 e8 00 90 	sw (r7+144),r8
    p->head++;
     76c:	35 29 00 01 	addi r9,r9,1
     770:	58 69 01 48 	sw (r3+328),r9
    if(p->head == FD_MAX_QUEUE_PULSES)
     774:	45 2a 00 22 	be r9,r10,7fc <enqueue_trigger+0x174>
    {
        out->miss_overflow ++;
        return;
    }

    out->last_enqueued = *ts;
     778:	28 85 00 00 	lw r5,(r4+0)
     77c:	b4 41 10 00 	add r2,r2,r1
     780:	34 43 00 30 	addi r3,r2,48
     784:	58 45 00 30 	sw (r2+48),r5
     788:	28 81 00 04 	lw r1,(r4+4)
     78c:	78 0a 00 00 	mvhi r10,0x0
     790:	39 4a 22 14 	ori r10,r10,0x2214
     794:	58 61 00 04 	sw (r3+4),r1
     798:	28 82 00 08 	lw r2,(r4+8)
     79c:	34 01 00 01 	mvi r1,1
     7a0:	58 62 00 08 	sw (r3+8),r2
     7a4:	78 03 00 00 	mvhi r3,0x0
     7a8:	38 63 22 10 	ori r3,r3,0x2210
     7ac:	28 62 00 00 	lw r2,(r3+0)
     7b0:	58 41 00 00 	sw (r2+0),r1
     7b4:	29 42 00 00 	lw r2,(r10+0)
     7b8:	58 41 00 00 	sw (r2+0),r1

    gpio_set(0);
    gpio_clear(0);

}
     7bc:	2b 8b 00 04 	lw r11,(sp+4)
     7c0:	37 9c 00 04 	addi sp,sp,4
     7c4:	c3 a0 00 00 	ret
//    if(!rule->enabled)
//	return;

    if(!check_dead_time(out, &adjusted))
    {
        out->miss_deadtime ++;
     7c8:	b4 41 10 00 	add r2,r2,r1
     7cc:	28 41 00 10 	lw r1,(r2+16)
     7d0:	34 21 00 01 	addi r1,r1,1
     7d4:	58 41 00 10 	sw (r2+16),r1
    out->last_enqueued = *ts;

    gpio_set(0);
    gpio_clear(0);

}
     7d8:	2b 8b 00 04 	lw r11,(sp+4)
     7dc:	37 9c 00 04 	addi sp,sp,4
     7e0:	c3 a0 00 00 	ret
        return;
    }

    if( pulse_queue_push( &out->queue, &adjusted ) < 0)
    {
        out->miss_overflow ++;
     7e4:	28 61 00 14 	lw r1,(r3+20)
     7e8:	34 21 00 01 	addi r1,r1,1
     7ec:	58 61 00 14 	sw (r3+20),r1
    out->last_enqueued = *ts;

    gpio_set(0);
    gpio_clear(0);

}
     7f0:	2b 8b 00 04 	lw r11,(sp+4)
     7f4:	37 9c 00 04 	addi sp,sp,4
     7f8:	c3 a0 00 00 	ret

    p->count++;
    p->data[p->head] = *ts;
    p->head++;
    if(p->head == FD_MAX_QUEUE_PULSES)
	   p->head = 0;
     7fc:	58 60 01 48 	sw (r3+328),r0
     800:	e3 ff ff de 	bi 778 <enqueue_trigger+0xf0>

00000804 <do_rx>:
    gpio_clear(0);

}

void do_rx()
{
     804:	37 9c ff b4 	addi sp,sp,-76
     808:	5b 8b 00 34 	sw (sp+52),r11
     80c:	5b 8c 00 30 	sw (sp+48),r12
     810:	5b 8d 00 2c 	sw (sp+44),r13
     814:	5b 8e 00 28 	sw (sp+40),r14
     818:	5b 8f 00 24 	sw (sp+36),r15
     81c:	5b 90 00 20 	sw (sp+32),r16
     820:	5b 91 00 1c 	sw (sp+28),r17
     824:	5b 92 00 18 	sw (sp+24),r18
     828:	5b 93 00 14 	sw (sp+20),r19
     82c:	5b 94 00 10 	sw (sp+16),r20
     830:	5b 95 00 0c 	sw (sp+12),r21
     834:	5b 96 00 08 	sw (sp+8),r22
     838:	5b 9d 00 04 	sw (sp+4),ra
  return *(volatile uint32_t *) ( REG_LR_POLL );
}

static inline uint32_t rmq_poll(int slot)
{
  return *(volatile uint32_t *) ( REG_LR_POLL ) & ( 1<< (16 + slot ));
     83c:	78 01 00 10 	mvhi r1,0x10
     840:	28 22 00 00 	lw r2,(r1+0)
     844:	78 01 00 01 	mvhi r1,0x1
     848:	a0 41 08 00 	and r1,r2,r1
    if( !rmq_poll( FD_IN_SLOT_REMOTE) )
     84c:	44 20 00 55 	be r1,r0,9a0 <do_rx+0x19c>
     850:	78 03 00 00 	mvhi r3,0x0
     854:	38 63 22 14 	ori r3,r3,0x2214
     858:	28 73 00 00 	lw r19,(r3+0)
	
    gpio_set(1);
    gpio_clear(1);
    struct list_trigger_message *msg = mq_map_in_buffer(1, FD_IN_SLOT_REMOTE) - sizeof(struct rmq_message_addr);
    int i, j;
    int cnt = msg->count;
     85c:	78 03 00 00 	mvhi r3,0x0
     860:	78 01 00 00 	mvhi r1,0x0
     864:	38 63 22 3c 	ori r3,r3,0x223c
     868:	38 21 22 10 	ori r1,r1,0x2210
     86c:	28 62 00 00 	lw r2,(r3+0)
     870:	28 34 00 00 	lw r20,(r1+0)
     874:	34 01 00 02 	mvi r1,2
     878:	28 56 00 14 	lw r22,(r2+20)
     87c:	5a 81 00 00 	sw (r20+0),r1
     880:	5a 61 00 00 	sw (r19+0),r1

    //tc.seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
    //tc.cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
    //pp_printf("Trx %d %d", tc.seconds, tc.cycles);
    //pp_printf("%d triggers\n", cnt);
    for(i=0; i < cnt; i++)
     884:	4c 16 00 42 	bge r0,r22,98c <do_rx+0x188>
     888:	78 01 00 00 	mvhi r1,0x0
     88c:	38 21 22 40 	ori r1,r1,0x2240
     890:	28 2c 00 00 	lw r12,(r1+0)
     894:	78 10 00 00 	mvhi r16,0x0
     898:	78 0d 00 00 	mvhi r13,0x0
     89c:	34 0e 00 00 	mvi r14,0
     8a0:	3a 10 4d 20 	ori r16,r16,0x4d20
     8a4:	39 ad 25 ec 	ori r13,r13,0x25ec
     8a8:	34 0f 00 01 	mvi r15,1
            for(j = 0; j < FD_NUM_CHANNELS; j++)
                if(ent->ocfg[j].state != HASH_ENT_EMPTY)
                {
                    gpio_set(0);
                    gpio_clear(0);
                    enqueue_trigger ( j, &ent->ocfg[j], &id, &ts, seq );
     8ac:	37 92 00 44 	addi r18,sp,68
     8b0:	37 91 00 38 	addi r17,sp,56
    //tc.cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
    //pp_printf("Trx %d %d", tc.seconds, tc.cycles);
    //pp_printf("%d triggers\n", cnt);
    for(i=0; i < cnt; i++)
    {
        struct list_id id = msg->triggers[i].id;
     8b4:	29 81 00 00 	lw r1,(r12+0)
     8b8:	5b 81 00 44 	sw (sp+68),r1
     8bc:	29 85 00 04 	lw r5,(r12+4)
struct lrt_hash_entry *hash_get_entry (int bucket, int pos);

static inline int hash_func( struct list_id *id )
{
    int h = 0;
    h += id->system * 10291;
     8c0:	08 22 28 33 	muli r2,r1,10291
     8c4:	5b 85 00 48 	sw (sp+72),r5
     8c8:	29 86 00 08 	lw r6,(r12+8)
    h += id->source_port * 10017;
     8cc:	08 a8 27 21 	muli r8,r5,10017
    h += id->trigger * 3111;
     8d0:	08 c7 0c 27 	muli r7,r6,3111

static inline int hash_func( struct list_id *id )
{
    int h = 0;
    h += id->system * 10291;
    h += id->source_port * 10017;
     8d4:	b5 02 10 00 	add r2,r8,r2
     8d8:	5b 86 00 4c 	sw (sp+76),r6
    h += id->trigger * 3111;
     8dc:	b4 47 10 00 	add r2,r2,r7
    return h & (FD_HASH_ENTRIES - 1); // hash table size must be a power of 2
     8e0:	20 42 00 7f 	andi r2,r2,0x7f

static inline struct lrt_hash_entry *hash_search( struct list_id *id, int *pos )
{
    int p = hash_func( id );

    struct lrt_hash_entry *ent = htab[ p ];
     8e4:	3c 42 00 02 	sli r2,r2,2
     8e8:	b6 02 10 00 	add r2,r16,r2
     8ec:	28 4b 00 00 	lw r11,(r2+0)
    if(pos)
        *pos = p;
    
    
    while (ent)
     8f0:	5d 60 00 04 	bne r11,r0,900 <do_rx+0xfc>
     8f4:	e0 00 00 3a 	bi 9dc <do_rx+0x1d8>
        if(ent->id.system == id->system &&
       ent->id.source_port == id->source_port &&
           ent->id.trigger == id->trigger)
            return ent;
    
    ent = ent->next;
     8f8:	29 6b 00 3c 	lw r11,(r11+60)
    struct lrt_hash_entry *ent = htab[ p ];
    if(pos)
        *pos = p;
    
    
    while (ent)
     8fc:	45 60 00 38 	be r11,r0,9dc <do_rx+0x1d8>
    {
        if(ent->id.system == id->system &&
     900:	29 62 00 00 	lw r2,(r11+0)
     904:	5c 22 ff fd 	bne r1,r2,8f8 <do_rx+0xf4>
     908:	29 62 00 04 	lw r2,(r11+4)
     90c:	5c a2 ff fb 	bne r5,r2,8f8 <do_rx+0xf4>
       ent->id.source_port == id->source_port &&
     910:	29 62 00 08 	lw r2,(r11+8)
     914:	5c c2 ff f9 	bne r6,r2,8f8 <do_rx+0xf4>

        rx_total++;

        if(ent)
        {
            struct list_timestamp ts = msg->triggers[i].ts;
     918:	29 83 ff f4 	lw r3,(r12+-12)
    for(i=0; i < cnt; i++)
    {
        struct list_id id = msg->triggers[i].id;
        struct lrt_hash_entry *ent = hash_search( &id, NULL );

        rx_total++;
     91c:	29 a2 00 00 	lw r2,(r13+0)
        if(ent)
        {
            struct list_timestamp ts = msg->triggers[i].ts;
            int seq = msg->triggers[i].seq;
            for(j = 0; j < FD_NUM_CHANNELS; j++)
                if(ent->ocfg[j].state != HASH_ENT_EMPTY)
     920:	2d 61 00 12 	lhu r1,(r11+18)

        rx_total++;

        if(ent)
        {
            struct list_timestamp ts = msg->triggers[i].ts;
     924:	5b 83 00 38 	sw (sp+56),r3
     928:	29 83 ff f8 	lw r3,(r12+-8)
    for(i=0; i < cnt; i++)
    {
        struct list_id id = msg->triggers[i].id;
        struct lrt_hash_entry *ent = hash_search( &id, NULL );

        rx_total++;
     92c:	34 42 00 01 	addi r2,r2,1
     930:	59 a2 00 00 	sw (r13+0),r2

        if(ent)
        {
            struct list_timestamp ts = msg->triggers[i].ts;
     934:	5b 83 00 3c 	sw (sp+60),r3
     938:	29 82 ff fc 	lw r2,(r12+-4)
     93c:	5b 82 00 40 	sw (sp+64),r2
            int seq = msg->triggers[i].seq;
     940:	29 95 00 0c 	lw r21,(r12+12)
            for(j = 0; j < FD_NUM_CHANNELS; j++)
                if(ent->ocfg[j].state != HASH_ENT_EMPTY)
     944:	5c 20 00 2d 	bne r1,r0,9f8 <do_rx+0x1f4>
     948:	2d 61 00 1e 	lhu r1,(r11+30)
     94c:	5c 20 00 35 	bne r1,r0,a20 <do_rx+0x21c>
     950:	2d 61 00 2a 	lhu r1,(r11+42)
     954:	5c 20 00 3d 	bne r1,r0,a48 <do_rx+0x244>
     958:	2d 61 00 36 	lhu r1,(r11+54)
     95c:	44 20 00 23 	be r1,r0,9e8 <do_rx+0x1e4>
     960:	5a 8f 00 00 	sw (r20+0),r15
                {
                    gpio_set(0);
                    gpio_clear(0);
                    enqueue_trigger ( j, &ent->ocfg[j], &id, &ts, seq );
     964:	34 01 00 03 	mvi r1,3
     968:	35 62 00 30 	addi r2,r11,48
     96c:	ba 40 18 00 	mv r3,r18
     970:	ba 20 20 00 	mv r4,r17
     974:	ba a0 28 00 	mv r5,r21
     978:	5a 6f 00 00 	sw (r19+0),r15

    //tc.seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
    //tc.cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
    //pp_printf("Trx %d %d", tc.seconds, tc.cycles);
    //pp_printf("%d triggers\n", cnt);
    for(i=0; i < cnt; i++)
     97c:	35 ce 00 01 	addi r14,r14,1
            for(j = 0; j < FD_NUM_CHANNELS; j++)
                if(ent->ocfg[j].state != HASH_ENT_EMPTY)
                {
                    gpio_set(0);
                    gpio_clear(0);
                    enqueue_trigger ( j, &ent->ocfg[j], &id, &ts, seq );
     980:	fb ff ff 42 	calli 688 <enqueue_trigger>

    //tc.seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
    //tc.cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
    //pp_printf("Trx %d %d", tc.seconds, tc.cycles);
    //pp_printf("%d triggers\n", cnt);
    for(i=0; i < cnt; i++)
     984:	35 8c 00 1c 	addi r12,r12,28
     988:	4a ce ff cb 	bg r22,r14,8b4 <do_rx+0xb0>
};

static inline void mq_writel( int remote, uint32_t val, uint32_t reg )
{
  if(remote)
    * (volatile uint32_t * ) (RMQ_BASE + reg) = val ;
     98c:	78 03 00 00 	mvhi r3,0x0
     990:	38 63 22 44 	ori r3,r3,0x2244
     994:	28 61 00 00 	lw r1,(r3+0)
     998:	78 02 08 00 	mvhi r2,0x800
     99c:	58 22 00 00 	sw (r1+0),r2
        }
    }
    //pp_printf("RX %d %d %d!", msg->triggers[0].id.system, msg->triggers[0].id.source_port, msg->triggers[0].id.trigger);
    mq_discard(1, FD_IN_SLOT_REMOTE);
    
}
     9a0:	2b 9d 00 04 	lw ra,(sp+4)
     9a4:	2b 8b 00 34 	lw r11,(sp+52)
     9a8:	2b 8c 00 30 	lw r12,(sp+48)
     9ac:	2b 8d 00 2c 	lw r13,(sp+44)
     9b0:	2b 8e 00 28 	lw r14,(sp+40)
     9b4:	2b 8f 00 24 	lw r15,(sp+36)
     9b8:	2b 90 00 20 	lw r16,(sp+32)
     9bc:	2b 91 00 1c 	lw r17,(sp+28)
     9c0:	2b 92 00 18 	lw r18,(sp+24)
     9c4:	2b 93 00 14 	lw r19,(sp+20)
     9c8:	2b 94 00 10 	lw r20,(sp+16)
     9cc:	2b 95 00 0c 	lw r21,(sp+12)
     9d0:	2b 96 00 08 	lw r22,(sp+8)
     9d4:	37 9c 00 4c 	addi sp,sp,76
     9d8:	c3 a0 00 00 	ret
    for(i=0; i < cnt; i++)
    {
        struct list_id id = msg->triggers[i].id;
        struct lrt_hash_entry *ent = hash_search( &id, NULL );

        rx_total++;
     9dc:	29 a1 00 00 	lw r1,(r13+0)
     9e0:	34 21 00 01 	addi r1,r1,1
     9e4:	59 a1 00 00 	sw (r13+0),r1

    //tc.seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
    //tc.cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
    //pp_printf("Trx %d %d", tc.seconds, tc.cycles);
    //pp_printf("%d triggers\n", cnt);
    for(i=0; i < cnt; i++)
     9e8:	35 ce 00 01 	addi r14,r14,1
     9ec:	35 8c 00 1c 	addi r12,r12,28
     9f0:	4a ce ff b1 	bg r22,r14,8b4 <do_rx+0xb0>
     9f4:	e3 ff ff e6 	bi 98c <do_rx+0x188>
     9f8:	5a 8f 00 00 	sw (r20+0),r15
            for(j = 0; j < FD_NUM_CHANNELS; j++)
                if(ent->ocfg[j].state != HASH_ENT_EMPTY)
                {
                    gpio_set(0);
                    gpio_clear(0);
                    enqueue_trigger ( j, &ent->ocfg[j], &id, &ts, seq );
     9fc:	34 01 00 00 	mvi r1,0
     a00:	35 62 00 0c 	addi r2,r11,12
     a04:	ba 40 18 00 	mv r3,r18
     a08:	ba 20 20 00 	mv r4,r17
     a0c:	ba a0 28 00 	mv r5,r21
     a10:	5a 6f 00 00 	sw (r19+0),r15
     a14:	fb ff ff 1d 	calli 688 <enqueue_trigger>
        if(ent)
        {
            struct list_timestamp ts = msg->triggers[i].ts;
            int seq = msg->triggers[i].seq;
            for(j = 0; j < FD_NUM_CHANNELS; j++)
                if(ent->ocfg[j].state != HASH_ENT_EMPTY)
     a18:	2d 61 00 1e 	lhu r1,(r11+30)
     a1c:	44 20 ff cd 	be r1,r0,950 <do_rx+0x14c>
     a20:	5a 8f 00 00 	sw (r20+0),r15
                {
                    gpio_set(0);
                    gpio_clear(0);
                    enqueue_trigger ( j, &ent->ocfg[j], &id, &ts, seq );
     a24:	34 01 00 01 	mvi r1,1
     a28:	35 62 00 18 	addi r2,r11,24
     a2c:	ba 40 18 00 	mv r3,r18
     a30:	ba 20 20 00 	mv r4,r17
     a34:	ba a0 28 00 	mv r5,r21
     a38:	5a 6f 00 00 	sw (r19+0),r15
     a3c:	fb ff ff 13 	calli 688 <enqueue_trigger>
        if(ent)
        {
            struct list_timestamp ts = msg->triggers[i].ts;
            int seq = msg->triggers[i].seq;
            for(j = 0; j < FD_NUM_CHANNELS; j++)
                if(ent->ocfg[j].state != HASH_ENT_EMPTY)
     a40:	2d 61 00 2a 	lhu r1,(r11+42)
     a44:	44 20 ff c5 	be r1,r0,958 <do_rx+0x154>
     a48:	5a 8f 00 00 	sw (r20+0),r15
                {
                    gpio_set(0);
                    gpio_clear(0);
                    enqueue_trigger ( j, &ent->ocfg[j], &id, &ts, seq );
     a4c:	34 01 00 02 	mvi r1,2
     a50:	35 62 00 24 	addi r2,r11,36
     a54:	ba 40 18 00 	mv r3,r18
     a58:	ba 20 20 00 	mv r4,r17
     a5c:	ba a0 28 00 	mv r5,r21
     a60:	5a 6f 00 00 	sw (r19+0),r15
     a64:	fb ff ff 09 	calli 688 <enqueue_trigger>
     a68:	e3 ff ff bc 	bi 958 <do_rx+0x154>

00000a6c <do_outputs>:
    mq_discard(1, FD_IN_SLOT_REMOTE);
    
}

void do_outputs()
{
     a6c:	37 9c ff fc 	addi sp,sp,-4
     a70:	5b 9d 00 04 	sw (sp+4),ra
    int i;
    
    for(i=0;i < FD_NUM_CHANNELS;i++)
        do_output(&outputs[i]);
     a74:	78 01 00 00 	mvhi r1,0x0
     a78:	38 21 47 b0 	ori r1,r1,0x47b0
     a7c:	fb ff fe 09 	calli 2a0 <do_output>
     a80:	78 01 00 00 	mvhi r1,0x0
     a84:	38 21 49 0c 	ori r1,r1,0x490c
     a88:	fb ff fe 06 	calli 2a0 <do_output>
     a8c:	78 01 00 00 	mvhi r1,0x0
     a90:	38 21 4a 68 	ori r1,r1,0x4a68
     a94:	fb ff fe 03 	calli 2a0 <do_output>
     a98:	78 01 00 00 	mvhi r1,0x0
     a9c:	38 21 4b c4 	ori r1,r1,0x4bc4
     aa0:	fb ff fe 00 	calli 2a0 <do_output>
}
     aa4:	2b 9d 00 04 	lw ra,(sp+4)
     aa8:	37 9c 00 04 	addi sp,sp,4
     aac:	c3 a0 00 00 	ret

00000ab0 <init>:

    mq_discard(0, FD_IN_SLOT_CONTROL);
}

void init() 
{
     ab0:	37 9c ff fc 	addi sp,sp,-4
     ab4:	5b 9d 00 04 	sw (sp+4),ra
    hash_init();
     ab8:	f8 00 02 b1 	calli 157c <hash_init>
{
    int i;

    for(i=0; i<FD_NUM_CHANNELS;i++)
    {
        outputs[i].base_addr = 0x100 + i * 0x100;
     abc:	78 01 00 00 	mvhi r1,0x0
     ac0:	38 21 47 b0 	ori r1,r1,0x47b0
     ac4:	34 05 01 00 	mvi r5,256
     ac8:	58 25 00 00 	sw (r1+0),r5
     acc:	34 05 02 00 	mvi r5,512
     ad0:	58 25 01 5c 	sw (r1+348),r5
     ad4:	34 05 03 00 	mvi r5,768
     ad8:	58 25 02 b8 	sw (r1+696),r5
        outputs[i].index = i;
     adc:	34 05 00 02 	mvi r5,2
     ae0:	58 25 02 bc 	sw (r1+700),r5
{
    int i;

    for(i=0; i<FD_NUM_CHANNELS;i++)
    {
        outputs[i].base_addr = 0x100 + i * 0x100;
     ae4:	34 05 04 00 	mvi r5,1024
        outputs[i].index = i;
        outputs[i].idle = 1;
     ae8:	34 02 00 01 	mvi r2,1
        outputs[i].dead_time = 80000 / 8; // 80 us
     aec:	34 04 27 10 	mvi r4,10000
        outputs[i].width_cycles = 1250; // 1us
     af0:	34 03 04 e2 	mvi r3,1250
{
    int i;

    for(i=0; i<FD_NUM_CHANNELS;i++)
    {
        outputs[i].base_addr = 0x100 + i * 0x100;
     af4:	58 25 04 14 	sw (r1+1044),r5
        outputs[i].index = i;
     af8:	34 05 00 03 	mvi r5,3
     afc:	58 20 00 04 	sw (r1+4),r0
        outputs[i].idle = 1;
     b00:	58 22 00 6c 	sw (r1+108),r2
        outputs[i].dead_time = 80000 / 8; // 80 us
     b04:	58 24 00 80 	sw (r1+128),r4
        outputs[i].width_cycles = 1250; // 1us
     b08:	58 23 00 84 	sw (r1+132),r3
    int i;

    for(i=0; i<FD_NUM_CHANNELS;i++)
    {
        outputs[i].base_addr = 0x100 + i * 0x100;
        outputs[i].index = i;
     b0c:	58 22 01 60 	sw (r1+352),r2
        outputs[i].idle = 1;
     b10:	58 22 01 c8 	sw (r1+456),r2
        outputs[i].dead_time = 80000 / 8; // 80 us
     b14:	58 24 01 dc 	sw (r1+476),r4
        outputs[i].width_cycles = 1250; // 1us
     b18:	58 23 01 e0 	sw (r1+480),r3

    for(i=0; i<FD_NUM_CHANNELS;i++)
    {
        outputs[i].base_addr = 0x100 + i * 0x100;
        outputs[i].index = i;
        outputs[i].idle = 1;
     b1c:	58 22 03 24 	sw (r1+804),r2
        outputs[i].dead_time = 80000 / 8; // 80 us
     b20:	58 24 03 38 	sw (r1+824),r4
        outputs[i].width_cycles = 1250; // 1us
     b24:	58 23 03 3c 	sw (r1+828),r3
    int i;

    for(i=0; i<FD_NUM_CHANNELS;i++)
    {
        outputs[i].base_addr = 0x100 + i * 0x100;
        outputs[i].index = i;
     b28:	58 25 04 18 	sw (r1+1048),r5
        outputs[i].idle = 1;
     b2c:	58 22 04 80 	sw (r1+1152),r2
        outputs[i].dead_time = 80000 / 8; // 80 us
     b30:	58 24 04 94 	sw (r1+1172),r4
        outputs[i].width_cycles = 1250; // 1us
     b34:	58 23 04 98 	sw (r1+1176),r3

void init() 
{
    hash_init();
    init_outputs();
}
     b38:	2b 9d 00 04 	lw ra,(sp+4)
     b3c:	37 9c 00 04 	addi sp,sp,4
     b40:	c3 a0 00 00 	ret

00000b44 <main>:


main()
{   
     b44:	37 9c ff 9c 	addi sp,sp,-100
     b48:	5b 8b 00 44 	sw (sp+68),r11
     b4c:	5b 8c 00 40 	sw (sp+64),r12
     b50:	5b 8d 00 3c 	sw (sp+60),r13
     b54:	5b 8e 00 38 	sw (sp+56),r14
     b58:	5b 8f 00 34 	sw (sp+52),r15
     b5c:	5b 90 00 30 	sw (sp+48),r16
     b60:	5b 91 00 2c 	sw (sp+44),r17
     b64:	5b 92 00 28 	sw (sp+40),r18
     b68:	5b 93 00 24 	sw (sp+36),r19
     b6c:	5b 94 00 20 	sw (sp+32),r20
     b70:	5b 95 00 1c 	sw (sp+28),r21
     b74:	5b 96 00 18 	sw (sp+24),r22
     b78:	5b 97 00 14 	sw (sp+20),r23
     b7c:	5b 98 00 10 	sw (sp+16),r24
     b80:	5b 99 00 0c 	sw (sp+12),r25
     b84:	5b 9b 00 08 	sw (sp+8),fp
     b88:	5b 9d 00 04 	sw (sp+4),ra
    rt_set_debug_slot(FD_OUT_SLOT_CONTROL);
     b8c:	34 01 00 01 	mvi r1,1
    mq_discard(0, FD_IN_SLOT_CONTROL);
}

void init() 
{
    hash_init();
     b90:	78 10 00 00 	mvhi r16,0x0
}


main()
{   
    rt_set_debug_slot(FD_OUT_SLOT_CONTROL);
     b94:	f8 00 04 77 	calli 1d70 <rt_set_debug_slot>
    mq_discard(0, FD_IN_SLOT_CONTROL);
}

void init() 
{
    hash_init();
     b98:	3a 10 47 b0 	ori r16,r16,0x47b0
     b9c:	f8 00 02 78 	calli 157c <hash_init>
     ba0:	ba 00 10 00 	mv r2,r16

void init_outputs()
{
    int i;

    for(i=0; i<FD_NUM_CHANNELS;i++)
     ba4:	34 03 00 00 	mvi r3,0
    {
        outputs[i].base_addr = 0x100 + i * 0x100;
        outputs[i].index = i;
        outputs[i].idle = 1;
     ba8:	34 08 00 01 	mvi r8,1
        outputs[i].dead_time = 80000 / 8; // 80 us
     bac:	34 07 27 10 	mvi r7,10000
        outputs[i].width_cycles = 1250; // 1us
     bb0:	34 06 04 e2 	mvi r6,1250

void init_outputs()
{
    int i;

    for(i=0; i<FD_NUM_CHANNELS;i++)
     bb4:	34 05 00 04 	mvi r5,4
    {
        outputs[i].base_addr = 0x100 + i * 0x100;
     bb8:	34 61 00 01 	addi r1,r3,1
     bbc:	3c 24 00 08 	sli r4,r1,8
        outputs[i].index = i;
     bc0:	58 43 00 04 	sw (r2+4),r3
{
    int i;

    for(i=0; i<FD_NUM_CHANNELS;i++)
    {
        outputs[i].base_addr = 0x100 + i * 0x100;
     bc4:	58 44 00 00 	sw (r2+0),r4
        outputs[i].index = i;
        outputs[i].idle = 1;
     bc8:	58 48 00 6c 	sw (r2+108),r8
        outputs[i].dead_time = 80000 / 8; // 80 us
     bcc:	58 47 00 80 	sw (r2+128),r7
        outputs[i].width_cycles = 1250; // 1us
     bd0:	58 46 00 84 	sw (r2+132),r6
     bd4:	34 42 01 5c 	addi r2,r2,348

void init_outputs()
{
    int i;

    for(i=0; i<FD_NUM_CHANNELS;i++)
     bd8:	44 25 00 03 	be r1,r5,be4 <main+0xa0>
     bdc:	b8 20 18 00 	mv r3,r1
     be0:	e3 ff ff f6 	bi bb8 <main+0x74>
     be4:	78 01 00 00 	mvhi r1,0x0
     be8:	38 21 22 10 	ori r1,r1,0x2210
     bec:	28 23 00 00 	lw r3,(r1+0)
        do_rx();
        do_outputs();
        do_control();
    }

     bf0:	78 02 00 00 	mvhi r2,0x0
     bf4:	78 0b 01 00 	mvhi r11,0x100
     bf8:	38 42 22 48 	ori r2,r2,0x2248
     bfc:	58 6b 00 00 	sw (r3+0),r11
     c00:	28 56 00 00 	lw r22,(r2+0)
     c04:	78 02 02 00 	mvhi r2,0x200
     c08:	58 62 00 00 	sw (r3+0),r2
     c0c:	78 07 00 00 	mvhi r7,0x0
    gpio_set(25);
    gpio_set(26);

    int loops =0;    

    pp_printf("RT_FD firmware initialized.");
     c10:	78 02 00 00 	mvhi r2,0x0
     c14:	78 09 00 00 	mvhi r9,0x0
     c18:	b8 40 08 00 	mv r1,r2
     c1c:	5b 87 00 48 	sw (sp+72),r7
     c20:	78 02 04 00 	mvhi r2,0x400
     c24:	5b 89 00 4c 	sw (sp+76),r9
  else
    * (volatile uint32_t * ) (HMQ_BASE + reg) = val ;
     c28:	78 04 00 00 	mvhi r4,0x0
    int channel = buf[0];

    struct lrt_output *st = &outputs[channel];
    uint32_t *obuf = ctl_claim_out();
  
    obuf[0] = ID_REP_STATE;
     c2c:	78 05 00 00 	mvhi r5,0x0
    obuf[1] = seq;
     c30:	78 06 00 00 	mvhi r6,0x0
     c34:	38 84 22 4c 	ori r4,r4,0x224c
    int channel = buf[0];

    struct lrt_output *st = &outputs[channel];
    uint32_t *obuf = ctl_claim_out();
  
    obuf[0] = ID_REP_STATE;
     c38:	38 a5 22 50 	ori r5,r5,0x2250
    obuf[1] = seq;
     c3c:	38 c6 22 54 	ori r6,r6,0x2254
     c40:	58 62 00 00 	sw (r3+0),r2
    gpio_set(25);
    gpio_set(26);

    int loops =0;    

    pp_printf("RT_FD firmware initialized.");
     c44:	38 21 21 e8 	ori r1,r1,0x21e8
     c48:	28 95 00 00 	lw r21,(r4+0)
    int channel = buf[0];

    struct lrt_output *st = &outputs[channel];
    uint32_t *obuf = ctl_claim_out();
  
    obuf[0] = ID_REP_STATE;
     c4c:	28 b9 00 00 	lw r25,(r5+0)
    obuf[1] = seq;
     c50:	28 d8 00 00 	lw r24,(r6+0)
    gpio_set(25);
    gpio_set(26);

    int loops =0;    

    pp_printf("RT_FD firmware initialized.");
     c54:	f8 00 04 2c 	calli 1d04 <pp_printf>
     c58:	2b 81 00 48 	lw r1,(sp+72)
     c5c:	2b 82 00 4c 	lw r2,(sp+76)
     c60:	78 13 00 00 	mvhi r19,0x0
     c64:	78 12 00 00 	mvhi r18,0x0
     c68:	78 11 00 00 	mvhi r17,0x0
     c6c:	38 21 22 c0 	ori r1,r1,0x22c0
     c70:	38 42 25 ec 	ori r2,r2,0x25ec
     c74:	3a 73 49 0c 	ori r19,r19,0x490c
     c78:	3a 52 4a 68 	ori r18,r18,0x4a68
     c7c:	3a 31 4b c4 	ori r17,r17,0x4bc4
     c80:	5b 81 00 48 	sw (sp+72),r1
     c84:	5b 82 00 4c 	sw (sp+76),r2
  return (void *) (base + MQ_IN (slot) + MQ_SLOT_DATA_START );
}

static inline uint32_t mq_poll()
{
  return *(volatile uint32_t *) ( REG_LR_POLL );
     c88:	78 14 00 10 	mvhi r20,0x10
static inline void mq_writel( int remote, uint32_t val, uint32_t reg )
{
  if(remote)
    * (volatile uint32_t * ) (RMQ_BASE + reg) = val ;
  else
    * (volatile uint32_t * ) (HMQ_BASE + reg) = val ;
     c8c:	78 1b 08 00 	mvhi fp,0x800
     c90:	b9 60 b8 00 	mv r23,r11

    
    for(;;)
    {
        do_rx();
     c94:	fb ff fe dc 	calli 804 <do_rx>
void do_outputs()
{
    int i;
    
    for(i=0;i < FD_NUM_CHANNELS;i++)
        do_output(&outputs[i]);
     c98:	ba 00 08 00 	mv r1,r16
     c9c:	fb ff fd 81 	calli 2a0 <do_output>
     ca0:	ba 60 08 00 	mv r1,r19
     ca4:	fb ff fd 7f 	calli 2a0 <do_output>
     ca8:	ba 40 08 00 	mv r1,r18
     cac:	fb ff fd 7d 	calli 2a0 <do_output>
     cb0:	ba 20 08 00 	mv r1,r17
     cb4:	fb ff fd 7b 	calli 2a0 <do_output>
  return (void *) (base + MQ_IN (slot) + MQ_SLOT_DATA_START );
}

static inline uint32_t mq_poll()
{
  return *(volatile uint32_t *) ( REG_LR_POLL );
     cb8:	2a 82 00 00 	lw r2,(r20+0)
static inline void do_control()
{
    uint32_t p = mq_poll();


    if(! ( p & ( 1<< FD_IN_SLOT_CONTROL )))
     cbc:	20 42 00 02 	andi r2,r2,0x2
     cc0:	44 40 ff f5 	be r2,r0,c94 <main+0x150>
    {                           \
        func(seq, buf + 2);     \
        break;                  \
    }

    switch(cmd)
     cc4:	78 04 40 01 	mvhi r4,0x4001
     cc8:	38 84 44 08 	ori r4,r4,0x4408
     ccc:	28 83 00 00 	lw r3,(r4+0)


    uint32_t *buf = mq_map_in_buffer( 0, FD_IN_SLOT_CONTROL );

    int cmd = buf[0];
    int seq = buf[1];
     cd0:	78 05 40 01 	mvhi r5,0x4001
     cd4:	38 a5 44 0c 	ori r5,r5,0x440c
    {                           \
        func(seq, buf + 2);     \
        break;                  \
    }

    switch(cmd)
     cd8:	34 02 00 0a 	mvi r2,10


    uint32_t *buf = mq_map_in_buffer( 0, FD_IN_SLOT_CONTROL );

    int cmd = buf[0];
    int seq = buf[1];
     cdc:	28 ad 00 00 	lw r13,(r5+0)
    {                           \
        func(seq, buf + 2);     \
        break;                  \
    }

    switch(cmd)
     ce0:	54 62 00 62 	bgu r3,r2,e68 <main+0x324>
     ce4:	2b 86 00 48 	lw r6,(sp+72)
     ce8:	3c 62 00 02 	sli r2,r3,2
     cec:	b4 c2 10 00 	add r2,r6,r2
     cf0:	28 42 00 00 	lw r2,(r2+0)
     cf4:	c0 40 00 00 	b r2


static inline void ctl_read_hash(int seq, uint32_t *buf)
{
    int bucket = buf[0];
    int pos = buf[1];
     cf8:	78 07 00 00 	mvhi r7,0x0
}


static inline void ctl_read_hash(int seq, uint32_t *buf)
{
    int bucket = buf[0];
     cfc:	2a c3 00 00 	lw r3,(r22+0)
    int pos = buf[1];
     d00:	38 e7 22 60 	ori r7,r7,0x2260
     d04:	28 e2 00 00 	lw r2,(r7+0)
    int ch = buf[2];
     d08:	78 09 00 00 	mvhi r9,0x0
     d0c:	39 29 22 64 	ori r9,r9,0x2264

    uint32_t *obuf = ctl_claim_out();

    struct lrt_hash_entry *ent = hash_get_entry (bucket, pos);
     d10:	b8 60 08 00 	mv r1,r3

static inline void ctl_read_hash(int seq, uint32_t *buf)
{
    int bucket = buf[0];
    int pos = buf[1];
    int ch = buf[2];
     d14:	29 23 00 00 	lw r3,(r9+0)


static inline void ctl_read_hash(int seq, uint32_t *buf)
{
    int bucket = buf[0];
    int pos = buf[1];
     d18:	28 42 00 00 	lw r2,(r2+0)
    int ch = buf[2];
     d1c:	28 6b 00 00 	lw r11,(r3+0)
static inline void mq_writel( int remote, uint32_t val, uint32_t reg )
{
  if(remote)
    * (volatile uint32_t * ) (RMQ_BASE + reg) = val ;
  else
    * (volatile uint32_t * ) (HMQ_BASE + reg) = val ;
     d20:	5a b7 00 00 	sw (r21+0),r23

    uint32_t *obuf = ctl_claim_out();

    struct lrt_hash_entry *ent = hash_get_entry (bucket, pos);
     d24:	f8 00 02 ec 	calli 18d4 <hash_get_entry>
    struct lrt_hash_entry *cond = NULL;
    
    obuf[0] = ID_REP_HASH_ENTRY;
     d28:	34 02 01 04 	mvi r2,260
    obuf[1] = seq;
    obuf[2] = ent ? 1 : 0;
     d2c:	7c 23 00 00 	cmpnei r3,r1,0
    uint32_t *obuf = ctl_claim_out();

    struct lrt_hash_entry *ent = hash_get_entry (bucket, pos);
    struct lrt_hash_entry *cond = NULL;
    
    obuf[0] = ID_REP_HASH_ENTRY;
     d30:	5b 22 00 00 	sw (r25+0),r2
    int pos = buf[1];
    int ch = buf[2];

    uint32_t *obuf = ctl_claim_out();

    struct lrt_hash_entry *ent = hash_get_entry (bucket, pos);
     d34:	b8 20 20 00 	mv r4,r1
    struct lrt_hash_entry *cond = NULL;
    
    obuf[0] = ID_REP_HASH_ENTRY;
    obuf[1] = seq;
    obuf[2] = ent ? 1 : 0;
     d38:	78 01 40 01 	mvhi r1,0x4001

    struct lrt_hash_entry *ent = hash_get_entry (bucket, pos);
    struct lrt_hash_entry *cond = NULL;
    
    obuf[0] = ID_REP_HASH_ENTRY;
    obuf[1] = seq;
     d3c:	5b 0d 00 00 	sw (r24+0),r13
    obuf[2] = ent ? 1 : 0;
     d40:	38 21 84 10 	ori r1,r1,0x8410
     d44:	58 23 00 00 	sw (r1+0),r3

    if(ent)
     d48:	44 80 00 44 	be r4,r0,e58 <main+0x314>
    {
        cond = (struct lrt_hash_entry *) ent->ocfg[ch].cond_ptr;
     d4c:	3d 63 00 02 	sli r3,r11,2
     d50:	3d 62 00 04 	sli r2,r11,4
        obuf[9] = (uint32_t) cond;
     d54:	78 06 40 01 	mvhi r6,0x4001
    obuf[1] = seq;
    obuf[2] = ent ? 1 : 0;

    if(ent)
    {
        cond = (struct lrt_hash_entry *) ent->ocfg[ch].cond_ptr;
     d58:	c8 43 28 00 	sub r5,r2,r3
     d5c:	b4 85 38 00 	add r7,r4,r5
     d60:	28 e3 00 14 	lw r3,(r7+20)
        obuf[9] = (uint32_t) cond;
        obuf[10] = (uint32_t) ent;
     d64:	78 02 40 01 	mvhi r2,0x4001
     d68:	38 42 84 30 	ori r2,r2,0x8430
     d6c:	58 44 00 00 	sw (r2+0),r4
    obuf[2] = ent ? 1 : 0;

    if(ent)
    {
        cond = (struct lrt_hash_entry *) ent->ocfg[ch].cond_ptr;
        obuf[9] = (uint32_t) cond;
     d70:	38 c6 84 2c 	ori r6,r6,0x842c
     d74:	58 c3 00 00 	sw (r6+0),r3
        obuf[10] = (uint32_t) ent;
        
        if(cond)
     d78:	44 60 01 a4 	be r3,r0,1408 <main+0x8c4>
    buf[2] = ts->frac;
}

static void bag_id(uint32_t *buf, struct list_id *id )
{
    buf[0] = id->system;
     d7c:	28 66 00 00 	lw r6,(r3+0)
     d80:	78 09 40 01 	mvhi r9,0x4001
        if(cond)
        {    	
       	    bag_id(obuf + 3, &cond->id);
            obuf[6] = cond->ocfg[ch].delay_cycles;
    	    obuf[7] = cond->ocfg[ch].delay_frac;
    	    obuf[8] = (ent->ocfg[ch].state & HASH_ENT_DISABLED) | HASH_ENT_CONDITION;
     d84:	34 e8 00 08 	addi r8,r7,8
    buf[2] = ts->frac;
}

static void bag_id(uint32_t *buf, struct list_id *id )
{
    buf[0] = id->system;
     d88:	39 29 84 14 	ori r9,r9,0x8414
        if(cond)
        {    	
       	    bag_id(obuf + 3, &cond->id);
            obuf[6] = cond->ocfg[ch].delay_cycles;
    	    obuf[7] = cond->ocfg[ch].delay_frac;
    	    obuf[8] = (ent->ocfg[ch].state & HASH_ENT_DISABLED) | HASH_ENT_CONDITION;
     d8c:	2d 02 00 0a 	lhu r2,(r8+10)
    buf[2] = ts->frac;
}

static void bag_id(uint32_t *buf, struct list_id *id )
{
    buf[0] = id->system;
     d90:	59 26 00 00 	sw (r9+0),r6
    buf[1] = id->source_port;
     d94:	28 66 00 04 	lw r6,(r3+4)
     d98:	78 09 40 01 	mvhi r9,0x4001
        obuf[10] = (uint32_t) ent;
        
        if(cond)
        {    	
       	    bag_id(obuf + 3, &cond->id);
            obuf[6] = cond->ocfg[ch].delay_cycles;
     d9c:	b4 65 28 00 	add r5,r3,r5
}

static void bag_id(uint32_t *buf, struct list_id *id )
{
    buf[0] = id->system;
    buf[1] = id->source_port;
     da0:	39 29 84 18 	ori r9,r9,0x8418
        
        if(cond)
        {    	
       	    bag_id(obuf + 3, &cond->id);
            obuf[6] = cond->ocfg[ch].delay_cycles;
    	    obuf[7] = cond->ocfg[ch].delay_frac;
     da4:	2c a1 00 10 	lhu r1,(r5+16)
}

static void bag_id(uint32_t *buf, struct list_id *id )
{
    buf[0] = id->system;
    buf[1] = id->source_port;
     da8:	59 26 00 00 	sw (r9+0),r6
    buf[2] = id->trigger;
     dac:	28 63 00 08 	lw r3,(r3+8)
     db0:	78 06 40 01 	mvhi r6,0x4001
     db4:	38 c6 84 1c 	ori r6,r6,0x841c
        
        if(cond)
        {    	
       	    bag_id(obuf + 3, &cond->id);
            obuf[6] = cond->ocfg[ch].delay_cycles;
    	    obuf[7] = cond->ocfg[ch].delay_frac;
     db8:	78 09 40 01 	mvhi r9,0x4001

static void bag_id(uint32_t *buf, struct list_id *id )
{
    buf[0] = id->system;
    buf[1] = id->source_port;
    buf[2] = id->trigger;
     dbc:	58 c3 00 00 	sw (r6+0),r3
        
        if(cond)
        {    	
       	    bag_id(obuf + 3, &cond->id);
            obuf[6] = cond->ocfg[ch].delay_cycles;
    	    obuf[7] = cond->ocfg[ch].delay_frac;
     dc0:	39 29 84 24 	ori r9,r9,0x8424
        obuf[10] = (uint32_t) ent;
        
        if(cond)
        {    	
       	    bag_id(obuf + 3, &cond->id);
            obuf[6] = cond->ocfg[ch].delay_cycles;
     dc4:	28 a3 00 0c 	lw r3,(r5+12)
    	    obuf[7] = cond->ocfg[ch].delay_frac;
    	    obuf[8] = (ent->ocfg[ch].state & HASH_ENT_DISABLED) | HASH_ENT_CONDITION;
     dc8:	20 42 00 08 	andi r2,r2,0x8
        
        if(cond)
        {    	
       	    bag_id(obuf + 3, &cond->id);
            obuf[6] = cond->ocfg[ch].delay_cycles;
    	    obuf[7] = cond->ocfg[ch].delay_frac;
     dcc:	59 21 00 00 	sw (r9+0),r1
    	    obuf[8] = (ent->ocfg[ch].state & HASH_ENT_DISABLED) | HASH_ENT_CONDITION;
     dd0:	78 01 40 01 	mvhi r1,0x4001
     dd4:	38 42 00 02 	ori r2,r2,0x2
     dd8:	38 21 84 28 	ori r1,r1,0x8428
     ddc:	58 22 00 00 	sw (r1+0),r2
        obuf[10] = (uint32_t) ent;
        
        if(cond)
        {    	
       	    bag_id(obuf + 3, &cond->id);
            obuf[6] = cond->ocfg[ch].delay_cycles;
     de0:	78 02 40 01 	mvhi r2,0x4001
     de4:	38 42 84 20 	ori r2,r2,0x8420
     de8:	58 43 00 00 	sw (r2+0),r3
    buf[2] = ts->frac;
}

static void bag_id(uint32_t *buf, struct list_id *id )
{
    buf[0] = id->system;
     dec:	28 82 00 00 	lw r2,(r4+0)
     df0:	78 05 40 01 	mvhi r5,0x4001
     df4:	38 a5 84 34 	ori r5,r5,0x8434
            obuf[6] = cond->ocfg[ch].delay_cycles;
    	    obuf[7] = cond->ocfg[ch].delay_frac;
    	    obuf[8] = (ent->ocfg[ch].state & HASH_ENT_DISABLED) | HASH_ENT_CONDITION;
            bag_id(obuf+11, &ent->id);
    	    obuf[14] = ent->ocfg[ch].delay_cycles;
    	    obuf[15] = ent->ocfg[ch].delay_frac;
     df8:	2d 03 00 08 	lhu r3,(r8+8)
    buf[2] = ts->frac;
}

static void bag_id(uint32_t *buf, struct list_id *id )
{
    buf[0] = id->system;
     dfc:	58 a2 00 00 	sw (r5+0),r2
    buf[1] = id->source_port;
     e00:	28 82 00 04 	lw r2,(r4+4)
     e04:	78 06 40 01 	mvhi r6,0x4001
     e08:	38 c6 84 38 	ori r6,r6,0x8438
     e0c:	58 c2 00 00 	sw (r6+0),r2
    buf[2] = id->trigger;
     e10:	28 82 00 08 	lw r2,(r4+8)
     e14:	78 09 40 01 	mvhi r9,0x4001
            obuf[6] = cond->ocfg[ch].delay_cycles;
    	    obuf[7] = cond->ocfg[ch].delay_frac;
    	    obuf[8] = (ent->ocfg[ch].state & HASH_ENT_DISABLED) | HASH_ENT_CONDITION;
            bag_id(obuf+11, &ent->id);
    	    obuf[14] = ent->ocfg[ch].delay_cycles;
    	    obuf[15] = ent->ocfg[ch].delay_frac;
     e18:	78 04 00 00 	mvhi r4,0x0

static void bag_id(uint32_t *buf, struct list_id *id )
{
    buf[0] = id->system;
    buf[1] = id->source_port;
    buf[2] = id->trigger;
     e1c:	39 29 84 3c 	ori r9,r9,0x843c
            obuf[6] = cond->ocfg[ch].delay_cycles;
    	    obuf[7] = cond->ocfg[ch].delay_frac;
    	    obuf[8] = (ent->ocfg[ch].state & HASH_ENT_DISABLED) | HASH_ENT_CONDITION;
            bag_id(obuf+11, &ent->id);
    	    obuf[14] = ent->ocfg[ch].delay_cycles;
    	    obuf[15] = ent->ocfg[ch].delay_frac;
     e20:	38 84 22 80 	ori r4,r4,0x2280

static void bag_id(uint32_t *buf, struct list_id *id )
{
    buf[0] = id->system;
    buf[1] = id->source_port;
    buf[2] = id->trigger;
     e24:	59 22 00 00 	sw (r9+0),r2
            obuf[6] = cond->ocfg[ch].delay_cycles;
    	    obuf[7] = cond->ocfg[ch].delay_frac;
    	    obuf[8] = (ent->ocfg[ch].state & HASH_ENT_DISABLED) | HASH_ENT_CONDITION;
            bag_id(obuf+11, &ent->id);
    	    obuf[14] = ent->ocfg[ch].delay_cycles;
    	    obuf[15] = ent->ocfg[ch].delay_frac;
     e28:	28 82 00 00 	lw r2,(r4+0)
    	    obuf[16] = HASH_ENT_CONDITIONAL;
     e2c:	78 05 00 00 	mvhi r5,0x0
     e30:	38 a5 22 84 	ori r5,r5,0x2284
       	    bag_id(obuf + 3, &cond->id);
            obuf[6] = cond->ocfg[ch].delay_cycles;
    	    obuf[7] = cond->ocfg[ch].delay_frac;
    	    obuf[8] = (ent->ocfg[ch].state & HASH_ENT_DISABLED) | HASH_ENT_CONDITION;
            bag_id(obuf+11, &ent->id);
    	    obuf[14] = ent->ocfg[ch].delay_cycles;
     e34:	28 e1 00 0c 	lw r1,(r7+12)
    	    obuf[15] = ent->ocfg[ch].delay_frac;
     e38:	58 43 00 00 	sw (r2+0),r3
    	    obuf[16] = HASH_ENT_CONDITIONAL;
     e3c:	28 a2 00 00 	lw r2,(r5+0)
       	    bag_id(obuf + 3, &cond->id);
            obuf[6] = cond->ocfg[ch].delay_cycles;
    	    obuf[7] = cond->ocfg[ch].delay_frac;
    	    obuf[8] = (ent->ocfg[ch].state & HASH_ENT_DISABLED) | HASH_ENT_CONDITION;
            bag_id(obuf+11, &ent->id);
    	    obuf[14] = ent->ocfg[ch].delay_cycles;
     e40:	78 06 00 00 	mvhi r6,0x0
    	    obuf[15] = ent->ocfg[ch].delay_frac;
    	    obuf[16] = HASH_ENT_CONDITIONAL;
     e44:	34 03 00 04 	mvi r3,4
       	    bag_id(obuf + 3, &cond->id);
            obuf[6] = cond->ocfg[ch].delay_cycles;
    	    obuf[7] = cond->ocfg[ch].delay_frac;
    	    obuf[8] = (ent->ocfg[ch].state & HASH_ENT_DISABLED) | HASH_ENT_CONDITION;
            bag_id(obuf+11, &ent->id);
    	    obuf[14] = ent->ocfg[ch].delay_cycles;
     e48:	38 c6 22 88 	ori r6,r6,0x2288
    	    obuf[15] = ent->ocfg[ch].delay_frac;
    	    obuf[16] = HASH_ENT_CONDITIONAL;
     e4c:	58 43 00 00 	sw (r2+0),r3
       	    bag_id(obuf + 3, &cond->id);
            obuf[6] = cond->ocfg[ch].delay_cycles;
    	    obuf[7] = cond->ocfg[ch].delay_frac;
    	    obuf[8] = (ent->ocfg[ch].state & HASH_ENT_DISABLED) | HASH_ENT_CONDITION;
            bag_id(obuf+11, &ent->id);
    	    obuf[14] = ent->ocfg[ch].delay_cycles;
     e50:	28 c2 00 00 	lw r2,(r6+0)
     e54:	58 41 00 00 	sw (r2+0),r1
     e58:	78 09 00 00 	mvhi r9,0x0
     e5c:	39 29 22 8c 	ori r9,r9,0x228c
     e60:	29 22 00 00 	lw r2,(r9+0)
     e64:	5a a2 00 00 	sw (r21+0),r2
     e68:	78 09 40 01 	mvhi r9,0x4001
     e6c:	39 29 44 00 	ori r9,r9,0x4400
     e70:	59 3b 00 00 	sw (r9+0),fp
     e74:	e3 ff ff 88 	bi c94 <main+0x150>
        do_rx();
        do_outputs();
        do_control();
    }

     e78:	2a c2 00 00 	lw r2,(r22+0)
     e7c:	78 09 40 01 	mvhi r9,0x4001
     e80:	39 29 44 00 	ori r9,r9,0x4400
{
    int channel = buf[0];
    struct lrt_output *st = &outputs[channel];

    st->flags |= LIST_ARMED;
    st->flags &= ~LIST_TRIGGERED;
     e84:	08 42 01 5c 	muli r2,r2,348
     e88:	b6 02 20 00 	add r4,r16,r2
static inline void ctl_chan_arm (int seq, uint32_t *buf)
{
    int channel = buf[0];
    struct lrt_output *st = &outputs[channel];

    st->flags |= LIST_ARMED;
     e8c:	28 83 00 78 	lw r3,(r4+120)
    st->flags &= ~LIST_TRIGGERED;
     e90:	34 02 ff ef 	mvi r2,-17
static inline void ctl_chan_arm (int seq, uint32_t *buf)
{
    int channel = buf[0];
    struct lrt_output *st = &outputs[channel];

    st->flags |= LIST_ARMED;
     e94:	38 63 00 08 	ori r3,r3,0x8
    st->flags &= ~LIST_TRIGGERED;
     e98:	a0 62 10 00 	and r2,r3,r2
     e9c:	58 82 00 78 	sw (r4+120),r2
     ea0:	78 04 00 00 	mvhi r4,0x0
     ea4:	5a b7 00 00 	sw (r21+0),r23
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     ea8:	34 02 01 00 	mvi r2,256
     eac:	38 84 22 7c 	ori r4,r4,0x227c
     eb0:	5b 22 00 00 	sw (r25+0),r2
     eb4:	28 82 00 00 	lw r2,(r4+0)
    buf[1] = seq;
     eb8:	5b 0d 00 00 	sw (r24+0),r13
     ebc:	5a a2 00 00 	sw (r21+0),r2
     ec0:	59 3b 00 00 	sw (r9+0),fp
     ec4:	e3 ff ff 74 	bi c94 <main+0x150>
    ctl_ack(seq);
}

static inline void ctl_chan_set_mode (int seq, uint32_t *buf)
{
    int channel = buf[0];
     ec8:	2a c2 00 00 	lw r2,(r22+0)
    struct lrt_output *st = &outputs[channel];

    st->mode = buf[1];
     ecc:	78 01 00 00 	mvhi r1,0x0
     ed0:	38 21 22 60 	ori r1,r1,0x2260
     ed4:	08 43 01 5c 	muli r3,r2,348
     ed8:	28 22 00 00 	lw r2,(r1+0)
     edc:	78 09 40 01 	mvhi r9,0x4001
     ee0:	b6 03 18 00 	add r3,r16,r3
    st->flags &= ~(LIST_ARMED | LIST_TRIGGERED | LIST_LAST_VALID) ;
     ee4:	28 65 00 78 	lw r5,(r3+120)
static inline void ctl_chan_set_mode (int seq, uint32_t *buf)
{
    int channel = buf[0];
    struct lrt_output *st = &outputs[channel];

    st->mode = buf[1];
     ee8:	28 44 00 00 	lw r4,(r2+0)
    st->flags &= ~(LIST_ARMED | LIST_TRIGGERED | LIST_LAST_VALID) ;
     eec:	34 02 ff e3 	mvi r2,-29
     ef0:	a0 a2 10 00 	and r2,r5,r2
     ef4:	58 62 00 78 	sw (r3+120),r2
static inline void ctl_chan_set_mode (int seq, uint32_t *buf)
{
    int channel = buf[0];
    struct lrt_output *st = &outputs[channel];

    st->mode = buf[1];
     ef8:	58 64 00 74 	sw (r3+116),r4
     efc:	78 03 00 00 	mvhi r3,0x0
     f00:	5a b7 00 00 	sw (r21+0),r23
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     f04:	34 02 01 00 	mvi r2,256
     f08:	38 63 22 7c 	ori r3,r3,0x227c
     f0c:	5b 22 00 00 	sw (r25+0),r2
     f10:	28 62 00 00 	lw r2,(r3+0)
    buf[1] = seq;
     f14:	5b 0d 00 00 	sw (r24+0),r13
     f18:	39 29 44 00 	ori r9,r9,0x4400
     f1c:	5a a2 00 00 	sw (r21+0),r2
     f20:	59 3b 00 00 	sw (r9+0),fp
     f24:	e3 ff ff 5c 	bi c94 <main+0x150>

static inline void ctl_chan_set_delay (int seq, uint32_t *buf)
{
    int channel = buf[0];
    
    struct lrt_hash_entry *ent = (struct lrt_hash_entry *) buf[1];
     f28:	78 01 00 00 	mvhi r1,0x0
     f2c:	38 21 22 60 	ori r1,r1,0x2260
    ctl_ack(seq);
}

static inline void ctl_chan_set_delay (int seq, uint32_t *buf)
{
    int channel = buf[0];
     f30:	2a c2 00 00 	lw r2,(r22+0)
    
    struct lrt_hash_entry *ent = (struct lrt_hash_entry *) buf[1];
     f34:	28 23 00 00 	lw r3,(r1+0)
    
    ent->ocfg[channel].delay_cycles = buf[2];
     f38:	78 04 00 00 	mvhi r4,0x0
     f3c:	38 84 22 64 	ori r4,r4,0x2264

static inline void ctl_chan_set_delay (int seq, uint32_t *buf)
{
    int channel = buf[0];
    
    struct lrt_hash_entry *ent = (struct lrt_hash_entry *) buf[1];
     f40:	28 65 00 00 	lw r5,(r3+0)
    
    ent->ocfg[channel].delay_cycles = buf[2];
     f44:	3c 46 00 02 	sli r6,r2,2
     f48:	28 83 00 00 	lw r3,(r4+0)
     f4c:	3c 42 00 04 	sli r2,r2,4
     f50:	78 09 40 01 	mvhi r9,0x4001
     f54:	c8 46 10 00 	sub r2,r2,r6
     f58:	28 64 00 00 	lw r4,(r3+0)
     f5c:	b4 a2 18 00 	add r3,r5,r2
    ent->ocfg[channel].delay_frac = buf[3];
     f60:	78 05 00 00 	mvhi r5,0x0
     f64:	38 a5 22 68 	ori r5,r5,0x2268
     f68:	28 a2 00 00 	lw r2,(r5+0)
{
    int channel = buf[0];
    
    struct lrt_hash_entry *ent = (struct lrt_hash_entry *) buf[1];
    
    ent->ocfg[channel].delay_cycles = buf[2];
     f6c:	58 64 00 0c 	sw (r3+12),r4
     f70:	78 06 00 00 	mvhi r6,0x0
    ent->ocfg[channel].delay_frac = buf[3];
     f74:	28 42 00 00 	lw r2,(r2+0)
     f78:	38 c6 22 7c 	ori r6,r6,0x227c
     f7c:	39 29 44 00 	ori r9,r9,0x4400
     f80:	0c 62 00 10 	sh (r3+16),r2
     f84:	5a b7 00 00 	sw (r21+0),r23
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     f88:	34 02 01 00 	mvi r2,256
     f8c:	5b 22 00 00 	sw (r25+0),r2
     f90:	28 c2 00 00 	lw r2,(r6+0)
    buf[1] = seq;
     f94:	5b 0d 00 00 	sw (r24+0),r13
     f98:	5a a2 00 00 	sw (r21+0),r2
     f9c:	59 3b 00 00 	sw (r9+0),fp
     fa0:	e3 ff ff 3d 	bi c94 <main+0x150>
        do_rx();
        do_outputs();
        do_control();
    }

     fa4:	2a c7 00 00 	lw r7,(r22+0)
    int channel = buf[0];

    struct lrt_output *st = &outputs[channel];
    uint32_t *obuf = ctl_claim_out();
  
    obuf[0] = ID_REP_STATE;
     fa8:	34 03 01 01 	mvi r3,257
     fac:	5a b7 00 00 	sw (r21+0),r23
    obuf[1] = seq;
    obuf[2] = channel;
    obuf[3] = st->hits;
     fb0:	08 e2 01 5c 	muli r2,r7,348
    int channel = buf[0];

    struct lrt_output *st = &outputs[channel];
    uint32_t *obuf = ctl_claim_out();
  
    obuf[0] = ID_REP_STATE;
     fb4:	5b 23 00 00 	sw (r25+0),r3
    obuf[1] = seq;
    obuf[2] = channel;
     fb8:	78 09 40 01 	mvhi r9,0x4001
    obuf[3] = st->hits;
     fbc:	b6 02 10 00 	add r2,r16,r2
     fc0:	28 46 00 08 	lw r6,(r2+8)

    struct lrt_output *st = &outputs[channel];
    uint32_t *obuf = ctl_claim_out();
  
    obuf[0] = ID_REP_STATE;
    obuf[1] = seq;
     fc4:	5b 0d 00 00 	sw (r24+0),r13
    obuf[2] = channel;
     fc8:	39 29 84 10 	ori r9,r9,0x8410
    obuf[3] = st->hits;
     fcc:	78 01 40 01 	mvhi r1,0x4001
    obuf[4] = st->miss_timeout;
     fd0:	28 45 00 0c 	lw r5,(r2+12)
    obuf[5] = st->miss_deadtime; 
     fd4:	28 44 00 10 	lw r4,(r2+16)
    obuf[6] = st->miss_overflow;
     fd8:	28 43 00 14 	lw r3,(r2+20)
    uint32_t *obuf = ctl_claim_out();
  
    obuf[0] = ID_REP_STATE;
    obuf[1] = seq;
    obuf[2] = channel;
    obuf[3] = st->hits;
     fdc:	38 21 84 14 	ori r1,r1,0x8414
    struct lrt_output *st = &outputs[channel];
    uint32_t *obuf = ctl_claim_out();
  
    obuf[0] = ID_REP_STATE;
    obuf[1] = seq;
    obuf[2] = channel;
     fe0:	59 27 00 00 	sw (r9+0),r7
    obuf[3] = st->hits;
     fe4:	58 26 00 00 	sw (r1+0),r6
    obuf[4] = st->miss_timeout;
     fe8:	78 06 40 01 	mvhi r6,0x4001
     fec:	38 c6 84 18 	ori r6,r6,0x8418
    obuf[5] = st->miss_deadtime; 
     ff0:	78 07 40 01 	mvhi r7,0x4001
  
    obuf[0] = ID_REP_STATE;
    obuf[1] = seq;
    obuf[2] = channel;
    obuf[3] = st->hits;
    obuf[4] = st->miss_timeout;
     ff4:	58 c5 00 00 	sw (r6+0),r5
    obuf[5] = st->miss_deadtime; 
     ff8:	38 e7 84 1c 	ori r7,r7,0x841c
    obuf[6] = st->miss_overflow;
     ffc:	78 09 40 01 	mvhi r9,0x4001
    obuf[0] = ID_REP_STATE;
    obuf[1] = seq;
    obuf[2] = channel;
    obuf[3] = st->hits;
    obuf[4] = st->miss_timeout;
    obuf[5] = st->miss_deadtime; 
    1000:	58 e4 00 00 	sw (r7+0),r4
    obuf[6] = st->miss_overflow;
    1004:	39 29 84 20 	ori r9,r9,0x8420
    1008:	59 23 00 00 	sw (r9+0),r3
    buf[2] = ts->frac;
}

static void bag_id(uint32_t *buf, struct list_id *id )
{
    buf[0] = id->system;
    100c:	28 43 00 18 	lw r3,(r2+24)
    1010:	78 01 40 01 	mvhi r1,0x4001
    1014:	38 21 84 24 	ori r1,r1,0x8424
    1018:	58 23 00 00 	sw (r1+0),r3
    buf[1] = id->source_port;
    101c:	28 43 00 1c 	lw r3,(r2+28)
    1020:	78 04 40 01 	mvhi r4,0x4001
    1024:	38 84 84 28 	ori r4,r4,0x8428
    1028:	58 83 00 00 	sw (r4+0),r3
    buf[2] = id->trigger;
    102c:	28 43 00 20 	lw r3,(r2+32)
    1030:	78 05 40 01 	mvhi r5,0x4001
    1034:	38 a5 84 2c 	ori r5,r5,0x842c
    1038:	58 a3 00 00 	sw (r5+0),r3
    ctl_ack(seq);
}

static void bag_timestamp(uint32_t *buf, struct list_timestamp *ts )
{
    buf[0] = ts->seconds;
    103c:	28 43 00 48 	lw r3,(r2+72)
    1040:	78 06 40 01 	mvhi r6,0x4001
    1044:	38 c6 84 30 	ori r6,r6,0x8430
    1048:	58 c3 00 00 	sw (r6+0),r3
    buf[1] = ts->cycles;
    104c:	28 43 00 4c 	lw r3,(r2+76)
    1050:	78 07 40 01 	mvhi r7,0x4001
    1054:	38 e7 84 34 	ori r7,r7,0x8434
    1058:	58 e3 00 00 	sw (r7+0),r3
    buf[2] = ts->frac;
    105c:	28 43 00 50 	lw r3,(r2+80)
    1060:	78 09 40 01 	mvhi r9,0x4001
    1064:	39 29 84 38 	ori r9,r9,0x8438
    1068:	59 23 00 00 	sw (r9+0),r3
    ctl_ack(seq);
}

static void bag_timestamp(uint32_t *buf, struct list_timestamp *ts )
{
    buf[0] = ts->seconds;
    106c:	28 43 00 54 	lw r3,(r2+84)
    1070:	78 01 40 01 	mvhi r1,0x4001
    1074:	38 21 84 3c 	ori r1,r1,0x843c
    buf[1] = ts->cycles;
    1078:	78 05 00 00 	mvhi r5,0x0
    ctl_ack(seq);
}

static void bag_timestamp(uint32_t *buf, struct list_timestamp *ts )
{
    buf[0] = ts->seconds;
    107c:	58 23 00 00 	sw (r1+0),r3
    buf[1] = ts->cycles;
    1080:	38 a5 22 88 	ori r5,r5,0x2288
    1084:	28 44 00 58 	lw r4,(r2+88)
    1088:	28 a3 00 00 	lw r3,(r5+0)
    buf[2] = ts->frac;
    108c:	78 06 00 00 	mvhi r6,0x0
    1090:	38 c6 22 80 	ori r6,r6,0x2280
}

static void bag_timestamp(uint32_t *buf, struct list_timestamp *ts )
{
    buf[0] = ts->seconds;
    buf[1] = ts->cycles;
    1094:	58 64 00 00 	sw (r3+0),r4
    buf[2] = ts->frac;
    1098:	28 44 00 5c 	lw r4,(r2+92)
    109c:	28 c3 00 00 	lw r3,(r6+0)
    ctl_ack(seq);
}

static void bag_timestamp(uint32_t *buf, struct list_timestamp *ts )
{
    buf[0] = ts->seconds;
    10a0:	78 07 00 00 	mvhi r7,0x0
    10a4:	38 e7 22 84 	ori r7,r7,0x2284
    buf[1] = ts->cycles;
    buf[2] = ts->frac;
    10a8:	58 64 00 00 	sw (r3+0),r4
    ctl_ack(seq);
}

static void bag_timestamp(uint32_t *buf, struct list_timestamp *ts )
{
    buf[0] = ts->seconds;
    10ac:	28 44 00 60 	lw r4,(r2+96)
    10b0:	28 e3 00 00 	lw r3,(r7+0)
    buf[1] = ts->cycles;
    10b4:	78 09 00 00 	mvhi r9,0x0
    10b8:	39 29 22 90 	ori r9,r9,0x2290
    ctl_ack(seq);
}

static void bag_timestamp(uint32_t *buf, struct list_timestamp *ts )
{
    buf[0] = ts->seconds;
    10bc:	58 64 00 00 	sw (r3+0),r4
    buf[1] = ts->cycles;
    10c0:	28 44 00 64 	lw r4,(r2+100)
    10c4:	29 23 00 00 	lw r3,(r9+0)
    buf[2] = ts->frac;
    10c8:	78 07 00 00 	mvhi r7,0x0
    10cc:	38 e7 22 94 	ori r7,r7,0x2294
}

static void bag_timestamp(uint32_t *buf, struct list_timestamp *ts )
{
    buf[0] = ts->seconds;
    buf[1] = ts->cycles;
    10d0:	58 64 00 00 	sw (r3+0),r4
    buf[2] = ts->frac;
    10d4:	28 44 00 68 	lw r4,(r2+104)
    10d8:	28 e3 00 00 	lw r3,(r7+0)
    bag_timestamp(obuf + 16, &st->l3);

//    bag_timestamp(obuf + 10, &st->last_executed);
//    bag_timestamp(obuf + 13, &st->last_enqueued);
//    bag_timestamp(obuf + 16, &st->last_programmed);
    obuf[19] = st->idle;
    10dc:	78 09 00 00 	mvhi r9,0x0
    10e0:	39 29 22 98 	ori r9,r9,0x2298
    10e4:	28 45 00 6c 	lw r5,(r2+108)
    obuf[20] = st->state;
    10e8:	28 41 00 70 	lw r1,(r2+112)
    obuf[21] = st->mode;
    10ec:	28 46 00 74 	lw r6,(r2+116)

static void bag_timestamp(uint32_t *buf, struct list_timestamp *ts )
{
    buf[0] = ts->seconds;
    buf[1] = ts->cycles;
    buf[2] = ts->frac;
    10f0:	58 64 00 00 	sw (r3+0),r4
    bag_timestamp(obuf + 16, &st->l3);

//    bag_timestamp(obuf + 10, &st->last_executed);
//    bag_timestamp(obuf + 13, &st->last_enqueued);
//    bag_timestamp(obuf + 16, &st->last_programmed);
    obuf[19] = st->idle;
    10f4:	29 23 00 00 	lw r3,(r9+0)
    obuf[20] = st->state;
    10f8:	78 04 00 00 	mvhi r4,0x0
    10fc:	38 84 22 9c 	ori r4,r4,0x229c
    bag_timestamp(obuf + 16, &st->l3);

//    bag_timestamp(obuf + 10, &st->last_executed);
//    bag_timestamp(obuf + 13, &st->last_enqueued);
//    bag_timestamp(obuf + 16, &st->last_programmed);
    obuf[19] = st->idle;
    1100:	58 65 00 00 	sw (r3+0),r5
    obuf[20] = st->state;
    1104:	28 83 00 00 	lw r3,(r4+0)
    obuf[21] = st->mode;
    1108:	78 05 00 00 	mvhi r5,0x0
    110c:	38 a5 22 a0 	ori r5,r5,0x22a0

//    bag_timestamp(obuf + 10, &st->last_executed);
//    bag_timestamp(obuf + 13, &st->last_enqueued);
//    bag_timestamp(obuf + 16, &st->last_programmed);
    obuf[19] = st->idle;
    obuf[20] = st->state;
    1110:	58 61 00 00 	sw (r3+0),r1
    obuf[21] = st->mode;
    1114:	28 a3 00 00 	lw r3,(r5+0)
    obuf[22] = st->flags;
    1118:	78 07 00 00 	mvhi r7,0x0
    111c:	38 e7 22 a4 	ori r7,r7,0x22a4
//    bag_timestamp(obuf + 10, &st->last_executed);
//    bag_timestamp(obuf + 13, &st->last_enqueued);
//    bag_timestamp(obuf + 16, &st->last_programmed);
    obuf[19] = st->idle;
    obuf[20] = st->state;
    obuf[21] = st->mode;
    1120:	58 66 00 00 	sw (r3+0),r6
    obuf[22] = st->flags;
    1124:	28 44 00 78 	lw r4,(r2+120)
    1128:	28 e3 00 00 	lw r3,(r7+0)
    obuf[23] = st->log_level;
    obuf[24] = st->dead_time;
    obuf[25] = st->width_cycles;
    obuf[26] = st->worst_latency;
    obuf[27] = rx_total;
    112c:	2b 89 00 4c 	lw r9,(sp+76)
//    bag_timestamp(obuf + 16, &st->last_programmed);
    obuf[19] = st->idle;
    obuf[20] = st->state;
    obuf[21] = st->mode;
    obuf[22] = st->flags;
    obuf[23] = st->log_level;
    1130:	78 07 00 00 	mvhi r7,0x0
    obuf[24] = st->dead_time;
    1134:	28 45 00 80 	lw r5,(r2+128)
    obuf[25] = st->width_cycles;
    1138:	28 46 00 84 	lw r6,(r2+132)
//    bag_timestamp(obuf + 16, &st->last_programmed);
    obuf[19] = st->idle;
    obuf[20] = st->state;
    obuf[21] = st->mode;
    obuf[22] = st->flags;
    obuf[23] = st->log_level;
    113c:	38 e7 22 a8 	ori r7,r7,0x22a8
//    bag_timestamp(obuf + 13, &st->last_enqueued);
//    bag_timestamp(obuf + 16, &st->last_programmed);
    obuf[19] = st->idle;
    obuf[20] = st->state;
    obuf[21] = st->mode;
    obuf[22] = st->flags;
    1140:	58 64 00 00 	sw (r3+0),r4
    obuf[23] = st->log_level;
    1144:	28 43 00 7c 	lw r3,(r2+124)
    obuf[24] = st->dead_time;
    obuf[25] = st->width_cycles;
    obuf[26] = st->worst_latency;
    1148:	28 44 01 54 	lw r4,(r2+340)
//    bag_timestamp(obuf + 16, &st->last_programmed);
    obuf[19] = st->idle;
    obuf[20] = st->state;
    obuf[21] = st->mode;
    obuf[22] = st->flags;
    obuf[23] = st->log_level;
    114c:	28 e2 00 00 	lw r2,(r7+0)
    obuf[24] = st->dead_time;
    obuf[25] = st->width_cycles;
    obuf[26] = st->worst_latency;
    obuf[27] = rx_total;
    1150:	29 21 00 00 	lw r1,(r9+0)
    obuf[19] = st->idle;
    obuf[20] = st->state;
    obuf[21] = st->mode;
    obuf[22] = st->flags;
    obuf[23] = st->log_level;
    obuf[24] = st->dead_time;
    1154:	78 09 00 00 	mvhi r9,0x0
    1158:	39 29 22 ac 	ori r9,r9,0x22ac
//    bag_timestamp(obuf + 16, &st->last_programmed);
    obuf[19] = st->idle;
    obuf[20] = st->state;
    obuf[21] = st->mode;
    obuf[22] = st->flags;
    obuf[23] = st->log_level;
    115c:	58 43 00 00 	sw (r2+0),r3
    obuf[24] = st->dead_time;
    1160:	29 22 00 00 	lw r2,(r9+0)
    obuf[25] = st->width_cycles;
    1164:	78 03 00 00 	mvhi r3,0x0
    1168:	38 63 22 b0 	ori r3,r3,0x22b0
    obuf[19] = st->idle;
    obuf[20] = st->state;
    obuf[21] = st->mode;
    obuf[22] = st->flags;
    obuf[23] = st->log_level;
    obuf[24] = st->dead_time;
    116c:	58 45 00 00 	sw (r2+0),r5
    obuf[25] = st->width_cycles;
    1170:	28 62 00 00 	lw r2,(r3+0)
    obuf[26] = st->worst_latency;
    1174:	78 05 00 00 	mvhi r5,0x0
    1178:	38 a5 22 b4 	ori r5,r5,0x22b4
    obuf[20] = st->state;
    obuf[21] = st->mode;
    obuf[22] = st->flags;
    obuf[23] = st->log_level;
    obuf[24] = st->dead_time;
    obuf[25] = st->width_cycles;
    117c:	58 46 00 00 	sw (r2+0),r6
    obuf[26] = st->worst_latency;
    1180:	28 a2 00 00 	lw r2,(r5+0)
    obuf[27] = rx_total;
    1184:	78 06 00 00 	mvhi r6,0x0
    1188:	38 c6 22 b8 	ori r6,r6,0x22b8
    obuf[21] = st->mode;
    obuf[22] = st->flags;
    obuf[23] = st->log_level;
    obuf[24] = st->dead_time;
    obuf[25] = st->width_cycles;
    obuf[26] = st->worst_latency;
    118c:	58 44 00 00 	sw (r2+0),r4
    obuf[27] = rx_total;
    1190:	28 c2 00 00 	lw r2,(r6+0)
    1194:	78 07 00 00 	mvhi r7,0x0
    1198:	38 e7 22 bc 	ori r7,r7,0x22bc
    119c:	58 41 00 00 	sw (r2+0),r1
    11a0:	28 e2 00 00 	lw r2,(r7+0)
    11a4:	78 09 40 01 	mvhi r9,0x4001
    11a8:	39 29 44 00 	ori r9,r9,0x4400
    11ac:	5a a2 00 00 	sw (r21+0),r2
    11b0:	59 3b 00 00 	sw (r9+0),fp
    11b4:	e3 ff fe b8 	bi c94 <main+0x150>
static inline void ctl_chan_remove_trigger (int seq, uint32_t *buf)
{
    struct lrt_trigger_handle handle;
    
    handle.channel = buf[0];
    handle.cond = (struct lrt_hash_entry *) buf[1];
    11b8:	78 06 00 00 	mvhi r6,0x0
    11bc:	38 c6 22 60 	ori r6,r6,0x2260
    11c0:	28 c3 00 00 	lw r3,(r6+0)
    handle.trig = (struct lrt_hash_entry *) buf[2];
    11c4:	78 07 00 00 	mvhi r7,0x0
    11c8:	38 e7 22 64 	ori r7,r7,0x2264
static inline void ctl_chan_remove_trigger (int seq, uint32_t *buf)
{
    struct lrt_trigger_handle handle;
    
    handle.channel = buf[0];
    handle.cond = (struct lrt_hash_entry *) buf[1];
    11cc:	28 64 00 00 	lw r4,(r3+0)
    handle.trig = (struct lrt_hash_entry *) buf[2];
    11d0:	28 e3 00 00 	lw r3,(r7+0)

static inline void ctl_chan_remove_trigger (int seq, uint32_t *buf)
{
    struct lrt_trigger_handle handle;
    
    handle.channel = buf[0];
    11d4:	2a cc 00 00 	lw r12,(r22+0)
    handle.cond = (struct lrt_hash_entry *) buf[1];
    handle.trig = (struct lrt_hash_entry *) buf[2];
    11d8:	28 6b 00 00 	lw r11,(r3+0)

    if(handle.cond)
    11dc:	44 80 00 04 	be r4,r0,11ec <main+0x6a8>
	   hash_remove(handle.cond, handle.channel);
    11e0:	b8 80 08 00 	mv r1,r4
    11e4:	b9 80 10 00 	mv r2,r12
    11e8:	f8 00 01 83 	calli 17f4 <hash_remove>
    hash_remove(handle.trig, handle.channel);
    11ec:	b9 80 10 00 	mv r2,r12
    11f0:	b9 60 08 00 	mv r1,r11
    11f4:	f8 00 01 80 	calli 17f4 <hash_remove>
    11f8:	78 09 00 00 	mvhi r9,0x0
    11fc:	5a b7 00 00 	sw (r21+0),r23
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
    1200:	34 02 01 00 	mvi r2,256
    1204:	39 29 22 7c 	ori r9,r9,0x227c
    1208:	5b 22 00 00 	sw (r25+0),r2
    120c:	29 22 00 00 	lw r2,(r9+0)
    buf[1] = seq;
    1210:	5b 0d 00 00 	sw (r24+0),r13
    1214:	78 09 40 01 	mvhi r9,0x4001
    1218:	5a a2 00 00 	sw (r21+0),r2
    121c:	39 29 44 00 	ori r9,r9,0x4400
    1220:	59 3b 00 00 	sw (r9+0),fp
    1224:	e3 ff fe 9c 	bi c94 <main+0x150>
{
    struct list_id id;
    struct lrt_output_rule rule;
    struct lrt_trigger_handle handle;
    int ch = buf[0];
    int is_cond = buf[4];
    1228:	78 07 00 00 	mvhi r7,0x0
    122c:	38 e7 22 58 	ori r7,r7,0x2258
    1230:	28 e3 00 00 	lw r3,(r7+0)
static inline void ctl_chan_assign_trigger (int seq, uint32_t *buf)
{
    struct list_id id;
    struct lrt_output_rule rule;
    struct lrt_trigger_handle handle;
    int ch = buf[0];
    1234:	2a ce 00 00 	lw r14,(r22+0)
    int is_cond = buf[4];
    1238:	28 6c 00 00 	lw r12,(r3+0)
    int n_req = is_cond ? 2 : 1;
    123c:	34 03 00 02 	mvi r3,2
    1240:	65 8b 00 00 	cmpei r11,r12,0
    1244:	c8 6b 58 00 	sub r11,r3,r11


//    pp_printf("assign ch %d, fc %d", ch, hash_free_count());

    if(hash_free_count() < n_req)
    1248:	f8 00 01 b8 	calli 1928 <hash_free_count>
    124c:	4c 2b 00 11 	bge r1,r11,1290 <main+0x74c>
    1250:	5a b7 00 00 	sw (r21+0),r23
}

static inline void ctl_nack( uint32_t seq, int err )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_NACK;
    1254:	34 02 01 02 	mvi r2,258
    1258:	5b 22 00 00 	sw (r25+0),r2
    buf[1] = seq;
    buf[2] = err;
    125c:	78 09 40 01 	mvhi r9,0x4001
    1260:	78 01 00 00 	mvhi r1,0x0

static inline void ctl_nack( uint32_t seq, int err )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_NACK;
    buf[1] = seq;
    1264:	5b 0d 00 00 	sw (r24+0),r13
    buf[2] = err;
    1268:	34 02 ff ff 	mvi r2,-1
    126c:	39 29 84 10 	ori r9,r9,0x8410
    1270:	38 21 22 5c 	ori r1,r1,0x225c
    1274:	59 22 00 00 	sw (r9+0),r2
    1278:	28 22 00 00 	lw r2,(r1+0)
    127c:	78 09 40 01 	mvhi r9,0x4001
    1280:	39 29 44 00 	ori r9,r9,0x4400
    1284:	5a a2 00 00 	sw (r21+0),r2
    1288:	59 3b 00 00 	sw (r9+0),fp
    128c:	e3 ff fe 82 	bi c94 <main+0x150>
    {
        ctl_nack(seq, -1);
        return;
    }

    id.system = buf[1];
    1290:	78 02 00 00 	mvhi r2,0x0
    1294:	38 42 22 60 	ori r2,r2,0x2260
    1298:	28 43 00 00 	lw r3,(r2+0)
    id.source_port = buf[2];
    129c:	78 04 00 00 	mvhi r4,0x0
    12a0:	38 84 22 64 	ori r4,r4,0x2264
    {
        ctl_nack(seq, -1);
        return;
    }

    id.system = buf[1];
    12a4:	28 63 00 00 	lw r3,(r3+0)
    id.source_port = buf[2];
    id.trigger = buf[3];
    12a8:	78 05 00 00 	mvhi r5,0x0
    12ac:	38 a5 22 68 	ori r5,r5,0x2268
    {
        ctl_nack(seq, -1);
        return;
    }

    id.system = buf[1];
    12b0:	5b 83 00 5c 	sw (sp+92),r3
    id.source_port = buf[2];
    12b4:	28 83 00 00 	lw r3,(r4+0)
    id.trigger = buf[3];

    rule.delay_cycles = 100000000 / 8000;
    12b8:	34 0f 30 d4 	mvi r15,12500
        ctl_nack(seq, -1);
        return;
    }

    id.system = buf[1];
    id.source_port = buf[2];
    12bc:	28 63 00 00 	lw r3,(r3+0)
    12c0:	5b 83 00 60 	sw (sp+96),r3
    id.trigger = buf[3];
    12c4:	28 a3 00 00 	lw r3,(r5+0)
    12c8:	28 63 00 00 	lw r3,(r3+0)

    rule.delay_cycles = 100000000 / 8000;
    12cc:	5b 8f 00 50 	sw (sp+80),r15
    rule.delay_frac = 0;
    12d0:	0f 80 00 54 	sh (sp+84),r0
        return;
    }

    id.system = buf[1];
    id.source_port = buf[2];
    id.trigger = buf[3];
    12d4:	5b 83 00 64 	sw (sp+100),r3

    rule.delay_cycles = 100000000 / 8000;
    rule.delay_frac = 0;
    rule.state = (is_cond ? HASH_ENT_CONDITIONAL : HASH_ENT_DIRECT) | HASH_ENT_DISABLED;
    12d8:	45 80 00 33 	be r12,r0,13a4 <main+0x860>
    12dc:	34 04 00 0c 	mvi r4,12
    rule.cond_ptr = NULL;

    handle.channel = ch;
    handle.cond = NULL;
    handle.trig = hash_add ( &id, ch, &rule );
    12e0:	b9 c0 10 00 	mv r2,r14
    12e4:	37 83 00 50 	addi r3,sp,80
    12e8:	37 81 00 5c 	addi r1,sp,92
    id.source_port = buf[2];
    id.trigger = buf[3];

    rule.delay_cycles = 100000000 / 8000;
    rule.delay_frac = 0;
    rule.state = (is_cond ? HASH_ENT_CONDITIONAL : HASH_ENT_DIRECT) | HASH_ENT_DISABLED;
    12ec:	0f 84 00 56 	sh (sp+86),r4
    rule.cond_ptr = NULL;
    12f0:	5b 80 00 58 	sw (sp+88),r0

    handle.channel = ch;
    handle.cond = NULL;
    handle.trig = hash_add ( &id, ch, &rule );
    12f4:	f8 00 00 eb 	calli 16a0 <hash_add>
        mq_send(0, FD_OUT_SLOT_CONTROL, 5);
        
	    return;
    }
    
    id.system = buf[5];
    12f8:	78 06 00 00 	mvhi r6,0x0
    12fc:	38 c6 22 6c 	ori r6,r6,0x226c
    1300:	28 c3 00 00 	lw r3,(r6+0)
    id.source_port = buf[6];
    1304:	78 07 00 00 	mvhi r7,0x0
    1308:	38 e7 22 70 	ori r7,r7,0x2270
        mq_send(0, FD_OUT_SLOT_CONTROL, 5);
        
	    return;
    }
    
    id.system = buf[5];
    130c:	28 63 00 00 	lw r3,(r3+0)
    id.source_port = buf[6];
    id.trigger = buf[7];
    1310:	78 09 00 00 	mvhi r9,0x0
    1314:	39 29 22 74 	ori r9,r9,0x2274
        mq_send(0, FD_OUT_SLOT_CONTROL, 5);
        
	    return;
    }
    
    id.system = buf[5];
    1318:	5b 83 00 5c 	sw (sp+92),r3
    id.source_port = buf[6];
    131c:	28 e3 00 00 	lw r3,(r7+0)
    rule.state = (is_cond ? HASH_ENT_CONDITIONAL : HASH_ENT_DIRECT) | HASH_ENT_DISABLED;
    rule.cond_ptr = NULL;

    handle.channel = ch;
    handle.cond = NULL;
    handle.trig = hash_add ( &id, ch, &rule );
    1320:	b8 20 58 00 	mv r11,r1
    rule.cond_ptr = (struct lrt_output_rule *) handle.trig;

//    pp_printf("add-cond %x:%x:%x out %d->%p", id.system, id.source_port, id.trigger, ch, handle.trig);
//    pp_printf("st-c %x\n", rule.state);

    handle.cond = hash_add ( &id, ch, &rule );
    1324:	b9 c0 10 00 	mv r2,r14
        
	    return;
    }
    
    id.system = buf[5];
    id.source_port = buf[6];
    1328:	28 64 00 00 	lw r4,(r3+0)
    rule.cond_ptr = (struct lrt_output_rule *) handle.trig;

//    pp_printf("add-cond %x:%x:%x out %d->%p", id.system, id.source_port, id.trigger, ch, handle.trig);
//    pp_printf("st-c %x\n", rule.state);

    handle.cond = hash_add ( &id, ch, &rule );
    132c:	37 81 00 5c 	addi r1,sp,92
    1330:	37 83 00 50 	addi r3,sp,80
        
	    return;
    }
    
    id.system = buf[5];
    id.source_port = buf[6];
    1334:	5b 84 00 60 	sw (sp+96),r4
    id.trigger = buf[7];
    1338:	29 24 00 00 	lw r4,(r9+0)
    133c:	28 84 00 00 	lw r4,(r4+0)

    rule.delay_cycles = 100000000 / 8000;
    1340:	5b 8f 00 50 	sw (sp+80),r15
    rule.delay_frac = 0;
    1344:	0f 80 00 54 	sh (sp+84),r0
	    return;
    }
    
    id.system = buf[5];
    id.source_port = buf[6];
    id.trigger = buf[7];
    1348:	5b 84 00 64 	sw (sp+100),r4

    rule.delay_cycles = 100000000 / 8000;
    rule.delay_frac = 0;
    rule.state = HASH_ENT_CONDITION | HASH_ENT_DISABLED; 
    134c:	34 04 00 0a 	mvi r4,10
    1350:	0f 84 00 56 	sh (sp+86),r4
    rule.cond_ptr = (struct lrt_output_rule *) handle.trig;
    1354:	5b 8b 00 58 	sw (sp+88),r11

//    pp_printf("add-cond %x:%x:%x out %d->%p", id.system, id.source_port, id.trigger, ch, handle.trig);
//    pp_printf("st-c %x\n", rule.state);

    handle.cond = hash_add ( &id, ch, &rule );
    1358:	f8 00 00 d2 	calli 16a0 <hash_add>

    uint32_t *obuf = ctl_claim_out();
    obuf[0] = ID_REP_TRIGGER_HANDLE;
    135c:	34 03 01 03 	mvi r3,259
    1360:	5a b7 00 00 	sw (r21+0),r23
    1364:	5b 23 00 00 	sw (r25+0),r3
    obuf[1] = seq;    
    obuf[2] = handle.channel;
    1368:	78 02 40 01 	mvhi r2,0x4001
    136c:	78 05 00 00 	mvhi r5,0x0

    handle.cond = hash_add ( &id, ch, &rule );

    uint32_t *obuf = ctl_claim_out();
    obuf[0] = ID_REP_TRIGGER_HANDLE;
    obuf[1] = seq;    
    1370:	5b 0d 00 00 	sw (r24+0),r13
    obuf[2] = handle.channel;
    1374:	38 42 84 10 	ori r2,r2,0x8410
    obuf[3] = (uint32_t) handle.cond;
    1378:	78 03 40 01 	mvhi r3,0x4001
    137c:	38 a5 22 78 	ori r5,r5,0x2278
    handle.cond = hash_add ( &id, ch, &rule );

    uint32_t *obuf = ctl_claim_out();
    obuf[0] = ID_REP_TRIGGER_HANDLE;
    obuf[1] = seq;    
    obuf[2] = handle.channel;
    1380:	58 4e 00 00 	sw (r2+0),r14
    obuf[3] = (uint32_t) handle.cond;
    1384:	38 63 84 14 	ori r3,r3,0x8414
    obuf[4] = (uint32_t) handle.trig;
    1388:	78 04 40 01 	mvhi r4,0x4001
    138c:	28 a2 00 00 	lw r2,(r5+0)

    uint32_t *obuf = ctl_claim_out();
    obuf[0] = ID_REP_TRIGGER_HANDLE;
    obuf[1] = seq;    
    obuf[2] = handle.channel;
    obuf[3] = (uint32_t) handle.cond;
    1390:	58 61 00 00 	sw (r3+0),r1
    obuf[4] = (uint32_t) handle.trig;
    1394:	38 84 84 18 	ori r4,r4,0x8418
    1398:	58 8b 00 00 	sw (r4+0),r11
    139c:	5a a2 00 00 	sw (r21+0),r2
    13a0:	e3 ff fe b2 	bi e68 <main+0x324>
    id.source_port = buf[2];
    id.trigger = buf[3];

    rule.delay_cycles = 100000000 / 8000;
    rule.delay_frac = 0;
    rule.state = (is_cond ? HASH_ENT_CONDITIONAL : HASH_ENT_DIRECT) | HASH_ENT_DISABLED;
    13a4:	34 04 00 09 	mvi r4,9
    rule.cond_ptr = NULL;

    handle.channel = ch;
    handle.cond = NULL;
    handle.trig = hash_add ( &id, ch, &rule );
    13a8:	b9 c0 10 00 	mv r2,r14
    13ac:	37 83 00 50 	addi r3,sp,80
    13b0:	37 81 00 5c 	addi r1,sp,92
    id.source_port = buf[2];
    id.trigger = buf[3];

    rule.delay_cycles = 100000000 / 8000;
    rule.delay_frac = 0;
    rule.state = (is_cond ? HASH_ENT_CONDITIONAL : HASH_ENT_DIRECT) | HASH_ENT_DISABLED;
    13b4:	0f 84 00 56 	sh (sp+86),r4
    rule.cond_ptr = NULL;
    13b8:	5b 80 00 58 	sw (sp+88),r0

    handle.channel = ch;
    handle.cond = NULL;
    handle.trig = hash_add ( &id, ch, &rule );
    13bc:	f8 00 00 b9 	calli 16a0 <hash_add>
//    pp_printf("st-t %x\n", rule.state);
    if(!is_cond) // unconditional trigger
    {
//	pp_printf("add-dt %x:%x:%x out %d->%p", id.system, id.source_port, id.trigger, ch, handle.trig);
        uint32_t *obuf = ctl_claim_out();
        obuf[0] = ID_REP_TRIGGER_HANDLE;
    13c0:	34 03 01 03 	mvi r3,259
    13c4:	5a b7 00 00 	sw (r21+0),r23
    13c8:	5b 23 00 00 	sw (r25+0),r3
        obuf[1] = seq;
        obuf[2] = handle.channel;
    13cc:	78 02 40 01 	mvhi r2,0x4001
    13d0:	78 05 00 00 	mvhi r5,0x0
    if(!is_cond) // unconditional trigger
    {
//	pp_printf("add-dt %x:%x:%x out %d->%p", id.system, id.source_port, id.trigger, ch, handle.trig);
        uint32_t *obuf = ctl_claim_out();
        obuf[0] = ID_REP_TRIGGER_HANDLE;
        obuf[1] = seq;
    13d4:	5b 0d 00 00 	sw (r24+0),r13
        obuf[2] = handle.channel;
    13d8:	38 42 84 10 	ori r2,r2,0x8410
        obuf[3] = (uint32_t) handle.cond;
    13dc:	78 03 40 01 	mvhi r3,0x4001
    13e0:	38 a5 22 78 	ori r5,r5,0x2278
    {
//	pp_printf("add-dt %x:%x:%x out %d->%p", id.system, id.source_port, id.trigger, ch, handle.trig);
        uint32_t *obuf = ctl_claim_out();
        obuf[0] = ID_REP_TRIGGER_HANDLE;
        obuf[1] = seq;
        obuf[2] = handle.channel;
    13e4:	58 4e 00 00 	sw (r2+0),r14
        obuf[3] = (uint32_t) handle.cond;
    13e8:	38 63 84 14 	ori r3,r3,0x8414
        obuf[4] = (uint32_t) handle.trig;
    13ec:	78 04 40 01 	mvhi r4,0x4001
    13f0:	28 a2 00 00 	lw r2,(r5+0)
//	pp_printf("add-dt %x:%x:%x out %d->%p", id.system, id.source_port, id.trigger, ch, handle.trig);
        uint32_t *obuf = ctl_claim_out();
        obuf[0] = ID_REP_TRIGGER_HANDLE;
        obuf[1] = seq;
        obuf[2] = handle.channel;
        obuf[3] = (uint32_t) handle.cond;
    13f4:	58 60 00 00 	sw (r3+0),r0
        obuf[4] = (uint32_t) handle.trig;
    13f8:	38 84 84 18 	ori r4,r4,0x8418
    13fc:	58 81 00 00 	sw (r4+0),r1
    1400:	5a a2 00 00 	sw (r21+0),r2
    1404:	e3 ff fe 99 	bi e68 <main+0x324>
    buf[2] = ts->frac;
}

static void bag_id(uint32_t *buf, struct list_id *id )
{
    buf[0] = id->system;
    1408:	28 83 00 00 	lw r3,(r4+0)
    140c:	78 09 40 01 	mvhi r9,0x4001
    	    obuf[15] = ent->ocfg[ch].delay_frac;
    	    obuf[16] = HASH_ENT_CONDITIONAL;
    	} else {
            bag_id(obuf + 3, &ent->id);
    	    obuf[6] = ent->ocfg[ch].delay_cycles;
    	    obuf[7] = ent->ocfg[ch].delay_frac;
    1410:	34 e2 00 08 	addi r2,r7,8
    buf[2] = ts->frac;
}

static void bag_id(uint32_t *buf, struct list_id *id )
{
    buf[0] = id->system;
    1414:	39 29 84 14 	ori r9,r9,0x8414
    	    obuf[15] = ent->ocfg[ch].delay_frac;
    	    obuf[16] = HASH_ENT_CONDITIONAL;
    	} else {
            bag_id(obuf + 3, &ent->id);
    	    obuf[6] = ent->ocfg[ch].delay_cycles;
    	    obuf[7] = ent->ocfg[ch].delay_frac;
    1418:	2c 41 00 08 	lhu r1,(r2+8)
    buf[2] = ts->frac;
}

static void bag_id(uint32_t *buf, struct list_id *id )
{
    buf[0] = id->system;
    141c:	59 23 00 00 	sw (r9+0),r3
    buf[1] = id->source_port;
    1420:	28 83 00 04 	lw r3,(r4+4)
    	    obuf[16] = HASH_ENT_CONDITIONAL;
    	} else {
            bag_id(obuf + 3, &ent->id);
    	    obuf[6] = ent->ocfg[ch].delay_cycles;
    	    obuf[7] = ent->ocfg[ch].delay_frac;
    	    obuf[8] = ent->ocfg[ch].state;
    1424:	2c 45 00 0a 	lhu r5,(r2+10)
}

static void bag_id(uint32_t *buf, struct list_id *id )
{
    buf[0] = id->system;
    buf[1] = id->source_port;
    1428:	78 02 40 01 	mvhi r2,0x4001
    142c:	38 42 84 18 	ori r2,r2,0x8418
    1430:	58 43 00 00 	sw (r2+0),r3
    buf[2] = id->trigger;
    1434:	28 82 00 08 	lw r2,(r4+8)
    1438:	78 03 40 01 	mvhi r3,0x4001
    143c:	38 63 84 1c 	ori r3,r3,0x841c
    1440:	58 62 00 00 	sw (r3+0),r2
    	    obuf[15] = ent->ocfg[ch].delay_frac;
    	    obuf[16] = HASH_ENT_CONDITIONAL;
    	} else {
            bag_id(obuf + 3, &ent->id);
    	    obuf[6] = ent->ocfg[ch].delay_cycles;
    	    obuf[7] = ent->ocfg[ch].delay_frac;
    1444:	78 04 40 01 	mvhi r4,0x4001
    	    obuf[14] = ent->ocfg[ch].delay_cycles;
    	    obuf[15] = ent->ocfg[ch].delay_frac;
    	    obuf[16] = HASH_ENT_CONDITIONAL;
    	} else {
            bag_id(obuf + 3, &ent->id);
    	    obuf[6] = ent->ocfg[ch].delay_cycles;
    1448:	28 e2 00 0c 	lw r2,(r7+12)
    	    obuf[7] = ent->ocfg[ch].delay_frac;
    144c:	38 84 84 24 	ori r4,r4,0x8424
    	    obuf[8] = ent->ocfg[ch].state;
    1450:	78 06 40 01 	mvhi r6,0x4001
    	    obuf[15] = ent->ocfg[ch].delay_frac;
    	    obuf[16] = HASH_ENT_CONDITIONAL;
    	} else {
            bag_id(obuf + 3, &ent->id);
    	    obuf[6] = ent->ocfg[ch].delay_cycles;
    	    obuf[7] = ent->ocfg[ch].delay_frac;
    1454:	58 81 00 00 	sw (r4+0),r1
    	    obuf[8] = ent->ocfg[ch].state;
    1458:	38 c6 84 28 	ori r6,r6,0x8428
    	    obuf[14] = ent->ocfg[ch].delay_cycles;
    	    obuf[15] = ent->ocfg[ch].delay_frac;
    	    obuf[16] = HASH_ENT_CONDITIONAL;
    	} else {
            bag_id(obuf + 3, &ent->id);
    	    obuf[6] = ent->ocfg[ch].delay_cycles;
    145c:	78 07 40 01 	mvhi r7,0x4001
    	    obuf[7] = ent->ocfg[ch].delay_frac;
    	    obuf[8] = ent->ocfg[ch].state;
    1460:	58 c5 00 00 	sw (r6+0),r5
    	    obuf[14] = ent->ocfg[ch].delay_cycles;
    	    obuf[15] = ent->ocfg[ch].delay_frac;
    	    obuf[16] = HASH_ENT_CONDITIONAL;
    	} else {
            bag_id(obuf + 3, &ent->id);
    	    obuf[6] = ent->ocfg[ch].delay_cycles;
    1464:	38 e7 84 20 	ori r7,r7,0x8420
    1468:	58 e2 00 00 	sw (r7+0),r2
    146c:	e3 ff fe 7b 	bi e58 <main+0x314>

00001470 <blockpool_init>:

void blockpool_init( struct blockpool *bp, int blk_size, int blk_count, void *data, void *queue )
{
    int i;

    bp->pool = data;
    1470:	58 24 00 18 	sw (r1+24),r4
    bp->fq = queue;
    1474:	58 25 00 08 	sw (r1+8),r5
    bp->blk_count = blk_count;
    1478:	58 23 00 04 	sw (r1+4),r3
    bp->blk_size = blk_size;
    147c:	58 22 00 00 	sw (r1+0),r2
    bp->fq_head = blk_count;
    1480:	58 23 00 0c 	sw (r1+12),r3
    bp->fq_tail = 0;
    1484:	58 20 00 10 	sw (r1+16),r0
    bp->fq_count = blk_count;
    1488:	58 23 00 14 	sw (r1+20),r3

    for(i=0;i<bp->blk_count;i++)
    148c:	4c 03 00 06 	bge r0,r3,14a4 <blockpool_init+0x34>
    1490:	34 01 00 00 	mvi r1,0
	bp->fq[i] = i;
    1494:	0c a1 00 00 	sh (r5+0),r1
    bp->blk_size = blk_size;
    bp->fq_head = blk_count;
    bp->fq_tail = 0;
    bp->fq_count = blk_count;

    for(i=0;i<bp->blk_count;i++)
    1498:	34 21 00 01 	addi r1,r1,1
    149c:	34 a5 00 02 	addi r5,r5,2
    14a0:	5c 23 ff fd 	bne r1,r3,1494 <blockpool_init+0x24>
    14a4:	c3 a0 00 00 	ret

000014a8 <blockpool_alloc>:
	bp->fq[i] = i;
}

void *blockpool_alloc( struct  blockpool *bp )
{
    if(bp->fq_head == bp->fq_tail)
    14a8:	28 23 00 10 	lw r3,(r1+16)
    14ac:	28 24 00 0c 	lw r4,(r1+12)
    for(i=0;i<bp->blk_count;i++)
	bp->fq[i] = i;
}

void *blockpool_alloc( struct  blockpool *bp )
{
    14b0:	b8 20 10 00 	mv r2,r1
    if(bp->fq_head == bp->fq_tail)
    {
	return NULL;
    14b4:	34 01 00 00 	mvi r1,0
	bp->fq[i] = i;
}

void *blockpool_alloc( struct  blockpool *bp )
{
    if(bp->fq_head == bp->fq_tail)
    14b8:	44 83 00 10 	be r4,r3,14f8 <blockpool_alloc+0x50>
    {
	return NULL;
    }

    void *blk = bp->pool + bp->blk_size * (int)bp->fq[bp->fq_tail];
    14bc:	28 45 00 08 	lw r5,(r2+8)
    14c0:	3c 64 00 01 	sli r4,r3,1
    14c4:	28 41 00 00 	lw r1,(r2+0)
    14c8:	b4 a4 20 00 	add r4,r5,r4
    14cc:	2c 84 00 00 	lhu r4,(r4+0)
    14d0:	28 46 00 18 	lw r6,(r2+24)
    if(bp->fq_tail == bp->blk_count)
    14d4:	28 45 00 04 	lw r5,(r2+4)
    if(bp->fq_head == bp->fq_tail)
    {
	return NULL;
    }

    void *blk = bp->pool + bp->blk_size * (int)bp->fq[bp->fq_tail];
    14d8:	88 81 08 00 	mul r1,r4,r1
    14dc:	b4 c1 08 00 	add r1,r6,r1
    if(bp->fq_tail == bp->blk_count)
    14e0:	44 65 00 07 	be r3,r5,14fc <blockpool_alloc+0x54>
	bp->fq_tail = 0;
    else
	bp->fq_tail++;
    14e4:	34 63 00 01 	addi r3,r3,1
    14e8:	58 43 00 10 	sw (r2+16),r3

    bp->fq_count--;
    14ec:	28 43 00 14 	lw r3,(r2+20)
    14f0:	34 63 ff ff 	addi r3,r3,-1
    14f4:	58 43 00 14 	sw (r2+20),r3

    

    return blk;
}
    14f8:	c3 a0 00 00 	ret
	return NULL;
    }

    void *blk = bp->pool + bp->blk_size * (int)bp->fq[bp->fq_tail];
    if(bp->fq_tail == bp->blk_count)
	bp->fq_tail = 0;
    14fc:	58 40 00 10 	sw (r2+16),r0
    1500:	e3 ff ff fb 	bi 14ec <blockpool_alloc+0x44>

00001504 <blockpool_free>:

    return blk;
}

void blockpool_free( struct blockpool *bp, void *ptr )
{
    1504:	37 9c ff f0 	addi sp,sp,-16
    1508:	5b 8b 00 10 	sw (sp+16),r11
    150c:	5b 8c 00 0c 	sw (sp+12),r12
    1510:	5b 8d 00 08 	sw (sp+8),r13
    1514:	5b 9d 00 04 	sw (sp+4),ra
    1518:	b8 20 58 00 	mv r11,r1
    int blk_id = (ptr - bp->pool) / bp->blk_size;
    151c:	28 21 00 18 	lw r1,(r1+24)
    bp->fq[bp->fq_head] = blk_id;
    1520:	29 6c 00 0c 	lw r12,(r11+12)
    1524:	29 6d 00 08 	lw r13,(r11+8)
    return blk;
}

void blockpool_free( struct blockpool *bp, void *ptr )
{
    int blk_id = (ptr - bp->pool) / bp->blk_size;
    1528:	c8 41 08 00 	sub r1,r2,r1
    152c:	29 62 00 00 	lw r2,(r11+0)
    bp->fq[bp->fq_head] = blk_id;
    1530:	3d 83 00 01 	sli r3,r12,1
    1534:	b5 a3 68 00 	add r13,r13,r3
    return blk;
}

void blockpool_free( struct blockpool *bp, void *ptr )
{
    int blk_id = (ptr - bp->pool) / bp->blk_size;
    1538:	f8 00 02 33 	calli 1e04 <__divsi3>
    bp->fq[bp->fq_head] = blk_id;

    if(bp->fq_head == bp->blk_count)
    153c:	29 62 00 04 	lw r2,(r11+4)
}

void blockpool_free( struct blockpool *bp, void *ptr )
{
    int blk_id = (ptr - bp->pool) / bp->blk_size;
    bp->fq[bp->fq_head] = blk_id;
    1540:	0d a1 00 00 	sh (r13+0),r1

    if(bp->fq_head == bp->blk_count)
    1544:	45 82 00 0c 	be r12,r2,1574 <blockpool_free+0x70>
	bp->fq_head = 0;
    else
	bp->fq_head++;
    1548:	35 8c 00 01 	addi r12,r12,1
    154c:	59 6c 00 0c 	sw (r11+12),r12
    bp->fq_count++;
    1550:	29 61 00 14 	lw r1,(r11+20)
    1554:	34 21 00 01 	addi r1,r1,1
    1558:	59 61 00 14 	sw (r11+20),r1
}
    155c:	2b 9d 00 04 	lw ra,(sp+4)
    1560:	2b 8b 00 10 	lw r11,(sp+16)
    1564:	2b 8c 00 0c 	lw r12,(sp+12)
    1568:	2b 8d 00 08 	lw r13,(sp+8)
    156c:	37 9c 00 10 	addi sp,sp,16
    1570:	c3 a0 00 00 	ret
{
    int blk_id = (ptr - bp->pool) / bp->blk_size;
    bp->fq[bp->fq_head] = blk_id;

    if(bp->fq_head == bp->blk_count)
	bp->fq_head = 0;
    1574:	59 60 00 0c 	sw (r11+12),r0
    1578:	e3 ff ff f6 	bi 1550 <blockpool_free+0x4c>

0000157c <hash_init>:
    bp->fq_count++;
}


void hash_init()
{
    157c:	37 9c ff fc 	addi sp,sp,-4
    1580:	5b 9d 00 04 	sw (sp+4),ra

void blockpool_init( struct blockpool *bp, int blk_size, int blk_count, void *data, void *queue )
{
    int i;

    bp->pool = data;
    1584:	78 01 00 00 	mvhi r1,0x0
    1588:	78 04 00 00 	mvhi r4,0x0
    158c:	38 21 26 0c 	ori r1,r1,0x260c
    1590:	38 84 26 28 	ori r4,r4,0x2628
    bp->fq = queue;
    1594:	78 02 00 00 	mvhi r2,0x0
    bp->blk_count = blk_count;
    1598:	34 03 00 80 	mvi r3,128

void blockpool_init( struct blockpool *bp, int blk_size, int blk_count, void *data, void *queue )
{
    int i;

    bp->pool = data;
    159c:	58 24 00 18 	sw (r1+24),r4
    bp->fq = queue;
    15a0:	38 42 46 28 	ori r2,r2,0x4628
    bp->blk_count = blk_count;
    bp->blk_size = blk_size;
    15a4:	34 04 00 40 	mvi r4,64
void blockpool_init( struct blockpool *bp, int blk_size, int blk_count, void *data, void *queue )
{
    int i;

    bp->pool = data;
    bp->fq = queue;
    15a8:	58 22 00 08 	sw (r1+8),r2
    bp->blk_count = blk_count;
    15ac:	58 23 00 04 	sw (r1+4),r3
    bp->blk_size = blk_size;
    15b0:	58 24 00 00 	sw (r1+0),r4
    bp->fq_head = blk_count;
    15b4:	58 23 00 0c 	sw (r1+12),r3
    bp->fq_tail = 0;
    15b8:	58 20 00 10 	sw (r1+16),r0
    bp->fq_count = blk_count;
    15bc:	58 23 00 14 	sw (r1+20),r3

    for(i=0;i<bp->blk_count;i++)
    15c0:	34 01 00 00 	mvi r1,0
	bp->fq[i] = i;
    15c4:	0c 41 00 00 	sh (r2+0),r1
    bp->blk_size = blk_size;
    bp->fq_head = blk_count;
    bp->fq_tail = 0;
    bp->fq_count = blk_count;

    for(i=0;i<bp->blk_count;i++)
    15c8:	34 21 00 01 	addi r1,r1,1
    15cc:	34 42 00 02 	addi r2,r2,2
    15d0:	5c 23 ff fd 	bne r1,r3,15c4 <hash_init+0x48>


void hash_init()
{
    blockpool_init(&hash_blockpool, sizeof(struct lrt_hash_entry), FD_HASH_ENTRIES, hash_pool_mem, hash_pool_queue);
    memset(&htab, 0, sizeof(htab) );
    15d4:	78 01 00 00 	mvhi r1,0x0
    15d8:	34 02 00 00 	mvi r2,0
    15dc:	34 03 02 00 	mvi r3,512
    15e0:	38 21 4d 20 	ori r1,r1,0x4d20
    15e4:	f8 00 02 74 	calli 1fb4 <memset>
}
    15e8:	2b 9d 00 04 	lw ra,(sp+4)
    15ec:	37 9c 00 04 	addi sp,sp,4
    15f0:	c3 a0 00 00 	ret

000015f4 <hash_alloc>:

struct lrt_hash_entry *hash_alloc( int pos )
{
    struct lrt_hash_entry *prev = NULL, *current = htab[pos];
    15f4:	78 04 00 00 	mvhi r4,0x0
    15f8:	3c 26 00 02 	sli r6,r1,2
    15fc:	38 84 4d 20 	ori r4,r4,0x4d20
    1600:	b4 86 08 00 	add r1,r4,r6
    1604:	28 22 00 00 	lw r2,(r1+0)
    1608:	34 05 00 00 	mvi r5,0
    
    while(current)
    160c:	5c 40 00 03 	bne r2,r0,1618 <hash_alloc+0x24>
    1610:	e0 00 00 05 	bi 1624 <hash_alloc+0x30>
    {
	prev = current;
	current = current->next;
    1614:	b8 60 10 00 	mv r2,r3
    1618:	28 43 00 3c 	lw r3,(r2+60)

struct lrt_hash_entry *hash_alloc( int pos )
{
    struct lrt_hash_entry *prev = NULL, *current = htab[pos];
    
    while(current)
    161c:	5c 60 ff fe 	bne r3,r0,1614 <hash_alloc+0x20>
    1620:	b8 40 28 00 	mv r5,r2
	bp->fq[i] = i;
}

void *blockpool_alloc( struct  blockpool *bp )
{
    if(bp->fq_head == bp->fq_tail)
    1624:	78 02 00 00 	mvhi r2,0x0
    1628:	38 42 26 0c 	ori r2,r2,0x260c
    162c:	28 43 00 10 	lw r3,(r2+16)
    1630:	28 47 00 0c 	lw r7,(r2+12)
    1634:	34 01 00 00 	mvi r1,0
    1638:	44 e3 00 10 	be r7,r3,1678 <hash_alloc+0x84>
    {
	return NULL;
    }

    void *blk = bp->pool + bp->blk_size * (int)bp->fq[bp->fq_tail];
    163c:	28 48 00 08 	lw r8,(r2+8)
    1640:	3c 67 00 01 	sli r7,r3,1
    1644:	28 41 00 00 	lw r1,(r2+0)
    1648:	b5 07 38 00 	add r7,r8,r7
    164c:	2c e7 00 00 	lhu r7,(r7+0)
    1650:	28 49 00 18 	lw r9,(r2+24)
    if(bp->fq_tail == bp->blk_count)
    1654:	28 48 00 04 	lw r8,(r2+4)
    if(bp->fq_head == bp->fq_tail)
    {
	return NULL;
    }

    void *blk = bp->pool + bp->blk_size * (int)bp->fq[bp->fq_tail];
    1658:	88 e1 08 00 	mul r1,r7,r1
    165c:	b5 21 08 00 	add r1,r9,r1
    if(bp->fq_tail == bp->blk_count)
    1660:	44 68 00 0a 	be r3,r8,1688 <hash_alloc+0x94>
	bp->fq_tail = 0;
    else
	bp->fq_tail++;
    1664:	34 63 00 01 	addi r3,r3,1
    1668:	58 43 00 10 	sw (r2+16),r3

    bp->fq_count--;
    166c:	28 43 00 14 	lw r3,(r2+20)
    1670:	34 63 ff ff 	addi r3,r3,-1
    1674:	58 43 00 14 	sw (r2+20),r3
	current = current->next;
    }
	
    current = blockpool_alloc(&hash_blockpool);

    if(!prev)
    1678:	44 a0 00 06 	be r5,r0,1690 <hash_alloc+0x9c>
        htab[pos] = current;
    else
       	prev->next = current;
    167c:	58 a1 00 3c 	sw (r5+60),r1

    current->next = NULL;
    1680:	58 20 00 3c 	sw (r1+60),r0
    return current;
}
    1684:	c3 a0 00 00 	ret
	return NULL;
    }

    void *blk = bp->pool + bp->blk_size * (int)bp->fq[bp->fq_tail];
    if(bp->fq_tail == bp->blk_count)
	bp->fq_tail = 0;
    1688:	58 40 00 10 	sw (r2+16),r0
    168c:	e3 ff ff f8 	bi 166c <hash_alloc+0x78>
    }
	
    current = blockpool_alloc(&hash_blockpool);

    if(!prev)
        htab[pos] = current;
    1690:	b4 86 20 00 	add r4,r4,r6
    1694:	58 81 00 00 	sw (r4+0),r1
    else
       	prev->next = current;

    current->next = NULL;
    1698:	58 20 00 3c 	sw (r1+60),r0
    return current;
}
    169c:	c3 a0 00 00 	ret

000016a0 <hash_add>:

struct lrt_hash_entry *hash_add ( struct list_id *id, int output, struct lrt_output_rule *rule )
{
    16a0:	37 9c ff f8 	addi sp,sp,-8
    16a4:	5b 8b 00 08 	sw (sp+8),r11
    16a8:	5b 8c 00 04 	sw (sp+4),r12
struct lrt_hash_entry *hash_get_entry (int bucket, int pos);

static inline int hash_func( struct list_id *id )
{
    int h = 0;
    h += id->system * 10291;
    16ac:	28 24 00 00 	lw r4,(r1+0)
    h += id->source_port * 10017;
    16b0:	28 27 00 04 	lw r7,(r1+4)
    h += id->trigger * 3111;
    16b4:	28 2a 00 08 	lw r10,(r1+8)
struct lrt_hash_entry *hash_get_entry (int bucket, int pos);

static inline int hash_func( struct list_id *id )
{
    int h = 0;
    h += id->system * 10291;
    16b8:	08 85 28 33 	muli r5,r4,10291
    h += id->source_port * 10017;
    16bc:	08 eb 27 21 	muli r11,r7,10017
    16c0:	b8 20 30 00 	mv r6,r1
    h += id->trigger * 3111;
    16c4:	09 41 0c 27 	muli r1,r10,3111

static inline int hash_func( struct list_id *id )
{
    int h = 0;
    h += id->system * 10291;
    h += id->source_port * 10017;
    16c8:	b5 65 58 00 	add r11,r11,r5

static inline struct lrt_hash_entry *hash_search( struct list_id *id, int *pos )
{
    int p = hash_func( id );

    struct lrt_hash_entry *ent = htab[ p ];
    16cc:	78 09 00 00 	mvhi r9,0x0
static inline int hash_func( struct list_id *id )
{
    int h = 0;
    h += id->system * 10291;
    h += id->source_port * 10017;
    h += id->trigger * 3111;
    16d0:	b5 61 58 00 	add r11,r11,r1
    return h & (FD_HASH_ENTRIES - 1); // hash table size must be a power of 2
    16d4:	21 6b 00 7f 	andi r11,r11,0x7f

static inline struct lrt_hash_entry *hash_search( struct list_id *id, int *pos )
{
    int p = hash_func( id );

    struct lrt_hash_entry *ent = htab[ p ];
    16d8:	3d 6b 00 02 	sli r11,r11,2
    16dc:	39 29 4d 20 	ori r9,r9,0x4d20
    16e0:	b5 2b 08 00 	add r1,r9,r11
    16e4:	28 28 00 00 	lw r8,(r1+0)
    if(pos)
        *pos = p;
    
    
    while (ent)
    16e8:	45 00 00 23 	be r8,r0,1774 <hash_add+0xd4>
    16ec:	b9 00 08 00 	mv r1,r8
    16f0:	e0 00 00 03 	bi 16fc <hash_add+0x5c>
        if(ent->id.system == id->system &&
       ent->id.source_port == id->source_port &&
           ent->id.trigger == id->trigger)
            return ent;
    
    ent = ent->next;
    16f4:	28 21 00 3c 	lw r1,(r1+60)
    struct lrt_hash_entry *ent = htab[ p ];
    if(pos)
        *pos = p;
    
    
    while (ent)
    16f8:	44 20 00 1d 	be r1,r0,176c <hash_add+0xcc>
    {
        if(ent->id.system == id->system &&
    16fc:	28 25 00 00 	lw r5,(r1+0)
    1700:	5c 85 ff fd 	bne r4,r5,16f4 <hash_add+0x54>
    1704:	28 25 00 04 	lw r5,(r1+4)
    1708:	5c e5 ff fb 	bne r7,r5,16f4 <hash_add+0x54>
       ent->id.source_port == id->source_port &&
    170c:	28 25 00 08 	lw r5,(r1+8)
    1710:	5d 45 ff f9 	bne r10,r5,16f4 <hash_add+0x54>
	   ent = hash_alloc( pos );

    if(!ent)
       return NULL;

    ent->id = *id;
    1714:	28 c5 00 00 	lw r5,(r6+0)
    ent->ocfg[output] = *rule;
    1718:	3c 44 00 02 	sli r4,r2,2
    171c:	3c 42 00 04 	sli r2,r2,4
	   ent = hash_alloc( pos );

    if(!ent)
       return NULL;

    ent->id = *id;
    1720:	58 25 00 00 	sw (r1+0),r5
    1724:	28 c5 00 04 	lw r5,(r6+4)
    ent->ocfg[output] = *rule;
    1728:	28 67 00 00 	lw r7,(r3+0)
    172c:	c8 44 10 00 	sub r2,r2,r4
    1730:	b4 22 10 00 	add r2,r1,r2
    1734:	34 42 00 08 	addi r2,r2,8
	   ent = hash_alloc( pos );

    if(!ent)
       return NULL;

    ent->id = *id;
    1738:	58 25 00 04 	sw (r1+4),r5
    173c:	28 c5 00 08 	lw r5,(r6+8)
    ent->ocfg[output] = *rule;
    1740:	58 47 00 04 	sw (r2+4),r7
    1744:	28 64 00 04 	lw r4,(r3+4)
	   ent = hash_alloc( pos );

    if(!ent)
       return NULL;

    ent->id = *id;
    1748:	58 25 00 08 	sw (r1+8),r5
    ent->ocfg[output] = *rule;
    174c:	58 44 00 08 	sw (r2+8),r4
    1750:	28 63 00 08 	lw r3,(r3+8)
    1754:	58 43 00 0c 	sw (r2+12),r3
    return ent;
}
    1758:	2b 8b 00 08 	lw r11,(sp+8)
    175c:	2b 8c 00 04 	lw r12,(sp+4)
    1760:	37 9c 00 08 	addi sp,sp,8
    1764:	c3 a0 00 00 	ret
    struct lrt_hash_entry *prev = NULL, *current = htab[pos];
    
    while(current)
    {
	prev = current;
	current = current->next;
    1768:	b8 20 40 00 	mv r8,r1
    176c:	29 01 00 3c 	lw r1,(r8+60)

struct lrt_hash_entry *hash_alloc( int pos )
{
    struct lrt_hash_entry *prev = NULL, *current = htab[pos];
    
    while(current)
    1770:	5c 20 ff fe 	bne r1,r0,1768 <hash_add+0xc8>
	bp->fq[i] = i;
}

void *blockpool_alloc( struct  blockpool *bp )
{
    if(bp->fq_head == bp->fq_tail)
    1774:	78 01 00 00 	mvhi r1,0x0
    1778:	38 21 26 0c 	ori r1,r1,0x260c
    177c:	28 25 00 10 	lw r5,(r1+16)
    1780:	28 27 00 0c 	lw r7,(r1+12)
    {
	return NULL;
    1784:	34 04 00 00 	mvi r4,0
	bp->fq[i] = i;
}

void *blockpool_alloc( struct  blockpool *bp )
{
    if(bp->fq_head == bp->fq_tail)
    1788:	44 e5 00 10 	be r7,r5,17c8 <hash_add+0x128>
    {
	return NULL;
    }

    void *blk = bp->pool + bp->blk_size * (int)bp->fq[bp->fq_tail];
    178c:	28 2a 00 08 	lw r10,(r1+8)
    1790:	3c a7 00 01 	sli r7,r5,1
    1794:	28 24 00 00 	lw r4,(r1+0)
    1798:	b5 47 38 00 	add r7,r10,r7
    179c:	2c e7 00 00 	lhu r7,(r7+0)
    17a0:	28 2c 00 18 	lw r12,(r1+24)
    if(bp->fq_tail == bp->blk_count)
    17a4:	28 2a 00 04 	lw r10,(r1+4)
    if(bp->fq_head == bp->fq_tail)
    {
	return NULL;
    }

    void *blk = bp->pool + bp->blk_size * (int)bp->fq[bp->fq_tail];
    17a8:	88 e4 20 00 	mul r4,r7,r4
    17ac:	b5 84 20 00 	add r4,r12,r4
    if(bp->fq_tail == bp->blk_count)
    17b0:	44 aa 00 0b 	be r5,r10,17dc <hash_add+0x13c>
	bp->fq_tail = 0;
    else
	bp->fq_tail++;
    17b4:	34 a5 00 01 	addi r5,r5,1
    17b8:	58 25 00 10 	sw (r1+16),r5

    bp->fq_count--;
    17bc:	28 25 00 14 	lw r5,(r1+20)
    17c0:	34 a5 ff ff 	addi r5,r5,-1
    17c4:	58 25 00 14 	sw (r1+20),r5
    {
	prev = current;
	current = current->next;
    }
	
    current = blockpool_alloc(&hash_blockpool);
    17c8:	b8 80 08 00 	mv r1,r4

    if(!prev)
    17cc:	45 00 00 06 	be r8,r0,17e4 <hash_add+0x144>
        htab[pos] = current;
    else
       	prev->next = current;
    17d0:	59 04 00 3c 	sw (r8+60),r4

    current->next = NULL;
    17d4:	58 80 00 3c 	sw (r4+60),r0
    17d8:	e3 ff ff cf 	bi 1714 <hash_add+0x74>
	return NULL;
    }

    void *blk = bp->pool + bp->blk_size * (int)bp->fq[bp->fq_tail];
    if(bp->fq_tail == bp->blk_count)
	bp->fq_tail = 0;
    17dc:	58 20 00 10 	sw (r1+16),r0
    17e0:	e3 ff ff f7 	bi 17bc <hash_add+0x11c>
    }
	
    current = blockpool_alloc(&hash_blockpool);

    if(!prev)
        htab[pos] = current;
    17e4:	b5 2b 48 00 	add r9,r9,r11
    17e8:	59 24 00 00 	sw (r9+0),r4
    else
       	prev->next = current;

    current->next = NULL;
    17ec:	58 80 00 3c 	sw (r4+60),r0
    17f0:	e3 ff ff c9 	bi 1714 <hash_add+0x74>

000017f4 <hash_remove>:
    ent->ocfg[output] = *rule;
    return ent;
}

int hash_remove ( struct lrt_hash_entry *ent, int output )
{
    17f4:	37 9c ff f0 	addi sp,sp,-16
    17f8:	5b 8b 00 10 	sw (sp+16),r11
    17fc:	5b 8c 00 0c 	sw (sp+12),r12
    1800:	5b 8d 00 08 	sw (sp+8),r13
    1804:	5b 9d 00 04 	sw (sp+4),ra
    int pos, i;
    ent->ocfg[output].state = HASH_ENT_EMPTY;
    1808:	3c 43 00 02 	sli r3,r2,2
    180c:	3c 42 00 04 	sli r2,r2,4
    1810:	c8 43 10 00 	sub r2,r2,r3
    1814:	b4 22 10 00 	add r2,r1,r2
    1818:	0c 40 00 12 	sh (r2+18),r0

    for(i = 0; i < FD_NUM_CHANNELS; i++)
        if(ent->ocfg[i].state != HASH_ENT_EMPTY) // the same ID is assigned to another output
    181c:	2c 22 00 12 	lhu r2,(r1+18)
    1820:	5c 40 00 21 	bne r2,r0,18a4 <hash_remove+0xb0>
    1824:	2c 23 00 1e 	lhu r3,(r1+30)
    1828:	5c 62 00 1f 	bne r3,r2,18a4 <hash_remove+0xb0>
    182c:	2c 22 00 2a 	lhu r2,(r1+42)
    1830:	5c 43 00 1d 	bne r2,r3,18a4 <hash_remove+0xb0>
    1834:	2c 23 00 36 	lhu r3,(r1+54)
    1838:	5c 62 00 1b 	bne r3,r2,18a4 <hash_remove+0xb0>
            return 0;

    if (ent == htab[pos])
    183c:	78 02 00 00 	mvhi r2,0x0
    1840:	38 42 4d 20 	ori r2,r2,0x4d20
    1844:	28 43 00 00 	lw r3,(r2+0)
    1848:	44 61 00 20 	be r3,r1,18c8 <hash_remove+0xd4>
        htab[pos] = ent->next;
    else {
        struct lrt_hash_entry *tmp = htab[pos]->next;
        htab[pos]->next = ent->next;
    184c:	28 24 00 3c 	lw r4,(r1+60)
            return 0;

    if (ent == htab[pos])
        htab[pos] = ent->next;
    else {
        struct lrt_hash_entry *tmp = htab[pos]->next;
    1850:	28 62 00 3c 	lw r2,(r3+60)
        htab[pos]->next = ent->next;
    1854:	58 64 00 3c 	sw (r3+60),r4
        ent->next = tmp;
    1858:	58 22 00 3c 	sw (r1+60),r2
}

void blockpool_free( struct blockpool *bp, void *ptr )
{
    int blk_id = (ptr - bp->pool) / bp->blk_size;
    bp->fq[bp->fq_head] = blk_id;
    185c:	78 0b 00 00 	mvhi r11,0x0
    1860:	39 6b 26 0c 	ori r11,r11,0x260c
    1864:	29 6c 00 0c 	lw r12,(r11+12)
    return blk;
}

void blockpool_free( struct blockpool *bp, void *ptr )
{
    int blk_id = (ptr - bp->pool) / bp->blk_size;
    1868:	29 64 00 18 	lw r4,(r11+24)
    186c:	29 62 00 00 	lw r2,(r11+0)
    bp->fq[bp->fq_head] = blk_id;
    1870:	29 6d 00 08 	lw r13,(r11+8)
    1874:	3d 83 00 01 	sli r3,r12,1
    return blk;
}

void blockpool_free( struct blockpool *bp, void *ptr )
{
    int blk_id = (ptr - bp->pool) / bp->blk_size;
    1878:	c8 24 08 00 	sub r1,r1,r4
    bp->fq[bp->fq_head] = blk_id;
    187c:	b5 a3 68 00 	add r13,r13,r3
    return blk;
}

void blockpool_free( struct blockpool *bp, void *ptr )
{
    int blk_id = (ptr - bp->pool) / bp->blk_size;
    1880:	f8 00 01 61 	calli 1e04 <__divsi3>
    bp->fq[bp->fq_head] = blk_id;

    if(bp->fq_head == bp->blk_count)
    1884:	29 62 00 04 	lw r2,(r11+4)
}

void blockpool_free( struct blockpool *bp, void *ptr )
{
    int blk_id = (ptr - bp->pool) / bp->blk_size;
    bp->fq[bp->fq_head] = blk_id;
    1888:	0d a1 00 00 	sh (r13+0),r1

    if(bp->fq_head == bp->blk_count)
    188c:	45 82 00 0d 	be r12,r2,18c0 <hash_remove+0xcc>
	bp->fq_head = 0;
    else
	bp->fq_head++;
    1890:	35 8c 00 01 	addi r12,r12,1
    1894:	59 6c 00 0c 	sw (r11+12),r12
    bp->fq_count++;
    1898:	29 61 00 14 	lw r1,(r11+20)
    189c:	34 21 00 01 	addi r1,r1,1
    18a0:	59 61 00 14 	sw (r11+20),r1
        ent->next = tmp;
    }

    blockpool_free(&hash_blockpool, ent);
    return 0;
}
    18a4:	34 01 00 00 	mvi r1,0
    18a8:	2b 9d 00 04 	lw ra,(sp+4)
    18ac:	2b 8b 00 10 	lw r11,(sp+16)
    18b0:	2b 8c 00 0c 	lw r12,(sp+12)
    18b4:	2b 8d 00 08 	lw r13,(sp+8)
    18b8:	37 9c 00 10 	addi sp,sp,16
    18bc:	c3 a0 00 00 	ret
{
    int blk_id = (ptr - bp->pool) / bp->blk_size;
    bp->fq[bp->fq_head] = blk_id;

    if(bp->fq_head == bp->blk_count)
	bp->fq_head = 0;
    18c0:	59 60 00 0c 	sw (r11+12),r0
    18c4:	e3 ff ff f5 	bi 1898 <hash_remove+0xa4>
    for(i = 0; i < FD_NUM_CHANNELS; i++)
        if(ent->ocfg[i].state != HASH_ENT_EMPTY) // the same ID is assigned to another output
            return 0;

    if (ent == htab[pos])
        htab[pos] = ent->next;
    18c8:	28 23 00 3c 	lw r3,(r1+60)
    18cc:	58 43 00 00 	sw (r2+0),r3
    18d0:	e3 ff ff e3 	bi 185c <hash_remove+0x68>

000018d4 <hash_get_entry>:
}

struct lrt_hash_entry *hash_get_entry (int bucket, int pos)
{
    int i;
    if(bucket < 0 || bucket >= FD_HASH_ENTRIES)
    18d4:	34 04 00 7f 	mvi r4,127
	return NULL;
    18d8:	34 03 00 00 	mvi r3,0
}

struct lrt_hash_entry *hash_get_entry (int bucket, int pos)
{
    int i;
    if(bucket < 0 || bucket >= FD_HASH_ENTRIES)
    18dc:	54 24 00 11 	bgu r1,r4,1920 <hash_get_entry+0x4c>
	return NULL;

    struct lrt_hash_entry *l = htab[bucket];
    18e0:	3c 21 00 02 	sli r1,r1,2
    18e4:	78 03 00 00 	mvhi r3,0x0
    18e8:	38 63 4d 20 	ori r3,r3,0x4d20
    18ec:	b4 61 18 00 	add r3,r3,r1
    18f0:	28 63 00 00 	lw r3,(r3+0)

    for(i = 0; l != NULL && i < pos; i++, l=l->next)
    18f4:	68 44 00 00 	cmpgi r4,r2,0
    18f8:	7c 61 00 00 	cmpnei r1,r3,0
    18fc:	a0 81 08 00 	and r1,r4,r1
    1900:	44 20 00 08 	be r1,r0,1920 <hash_get_entry+0x4c>
    1904:	34 01 00 00 	mvi r1,0
    1908:	28 63 00 3c 	lw r3,(r3+60)
    190c:	34 21 00 01 	addi r1,r1,1
    1910:	e8 41 28 00 	cmpg r5,r2,r1
    1914:	7c 64 00 00 	cmpnei r4,r3,0
    1918:	a0 a4 20 00 	and r4,r5,r4
    191c:	5c 80 ff fb 	bne r4,r0,1908 <hash_get_entry+0x34>
	if(!l)
	    return NULL;

    return l;
}
    1920:	b8 60 08 00 	mv r1,r3
    1924:	c3 a0 00 00 	ret

00001928 <hash_free_count>:

int hash_free_count()
{
    return hash_blockpool.fq_count;
    1928:	78 01 00 00 	mvhi r1,0x0
    192c:	38 21 26 0c 	ori r1,r1,0x260c
}
    1930:	28 21 00 14 	lw r1,(r1+20)
    1934:	c3 a0 00 00 	ret

00001938 <pp_vsprintf>:
		*(out++) = tmp[i++];
	return ret;
}

int pp_vsprintf(char *buf, const char *fmt, va_list args)
{
    1938:	37 9c ff a8 	addi sp,sp,-88
    193c:	5b 8b 00 44 	sw (sp+68),r11
    1940:	5b 8c 00 40 	sw (sp+64),r12
    1944:	5b 8d 00 3c 	sw (sp+60),r13
    1948:	5b 8e 00 38 	sw (sp+56),r14
    194c:	5b 8f 00 34 	sw (sp+52),r15
    1950:	5b 90 00 30 	sw (sp+48),r16
    1954:	5b 91 00 2c 	sw (sp+44),r17
    1958:	5b 92 00 28 	sw (sp+40),r18
    195c:	5b 93 00 24 	sw (sp+36),r19
    1960:	5b 94 00 20 	sw (sp+32),r20
    1964:	5b 95 00 1c 	sw (sp+28),r21
    1968:	5b 96 00 18 	sw (sp+24),r22
    196c:	5b 97 00 14 	sw (sp+20),r23
    1970:	5b 98 00 10 	sw (sp+16),r24
    1974:	5b 99 00 0c 	sw (sp+12),r25
    1978:	5b 9b 00 08 	sw (sp+8),fp
    197c:	5b 9d 00 04 	sw (sp+4),ra
    1980:	b8 40 70 00 	mv r14,r2
	char *s, *str = buf;
	int base, lead, wid;

	for (; *fmt ; ++fmt) {
    1984:	40 42 00 00 	lbu r2,(r2+0)
		*(out++) = tmp[i++];
	return ret;
}

int pp_vsprintf(char *buf, const char *fmt, va_list args)
{
    1988:	b8 20 c0 00 	mv r24,r1
	char *s, *str = buf;
    198c:	b8 20 68 00 	mv r13,r1
		*(out++) = tmp[i++];
	return ret;
}

int pp_vsprintf(char *buf, const char *fmt, va_list args)
{
    1990:	b8 60 98 00 	mv r19,r3
	char *s, *str = buf;
	int base, lead, wid;

	for (; *fmt ; ++fmt) {
    1994:	34 01 00 00 	mvi r1,0
    1998:	44 40 00 1b 	be r2,r0,1a04 <pp_vsprintf+0xcc>
    199c:	78 16 00 00 	mvhi r22,0x0
    19a0:	78 11 00 00 	mvhi r17,0x0
		if (*fmt != '%') {
    19a4:	34 14 00 25 	mvi r20,37
    19a8:	3a d6 23 00 	ori r22,r22,0x2300
		base = 10;
		lead = ' ';
		wid = 1;
	repeat:
		fmt++;		/* Skip '%' initially, other stuff later */
		switch(*fmt) {
    19ac:	34 19 00 78 	mvi r25,120
	while (i > 16 - wid + negative)
		tmp[--i] = lead;
	if (negative)
		tmp[--i] = '-';
	ret = 16 - i;
	while (i < 16)
    19b0:	34 12 00 0f 	mvi r18,15
		*(out++) = tmp[i++];
	return ret;
}

int pp_vsprintf(char *buf, const char *fmt, va_list args)
    19b4:	37 8f 00 4c 	addi r15,sp,76
    19b8:	3a 31 22 ec 	ori r17,r17,0x22ec
    19bc:	e0 00 00 06 	bi 19d4 <pp_vsprintf+0x9c>
	char *s, *str = buf;
	int base, lead, wid;

	for (; *fmt ; ++fmt) {
		if (*fmt != '%') {
			*str++ = *fmt;
    19c0:	31 a2 00 00 	sb (r13+0),r2
    19c4:	35 ad 00 01 	addi r13,r13,1
int pp_vsprintf(char *buf, const char *fmt, va_list args)
{
	char *s, *str = buf;
	int base, lead, wid;

	for (; *fmt ; ++fmt) {
    19c8:	35 ce 00 01 	addi r14,r14,1
    19cc:	41 c2 00 00 	lbu r2,(r14+0)
    19d0:	44 40 00 0c 	be r2,r0,1a00 <pp_vsprintf+0xc8>
		if (*fmt != '%') {
    19d4:	34 1b 00 01 	mvi fp,1
    19d8:	34 15 00 20 	mvi r21,32
    19dc:	34 10 00 0a 	mvi r16,10
    19e0:	5c 54 ff f8 	bne r2,r20,19c0 <pp_vsprintf+0x88>

		base = 10;
		lead = ' ';
		wid = 1;
	repeat:
		fmt++;		/* Skip '%' initially, other stuff later */
    19e4:	35 ce 00 01 	addi r14,r14,1
		switch(*fmt) {
    19e8:	41 c2 00 00 	lbu r2,(r14+0)
    19ec:	54 59 00 99 	bgu r2,r25,1c50 <pp_vsprintf+0x318>
    19f0:	3c 41 00 02 	sli r1,r2,2
    19f4:	b6 c1 08 00 	add r1,r22,r1
    19f8:	28 21 00 00 	lw r1,(r1+0)
    19fc:	c0 20 00 00 	b r1
int pp_vsprintf(char *buf, const char *fmt, va_list args)
{
	char *s, *str = buf;
	int base, lead, wid;

	for (; *fmt ; ++fmt) {
    1a00:	c9 b8 08 00 	sub r1,r13,r24
			str += number(str, va_arg(args, int), base, lead, wid);
			break;
		}
	}
 ret:
	*str = '\0';
    1a04:	31 a0 00 00 	sb (r13+0),r0
	return str - buf;


}
    1a08:	2b 9d 00 04 	lw ra,(sp+4)
    1a0c:	2b 8b 00 44 	lw r11,(sp+68)
    1a10:	2b 8c 00 40 	lw r12,(sp+64)
    1a14:	2b 8d 00 3c 	lw r13,(sp+60)
    1a18:	2b 8e 00 38 	lw r14,(sp+56)
    1a1c:	2b 8f 00 34 	lw r15,(sp+52)
    1a20:	2b 90 00 30 	lw r16,(sp+48)
    1a24:	2b 91 00 2c 	lw r17,(sp+44)
    1a28:	2b 92 00 28 	lw r18,(sp+40)
    1a2c:	2b 93 00 24 	lw r19,(sp+36)
    1a30:	2b 94 00 20 	lw r20,(sp+32)
    1a34:	2b 95 00 1c 	lw r21,(sp+28)
    1a38:	2b 96 00 18 	lw r22,(sp+24)
    1a3c:	2b 97 00 14 	lw r23,(sp+20)
    1a40:	2b 98 00 10 	lw r24,(sp+16)
    1a44:	2b 99 00 0c 	lw r25,(sp+12)
    1a48:	2b 9b 00 08 	lw fp,(sp+8)
    1a4c:	37 9c 00 58 	addi sp,sp,88
    1a50:	c3 a0 00 00 	ret
			if (base == 10) /* yet unchaged */
				base = 8;
		case 'd':
		case 'i':
		case 'u':
			str += number(str, va_arg(args, int), base, lead, wid);
    1a54:	2a 6c 00 00 	lw r12,(r19+0)
{
	char tmp[16];
	int i = 16, ret, negative = 0;

	/* No error checking at all: it is as ugly as possible */
	if ((signed)value < 0 && base == 10) {
    1a58:	66 01 00 0a 	cmpei r1,r16,10
    1a5c:	5b 80 00 48 	sw (sp+72),r0
    1a60:	01 82 00 1f 	srui r2,r12,31
			if (base == 10) /* yet unchaged */
				base = 8;
		case 'd':
		case 'i':
		case 'u':
			str += number(str, va_arg(args, int), base, lead, wid);
    1a64:	36 73 00 04 	addi r19,r19,4
{
	char tmp[16];
	int i = 16, ret, negative = 0;

	/* No error checking at all: it is as ugly as possible */
	if ((signed)value < 0 && base == 10) {
    1a68:	a0 41 08 00 	and r1,r2,r1
static const char hex[] = "0123456789abcdef";

static int number(char *out, unsigned value, int base, int lead, int wid)
{
	char tmp[16];
	int i = 16, ret, negative = 0;
    1a6c:	34 17 00 00 	mvi r23,0

	/* No error checking at all: it is as ugly as possible */
	if ((signed)value < 0 && base == 10) {
    1a70:	44 20 00 06 	be r1,r0,1a88 <pp_vsprintf+0x150>
		negative = 1;
		value = -value;
    1a74:	66 a3 00 20 	cmpei r3,r21,32
    1a78:	c8 0c 60 00 	sub r12,r0,r12
    1a7c:	5b 83 00 48 	sw (sp+72),r3
    1a80:	34 10 00 0a 	mvi r16,10
	char tmp[16];
	int i = 16, ret, negative = 0;

	/* No error checking at all: it is as ugly as possible */
	if ((signed)value < 0 && base == 10) {
		negative = 1;
    1a84:	34 17 00 01 	mvi r23,1
		value = -value;
	}
	while (value && i) {
    1a88:	45 80 00 78 	be r12,r0,1c68 <pp_vsprintf+0x330>
    1a8c:	34 0b 00 10 	mvi r11,16
		tmp[--i] = hex[value % base];
    1a90:	b9 80 08 00 	mv r1,r12
    1a94:	ba 00 10 00 	mv r2,r16
    1a98:	f8 00 01 1b 	calli 1f04 <__umodsi3>
    1a9c:	b6 21 08 00 	add r1,r17,r1
    1aa0:	40 24 00 00 	lbu r4,(r1+0)
    1aa4:	35 6b ff ff 	addi r11,r11,-1
	while (i < 16)
		*(out++) = tmp[i++];
	return ret;
}

int pp_vsprintf(char *buf, const char *fmt, va_list args)
    1aa8:	b5 eb 10 00 	add r2,r15,r11
	if ((signed)value < 0 && base == 10) {
		negative = 1;
		value = -value;
	}
	while (value && i) {
		tmp[--i] = hex[value % base];
    1aac:	30 44 00 00 	sb (r2+0),r4
		value /= base;
    1ab0:	b9 80 08 00 	mv r1,r12
    1ab4:	ba 00 10 00 	mv r2,r16
    1ab8:	f8 00 01 03 	calli 1ec4 <__udivsi3>
	/* No error checking at all: it is as ugly as possible */
	if ((signed)value < 0 && base == 10) {
		negative = 1;
		value = -value;
	}
	while (value && i) {
    1abc:	7d 64 00 00 	cmpnei r4,r11,0
    1ac0:	7c 22 00 00 	cmpnei r2,r1,0
		tmp[--i] = hex[value % base];
		value /= base;
    1ac4:	b8 20 60 00 	mv r12,r1
	/* No error checking at all: it is as ugly as possible */
	if ((signed)value < 0 && base == 10) {
		negative = 1;
		value = -value;
	}
	while (value && i) {
    1ac8:	a0 82 10 00 	and r2,r4,r2
    1acc:	5c 40 ff f1 	bne r2,r0,1a90 <pp_vsprintf+0x158>
		tmp[--i] = hex[value % base];
		value /= base;
	}
	if (i == 16)
		tmp[--i] = '0';
	if (negative && lead == ' ') {
    1ad0:	2b 82 00 48 	lw r2,(sp+72)
    1ad4:	44 40 00 06 	be r2,r0,1aec <pp_vsprintf+0x1b4>
		tmp[--i] = '-';
    1ad8:	35 6b ff ff 	addi r11,r11,-1
    1adc:	b5 eb 08 00 	add r1,r15,r11
    1ae0:	34 03 00 2d 	mvi r3,45
    1ae4:	30 23 00 00 	sb (r1+0),r3
		negative = 0;
    1ae8:	34 17 00 00 	mvi r23,0
	}
	while (i > 16 - wid + negative)
    1aec:	34 01 00 10 	mvi r1,16
    1af0:	c8 3b 20 00 	sub r4,r1,fp
    1af4:	b4 97 20 00 	add r4,r4,r23
    1af8:	4c 8b 00 06 	bge r4,r11,1b10 <pp_vsprintf+0x1d8>
    1afc:	22 a3 00 ff 	andi r3,r21,0xff
		tmp[--i] = lead;
    1b00:	35 6b ff ff 	addi r11,r11,-1
	while (i < 16)
		*(out++) = tmp[i++];
	return ret;
}

int pp_vsprintf(char *buf, const char *fmt, va_list args)
    1b04:	b5 eb 10 00 	add r2,r15,r11
	if (negative && lead == ' ') {
		tmp[--i] = '-';
		negative = 0;
	}
	while (i > 16 - wid + negative)
		tmp[--i] = lead;
    1b08:	30 43 00 00 	sb (r2+0),r3
		tmp[--i] = '0';
	if (negative && lead == ' ') {
		tmp[--i] = '-';
		negative = 0;
	}
	while (i > 16 - wid + negative)
    1b0c:	5d 64 ff fd 	bne r11,r4,1b00 <pp_vsprintf+0x1c8>
		tmp[--i] = lead;
	if (negative)
    1b10:	46 e0 00 05 	be r23,r0,1b24 <pp_vsprintf+0x1ec>
		tmp[--i] = '-';
    1b14:	35 6b ff ff 	addi r11,r11,-1
    1b18:	b5 eb 08 00 	add r1,r15,r11
    1b1c:	34 02 00 2d 	mvi r2,45
    1b20:	30 22 00 00 	sb (r1+0),r2
	ret = 16 - i;
    1b24:	34 03 00 10 	mvi r3,16
    1b28:	c8 6b 08 00 	sub r1,r3,r11
	while (i < 16)
    1b2c:	49 72 00 20 	bg r11,r18,1bac <pp_vsprintf+0x274>
		*(out++) = tmp[i++];
	return ret;
}

int pp_vsprintf(char *buf, const char *fmt, va_list args)
    1b30:	a5 60 48 00 	not r9,r11
    1b34:	35 29 00 11 	addi r9,r9,17
    1b38:	b5 eb 38 00 	add r7,r15,r11
    1b3c:	01 28 00 02 	srui r8,r9,2
    1b40:	b8 ed 28 00 	or r5,r7,r13
    1b44:	20 a5 00 03 	andi r5,r5,0x3
    1b48:	3d 03 00 02 	sli r3,r8,2
    1b4c:	64 a5 00 00 	cmpei r5,r5,0
    1b50:	75 22 00 03 	cmpgui r2,r9,0x3
    1b54:	64 64 00 00 	cmpei r4,r3,0
    1b58:	a0 a2 10 00 	and r2,r5,r2
    1b5c:	18 42 00 01 	xori r2,r2,0x1
    1b60:	b8 82 10 00 	or r2,r4,r2
    1b64:	5c 40 00 45 	bne r2,r0,1c78 <pp_vsprintf+0x340>
    1b68:	34 04 00 00 	mvi r4,0
    1b6c:	b4 e4 28 00 	add r5,r7,r4
		tmp[--i] = lead;
	if (negative)
		tmp[--i] = '-';
	ret = 16 - i;
	while (i < 16)
		*(out++) = tmp[i++];
    1b70:	28 a6 00 00 	lw r6,(r5+0)
	return ret;
}

int pp_vsprintf(char *buf, const char *fmt, va_list args)
    1b74:	b5 a4 28 00 	add r5,r13,r4
		tmp[--i] = lead;
	if (negative)
		tmp[--i] = '-';
	ret = 16 - i;
	while (i < 16)
		*(out++) = tmp[i++];
    1b78:	34 42 00 01 	addi r2,r2,1
    1b7c:	58 a6 00 00 	sw (r5+0),r6
    1b80:	34 84 00 04 	addi r4,r4,4
    1b84:	55 02 ff fa 	bgu r8,r2,1b6c <pp_vsprintf+0x234>
    1b88:	b5 63 58 00 	add r11,r11,r3
    1b8c:	b5 a3 10 00 	add r2,r13,r3
    1b90:	44 69 00 07 	be r3,r9,1bac <pp_vsprintf+0x274>
	return ret;
}

int pp_vsprintf(char *buf, const char *fmt, va_list args)
    1b94:	b5 eb 20 00 	add r4,r15,r11
		tmp[--i] = lead;
	if (negative)
		tmp[--i] = '-';
	ret = 16 - i;
	while (i < 16)
		*(out++) = tmp[i++];
    1b98:	40 83 00 00 	lbu r3,(r4+0)
    1b9c:	35 6b 00 01 	addi r11,r11,1
    1ba0:	30 43 00 00 	sb (r2+0),r3
    1ba4:	34 42 00 01 	addi r2,r2,1
	while (i > 16 - wid + negative)
		tmp[--i] = lead;
	if (negative)
		tmp[--i] = '-';
	ret = 16 - i;
	while (i < 16)
    1ba8:	4e 4b ff fb 	bge r18,r11,1b94 <pp_vsprintf+0x25c>
			if (base == 10) /* yet unchaged */
				base = 8;
		case 'd':
		case 'i':
		case 'u':
			str += number(str, va_arg(args, int), base, lead, wid);
    1bac:	b5 a1 68 00 	add r13,r13,r1
			break;
    1bb0:	e3 ff ff 86 	bi 19c8 <pp_vsprintf+0x90>

		case 'c': /* char: supported */
			*str++ = (unsigned char) va_arg(args, int);
			break;
		case 's': /* string: supported */
			s = va_arg(args, char *);
    1bb4:	2a 63 00 00 	lw r3,(r19+0)
    1bb8:	36 73 00 04 	addi r19,r19,4
			while (*s)
    1bbc:	40 62 00 00 	lbu r2,(r3+0)
    1bc0:	44 40 ff 82 	be r2,r0,19c8 <pp_vsprintf+0x90>
				*str++ = *s++;
    1bc4:	31 a2 00 00 	sb (r13+0),r2
    1bc8:	34 63 00 01 	addi r3,r3,1
		case 'c': /* char: supported */
			*str++ = (unsigned char) va_arg(args, int);
			break;
		case 's': /* string: supported */
			s = va_arg(args, char *);
			while (*s)
    1bcc:	40 62 00 00 	lbu r2,(r3+0)
				*str++ = *s++;
    1bd0:	35 ad 00 01 	addi r13,r13,1
		case 'c': /* char: supported */
			*str++ = (unsigned char) va_arg(args, int);
			break;
		case 's': /* string: supported */
			s = va_arg(args, char *);
			while (*s)
    1bd4:	5c 40 ff fc 	bne r2,r0,1bc4 <pp_vsprintf+0x28c>
    1bd8:	e3 ff ff 7c 	bi 19c8 <pp_vsprintf+0x90>
		case 'p':
		case 'x':
		case 'X':
			base = 16;
		case 'o':
			if (base == 10) /* yet unchaged */
    1bdc:	34 02 00 0a 	mvi r2,10
		base = 10;
		lead = ' ';
		wid = 1;
	repeat:
		fmt++;		/* Skip '%' initially, other stuff later */
		switch(*fmt) {
    1be0:	ba 60 08 00 	mv r1,r19
		case 'p':
		case 'x':
		case 'X':
			base = 16;
		case 'o':
			if (base == 10) /* yet unchaged */
    1be4:	5e 02 ff 9c 	bne r16,r2,1a54 <pp_vsprintf+0x11c>
				base = 8;
		case 'd':
		case 'i':
		case 'u':
			str += number(str, va_arg(args, int), base, lead, wid);
    1be8:	28 2c 00 00 	lw r12,(r1+0)
    1bec:	36 73 00 04 	addi r19,r19,4
    1bf0:	5b 80 00 48 	sw (sp+72),r0
		case 'x':
		case 'X':
			base = 16;
		case 'o':
			if (base == 10) /* yet unchaged */
				base = 8;
    1bf4:	34 10 00 08 	mvi r16,8
static const char hex[] = "0123456789abcdef";

static int number(char *out, unsigned value, int base, int lead, int wid)
{
	char tmp[16];
	int i = 16, ret, negative = 0;
    1bf8:	34 17 00 00 	mvi r23,0
    1bfc:	e3 ff ff a3 	bi 1a88 <pp_vsprintf+0x150>
			goto repeat;

			/* Special cases for conversions */

		case 'c': /* char: supported */
			*str++ = (unsigned char) va_arg(args, int);
    1c00:	2a 61 00 00 	lw r1,(r19+0)
    1c04:	36 73 00 04 	addi r19,r19,4
    1c08:	31 a1 00 00 	sb (r13+0),r1
    1c0c:	35 ad 00 01 	addi r13,r13,1
			break;
    1c10:	e3 ff ff 6e 	bi 19c8 <pp_vsprintf+0x90>
			if (base == 10) /* yet unchaged */
				base = 8;
		case 'd':
		case 'i':
		case 'u':
			str += number(str, va_arg(args, int), base, lead, wid);
    1c14:	2a 6c 00 00 	lw r12,(r19+0)

			/* integers are more or less printed */
		case 'p':
		case 'x':
		case 'X':
			base = 16;
    1c18:	34 10 00 10 	mvi r16,16
			if (base == 10) /* yet unchaged */
				base = 8;
		case 'd':
		case 'i':
		case 'u':
			str += number(str, va_arg(args, int), base, lead, wid);
    1c1c:	5b 80 00 48 	sw (sp+72),r0
    1c20:	36 73 00 04 	addi r19,r19,4
static const char hex[] = "0123456789abcdef";

static int number(char *out, unsigned value, int base, int lead, int wid)
{
	char tmp[16];
	int i = 16, ret, negative = 0;
    1c24:	34 17 00 00 	mvi r23,0
    1c28:	e3 ff ff 98 	bi 1a88 <pp_vsprintf+0x150>
		fmt++;		/* Skip '%' initially, other stuff later */
		switch(*fmt) {
		case '\0':
			goto ret;
		case '0':
			lead = '0';
    1c2c:	34 15 00 30 	mvi r21,48
    1c30:	e3 ff ff 6d 	bi 19e4 <pp_vsprintf+0xac>
			goto repeat;

		case '*':
			/* should be precision, just eat it */
			base = va_arg(args, int);
    1c34:	2a 70 00 00 	lw r16,(r19+0)
    1c38:	36 73 00 04 	addi r19,r19,4
    1c3c:	e3 ff ff 6a 	bi 19e4 <pp_vsprintf+0xac>
				*str++ = *s++;
			break;
		case 'n': /* number-thus-far: not supported */
			break;
		case '%': /* supported */
			*str++ = '%';
    1c40:	34 01 00 25 	mvi r1,37
    1c44:	31 a1 00 00 	sb (r13+0),r1
    1c48:	35 ad 00 01 	addi r13,r13,1
			break;
    1c4c:	e3 ff ff 5f 	bi 19c8 <pp_vsprintf+0x90>
		case '*':
			/* should be precision, just eat it */
			base = va_arg(args, int);
			/* fall through: discard unknown stuff */
		default:
			if (*fmt >= '1' && *fmt <= '9')
    1c50:	34 41 ff cf 	addi r1,r2,-49
    1c54:	20 21 00 ff 	andi r1,r1,0xff
    1c58:	34 03 00 08 	mvi r3,8
    1c5c:	54 23 ff 62 	bgu r1,r3,19e4 <pp_vsprintf+0xac>
				wid = *fmt - '0';
    1c60:	34 5b ff d0 	addi fp,r2,-48
    1c64:	e3 ff ff 60 	bi 19e4 <pp_vsprintf+0xac>
	while (value && i) {
		tmp[--i] = hex[value % base];
		value /= base;
	}
	if (i == 16)
		tmp[--i] = '0';
    1c68:	34 01 00 30 	mvi r1,48
    1c6c:	33 81 00 5b 	sb (sp+91),r1
    1c70:	34 0b 00 0f 	mvi r11,15
    1c74:	e3 ff ff 97 	bi 1ad0 <pp_vsprintf+0x198>
	while (i < 16)
		*(out++) = tmp[i++];
	return ret;
}

int pp_vsprintf(char *buf, const char *fmt, va_list args)
    1c78:	b9 a0 10 00 	mv r2,r13
    1c7c:	e3 ff ff c6 	bi 1b94 <pp_vsprintf+0x25c>

00001c80 <pp_vprintf>:
#include <pp-printf.h>

static char print_buf[CONFIG_PRINT_BUFSIZE];

int pp_vprintf(const char *fmt, va_list args)
{
    1c80:	37 9c ff f4 	addi sp,sp,-12
    1c84:	5b 8b 00 0c 	sw (sp+12),r11
    1c88:	5b 8c 00 08 	sw (sp+8),r12
    1c8c:	5b 9d 00 04 	sw (sp+4),ra
	int ret;

	ret = pp_vsprintf(print_buf, fmt, args);
    1c90:	78 0b 00 00 	mvhi r11,0x0
    1c94:	39 6b 47 2c 	ori r11,r11,0x472c
#include <pp-printf.h>

static char print_buf[CONFIG_PRINT_BUFSIZE];

int pp_vprintf(const char *fmt, va_list args)
{
    1c98:	b8 40 18 00 	mv r3,r2
	int ret;

	ret = pp_vsprintf(print_buf, fmt, args);
    1c9c:	b8 20 10 00 	mv r2,r1
    1ca0:	b9 60 08 00 	mv r1,r11
    1ca4:	fb ff ff 25 	calli 1938 <pp_vsprintf>
    1ca8:	b8 20 60 00 	mv r12,r1
	puts(print_buf);
    1cac:	b9 60 08 00 	mv r1,r11
    1cb0:	f8 00 00 34 	calli 1d80 <puts>
	return ret;
}
    1cb4:	b9 80 08 00 	mv r1,r12
    1cb8:	2b 9d 00 04 	lw ra,(sp+4)
    1cbc:	2b 8b 00 0c 	lw r11,(sp+12)
    1cc0:	2b 8c 00 08 	lw r12,(sp+8)
    1cc4:	37 9c 00 0c 	addi sp,sp,12
    1cc8:	c3 a0 00 00 	ret

00001ccc <pp_sprintf>:

int pp_sprintf(char *s, const char *fmt, ...)
{
    1ccc:	37 9c ff e0 	addi sp,sp,-32
    1cd0:	5b 9d 00 04 	sw (sp+4),ra
    1cd4:	5b 83 00 0c 	sw (sp+12),r3
	va_list args;
	int ret;

	va_start(args, fmt);
	ret = pp_vsprintf(s, fmt, args);
    1cd8:	37 83 00 0c 	addi r3,sp,12
	puts(print_buf);
	return ret;
}

int pp_sprintf(char *s, const char *fmt, ...)
{
    1cdc:	5b 82 00 08 	sw (sp+8),r2
    1ce0:	5b 84 00 10 	sw (sp+16),r4
    1ce4:	5b 85 00 14 	sw (sp+20),r5
    1ce8:	5b 86 00 18 	sw (sp+24),r6
    1cec:	5b 87 00 1c 	sw (sp+28),r7
    1cf0:	5b 88 00 20 	sw (sp+32),r8
	va_list args;
	int ret;

	va_start(args, fmt);
	ret = pp_vsprintf(s, fmt, args);
    1cf4:	fb ff ff 11 	calli 1938 <pp_vsprintf>
	va_end(args);
	return ret;
}
    1cf8:	2b 9d 00 04 	lw ra,(sp+4)
    1cfc:	37 9c 00 20 	addi sp,sp,32
    1d00:	c3 a0 00 00 	ret

00001d04 <pp_printf>:


int pp_printf(const char *fmt, ...)
{
    1d04:	37 9c ff d4 	addi sp,sp,-44
    1d08:	5b 8b 00 0c 	sw (sp+12),r11
    1d0c:	5b 8c 00 08 	sw (sp+8),r12
    1d10:	5b 9d 00 04 	sw (sp+4),ra

int pp_vprintf(const char *fmt, va_list args)
{
	int ret;

	ret = pp_vsprintf(print_buf, fmt, args);
    1d14:	78 0b 00 00 	mvhi r11,0x0
    1d18:	39 6b 47 2c 	ori r11,r11,0x472c
	return ret;
}


int pp_printf(const char *fmt, ...)
{
    1d1c:	5b 81 00 10 	sw (sp+16),r1
    1d20:	5b 82 00 14 	sw (sp+20),r2
    1d24:	5b 83 00 18 	sw (sp+24),r3

int pp_vprintf(const char *fmt, va_list args)
{
	int ret;

	ret = pp_vsprintf(print_buf, fmt, args);
    1d28:	b8 20 10 00 	mv r2,r1
    1d2c:	37 83 00 14 	addi r3,sp,20
    1d30:	b9 60 08 00 	mv r1,r11
	return ret;
}


int pp_printf(const char *fmt, ...)
{
    1d34:	5b 84 00 1c 	sw (sp+28),r4
    1d38:	5b 85 00 20 	sw (sp+32),r5
    1d3c:	5b 86 00 24 	sw (sp+36),r6
    1d40:	5b 87 00 28 	sw (sp+40),r7
    1d44:	5b 88 00 2c 	sw (sp+44),r8

int pp_vprintf(const char *fmt, va_list args)
{
	int ret;

	ret = pp_vsprintf(print_buf, fmt, args);
    1d48:	fb ff fe fc 	calli 1938 <pp_vsprintf>
    1d4c:	b8 20 60 00 	mv r12,r1
	puts(print_buf);
    1d50:	b9 60 08 00 	mv r1,r11
    1d54:	f8 00 00 0b 	calli 1d80 <puts>
	va_start(args, fmt);
	ret = pp_vprintf(fmt, args);
	va_end(args);

	return ret;
}
    1d58:	b9 80 08 00 	mv r1,r12
    1d5c:	2b 9d 00 04 	lw ra,(sp+4)
    1d60:	2b 8b 00 0c 	lw r11,(sp+12)
    1d64:	2b 8c 00 08 	lw r12,(sp+8)
    1d68:	37 9c 00 2c 	addi sp,sp,44
    1d6c:	c3 a0 00 00 	ret

00001d70 <rt_set_debug_slot>:

static int debug_slot;

void rt_set_debug_slot(int slot)
{
    debug_slot = slot;
    1d70:	78 02 00 00 	mvhi r2,0x0
    1d74:	38 42 47 ac 	ori r2,r2,0x47ac
    1d78:	58 41 00 00 	sw (r2+0),r1
}
    1d7c:	c3 a0 00 00 	ret

00001d80 <puts>:
}

static void *mq_map_out_buffer(int remote, int slot)
{
  uint32_t base = remote ? RMQ_BASE : HMQ_BASE;
  return (void *) (base + MQ_OUT (slot) + MQ_SLOT_DATA_START );
    1d80:	78 02 00 00 	mvhi r2,0x0
    1d84:	38 42 47 ac 	ori r2,r2,0x47ac
    1d88:	28 42 00 00 	lw r2,(r2+0)
    1d8c:	78 06 40 01 	mvhi r6,0x4001
static inline void mq_writel( int remote, uint32_t val, uint32_t reg )
{
  if(remote)
    * (volatile uint32_t * ) (RMQ_BASE + reg) = val ;
  else
    * (volatile uint32_t * ) (HMQ_BASE + reg) = val ;
    1d90:	78 03 01 00 	mvhi r3,0x100
}

static void *mq_map_out_buffer(int remote, int slot)
{
  uint32_t base = remote ? RMQ_BASE : HMQ_BASE;
  return (void *) (base + MQ_OUT (slot) + MQ_SLOT_DATA_START );
    1d94:	34 42 00 20 	addi r2,r2,32
    1d98:	3c 42 00 0a 	sli r2,r2,10

int puts(const char *p)
{
    1d9c:	b8 20 20 00 	mv r4,r1
    1da0:	b4 46 30 00 	add r6,r2,r6
static inline void mq_writel( int remote, uint32_t val, uint32_t reg )
{
  if(remote)
    * (volatile uint32_t * ) (RMQ_BASE + reg) = val ;
  else
    * (volatile uint32_t * ) (HMQ_BASE + reg) = val ;
    1da4:	58 c3 00 00 	sw (r6+0),r3
    int i;
    volatile uint32_t *buf = mq_map_out_buffer(0, debug_slot);
    
    mq_claim(0, debug_slot);
    
    buf[0] = 0xdeadbeef;
    1da8:	78 03 00 00 	mvhi r3,0x0
    1dac:	38 63 24 e4 	ori r3,r3,0x24e4
    1db0:	28 61 00 00 	lw r1,(r3+0)
    1db4:	78 03 00 00 	mvhi r3,0x0
    1db8:	38 63 24 e8 	ori r3,r3,0x24e8
    1dbc:	58 c1 00 08 	sw (r6+8),r1
    1dc0:	28 61 00 00 	lw r1,(r3+0)
    for(i=0;i<127;i++,p++)
    1dc4:	34 05 00 7f 	mvi r5,127
    int i;
    volatile uint32_t *buf = mq_map_out_buffer(0, debug_slot);
    
    mq_claim(0, debug_slot);
    
    buf[0] = 0xdeadbeef;
    1dc8:	b4 41 10 00 	add r2,r2,r1
    for(i=0;i<127;i++,p++)
    1dcc:	34 01 00 00 	mvi r1,0
    1dd0:	e0 00 00 05 	bi 1de4 <puts+0x64>
      {
  	   if(*p)
	       buf[i+1] = *p;
    1dd4:	58 43 00 00 	sw (r2+0),r3
    volatile uint32_t *buf = mq_map_out_buffer(0, debug_slot);
    
    mq_claim(0, debug_slot);
    
    buf[0] = 0xdeadbeef;
    for(i=0;i<127;i++,p++)
    1dd8:	34 21 00 01 	addi r1,r1,1
    1ddc:	34 42 00 04 	addi r2,r2,4
    1de0:	44 25 00 04 	be r1,r5,1df0 <puts+0x70>
int	_EXFUN(getc, (FILE *));
int	_EXFUN(getchar, (void));
char *  _EXFUN(gets, (char *));
int	_EXFUN(putc, (int, FILE *));
int	_EXFUN(putchar, (int));
int	_EXFUN(puts, (const char *));
    1de4:	b4 81 18 00 	add r3,r4,r1
      {
  	   if(*p)
    1de8:	40 63 00 00 	lbu r3,(r3+0)
    1dec:	5c 60 ff fa 	bne r3,r0,1dd4 <puts+0x54>
	       buf[i+1] = *p;
	     else
	       break;
      }

    mq_send(0, debug_slot, i + 1);
    1df0:	34 23 00 01 	addi r3,r1,1
  mq_writel ( remote, MQ_CMD_CLAIM, MQ_OUT(slot) + MQ_SLOT_COMMAND );
}

static inline void mq_send( int remote, int slot, int count)
{
  mq_writel ( remote, MQ_CMD_READY | count, MQ_OUT(slot) + MQ_SLOT_COMMAND );
    1df4:	78 02 04 00 	mvhi r2,0x400
    1df8:	b8 62 10 00 	or r2,r3,r2
static inline void mq_writel( int remote, uint32_t val, uint32_t reg )
{
  if(remote)
    * (volatile uint32_t * ) (RMQ_BASE + reg) = val ;
  else
    * (volatile uint32_t * ) (HMQ_BASE + reg) = val ;
    1dfc:	58 c2 00 00 	sw (r6+0),r2
    return i;
}
    1e00:	c3 a0 00 00 	ret

00001e04 <__divsi3>:
    1e04:	37 9c ff f8 	addi sp,sp,-8
    1e08:	5b 8b 00 08 	sw (sp+8),r11
    1e0c:	5b 9d 00 04 	sw (sp+4),ra
    1e10:	44 40 00 25 	be r2,r0,1ea4 <__divsi3+0xa0>
    1e14:	b8 41 20 00 	or r4,r2,r1
    1e18:	34 03 00 0f 	mvi r3,15
    1e1c:	54 83 00 0e 	bgu r4,r3,1e54 <__divsi3+0x50>
    1e20:	b4 21 08 00 	add r1,r1,r1
    1e24:	78 03 00 00 	mvhi r3,0x0
    1e28:	b4 21 08 00 	add r1,r1,r1
    1e2c:	38 63 24 ec 	ori r3,r3,0x24ec
    1e30:	b4 21 08 00 	add r1,r1,r1
    1e34:	b4 62 10 00 	add r2,r3,r2
    1e38:	b4 21 08 00 	add r1,r1,r1
    1e3c:	b4 41 08 00 	add r1,r2,r1
    1e40:	40 21 00 00 	lbu r1,(r1+0)
    1e44:	2b 9d 00 04 	lw ra,(sp+4)
    1e48:	2b 8b 00 08 	lw r11,(sp+8)
    1e4c:	37 9c 00 08 	addi sp,sp,8
    1e50:	c3 a0 00 00 	ret
    1e54:	34 0b 00 00 	mvi r11,0
    1e58:	4c 20 00 03 	bge r1,r0,1e64 <__divsi3+0x60>
    1e5c:	c8 01 08 00 	sub r1,r0,r1
    1e60:	34 0b 00 01 	mvi r11,1
    1e64:	4c 40 00 03 	bge r2,r0,1e70 <__divsi3+0x6c>
    1e68:	c8 02 10 00 	sub r2,r0,r2
    1e6c:	19 6b 00 01 	xori r11,r11,0x1
    1e70:	90 c0 18 00 	rcsr r3,CFG
    1e74:	20 63 00 02 	andi r3,r3,0x2
    1e78:	44 60 00 08 	be r3,r0,1e98 <__divsi3+0x94>
    1e7c:	8c 22 08 00 	divu r1,r1,r2
    1e80:	45 60 00 02 	be r11,r0,1e88 <__divsi3+0x84>
    1e84:	c8 01 08 00 	sub r1,r0,r1
    1e88:	2b 9d 00 04 	lw ra,(sp+4)
    1e8c:	2b 8b 00 08 	lw r11,(sp+8)
    1e90:	37 9c 00 08 	addi sp,sp,8
    1e94:	c3 a0 00 00 	ret
    1e98:	34 03 00 00 	mvi r3,0
    1e9c:	f8 00 00 2a 	calli 1f44 <__udivmodsi4>
    1ea0:	e3 ff ff f8 	bi 1e80 <__divsi3+0x7c>
    1ea4:	90 00 08 00 	rcsr r1,IE
    1ea8:	20 21 00 01 	andi r1,r1,0x1
    1eac:	b4 21 08 00 	add r1,r1,r1
    1eb0:	d0 01 00 00 	wcsr IE,r1
    1eb4:	90 e0 08 00 	rcsr r1,EBA
    1eb8:	bb a0 f0 00 	mv ea,ra
    1ebc:	34 21 00 a0 	addi r1,r1,160
    1ec0:	c0 20 00 00 	b r1

00001ec4 <__udivsi3>:
    1ec4:	37 9c ff fc 	addi sp,sp,-4
    1ec8:	5b 9d 00 04 	sw (sp+4),ra
    1ecc:	44 40 00 06 	be r2,r0,1ee4 <__udivsi3+0x20>
    1ed0:	34 03 00 00 	mvi r3,0
    1ed4:	f8 00 00 1c 	calli 1f44 <__udivmodsi4>
    1ed8:	2b 9d 00 04 	lw ra,(sp+4)
    1edc:	37 9c 00 04 	addi sp,sp,4
    1ee0:	c3 a0 00 00 	ret
    1ee4:	90 00 08 00 	rcsr r1,IE
    1ee8:	20 21 00 01 	andi r1,r1,0x1
    1eec:	b4 21 08 00 	add r1,r1,r1
    1ef0:	d0 01 00 00 	wcsr IE,r1
    1ef4:	90 e0 08 00 	rcsr r1,EBA
    1ef8:	bb a0 f0 00 	mv ea,ra
    1efc:	34 21 00 a0 	addi r1,r1,160
    1f00:	c0 20 00 00 	b r1

00001f04 <__umodsi3>:
    1f04:	37 9c ff fc 	addi sp,sp,-4
    1f08:	5b 9d 00 04 	sw (sp+4),ra
    1f0c:	44 40 00 06 	be r2,r0,1f24 <__umodsi3+0x20>
    1f10:	34 03 00 01 	mvi r3,1
    1f14:	f8 00 00 0c 	calli 1f44 <__udivmodsi4>
    1f18:	2b 9d 00 04 	lw ra,(sp+4)
    1f1c:	37 9c 00 04 	addi sp,sp,4
    1f20:	c3 a0 00 00 	ret
    1f24:	90 00 08 00 	rcsr r1,IE
    1f28:	20 21 00 01 	andi r1,r1,0x1
    1f2c:	b4 21 08 00 	add r1,r1,r1
    1f30:	d0 01 00 00 	wcsr IE,r1
    1f34:	90 e0 08 00 	rcsr r1,EBA
    1f38:	bb a0 f0 00 	mv ea,ra
    1f3c:	34 21 00 a0 	addi r1,r1,160
    1f40:	c0 20 00 00 	b r1

00001f44 <__udivmodsi4>:
    1f44:	f4 22 20 00 	cmpgu r4,r1,r2
    1f48:	44 80 00 18 	be r4,r0,1fa8 <__udivmodsi4+0x64>
    1f4c:	34 04 00 01 	mvi r4,1
    1f50:	4c 40 00 0b 	bge r2,r0,1f7c <__udivmodsi4+0x38>
    1f54:	34 05 00 00 	mvi r5,0
    1f58:	54 41 00 03 	bgu r2,r1,1f64 <__udivmodsi4+0x20>
    1f5c:	c8 22 08 00 	sub r1,r1,r2
    1f60:	b8 a4 28 00 	or r5,r5,r4
    1f64:	00 84 00 01 	srui r4,r4,1
    1f68:	00 42 00 01 	srui r2,r2,1
    1f6c:	5c 80 ff fb 	bne r4,r0,1f58 <__udivmodsi4+0x14>
    1f70:	5c 60 00 02 	bne r3,r0,1f78 <__udivmodsi4+0x34>
    1f74:	b8 a0 08 00 	mv r1,r5
    1f78:	c3 a0 00 00 	ret
    1f7c:	b4 42 10 00 	add r2,r2,r2
    1f80:	b4 84 20 00 	add r4,r4,r4
    1f84:	7c 86 00 00 	cmpnei r6,r4,0
    1f88:	f4 22 28 00 	cmpgu r5,r1,r2
    1f8c:	a0 c5 28 00 	and r5,r6,r5
    1f90:	44 a0 00 02 	be r5,r0,1f98 <__udivmodsi4+0x54>
    1f94:	4c 40 ff fa 	bge r2,r0,1f7c <__udivmodsi4+0x38>
    1f98:	34 05 00 00 	mvi r5,0
    1f9c:	44 80 ff f5 	be r4,r0,1f70 <__udivmodsi4+0x2c>
    1fa0:	34 05 00 00 	mvi r5,0
    1fa4:	e3 ff ff ed 	bi 1f58 <__udivmodsi4+0x14>
    1fa8:	34 04 00 01 	mvi r4,1
    1fac:	34 05 00 00 	mvi r5,0
    1fb0:	e3 ff ff ea 	bi 1f58 <__udivmodsi4+0x14>

00001fb4 <memset>:
    1fb4:	37 9c ff e8 	addi sp,sp,-24
    1fb8:	5b 8b 00 14 	sw (sp+20),r11
    1fbc:	5b 8c 00 10 	sw (sp+16),r12
    1fc0:	5b 8d 00 0c 	sw (sp+12),r13
    1fc4:	5b 8e 00 08 	sw (sp+8),r14
    1fc8:	5b 9d 00 04 	sw (sp+4),ra
    1fcc:	b8 20 60 00 	mv r12,r1
    1fd0:	20 21 00 03 	andi r1,r1,0x3
    1fd4:	b8 40 68 00 	mv r13,r2
    1fd8:	b9 80 58 00 	mv r11,r12
    1fdc:	44 20 00 0b 	be r1,r0,2008 <memset+0x54>
    1fe0:	44 60 00 39 	be r3,r0,20c4 <memset+0x110>
    1fe4:	34 63 ff ff 	addi r3,r3,-1
    1fe8:	20 45 00 ff 	andi r5,r2,0xff
    1fec:	e0 00 00 03 	bi 1ff8 <memset+0x44>
    1ff0:	44 60 00 35 	be r3,r0,20c4 <memset+0x110>
    1ff4:	34 63 ff ff 	addi r3,r3,-1
    1ff8:	31 65 00 00 	sb (r11+0),r5
    1ffc:	35 6b 00 01 	addi r11,r11,1
    2000:	21 64 00 03 	andi r4,r11,0x3
    2004:	5c 80 ff fb 	bne r4,r0,1ff0 <memset+0x3c>
    2008:	34 01 00 03 	mvi r1,3
    200c:	50 23 00 27 	bgeu r1,r3,20a8 <memset+0xf4>
    2010:	21 a5 00 ff 	andi r5,r13,0xff
    2014:	b4 a5 20 00 	add r4,r5,r5
    2018:	b4 84 20 00 	add r4,r4,r4
    201c:	b4 84 20 00 	add r4,r4,r4
    2020:	b4 84 20 00 	add r4,r4,r4
    2024:	b4 84 20 00 	add r4,r4,r4
    2028:	b4 84 20 00 	add r4,r4,r4
    202c:	b4 84 20 00 	add r4,r4,r4
    2030:	b4 84 20 00 	add r4,r4,r4
    2034:	b8 85 70 00 	or r14,r4,r5
    2038:	34 02 00 10 	mvi r2,16
    203c:	b9 c0 08 00 	mv r1,r14
    2040:	5b 83 00 18 	sw (sp+24),r3
    2044:	f8 00 00 3f 	calli 2140 <__ashlsi3>
    2048:	2b 83 00 18 	lw r3,(sp+24)
    204c:	34 02 00 0f 	mvi r2,15
    2050:	b8 2e 28 00 	or r5,r1,r14
    2054:	b9 60 38 00 	mv r7,r11
    2058:	b9 60 20 00 	mv r4,r11
    205c:	b8 60 30 00 	mv r6,r3
    2060:	34 01 00 0f 	mvi r1,15
    2064:	54 62 00 20 	bgu r3,r2,20e4 <memset+0x130>
    2068:	34 04 00 00 	mvi r4,0
    206c:	34 02 00 03 	mvi r2,3
    2070:	b4 e4 08 00 	add r1,r7,r4
    2074:	34 84 00 04 	addi r4,r4,4
    2078:	58 25 00 00 	sw (r1+0),r5
    207c:	c8 64 08 00 	sub r1,r3,r4
    2080:	54 22 ff fc 	bgu r1,r2,2070 <memset+0xbc>
    2084:	34 63 ff fc 	addi r3,r3,-4
    2088:	00 61 00 01 	srui r1,r3,1
    208c:	20 63 00 03 	andi r3,r3,0x3
    2090:	00 21 00 01 	srui r1,r1,1
    2094:	34 21 00 01 	addi r1,r1,1
    2098:	b4 21 08 00 	add r1,r1,r1
    209c:	b4 21 08 00 	add r1,r1,r1
    20a0:	b4 e1 38 00 	add r7,r7,r1
    20a4:	b8 e0 58 00 	mv r11,r7
    20a8:	44 60 00 07 	be r3,r0,20c4 <memset+0x110>
    20ac:	21 a2 00 ff 	andi r2,r13,0xff
    20b0:	34 04 00 00 	mvi r4,0
    20b4:	b5 64 28 00 	add r5,r11,r4
    20b8:	30 a2 00 00 	sb (r5+0),r2
    20bc:	34 84 00 01 	addi r4,r4,1
    20c0:	5c 64 ff fd 	bne r3,r4,20b4 <memset+0x100>
    20c4:	b9 80 08 00 	mv r1,r12
    20c8:	2b 9d 00 04 	lw ra,(sp+4)
    20cc:	2b 8b 00 14 	lw r11,(sp+20)
    20d0:	2b 8c 00 10 	lw r12,(sp+16)
    20d4:	2b 8d 00 0c 	lw r13,(sp+12)
    20d8:	2b 8e 00 08 	lw r14,(sp+8)
    20dc:	37 9c 00 18 	addi sp,sp,24
    20e0:	c3 a0 00 00 	ret
    20e4:	58 85 00 00 	sw (r4+0),r5
    20e8:	58 85 00 04 	sw (r4+4),r5
    20ec:	58 85 00 08 	sw (r4+8),r5
    20f0:	58 85 00 0c 	sw (r4+12),r5
    20f4:	34 c6 ff f0 	addi r6,r6,-16
    20f8:	34 84 00 10 	addi r4,r4,16
    20fc:	54 c1 ff fa 	bgu r6,r1,20e4 <memset+0x130>
    2100:	34 63 ff f0 	addi r3,r3,-16
    2104:	00 67 00 01 	srui r7,r3,1
    2108:	34 01 00 03 	mvi r1,3
    210c:	00 e7 00 01 	srui r7,r7,1
    2110:	20 63 00 0f 	andi r3,r3,0xf
    2114:	00 e7 00 01 	srui r7,r7,1
    2118:	00 e7 00 01 	srui r7,r7,1
    211c:	34 e7 00 01 	addi r7,r7,1
    2120:	b4 e7 38 00 	add r7,r7,r7
    2124:	b4 e7 38 00 	add r7,r7,r7
    2128:	b4 e7 38 00 	add r7,r7,r7
    212c:	b4 e7 38 00 	add r7,r7,r7
    2130:	b5 67 38 00 	add r7,r11,r7
    2134:	54 61 ff cd 	bgu r3,r1,2068 <memset+0xb4>
    2138:	b8 e0 58 00 	mv r11,r7
    213c:	e3 ff ff db 	bi 20a8 <memset+0xf4>

00002140 <__ashlsi3>:
    2140:	20 42 00 1f 	andi r2,r2,0x1f
    2144:	78 03 00 00 	mvhi r3,0x0
    2148:	38 63 21 d8 	ori r3,r3,0x21d8
    214c:	b4 42 10 00 	add r2,r2,r2
    2150:	b4 42 10 00 	add r2,r2,r2
    2154:	c8 62 18 00 	sub r3,r3,r2
    2158:	c0 60 00 00 	b r3

0000215c <__ashlsi3_31>:
    215c:	b4 21 08 00 	add r1,r1,r1

00002160 <__ashlsi3_30>:
    2160:	b4 21 08 00 	add r1,r1,r1

00002164 <__ashlsi3_29>:
    2164:	b4 21 08 00 	add r1,r1,r1

00002168 <__ashlsi3_28>:
    2168:	b4 21 08 00 	add r1,r1,r1

0000216c <__ashlsi3_27>:
    216c:	b4 21 08 00 	add r1,r1,r1

00002170 <__ashlsi3_26>:
    2170:	b4 21 08 00 	add r1,r1,r1

00002174 <__ashlsi3_25>:
    2174:	b4 21 08 00 	add r1,r1,r1

00002178 <__ashlsi3_24>:
    2178:	b4 21 08 00 	add r1,r1,r1

0000217c <__ashlsi3_23>:
    217c:	b4 21 08 00 	add r1,r1,r1

00002180 <__ashlsi3_22>:
    2180:	b4 21 08 00 	add r1,r1,r1

00002184 <__ashlsi3_21>:
    2184:	b4 21 08 00 	add r1,r1,r1

00002188 <__ashlsi3_20>:
    2188:	b4 21 08 00 	add r1,r1,r1

0000218c <__ashlsi3_19>:
    218c:	b4 21 08 00 	add r1,r1,r1

00002190 <__ashlsi3_18>:
    2190:	b4 21 08 00 	add r1,r1,r1

00002194 <__ashlsi3_17>:
    2194:	b4 21 08 00 	add r1,r1,r1

00002198 <__ashlsi3_16>:
    2198:	b4 21 08 00 	add r1,r1,r1

0000219c <__ashlsi3_15>:
    219c:	b4 21 08 00 	add r1,r1,r1

000021a0 <__ashlsi3_14>:
    21a0:	b4 21 08 00 	add r1,r1,r1

000021a4 <__ashlsi3_13>:
    21a4:	b4 21 08 00 	add r1,r1,r1

000021a8 <__ashlsi3_12>:
    21a8:	b4 21 08 00 	add r1,r1,r1

000021ac <__ashlsi3_11>:
    21ac:	b4 21 08 00 	add r1,r1,r1

000021b0 <__ashlsi3_10>:
    21b0:	b4 21 08 00 	add r1,r1,r1

000021b4 <__ashlsi3_9>:
    21b4:	b4 21 08 00 	add r1,r1,r1

000021b8 <__ashlsi3_8>:
    21b8:	b4 21 08 00 	add r1,r1,r1

000021bc <__ashlsi3_7>:
    21bc:	b4 21 08 00 	add r1,r1,r1

000021c0 <__ashlsi3_6>:
    21c0:	b4 21 08 00 	add r1,r1,r1

000021c4 <__ashlsi3_5>:
    21c4:	b4 21 08 00 	add r1,r1,r1

000021c8 <__ashlsi3_4>:
    21c8:	b4 21 08 00 	add r1,r1,r1

000021cc <__ashlsi3_3>:
    21cc:	b4 21 08 00 	add r1,r1,r1

000021d0 <__ashlsi3_2>:
    21d0:	b4 21 08 00 	add r1,r1,r1

000021d4 <__ashlsi3_1>:
    21d4:	b4 21 08 00 	add r1,r1,r1

000021d8 <__ashlsi3_0>:
    21d8:	c3 a0 00 00 	ret
