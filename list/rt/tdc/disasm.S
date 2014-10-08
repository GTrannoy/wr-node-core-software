
rt-tdc.elf:     file format elf32-lm32


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
 11c:	3b 5a 9c 70 	ori gp,gp,0x9c70
 120:	78 01 00 00 	mvhi r1,0x0
 124:	38 21 1c 80 	ori r1,r1,0x1c80
 128:	34 02 00 00 	mvi r2,0
 12c:	78 03 00 00 	mvhi r3,0x0
 130:	38 63 1f 68 	ori r3,r3,0x1f68
 134:	c8 61 18 00 	sub r3,r3,r1
 138:	34 01 00 00 	mvi r1,0
 13c:	34 02 00 00 	mvi r2,0
 140:	34 03 00 00 	mvi r3,0
 144:	f8 00 00 3e 	calli 23c <main>

00000148 <loopf>:
 148:	e0 00 00 00 	bi 148 <loopf>

Disassembly of section .text:

0000014c <init>:

	mq_discard(0, TDC_IN_SLOT_CONTROL);
}

void init()
{
     14c:	37 9c ff f0 	addi sp,sp,-16
     150:	5b 8b 00 10 	sw (sp+16),r11
     154:	5b 8c 00 0c 	sw (sp+12),r12
     158:	5b 8d 00 08 	sw (sp+8),r13
     15c:	5b 9d 00 04 	sw (sp+4),ra
  return *(volatile uint32_t *) ( reg + CPU_DP_BASE );
}

static inline void dp_writel ( uint32_t value, uint32_t reg )
{
     *(volatile uint32_t *) ( reg + CPU_DP_BASE ) = value;
     160:	78 01 00 20 	mvhi r1,0x20
    dp_writel( 0x0, DR_REG_CHAN_ENABLE);
    dp_writel( DEFAULT_DEAD_TIME, DR_REG_DEAD_TIME);

    for(i=0;i<TDC_NUM_CHANNELS;i++)
    {
    	memset(&channels[i], 0, sizeof(struct tdc_channel_state));
     164:	78 0b 00 00 	mvhi r11,0x0
     168:	39 6b 1c 88 	ori r11,r11,0x1c88
     16c:	34 0c 13 88 	mvi r12,5000
     170:	38 24 00 04 	ori r4,r1,0x4
     174:	58 20 00 00 	sw (r1+0),r0
     178:	58 8c 00 00 	sw (r4+0),r12
     17c:	34 02 00 00 	mvi r2,0
     180:	34 03 00 78 	mvi r3,120
     184:	b9 60 08 00 	mv r1,r11
     188:	f8 00 05 6c 	calli 1738 <memset>
     18c:	78 01 00 00 	mvhi r1,0x0
        channels[i].n = i;
        channels[i].mode = LIST_TRIGGER_MODE_AUTO;
     190:	34 0d 00 02 	mvi r13,2
    dp_writel( 0x0, DR_REG_CHAN_ENABLE);
    dp_writel( DEFAULT_DEAD_TIME, DR_REG_DEAD_TIME);

    for(i=0;i<TDC_NUM_CHANNELS;i++)
    {
    	memset(&channels[i], 0, sizeof(struct tdc_channel_state));
     194:	34 02 00 00 	mvi r2,0
     198:	34 03 00 78 	mvi r3,120
     19c:	38 21 1d 00 	ori r1,r1,0x1d00
        channels[i].n = i;
        channels[i].mode = LIST_TRIGGER_MODE_AUTO;
     1a0:	59 6d 00 60 	sw (r11+96),r13
        channels[i].dead_time = DEFAULT_DEAD_TIME;
     1a4:	59 6c 00 74 	sw (r11+116),r12
    dp_writel( 0x0, DR_REG_CHAN_ENABLE);
    dp_writel( DEFAULT_DEAD_TIME, DR_REG_DEAD_TIME);

    for(i=0;i<TDC_NUM_CHANNELS;i++)
    {
    	memset(&channels[i], 0, sizeof(struct tdc_channel_state));
     1a8:	f8 00 05 64 	calli 1738 <memset>
     1ac:	78 01 00 00 	mvhi r1,0x0
        channels[i].n = i;
     1b0:	34 04 00 01 	mvi r4,1
    dp_writel( 0x0, DR_REG_CHAN_ENABLE);
    dp_writel( DEFAULT_DEAD_TIME, DR_REG_DEAD_TIME);

    for(i=0;i<TDC_NUM_CHANNELS;i++)
    {
    	memset(&channels[i], 0, sizeof(struct tdc_channel_state));
     1b4:	34 02 00 00 	mvi r2,0
     1b8:	34 03 00 78 	mvi r3,120
     1bc:	38 21 1d 78 	ori r1,r1,0x1d78
        channels[i].n = i;
     1c0:	59 64 00 dc 	sw (r11+220),r4
        channels[i].mode = LIST_TRIGGER_MODE_AUTO;
     1c4:	59 6d 00 d8 	sw (r11+216),r13
        channels[i].dead_time = DEFAULT_DEAD_TIME;
     1c8:	59 6c 00 ec 	sw (r11+236),r12
    dp_writel( 0x0, DR_REG_CHAN_ENABLE);
    dp_writel( DEFAULT_DEAD_TIME, DR_REG_DEAD_TIME);

    for(i=0;i<TDC_NUM_CHANNELS;i++)
    {
    	memset(&channels[i], 0, sizeof(struct tdc_channel_state));
     1cc:	f8 00 05 5b 	calli 1738 <memset>
     1d0:	78 01 00 00 	mvhi r1,0x0
     1d4:	34 02 00 00 	mvi r2,0
     1d8:	34 03 00 78 	mvi r3,120
     1dc:	38 21 1d f0 	ori r1,r1,0x1df0
        channels[i].n = i;
     1e0:	59 6d 01 54 	sw (r11+340),r13
        channels[i].mode = LIST_TRIGGER_MODE_AUTO;
     1e4:	59 6d 01 50 	sw (r11+336),r13
        channels[i].dead_time = DEFAULT_DEAD_TIME;
     1e8:	59 6c 01 64 	sw (r11+356),r12
    dp_writel( 0x0, DR_REG_CHAN_ENABLE);
    dp_writel( DEFAULT_DEAD_TIME, DR_REG_DEAD_TIME);

    for(i=0;i<TDC_NUM_CHANNELS;i++)
    {
    	memset(&channels[i], 0, sizeof(struct tdc_channel_state));
     1ec:	f8 00 05 53 	calli 1738 <memset>
     1f0:	78 01 00 00 	mvhi r1,0x0
        channels[i].n = i;
     1f4:	34 04 00 03 	mvi r4,3
    dp_writel( 0x0, DR_REG_CHAN_ENABLE);
    dp_writel( DEFAULT_DEAD_TIME, DR_REG_DEAD_TIME);

    for(i=0;i<TDC_NUM_CHANNELS;i++)
    {
    	memset(&channels[i], 0, sizeof(struct tdc_channel_state));
     1f8:	34 02 00 00 	mvi r2,0
     1fc:	34 03 00 78 	mvi r3,120
     200:	38 21 1e 68 	ori r1,r1,0x1e68
        channels[i].n = i;
     204:	59 64 01 cc 	sw (r11+460),r4
        channels[i].mode = LIST_TRIGGER_MODE_AUTO;
     208:	59 6d 01 c8 	sw (r11+456),r13
        channels[i].dead_time = DEFAULT_DEAD_TIME;
     20c:	59 6c 01 dc 	sw (r11+476),r12
    dp_writel( 0x0, DR_REG_CHAN_ENABLE);
    dp_writel( DEFAULT_DEAD_TIME, DR_REG_DEAD_TIME);

    for(i=0;i<TDC_NUM_CHANNELS;i++)
    {
    	memset(&channels[i], 0, sizeof(struct tdc_channel_state));
     210:	f8 00 05 4a 	calli 1738 <memset>
        channels[i].n = i;
     214:	34 01 00 04 	mvi r1,4
     218:	59 61 02 44 	sw (r11+580),r1
        channels[i].mode = LIST_TRIGGER_MODE_AUTO;
     21c:	59 6d 02 40 	sw (r11+576),r13
        channels[i].dead_time = DEFAULT_DEAD_TIME;
     220:	59 6c 02 54 	sw (r11+596),r12
    }
}
     224:	2b 9d 00 04 	lw ra,(sp+4)
     228:	2b 8b 00 10 	lw r11,(sp+16)
     22c:	2b 8c 00 0c 	lw r12,(sp+12)
     230:	2b 8d 00 08 	lw r13,(sp+8)
     234:	37 9c 00 10 	addi sp,sp,16
     238:	c3 a0 00 00 	ret

0000023c <main>:


main()
{   
     23c:	37 9c ff 94 	addi sp,sp,-108
     240:	5b 8b 00 44 	sw (sp+68),r11
     244:	5b 8c 00 40 	sw (sp+64),r12
     248:	5b 8d 00 3c 	sw (sp+60),r13
     24c:	5b 8e 00 38 	sw (sp+56),r14
     250:	5b 8f 00 34 	sw (sp+52),r15
     254:	5b 90 00 30 	sw (sp+48),r16
     258:	5b 91 00 2c 	sw (sp+44),r17
     25c:	5b 92 00 28 	sw (sp+40),r18
     260:	5b 93 00 24 	sw (sp+36),r19
     264:	5b 94 00 20 	sw (sp+32),r20
     268:	5b 95 00 1c 	sw (sp+28),r21
     26c:	5b 96 00 18 	sw (sp+24),r22
     270:	5b 97 00 14 	sw (sp+20),r23
     274:	5b 98 00 10 	sw (sp+16),r24
     278:	5b 99 00 0c 	sw (sp+12),r25
     27c:	5b 9b 00 08 	sw (sp+8),fp
     280:	5b 9d 00 04 	sw (sp+4),ra
    int i = 0;
    rt_set_debug_slot(TDC_OUT_SLOT_CONTROL);
     284:	34 01 00 00 	mvi r1,0
     288:	f8 00 04 cb 	calli 15b4 <rt_set_debug_slot>
     28c:	78 03 00 00 	mvhi r3,0x0
     290:	78 01 00 20 	mvhi r1,0x20
     294:	38 63 19 88 	ori r3,r3,0x1988
     298:	58 20 00 00 	sw (r1+0),r0
     29c:	28 61 00 00 	lw r1,(r3+0)
     2a0:	78 16 00 00 	mvhi r22,0x0
     2a4:	3a d6 1c 88 	ori r22,r22,0x1c88
     2a8:	34 02 13 88 	mvi r2,5000
     2ac:	58 22 00 00 	sw (r1+0),r2
     2b0:	ba c0 58 00 	mv r11,r22
    int i;

    dp_writel( 0x0, DR_REG_CHAN_ENABLE);
    dp_writel( DEFAULT_DEAD_TIME, DR_REG_DEAD_TIME);

    for(i=0;i<TDC_NUM_CHANNELS;i++)
     2b4:	34 0c 00 00 	mvi r12,0
    {
    	memset(&channels[i], 0, sizeof(struct tdc_channel_state));
        channels[i].n = i;
        channels[i].mode = LIST_TRIGGER_MODE_AUTO;
     2b8:	34 0f 00 02 	mvi r15,2
        channels[i].dead_time = DEFAULT_DEAD_TIME;
     2bc:	34 0e 13 88 	mvi r14,5000
    int i;

    dp_writel( 0x0, DR_REG_CHAN_ENABLE);
    dp_writel( DEFAULT_DEAD_TIME, DR_REG_DEAD_TIME);

    for(i=0;i<TDC_NUM_CHANNELS;i++)
     2c0:	34 0d 00 05 	mvi r13,5
    {
    	memset(&channels[i], 0, sizeof(struct tdc_channel_state));
     2c4:	b9 60 08 00 	mv r1,r11
     2c8:	34 02 00 00 	mvi r2,0
     2cc:	34 03 00 78 	mvi r3,120
     2d0:	f8 00 05 1a 	calli 1738 <memset>
        channels[i].n = i;
     2d4:	59 6c 00 64 	sw (r11+100),r12
        channels[i].mode = LIST_TRIGGER_MODE_AUTO;
     2d8:	59 6f 00 60 	sw (r11+96),r15
        channels[i].dead_time = DEFAULT_DEAD_TIME;
     2dc:	59 6e 00 74 	sw (r11+116),r14
    int i;

    dp_writel( 0x0, DR_REG_CHAN_ENABLE);
    dp_writel( DEFAULT_DEAD_TIME, DR_REG_DEAD_TIME);

    for(i=0;i<TDC_NUM_CHANNELS;i++)
     2e0:	35 8c 00 01 	addi r12,r12,1
     2e4:	35 6b 00 78 	addi r11,r11,120
     2e8:	5d 8d ff f7 	bne r12,r13,2c4 <main+0x88>
{   
    int i = 0;
    rt_set_debug_slot(TDC_OUT_SLOT_CONTROL);
    init();

    pp_printf("RT_TDC firmware initialized.");
     2ec:	78 01 00 00 	mvhi r1,0x0
     2f0:	38 21 19 60 	ori r1,r1,0x1960
     2f4:	f8 00 04 95 	calli 1548 <pp_printf>
  return *(volatile uint32_t *) ( reg + CPU_LR_BASE );
}

static inline uint32_t lr_writel ( uint32_t value, uint32_t reg )
{
  *(volatile uint32_t *) ( reg + CPU_LR_BASE ) = value;
     2f8:	78 04 00 00 	mvhi r4,0x0
     2fc:	38 84 19 8c 	ori r4,r4,0x198c
     300:	28 82 00 00 	lw r2,(r4+0)
static inline void log_raw_timestamp (struct tdc_channel_state *ch, struct list_timestamp *ts)
{
    volatile struct list_log_entry *msg = mq_map_out_buffer(0, TDC_OUT_SLOT_LOGGING);
    
    mq_claim(0, TDC_OUT_SLOT_LOGGING);
    msg->type = LIST_LOG_RAW;
     304:	78 01 00 00 	mvhi r1,0x0
     308:	38 21 19 9c 	ori r1,r1,0x199c
     30c:	78 03 01 00 	mvhi r3,0x100
#endif

    	ts.cycles += ts.frac >> 12;
    	ts.frac &= 0xfff;
        
    	if (ts.cycles >= 125000000)
     310:	78 05 00 00 	mvhi r5,0x0
    	{
    		ts.cycles -= 125000000;
     314:	78 06 00 00 	mvhi r6,0x0
static inline void mq_writel( int remote, uint32_t val, uint32_t reg )
{
  if(remote)
    * (volatile uint32_t * ) (RMQ_BASE + reg) = val ;
  else
    * (volatile uint32_t * ) (HMQ_BASE + reg) = val ;
     318:	78 0f 00 00 	mvhi r15,0x0
    
static inline void send_trigger (struct tdc_channel_state *ch, struct list_timestamp *ts)
{
    volatile struct list_trigger_message *msg = mq_map_out_buffer(1, TDC_OUT_SLOT_REMOTE);

    msg->triggers[coalesce_count].id = ch->id;
     31c:	78 04 00 00 	mvhi r4,0x0
     320:	58 43 00 00 	sw (r2+0),r3
#endif

    	ts.cycles += ts.frac >> 12;
    	ts.frac &= 0xfff;
        
    	if (ts.cycles >= 125000000)
     324:	38 a5 19 90 	ori r5,r5,0x1990
    	{
    		ts.cycles -= 125000000;
     328:	38 c6 19 94 	ori r6,r6,0x1994
     32c:	39 ef 19 98 	ori r15,r15,0x1998
    
static inline void send_trigger (struct tdc_channel_state *ch, struct list_timestamp *ts)
{
    volatile struct list_trigger_message *msg = mq_map_out_buffer(1, TDC_OUT_SLOT_REMOTE);

    msg->triggers[coalesce_count].id = ch->id;
     330:	38 84 19 a0 	ori r4,r4,0x19a0
static inline void log_raw_timestamp (struct tdc_channel_state *ch, struct list_timestamp *ts)
{
    volatile struct list_log_entry *msg = mq_map_out_buffer(0, TDC_OUT_SLOT_LOGGING);
    
    mq_claim(0, TDC_OUT_SLOT_LOGGING);
    msg->type = LIST_LOG_RAW;
     334:	28 37 00 00 	lw r23,(r1+0)
     338:	78 01 02 00 	mvhi r1,0x200
     33c:	58 41 00 00 	sw (r2+0),r1
#endif

    	ts.cycles += ts.frac >> 12;
    	ts.frac &= 0xfff;
        
    	if (ts.cycles >= 125000000)
     340:	28 b9 00 00 	lw r25,(r5+0)
    	{
    		ts.cycles -= 125000000;
     344:	28 c9 00 00 	lw r9,(r6+0)
     348:	29 fb 00 00 	lw fp,(r15+0)
    
static inline void send_trigger (struct tdc_channel_state *ch, struct list_timestamp *ts)
{
    volatile struct list_trigger_message *msg = mq_map_out_buffer(1, TDC_OUT_SLOT_REMOTE);

    msg->triggers[coalesce_count].id = ch->id;
     34c:	28 98 00 00 	lw r24,(r4+0)
     350:	78 05 00 00 	mvhi r5,0x0
     354:	78 04 04 00 	mvhi r4,0x400
     358:	78 0a 00 00 	mvhi r10,0x0
     35c:	78 06 00 00 	mvhi r6,0x0
     360:	78 0f 00 00 	mvhi r15,0x0
     364:	58 44 00 00 	sw (r2+0),r4
     368:	38 a5 19 80 	ori r5,r5,0x1980
     36c:	39 4a 1c 84 	ori r10,r10,0x1c84
     370:	38 c6 1e e0 	ori r6,r6,0x1ee0
     374:	39 ef 1a 40 	ori r15,r15,0x1a40
     378:	5b 85 00 5c 	sw (sp+92),r5
     37c:	5b 86 00 58 	sw (sp+88),r6
     380:	5b 8f 00 60 	sw (sp+96),r15
  return (void *) (base + MQ_IN (slot) + MQ_SLOT_DATA_START );
}

static inline uint32_t mq_poll()
{
  return *(volatile uint32_t *) ( REG_LR_POLL );
     384:	b9 40 a0 00 	mv r20,r10
};

static inline void mq_writel( int remote, uint32_t val, uint32_t reg )
{
  if(remote)
    * (volatile uint32_t * ) (RMQ_BASE + reg) = val ;
     388:	78 01 40 02 	mvhi r1,0x4002
     38c:	78 02 01 00 	mvhi r2,0x100
     390:	38 21 80 00 	ori r1,r1,0x8000
     394:	58 22 00 00 	sw (r1+0),r2
     398:	34 0f 00 05 	mvi r15,5
    	a->seconds--;
    }

    a->seconds -= b->seconds;

    if(a->seconds == -1)
     39c:	34 12 ff ff 	mvi r18,-1
static inline void log_raw_timestamp (struct tdc_channel_state *ch, struct list_timestamp *ts)
{
    volatile struct list_log_entry *msg = mq_map_out_buffer(0, TDC_OUT_SLOT_LOGGING);
    
    mq_claim(0, TDC_OUT_SLOT_LOGGING);
    msg->type = LIST_LOG_RAW;
     3a0:	34 13 00 01 	mvi r19,1
#ifdef WRNODE_RT
static inline void ts_add(struct list_timestamp *a, const struct list_timestamp *b)
{
    a->frac += b->frac;

    if(a->frac >= 4096)
     3a4:	34 11 0f ff 	mvi r17,4095
    if(delta < 0)
	pp_printf("FUCK!");

    ch->prev = *ts;

    if( (ch->flags & LIST_TRIGGER_ASSIGNED ) && (ch->flags & LIST_ARMED) )
     3a8:	34 10 00 0a 	mvi r16,10
static inline void log_sent_trigger(struct tdc_channel_state *ch, struct list_timestamp *ts)
{
    volatile struct list_log_entry *msg =  mq_map_out_buffer(0, TDC_OUT_SLOT_LOGGING);
    
    mq_claim(0, TDC_OUT_SLOT_LOGGING);
    msg->type = ID_LOG_SENT_TRIGGER;
     3ac:	34 15 00 02 	mvi r21,2
    if( (ch->flags & LIST_TRIGGER_ASSIGNED ) && (ch->flags & LIST_ARMED) )
    {
    	ch->seq++;
    	ch->flags |= LIST_TRIGGERED;
    	if(ch->mode == LIST_TRIGGER_MODE_SINGLE )
    	    ch->flags &= ~LIST_ARMED;
     3b0:	34 0a ff f7 	mvi r10,-9
     3b4:	5b 8f 00 50 	sw (sp+80),r15
     3b8:	5b 94 00 54 	sw (sp+84),r20

void rt_set_debug_slot(int slot);

static inline uint32_t dp_readl ( uint32_t reg )
{
  return *(volatile uint32_t *) ( reg + CPU_DP_BASE );
     3bc:	78 03 00 20 	mvhi r3,0x20
     3c0:	38 63 00 14 	ori r3,r3,0x14
     3c4:	28 62 00 00 	lw r2,(r3+0)
    {
        uint32_t fifo_sr = dp_readl (DR_REG_FIFO_CSR);
        struct list_timestamp ts;
        int meta;

        if(fifo_sr & DR_FIFO_CSR_EMPTY)
     3c8:	78 04 00 02 	mvhi r4,0x2
     3cc:	a0 44 10 00 	and r2,r2,r4
     3d0:	5c 40 00 74 	bne r2,r0,5a0 <main+0x364>
     3d4:	78 05 00 20 	mvhi r5,0x20
     3d8:	78 0f 00 20 	mvhi r15,0x20
     3dc:	78 01 00 20 	mvhi r1,0x20
     3e0:	38 a5 00 08 	ori r5,r5,0x8
     3e4:	39 ef 00 0c 	ori r15,r15,0xc
     3e8:	38 21 00 10 	ori r1,r1,0x10
     3ec:	28 ac 00 00 	lw r12,(r5+0)
     3f0:	29 e6 00 00 	lw r6,(r15+0)
     3f4:	28 25 00 00 	lw r5,(r1+0)
        ts.seconds = dp_readl(DR_REG_FIFO_R0);
        ts.cycles  = dp_readl(DR_REG_FIFO_R1);
        meta   = dp_readl(DR_REG_FIFO_R2);

        // convert from ACAM bins (81ps) to WR time format
        ts.frac = ( (meta & 0x3ffff) * 5308 ) >> 7;
     3f8:	78 02 00 03 	mvhi r2,0x3
     3fc:	38 42 ff ff 	ori r2,r2,0xffff
     400:	a0 a2 20 00 	and r4,r5,r2
     404:	08 84 14 bc 	muli r4,r4,5308
            ts.cycles -= 125000000;
            ts.seconds --;
        }
#endif

    	ts.cycles += ts.frac >> 12;
     408:	14 82 00 13 	sri r2,r4,19
        ts.seconds = dp_readl(DR_REG_FIFO_R0);
        ts.cycles  = dp_readl(DR_REG_FIFO_R1);
        meta   = dp_readl(DR_REG_FIFO_R2);

        // convert from ACAM bins (81ps) to WR time format
        ts.frac = ( (meta & 0x3ffff) * 5308 ) >> 7;
     40c:	14 84 00 07 	sri r4,r4,7
            ts.cycles -= 125000000;
            ts.seconds --;
        }
#endif

    	ts.cycles += ts.frac >> 12;
     410:	b4 46 38 00 	add r7,r2,r6
    	ts.frac &= 0xfff;
     414:	20 8e 0f ff 	andi r14,r4,0xfff
        
    	if (ts.cycles >= 125000000)
     418:	4f 27 00 03 	bge r25,r7,424 <main+0x1e8>
    	{
    		ts.cycles -= 125000000;
     41c:	b4 e9 38 00 	add r7,r7,r9
    		ts.seconds ++;
     420:	35 8c 00 01 	addi r12,r12,1
    	}

    	int channel = (meta >> 19) & 0x7;
     424:	14 a5 00 13 	sri r5,r5,19
     428:	20 a5 00 07 	andi r5,r5,0x7
    a->seconds += b->seconds;
}

static inline void ts_sub(struct list_timestamp *a, const struct list_timestamp *b)
{
    a->frac -= b->frac;
     42c:	3c a6 00 03 	sli r6,r5,3
     430:	3c a5 00 07 	sli r5,r5,7
     434:	c8 a6 78 00 	sub r15,r5,r6
     438:	b6 cf 28 00 	add r5,r22,r15
     43c:	28 a4 00 20 	lw r4,(r5+32)
     440:	c9 c4 70 00 	sub r14,r14,r4

    if(a->frac < 0)
     444:	4d c0 00 03 	bge r14,r0,450 <main+0x214>
    {
    	a->frac += 4096;
     448:	35 ce 10 00 	addi r14,r14,4096
    	a->cycles --;
     44c:	34 e7 ff ff 	addi r7,r7,-1
    }

    a->cycles -= b->cycles;
     450:	b6 cf 20 00 	add r4,r22,r15
     454:	28 82 00 1c 	lw r2,(r4+28)
     458:	c8 e2 68 00 	sub r13,r7,r2

    if(a->cycles < 0)
     45c:	4d a0 00 05 	bge r13,r0,470 <main+0x234>
    {
    	a->cycles += 125000000;
     460:	78 03 07 73 	mvhi r3,0x773
     464:	38 63 59 40 	ori r3,r3,0x5940
     468:	b5 a3 68 00 	add r13,r13,r3
    	a->seconds--;
     46c:	35 8c ff ff 	addi r12,r12,-1
    }

    a->seconds -= b->seconds;
     470:	b6 cf 20 00 	add r4,r22,r15
     474:	28 83 00 18 	lw r3,(r4+24)
     478:	c9 83 60 00 	sub r12,r12,r3

    if(a->seconds == -1)
     47c:	45 92 02 ce 	be r12,r18,fb4 <main+0xd78>
    {
      a->seconds = 0;
      a->cycles -= 125000000;
     480:	b6 cf 38 00 	add r7,r22,r15
    struct tdc_channel_state *ch = &channels[channel];

    ts_sub(ts, &ch->timebase_offset);    
    ch->last = *ts;

	if(ch->log_level & LIST_LOG_RAW)
     484:	28 e3 00 5c 	lw r3,(r7+92)
     488:	58 ec 00 24 	sw (r7+36),r12
     48c:	58 ed 00 28 	sw (r7+40),r13
     490:	58 ee 00 2c 	sw (r7+44),r14
     494:	20 63 00 01 	andi r3,r3,0x1
     498:	44 60 00 13 	be r3,r0,4e4 <main+0x2a8>
  else
    * (volatile uint32_t * ) (HMQ_BASE + reg) = val ;
     49c:	78 04 01 00 	mvhi r4,0x100
     4a0:	5b 64 00 00 	sw (fp+0),r4
static inline void log_raw_timestamp (struct tdc_channel_state *ch, struct list_timestamp *ts)
{
    volatile struct list_log_entry *msg = mq_map_out_buffer(0, TDC_OUT_SLOT_LOGGING);
    
    mq_claim(0, TDC_OUT_SLOT_LOGGING);
    msg->type = LIST_LOG_RAW;
     4a4:	5a f3 00 00 	sw (r23+0),r19
    msg->channel = ch->n;
     4a8:	28 eb 00 64 	lw r11,(r7+100)
    msg->seq = ch->total_pulses;
     4ac:	28 e7 00 68 	lw r7,(r7+104)
     4b0:	78 05 04 00 	mvhi r5,0x400
{
    volatile struct list_log_entry *msg = mq_map_out_buffer(0, TDC_OUT_SLOT_LOGGING);
    
    mq_claim(0, TDC_OUT_SLOT_LOGGING);
    msg->type = LIST_LOG_RAW;
    msg->channel = ch->n;
     4b4:	5a eb 00 08 	sw (r23+8),r11
    msg->seq = ch->total_pulses;
     4b8:	5a e7 00 04 	sw (r23+4),r7
     4bc:	5b 8d 00 68 	sw (sp+104),r13
     4c0:	5b 8e 00 6c 	sw (sp+108),r14
     4c4:	5b 8c 00 64 	sw (sp+100),r12
    msg->ts = *ts;
     4c8:	5a ec 00 18 	sw (r23+24),r12
     4cc:	2b 87 00 68 	lw r7,(sp+104)
     4d0:	38 a5 00 09 	ori r5,r5,0x9
     4d4:	5a e7 00 1c 	sw (r23+28),r7
     4d8:	2b 87 00 6c 	lw r7,(sp+108)
     4dc:	5a e7 00 20 	sw (r23+32),r7
     4e0:	5b 65 00 00 	sw (fp+0),r5
     4e4:	b6 cf 30 00 	add r6,r22,r15

	if(ch->log_level & LIST_LOG_RAW)
        log_raw_timestamp(ch, ts);

    ch->last = *ts;
    ch->total_pulses++;
     4e8:	28 c7 00 68 	lw r7,(r6+104)


#ifdef WRNODE_RT
static inline void ts_add(struct list_timestamp *a, const struct list_timestamp *b)
{
    a->frac += b->frac;
     4ec:	28 cb 00 14 	lw r11,(r6+20)
     4f0:	58 cc 00 24 	sw (r6+36),r12
     4f4:	34 e7 00 01 	addi r7,r7,1
     4f8:	58 cd 00 28 	sw (r6+40),r13
     4fc:	58 ce 00 2c 	sw (r6+44),r14
     500:	58 c7 00 68 	sw (r6+104),r7
     504:	b5 cb a0 00 	add r20,r14,r11

    if(a->frac >= 4096)
     508:	4e 34 00 03 	bge r17,r20,514 <main+0x2d8>
    {
    	a->frac -= 4096;
     50c:	36 94 f0 00 	addi r20,r20,-4096
    	a->cycles ++;
     510:	35 ad 00 01 	addi r13,r13,1
    }

    a->cycles += b->cycles;
     514:	b6 cf 18 00 	add r3,r22,r15
     518:	28 62 00 10 	lw r2,(r3+16)
     51c:	b5 a2 68 00 	add r13,r13,r2

    if(a->cycles >= 125000000)
     520:	4f 2d 00 03 	bge r25,r13,52c <main+0x2f0>
    {
    	a->cycles -= 125000000;
     524:	b5 a9 68 00 	add r13,r13,r9
    	a->seconds++;
     528:	35 8c 00 01 	addi r12,r12,1
    }

    a->seconds += b->seconds;
     52c:	b6 cf 20 00 	add r4,r22,r15
     530:	28 88 00 0c 	lw r8,(r4+12)

    ts_add(ts, &ch->delay);

    int delta = ts->seconds - ch->prev.seconds;
     534:	28 87 00 30 	lw r7,(r4+48)
    delta *= 125000000;    
     538:	78 06 07 73 	mvhi r6,0x773
     53c:	b5 88 60 00 	add r12,r12,r8
    delta += ts->cycles - ch->prev.cycles;
     540:	28 8b 00 34 	lw r11,(r4+52)
    ch->last = *ts;
    ch->total_pulses++;

    ts_add(ts, &ch->delay);

    int delta = ts->seconds - ch->prev.seconds;
     544:	c9 87 18 00 	sub r3,r12,r7
    delta *= 125000000;    
     548:	38 c6 59 40 	ori r6,r6,0x5940
     54c:	88 66 18 00 	mul r3,r3,r6
    delta += ts->cycles - ch->prev.cycles;
     550:	c9 ab 10 00 	sub r2,r13,r11
     554:	b4 43 10 00 	add r2,r2,r3

    if(delta < 0)
     558:	4c 40 00 07 	bge r2,r0,574 <main+0x338>
	pp_printf("FUCK!");
     55c:	2b 81 00 5c 	lw r1,(sp+92)
     560:	5b 89 00 48 	sw (sp+72),r9
     564:	5b 8a 00 4c 	sw (sp+76),r10
     568:	f8 00 03 f8 	calli 1548 <pp_printf>
     56c:	2b 8a 00 4c 	lw r10,(sp+76)
     570:	2b 89 00 48 	lw r9,(sp+72)
     574:	b6 cf 38 00 	add r7,r22,r15

    ch->prev = *ts;

    if( (ch->flags & LIST_TRIGGER_ASSIGNED ) && (ch->flags & LIST_ARMED) )
     578:	28 ee 00 58 	lw r14,(r7+88)
    int delta = ts->seconds - ch->prev.seconds;
    delta *= 125000000;    
    delta += ts->cycles - ch->prev.cycles;

    if(delta < 0)
	pp_printf("FUCK!");
     57c:	58 ec 00 30 	sw (r7+48),r12
     580:	58 ed 00 34 	sw (r7+52),r13
     584:	58 f4 00 38 	sw (r7+56),r20

    ch->prev = *ts;

    if( (ch->flags & LIST_TRIGGER_ASSIGNED ) && (ch->flags & LIST_ARMED) )
     588:	21 c2 00 0a 	andi r2,r14,0xa
     58c:	44 50 02 8d 	be r2,r16,fc0 <main+0xd84>
     590:	2b 84 00 50 	lw r4,(sp+80)
     594:	34 84 ff ff 	addi r4,r4,-1
     598:	5b 84 00 50 	sw (sp+80),r4
{
    int i;

    claim_tx();

    for(i = 0; i < TDC_TRIGGER_COALESCE_LIMIT; i++)
     59c:	5c 80 ff 88 	bne r4,r0,3bc <main+0x180>
     5a0:	2b 94 00 54 	lw r20,(sp+84)
    	int channel = (meta >> 19) & 0x7;

        do_channel( channel, &ts );
    }
    
    if(coalesce_count)
     5a4:	2a 84 00 00 	lw r4,(r20+0)
     5a8:	44 80 00 1d 	be r4,r0,61c <main+0x3e0>

static inline void flush_tx ()
{
    volatile struct list_trigger_message *msg = mq_map_out_buffer(1, TDC_OUT_SLOT_REMOTE);

    msg->hdr.target_ip = 0xffffffff; // broadcast
     5ac:	34 02 ff ff 	mvi r2,-1
     5b0:	5b 02 00 00 	sw (r24+0),r2
    msg->hdr.target_port = 0xebd0;   // port
     5b4:	38 05 eb d0 	mvu r5,0xebd0
     5b8:	5b 05 00 04 	sw (r24+4),r5
    msg->hdr.target_offset = 0x4000;// target EB slot
     5bc:	34 02 40 00 	mvi r2,16384
     *(volatile uint32_t *) ( reg + CPU_DP_BASE ) = value;
}

static inline uint32_t lr_readl ( uint32_t reg )
{
  return *(volatile uint32_t *) ( reg + CPU_LR_BASE );
     5c0:	78 06 00 10 	mvhi r6,0x10
     5c4:	5b 02 00 08 	sw (r24+8),r2
     5c8:	38 c6 00 0c 	ori r6,r6,0xc
     5cc:	28 c3 00 00 	lw r3,(r6+0)
     5d0:	78 0f 00 10 	mvhi r15,0x10
     5d4:	39 ef 00 08 	ori r15,r15,0x8
    msg->transmit_seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
     5d8:	5b 03 00 0c 	sw (r24+12),r3
     5dc:	29 e3 00 00 	lw r3,(r15+0)
    msg->transmit_cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
    msg->count = coalesce_count;
    mq_send(1, TDC_OUT_SLOT_REMOTE, 6 + 7 * coalesce_count);
    coalesce_count = 0;
    sent_packets ++;
     5e0:	2b 81 00 58 	lw r1,(sp+88)
    msg->hdr.target_port = 0xebd0;   // port
    msg->hdr.target_offset = 0x4000;// target EB slot
    msg->transmit_seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
    msg->transmit_cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
    msg->count = coalesce_count;
    mq_send(1, TDC_OUT_SLOT_REMOTE, 6 + 7 * coalesce_count);
     5e4:	3c 82 00 03 	sli r2,r4,3

    msg->hdr.target_ip = 0xffffffff; // broadcast
    msg->hdr.target_port = 0xebd0;   // port
    msg->hdr.target_offset = 0x4000;// target EB slot
    msg->transmit_seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
    msg->transmit_cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
     5e8:	5b 03 00 10 	sw (r24+16),r3
    msg->count = coalesce_count;
     5ec:	5b 04 00 14 	sw (r24+20),r4
    mq_send(1, TDC_OUT_SLOT_REMOTE, 6 + 7 * coalesce_count);
    coalesce_count = 0;
    sent_packets ++;
     5f0:	28 23 00 00 	lw r3,(r1+0)
    msg->hdr.target_port = 0xebd0;   // port
    msg->hdr.target_offset = 0x4000;// target EB slot
    msg->transmit_seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
    msg->transmit_cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
    msg->count = coalesce_count;
    mq_send(1, TDC_OUT_SLOT_REMOTE, 6 + 7 * coalesce_count);
     5f4:	c8 44 10 00 	sub r2,r2,r4
     5f8:	34 42 00 06 	addi r2,r2,6
  mq_writel ( remote, MQ_CMD_CLAIM, MQ_OUT(slot) + MQ_SLOT_COMMAND );
}

static inline void mq_send( int remote, int slot, int count)
{
  mq_writel ( remote, MQ_CMD_READY | count, MQ_OUT(slot) + MQ_SLOT_COMMAND );
     5fc:	78 04 04 00 	mvhi r4,0x400
};

static inline void mq_writel( int remote, uint32_t val, uint32_t reg )
{
  if(remote)
    * (volatile uint32_t * ) (RMQ_BASE + reg) = val ;
     600:	78 05 40 02 	mvhi r5,0x4002
  mq_writel ( remote, MQ_CMD_CLAIM, MQ_OUT(slot) + MQ_SLOT_COMMAND );
}

static inline void mq_send( int remote, int slot, int count)
{
  mq_writel ( remote, MQ_CMD_READY | count, MQ_OUT(slot) + MQ_SLOT_COMMAND );
     604:	b8 44 10 00 	or r2,r2,r4
};

static inline void mq_writel( int remote, uint32_t val, uint32_t reg )
{
  if(remote)
    * (volatile uint32_t * ) (RMQ_BASE + reg) = val ;
     608:	38 a5 80 00 	ori r5,r5,0x8000
     60c:	58 a2 00 00 	sw (r5+0),r2
    coalesce_count = 0;
    sent_packets ++;
     610:	34 62 00 01 	addi r2,r3,1
    msg->hdr.target_offset = 0x4000;// target EB slot
    msg->transmit_seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
    msg->transmit_cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
    msg->count = coalesce_count;
    mq_send(1, TDC_OUT_SLOT_REMOTE, 6 + 7 * coalesce_count);
    coalesce_count = 0;
     614:	5a 80 00 00 	sw (r20+0),r0
    sent_packets ++;
     618:	58 22 00 00 	sw (r1+0),r2
  return (void *) (base + MQ_IN (slot) + MQ_SLOT_DATA_START );
}

static inline uint32_t mq_poll()
{
  return *(volatile uint32_t *) ( REG_LR_POLL );
     61c:	78 06 00 10 	mvhi r6,0x10
     620:	28 c2 00 00 	lw r2,(r6+0)
static inline void do_control()
{
    uint32_t p = mq_poll();


    if(! ( p & ( 1<< TDC_IN_SLOT_CONTROL )))
     624:	20 42 00 01 	andi r2,r2,0x1
     628:	44 40 ff 58 	be r2,r0,388 <main+0x14c>
    {                           \
        func(seq, buf + 2);     \
        break;                  \
    }

	switch(cmd)
     62c:	78 0f 40 01 	mvhi r15,0x4001
     630:	39 ef 40 08 	ori r15,r15,0x4008
     634:	29 e3 00 00 	lw r3,(r15+0)
        return;

    uint32_t *buf = mq_map_in_buffer( 0, TDC_IN_SLOT_CONTROL );

	int cmd = buf[0];
	int seq = buf[1];
     638:	78 01 40 01 	mvhi r1,0x4001
     63c:	38 21 40 0c 	ori r1,r1,0x400c
    {                           \
        func(seq, buf + 2);     \
        break;                  \
    }

	switch(cmd)
     640:	34 02 00 0f 	mvi r2,15
        return;

    uint32_t *buf = mq_map_in_buffer( 0, TDC_IN_SLOT_CONTROL );

	int cmd = buf[0];
	int seq = buf[1];
     644:	28 27 00 00 	lw r7,(r1+0)
    {                           \
        func(seq, buf + 2);     \
        break;                  \
    }

	switch(cmd)
     648:	54 62 00 2f 	bgu r3,r2,704 <main+0x4c8>
     64c:	3c 62 00 02 	sli r2,r3,2
     650:	2b 83 00 60 	lw r3,(sp+96)
     654:	b4 62 10 00 	add r2,r3,r2
     658:	28 42 00 00 	lw r2,(r2+0)
     65c:	c0 40 00 00 	b r2
    ctl_ack(seq);
}

static inline void ctl_chan_set_timebase_offset (int seq, uint32_t *buf)
{
    int channel = buf[0];
     660:	78 03 00 00 	mvhi r3,0x0
     664:	38 63 19 a8 	ori r3,r3,0x19a8
     668:	28 62 00 00 	lw r2,(r3+0)
    struct tdc_channel_state *ch = &channels[channel];

    ch->timebase_offset.seconds = buf[1];
     66c:	78 04 00 00 	mvhi r4,0x0
     670:	38 84 19 a4 	ori r4,r4,0x19a4
    ctl_ack(seq);
}

static inline void ctl_chan_set_timebase_offset (int seq, uint32_t *buf)
{
    int channel = buf[0];
     674:	28 43 00 00 	lw r3,(r2+0)
    struct tdc_channel_state *ch = &channels[channel];

    ch->timebase_offset.seconds = buf[1];
     678:	28 82 00 00 	lw r2,(r4+0)
    ch->timebase_offset.cycles = buf[2];
     67c:	78 01 00 00 	mvhi r1,0x0
static inline void ctl_chan_set_timebase_offset (int seq, uint32_t *buf)
{
    int channel = buf[0];
    struct tdc_channel_state *ch = &channels[channel];

    ch->timebase_offset.seconds = buf[1];
     680:	3c 64 00 03 	sli r4,r3,3
     684:	28 45 00 00 	lw r5,(r2+0)
     688:	3c 62 00 07 	sli r2,r3,7
    ch->timebase_offset.cycles = buf[2];
     68c:	38 21 19 bc 	ori r1,r1,0x19bc
static inline void ctl_chan_set_timebase_offset (int seq, uint32_t *buf)
{
    int channel = buf[0];
    struct tdc_channel_state *ch = &channels[channel];

    ch->timebase_offset.seconds = buf[1];
     690:	c8 44 10 00 	sub r2,r2,r4
     694:	b6 c2 20 00 	add r4,r22,r2
    ch->timebase_offset.cycles = buf[2];
     698:	28 22 00 00 	lw r2,(r1+0)
static inline void ctl_chan_set_timebase_offset (int seq, uint32_t *buf)
{
    int channel = buf[0];
    struct tdc_channel_state *ch = &channels[channel];

    ch->timebase_offset.seconds = buf[1];
     69c:	58 85 00 18 	sw (r4+24),r5
static inline void mq_writel( int remote, uint32_t val, uint32_t reg )
{
  if(remote)
    * (volatile uint32_t * ) (RMQ_BASE + reg) = val ;
  else
    * (volatile uint32_t * ) (HMQ_BASE + reg) = val ;
     6a0:	78 0f 00 00 	mvhi r15,0x0
    ch->timebase_offset.cycles = buf[2];
     6a4:	28 43 00 00 	lw r3,(r2+0)
     6a8:	39 ef 19 ac 	ori r15,r15,0x19ac
     6ac:	29 e6 00 00 	lw r6,(r15+0)
     6b0:	58 83 00 1c 	sw (r4+28),r3
    ch->timebase_offset.frac = buf[3];
     6b4:	78 03 00 00 	mvhi r3,0x0
     6b8:	38 63 19 c0 	ori r3,r3,0x19c0
     6bc:	28 62 00 00 	lw r2,(r3+0)
     6c0:	28 43 00 00 	lw r3,(r2+0)
     6c4:	58 83 00 20 	sw (r4+32),r3
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     6c8:	78 05 00 00 	mvhi r5,0x0
     6cc:	38 a5 19 b0 	ori r5,r5,0x19b0
     6d0:	28 a2 00 00 	lw r2,(r5+0)
     6d4:	78 04 01 00 	mvhi r4,0x100
    buf[1] = seq;
     6d8:	78 0f 00 00 	mvhi r15,0x0
     6dc:	58 c4 00 00 	sw (r6+0),r4
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     6e0:	34 03 01 00 	mvi r3,256
    buf[1] = seq;
     6e4:	39 ef 19 b4 	ori r15,r15,0x19b4
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     6e8:	58 43 00 00 	sw (r2+0),r3
    buf[1] = seq;
     6ec:	29 e2 00 00 	lw r2,(r15+0)
     6f0:	78 01 00 00 	mvhi r1,0x0
     6f4:	38 21 19 b8 	ori r1,r1,0x19b8
     6f8:	58 47 00 00 	sw (r2+0),r7
     6fc:	28 22 00 00 	lw r2,(r1+0)
     700:	58 c2 00 00 	sw (r6+0),r2
     704:	78 04 40 01 	mvhi r4,0x4001
     708:	78 02 08 00 	mvhi r2,0x800
     70c:	38 84 40 00 	ori r4,r4,0x4000
     710:	58 82 00 00 	sw (r4+0),r2
     714:	e3 ff ff 1d 	bi 388 <main+0x14c>
    {
        do_input();
        do_control();
    }

     718:	78 06 00 00 	mvhi r6,0x0
     71c:	38 c6 19 a8 	ori r6,r6,0x19a8
     720:	28 c2 00 00 	lw r2,(r6+0)
     724:	78 04 00 00 	mvhi r4,0x0
     728:	38 84 19 ac 	ori r4,r4,0x19ac
     72c:	28 42 00 00 	lw r2,(r2+0)
     730:	28 85 00 00 	lw r5,(r4+0)
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     734:	78 01 00 00 	mvhi r1,0x0
static inline void ctl_chan_reset_counters (int seq, uint32_t *buf)
{
    int channel = buf[0];
    struct tdc_channel_state *ch = &channels[channel];    
    
    ch->total_pulses = 0;
     738:	3c 43 00 03 	sli r3,r2,3
     73c:	3c 42 00 07 	sli r2,r2,7
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     740:	38 21 19 b0 	ori r1,r1,0x19b0
static inline void ctl_chan_reset_counters (int seq, uint32_t *buf)
{
    int channel = buf[0];
    struct tdc_channel_state *ch = &channels[channel];    
    
    ch->total_pulses = 0;
     744:	c8 43 10 00 	sub r2,r2,r3
     748:	b6 c2 18 00 	add r3,r22,r2
    ch->sent_pulses = 0;
    ch->flags &= ~LIST_LAST_VALID;
     74c:	28 64 00 58 	lw r4,(r3+88)
     750:	34 02 ff fb 	mvi r2,-5
static inline void ctl_chan_reset_counters (int seq, uint32_t *buf)
{
    int channel = buf[0];
    struct tdc_channel_state *ch = &channels[channel];    
    
    ch->total_pulses = 0;
     754:	58 60 00 68 	sw (r3+104),r0
    ch->sent_pulses = 0;
    ch->flags &= ~LIST_LAST_VALID;
     758:	a0 82 10 00 	and r2,r4,r2
     75c:	58 62 00 58 	sw (r3+88),r2
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     760:	28 22 00 00 	lw r2,(r1+0)
{
    int channel = buf[0];
    struct tdc_channel_state *ch = &channels[channel];    
    
    ch->total_pulses = 0;
    ch->sent_pulses = 0;
     764:	58 60 00 6c 	sw (r3+108),r0
     768:	78 0f 01 00 	mvhi r15,0x100
     76c:	58 af 00 00 	sw (r5+0),r15
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     770:	34 03 01 00 	mvi r3,256
     774:	58 43 00 00 	sw (r2+0),r3
    buf[1] = seq;
     778:	78 03 00 00 	mvhi r3,0x0
     77c:	38 63 19 b4 	ori r3,r3,0x19b4
     780:	28 62 00 00 	lw r2,(r3+0)
     784:	78 04 00 00 	mvhi r4,0x0
     788:	38 84 19 b8 	ori r4,r4,0x19b8
     78c:	58 47 00 00 	sw (r2+0),r7
     790:	28 82 00 00 	lw r2,(r4+0)
     794:	78 04 40 01 	mvhi r4,0x4001
     798:	38 84 40 00 	ori r4,r4,0x4000
     79c:	58 a2 00 00 	sw (r5+0),r2
     7a0:	78 02 08 00 	mvhi r2,0x800
     7a4:	58 82 00 00 	sw (r4+0),r2
     7a8:	e3 ff fe f8 	bi 388 <main+0x14c>
static inline void ctl_chan_set_log_level (int seq, uint32_t *buf)
{
    int channel = buf[0];
    struct tdc_channel_state *ch = &channels[channel];    
    
    ch->log_level = buf[1];
     7ac:	78 03 00 00 	mvhi r3,0x0
     7b0:	38 63 19 a8 	ori r3,r3,0x19a8
     7b4:	28 62 00 00 	lw r2,(r3+0)
     7b8:	78 04 00 00 	mvhi r4,0x0
     7bc:	38 84 19 a4 	ori r4,r4,0x19a4
     7c0:	28 42 00 00 	lw r2,(r2+0)
     7c4:	28 83 00 00 	lw r3,(r4+0)
     7c8:	78 01 00 00 	mvhi r1,0x0
     7cc:	38 21 19 ac 	ori r1,r1,0x19ac
     7d0:	28 64 00 00 	lw r4,(r3+0)
     7d4:	3c 43 00 03 	sli r3,r2,3
     7d8:	3c 42 00 07 	sli r2,r2,7
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     7dc:	78 0f 00 00 	mvhi r15,0x0
static inline void ctl_chan_set_log_level (int seq, uint32_t *buf)
{
    int channel = buf[0];
    struct tdc_channel_state *ch = &channels[channel];    
    
    ch->log_level = buf[1];
     7e0:	c8 43 10 00 	sub r2,r2,r3
     7e4:	28 25 00 00 	lw r5,(r1+0)
     7e8:	b6 c2 10 00 	add r2,r22,r2
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     7ec:	39 ef 19 b0 	ori r15,r15,0x19b0
static inline void ctl_chan_set_log_level (int seq, uint32_t *buf)
{
    int channel = buf[0];
    struct tdc_channel_state *ch = &channels[channel];    
    
    ch->log_level = buf[1];
     7f0:	58 44 00 5c 	sw (r2+92),r4
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     7f4:	29 e2 00 00 	lw r2,(r15+0)
     7f8:	78 06 01 00 	mvhi r6,0x100
    buf[1] = seq;
     7fc:	78 01 00 00 	mvhi r1,0x0
     800:	58 a6 00 00 	sw (r5+0),r6
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     804:	34 03 01 00 	mvi r3,256
    buf[1] = seq;
     808:	38 21 19 b4 	ori r1,r1,0x19b4
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     80c:	58 43 00 00 	sw (r2+0),r3
    buf[1] = seq;
     810:	28 22 00 00 	lw r2,(r1+0)
     814:	78 03 00 00 	mvhi r3,0x0
     818:	38 63 19 b8 	ori r3,r3,0x19b8
     81c:	58 47 00 00 	sw (r2+0),r7
     820:	28 62 00 00 	lw r2,(r3+0)
     824:	78 04 40 01 	mvhi r4,0x4001
     828:	38 84 40 00 	ori r4,r4,0x4000
     82c:	58 a2 00 00 	sw (r5+0),r2
     830:	78 02 08 00 	mvhi r2,0x800
     834:	58 82 00 00 	sw (r4+0),r2
     838:	e3 ff fe d4 	bi 388 <main+0x14c>
    msg->hdr.target_offset = 0x4000;// target EB slot
    msg->transmit_seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
    msg->transmit_cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
    msg->count = 1;

    memcpy(&msg->triggers[0], buf, sizeof(struct list_trigger_entry) );
     83c:	78 0f 00 00 	mvhi r15,0x0
     840:	39 ef 19 a8 	ori r15,r15,0x19a8
};

static inline void mq_writel( int remote, uint32_t val, uint32_t reg )
{
  if(remote)
    * (volatile uint32_t * ) (RMQ_BASE + reg) = val ;
     844:	78 01 40 02 	mvhi r1,0x4002
     848:	29 e2 00 00 	lw r2,(r15+0)
     84c:	78 03 01 00 	mvhi r3,0x100
     850:	38 21 80 00 	ori r1,r1,0x8000
  else
    * (volatile uint32_t * ) (HMQ_BASE + reg) = val ;
     854:	78 05 00 00 	mvhi r5,0x0
     858:	78 06 00 00 	mvhi r6,0x0
     85c:	78 0f 00 10 	mvhi r15,0x10
};

static inline void mq_writel( int remote, uint32_t val, uint32_t reg )
{
  if(remote)
    * (volatile uint32_t * ) (RMQ_BASE + reg) = val ;
     860:	58 23 00 00 	sw (r1+0),r3
  else
    * (volatile uint32_t * ) (HMQ_BASE + reg) = val ;
     864:	38 a5 19 ac 	ori r5,r5,0x19ac
     868:	38 c6 1a 34 	ori r6,r6,0x1a34
     86c:	39 ef 00 0c 	ori r15,r15,0xc
     870:	28 c3 00 00 	lw r3,(r6+0)
     874:	28 a4 00 00 	lw r4,(r5+0)
     878:	78 06 00 10 	mvhi r6,0x10
     87c:	29 e5 00 00 	lw r5,(r15+0)
};

static inline void mq_writel( int remote, uint32_t val, uint32_t reg )
{
  if(remote)
    * (volatile uint32_t * ) (RMQ_BASE + reg) = val ;
     880:	78 0f 00 00 	mvhi r15,0x0
     884:	38 c6 00 08 	ori r6,r6,0x8
     888:	39 ef 1a 38 	ori r15,r15,0x1a38
     88c:	28 4e 00 00 	lw r14,(r2+0)
     890:	28 c1 00 00 	lw r1,(r6+0)
     894:	28 4d 00 04 	lw r13,(r2+4)
     898:	28 4c 00 08 	lw r12,(r2+8)
     89c:	28 4b 00 0c 	lw r11,(r2+12)
     8a0:	28 4a 00 10 	lw r10,(r2+16)
     8a4:	28 48 00 14 	lw r8,(r2+20)
     8a8:	28 46 00 18 	lw r6,(r2+24)
     8ac:	29 e2 00 00 	lw r2,(r15+0)
     8b0:	78 0f 40 02 	mvhi r15,0x4002
     8b4:	39 ef 80 00 	ori r15,r15,0x8000
     8b8:	59 e2 00 00 	sw (r15+0),r2
  else
    * (volatile uint32_t * ) (HMQ_BASE + reg) = val ;
     8bc:	78 02 01 00 	mvhi r2,0x100
     8c0:	58 82 00 00 	sw (r4+0),r2
{
    struct list_trigger_message *msg = mq_map_out_buffer(1, TDC_OUT_SLOT_REMOTE);

    mq_claim(1, TDC_OUT_SLOT_REMOTE);
    
    msg->hdr.target_ip = 0xffffffff; // broadcast
     8c4:	34 02 ff ff 	mvi r2,-1
     8c8:	5b 02 00 00 	sw (r24+0),r2
    msg->hdr.target_port = 0xebd0;   // port
    msg->hdr.target_offset = 0x4000;// target EB slot
     8cc:	34 02 40 00 	mvi r2,16384
    struct list_trigger_message *msg = mq_map_out_buffer(1, TDC_OUT_SLOT_REMOTE);

    mq_claim(1, TDC_OUT_SLOT_REMOTE);
    
    msg->hdr.target_ip = 0xffffffff; // broadcast
    msg->hdr.target_port = 0xebd0;   // port
     8d0:	38 0f eb d0 	mvu r15,0xebd0
    msg->hdr.target_offset = 0x4000;// target EB slot
     8d4:	5b 02 00 08 	sw (r24+8),r2
    msg->transmit_seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
    msg->transmit_cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
    msg->count = 1;
     8d8:	34 02 00 01 	mvi r2,1
    struct list_trigger_message *msg = mq_map_out_buffer(1, TDC_OUT_SLOT_REMOTE);

    mq_claim(1, TDC_OUT_SLOT_REMOTE);
    
    msg->hdr.target_ip = 0xffffffff; // broadcast
    msg->hdr.target_port = 0xebd0;   // port
     8dc:	5b 0f 00 04 	sw (r24+4),r15
    msg->hdr.target_offset = 0x4000;// target EB slot
    msg->transmit_seconds = lr_readl(WRN_CPU_LR_REG_TAI_SEC);
     8e0:	5b 05 00 0c 	sw (r24+12),r5
    msg->transmit_cycles = lr_readl(WRN_CPU_LR_REG_TAI_CYCLES);
     8e4:	5b 01 00 10 	sw (r24+16),r1
    msg->count = 1;
     8e8:	5b 02 00 14 	sw (r24+20),r2

    memcpy(&msg->triggers[0], buf, sizeof(struct list_trigger_entry) );
     8ec:	58 6e 00 00 	sw (r3+0),r14
     8f0:	58 6d 00 04 	sw (r3+4),r13
     8f4:	58 6c 00 08 	sw (r3+8),r12
     8f8:	58 6b 00 0c 	sw (r3+12),r11
     8fc:	58 6a 00 10 	sw (r3+16),r10
     900:	58 68 00 14 	sw (r3+20),r8
     904:	58 66 00 18 	sw (r3+24),r6
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     908:	78 01 00 00 	mvhi r1,0x0
     90c:	38 21 19 b0 	ori r1,r1,0x19b0
     910:	28 22 00 00 	lw r2,(r1+0)
     914:	34 03 01 00 	mvi r3,256
     918:	78 05 00 00 	mvhi r5,0x0
     91c:	58 43 00 00 	sw (r2+0),r3
    buf[1] = seq;
     920:	78 03 00 00 	mvhi r3,0x0
     924:	38 63 19 b4 	ori r3,r3,0x19b4
     928:	28 62 00 00 	lw r2,(r3+0)
     92c:	38 a5 19 b8 	ori r5,r5,0x19b8
     930:	58 47 00 00 	sw (r2+0),r7
     934:	28 a2 00 00 	lw r2,(r5+0)
     938:	58 82 00 00 	sw (r4+0),r2
     93c:	78 04 40 01 	mvhi r4,0x4001
     940:	78 02 08 00 	mvhi r2,0x800
     944:	38 84 40 00 	ori r4,r4,0x4000
     948:	58 82 00 00 	sw (r4+0),r2
     94c:	e3 ff fe 8f 	bi 388 <main+0x14c>
     950:	78 05 00 00 	mvhi r5,0x0
     954:	38 a5 19 ac 	ori r5,r5,0x19ac
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     958:	78 0f 00 00 	mvhi r15,0x0
     95c:	28 a4 00 00 	lw r4,(r5+0)
     960:	39 ef 19 b0 	ori r15,r15,0x19b0
     964:	29 e2 00 00 	lw r2,(r15+0)
     968:	78 06 01 00 	mvhi r6,0x100
    buf[1] = seq;
     96c:	78 01 00 00 	mvhi r1,0x0
     970:	58 86 00 00 	sw (r4+0),r6
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     974:	34 03 01 00 	mvi r3,256
    buf[1] = seq;
     978:	38 21 19 b4 	ori r1,r1,0x19b4
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     97c:	58 43 00 00 	sw (r2+0),r3
    buf[1] = seq;
     980:	28 22 00 00 	lw r2,(r1+0)
     984:	78 03 00 00 	mvhi r3,0x0
     988:	38 63 19 b8 	ori r3,r3,0x19b8
     98c:	58 47 00 00 	sw (r2+0),r7
     990:	28 62 00 00 	lw r2,(r3+0)
     994:	58 82 00 00 	sw (r4+0),r2
     998:	78 04 40 01 	mvhi r4,0x4001
     99c:	78 02 08 00 	mvhi r2,0x800
     9a0:	38 84 40 00 	ori r4,r4,0x4000
     9a4:	58 82 00 00 	sw (r4+0),r2
     9a8:	e3 ff fe 78 	bi 388 <main+0x14c>
{
    int channel = buf[0];

    struct tdc_channel_state *ch = &channels[channel];    

    int do_assign = buf[1];
     9ac:	78 06 00 00 	mvhi r6,0x0
     9b0:	38 c6 19 a4 	ori r6,r6,0x19a4
     9b4:	28 c2 00 00 	lw r2,(r6+0)
    ctl_ack(seq);
}

static inline void ctl_chan_assign_trigger (int seq, uint32_t *buf)
{
    int channel = buf[0];
     9b8:	78 0f 00 00 	mvhi r15,0x0
     9bc:	39 ef 19 a8 	ori r15,r15,0x19a8

    struct tdc_channel_state *ch = &channels[channel];    

    int do_assign = buf[1];
    
    if(do_assign)
     9c0:	28 43 00 00 	lw r3,(r2+0)
    ctl_ack(seq);
}

static inline void ctl_chan_assign_trigger (int seq, uint32_t *buf)
{
    int channel = buf[0];
     9c4:	29 e2 00 00 	lw r2,(r15+0)
     9c8:	28 42 00 00 	lw r2,(r2+0)

    struct tdc_channel_state *ch = &channels[channel];    

    int do_assign = buf[1];
    
    if(do_assign)
     9cc:	44 60 01 d0 	be r3,r0,110c <main+0xed0>
    {
        ch->id.system = buf[2];
     9d0:	78 01 00 00 	mvhi r1,0x0
     9d4:	38 21 19 bc 	ori r1,r1,0x19bc
     9d8:	08 44 00 78 	muli r4,r2,120
     9dc:	28 22 00 00 	lw r2,(r1+0)
        ch->id.source_port = buf[3];
     9e0:	78 03 00 00 	mvhi r3,0x0

    int do_assign = buf[1];
    
    if(do_assign)
    {
        ch->id.system = buf[2];
     9e4:	b6 c4 20 00 	add r4,r22,r4
     9e8:	28 42 00 00 	lw r2,(r2+0)
        ch->id.source_port = buf[3];
     9ec:	38 63 19 c0 	ori r3,r3,0x19c0
        ch->id.trigger = buf[4];
     9f0:	78 05 00 00 	mvhi r5,0x0

    int do_assign = buf[1];
    
    if(do_assign)
    {
        ch->id.system = buf[2];
     9f4:	58 82 00 00 	sw (r4+0),r2
        ch->id.source_port = buf[3];
     9f8:	28 62 00 00 	lw r2,(r3+0)
        ch->id.trigger = buf[4];
     9fc:	38 a5 1a 3c 	ori r5,r5,0x1a3c
        ch->flags |= LIST_TRIGGER_ASSIGNED;
     a00:	28 83 00 58 	lw r3,(r4+88)
    int do_assign = buf[1];
    
    if(do_assign)
    {
        ch->id.system = buf[2];
        ch->id.source_port = buf[3];
     a04:	28 42 00 00 	lw r2,(r2+0)
        ch->id.trigger = buf[4];
        ch->flags |= LIST_TRIGGER_ASSIGNED;
     a08:	38 63 00 02 	ori r3,r3,0x2
    int do_assign = buf[1];
    
    if(do_assign)
    {
        ch->id.system = buf[2];
        ch->id.source_port = buf[3];
     a0c:	58 82 00 04 	sw (r4+4),r2
        ch->id.trigger = buf[4];
     a10:	28 a2 00 00 	lw r2,(r5+0)
     a14:	28 45 00 00 	lw r5,(r2+0)
        ch->flags |= LIST_TRIGGER_ASSIGNED;
        ch->flags &= ~LIST_LAST_VALID;
     a18:	34 02 ff fb 	mvi r2,-5
     a1c:	a0 62 10 00 	and r2,r3,r2
    
    if(do_assign)
    {
        ch->id.system = buf[2];
        ch->id.source_port = buf[3];
        ch->id.trigger = buf[4];
     a20:	58 85 00 08 	sw (r4+8),r5
        ch->flags |= LIST_TRIGGER_ASSIGNED;
        ch->flags &= ~LIST_LAST_VALID;
     a24:	58 82 00 58 	sw (r4+88),r2
     a28:	78 06 00 00 	mvhi r6,0x0
     a2c:	38 c6 19 ac 	ori r6,r6,0x19ac
     a30:	28 c4 00 00 	lw r4,(r6+0)
     a34:	78 0f 01 00 	mvhi r15,0x100
     a38:	58 8f 00 00 	sw (r4+0),r15
     a3c:	e3 ff ff b3 	bi 908 <main+0x6cc>
static inline void ctl_chan_set_seq (int seq, uint32_t *buf)
{
    int channel = buf[0];

    struct tdc_channel_state *ch = &channels[channel];
    ch->seq = buf[1];
     a40:	78 0f 00 00 	mvhi r15,0x0
     a44:	39 ef 19 a8 	ori r15,r15,0x19a8
     a48:	29 e2 00 00 	lw r2,(r15+0)
     a4c:	78 01 00 00 	mvhi r1,0x0
     a50:	38 21 19 a4 	ori r1,r1,0x19a4
     a54:	28 42 00 00 	lw r2,(r2+0)
     a58:	28 23 00 00 	lw r3,(r1+0)
     a5c:	78 06 00 00 	mvhi r6,0x0
     a60:	38 c6 19 ac 	ori r6,r6,0x19ac
     a64:	28 64 00 00 	lw r4,(r3+0)
     a68:	3c 43 00 03 	sli r3,r2,3
     a6c:	3c 42 00 07 	sli r2,r2,7
     a70:	28 c5 00 00 	lw r5,(r6+0)
     a74:	c8 43 10 00 	sub r2,r2,r3
     a78:	b6 c2 10 00 	add r2,r22,r2
     a7c:	58 44 00 70 	sw (r2+112),r4
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     a80:	78 04 00 00 	mvhi r4,0x0
     a84:	78 02 01 00 	mvhi r2,0x100
     a88:	38 84 19 b0 	ori r4,r4,0x19b0
     a8c:	58 a2 00 00 	sw (r5+0),r2
     a90:	28 82 00 00 	lw r2,(r4+0)
    buf[1] = seq;
     a94:	78 06 00 00 	mvhi r6,0x0
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     a98:	34 03 01 00 	mvi r3,256
    buf[1] = seq;
     a9c:	38 c6 19 b4 	ori r6,r6,0x19b4
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     aa0:	58 43 00 00 	sw (r2+0),r3
    buf[1] = seq;
     aa4:	28 c2 00 00 	lw r2,(r6+0)
     aa8:	78 0f 00 00 	mvhi r15,0x0
     aac:	39 ef 19 b8 	ori r15,r15,0x19b8
     ab0:	58 47 00 00 	sw (r2+0),r7
     ab4:	29 e2 00 00 	lw r2,(r15+0)
     ab8:	78 04 40 01 	mvhi r4,0x4001
     abc:	38 84 40 00 	ori r4,r4,0x4000
     ac0:	58 a2 00 00 	sw (r5+0),r2
     ac4:	78 02 08 00 	mvhi r2,0x800
     ac8:	58 82 00 00 	sw (r4+0),r2
     acc:	e3 ff fe 2f 	bi 388 <main+0x14c>
    ctl_ack(seq);
}

static inline void ctl_chan_set_mode (int seq, uint32_t *buf)
{
    int channel = buf[0];
     ad0:	78 01 00 00 	mvhi r1,0x0
     ad4:	38 21 19 a8 	ori r1,r1,0x19a8
     ad8:	28 22 00 00 	lw r2,(r1+0)

    struct tdc_channel_state *ch = &channels[channel];

    ch->mode = buf[1];
     adc:	78 04 00 00 	mvhi r4,0x0
     ae0:	38 84 19 a4 	ori r4,r4,0x19a4
    ctl_ack(seq);
}

static inline void ctl_chan_set_mode (int seq, uint32_t *buf)
{
    int channel = buf[0];
     ae4:	28 42 00 00 	lw r2,(r2+0)

    struct tdc_channel_state *ch = &channels[channel];

    ch->mode = buf[1];
     ae8:	28 83 00 00 	lw r3,(r4+0)
     aec:	78 0f 00 00 	mvhi r15,0x0
     af0:	39 ef 19 ac 	ori r15,r15,0x19ac
     af4:	28 65 00 00 	lw r5,(r3+0)
     af8:	3c 43 00 03 	sli r3,r2,3
     afc:	3c 42 00 07 	sli r2,r2,7
     b00:	29 e6 00 00 	lw r6,(r15+0)
     b04:	c8 43 10 00 	sub r2,r2,r3
     b08:	b6 c2 18 00 	add r3,r22,r2
    ch->flags &= ~(LIST_ARMED | LIST_TRIGGERED | LIST_LAST_VALID) ;
     b0c:	28 64 00 58 	lw r4,(r3+88)
     b10:	34 02 ff e3 	mvi r2,-29
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     b14:	78 0f 00 00 	mvhi r15,0x0
    int channel = buf[0];

    struct tdc_channel_state *ch = &channels[channel];

    ch->mode = buf[1];
    ch->flags &= ~(LIST_ARMED | LIST_TRIGGERED | LIST_LAST_VALID) ;
     b18:	a0 82 10 00 	and r2,r4,r2
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     b1c:	39 ef 19 b0 	ori r15,r15,0x19b0
    int channel = buf[0];

    struct tdc_channel_state *ch = &channels[channel];

    ch->mode = buf[1];
    ch->flags &= ~(LIST_ARMED | LIST_TRIGGERED | LIST_LAST_VALID) ;
     b20:	58 62 00 58 	sw (r3+88),r2
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     b24:	29 e2 00 00 	lw r2,(r15+0)
{
    int channel = buf[0];

    struct tdc_channel_state *ch = &channels[channel];

    ch->mode = buf[1];
     b28:	58 65 00 60 	sw (r3+96),r5

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
    buf[1] = seq;
     b2c:	78 01 00 00 	mvhi r1,0x0
     b30:	78 05 01 00 	mvhi r5,0x100
     b34:	58 c5 00 00 	sw (r6+0),r5
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     b38:	34 03 01 00 	mvi r3,256
    buf[1] = seq;
     b3c:	38 21 19 b4 	ori r1,r1,0x19b4
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     b40:	58 43 00 00 	sw (r2+0),r3
    buf[1] = seq;
     b44:	28 22 00 00 	lw r2,(r1+0)
     b48:	78 03 00 00 	mvhi r3,0x0
     b4c:	38 63 19 b8 	ori r3,r3,0x19b8
     b50:	58 47 00 00 	sw (r2+0),r7
     b54:	28 62 00 00 	lw r2,(r3+0)
     b58:	78 04 40 01 	mvhi r4,0x4001
     b5c:	38 84 40 00 	ori r4,r4,0x4000
     b60:	58 c2 00 00 	sw (r6+0),r2
     b64:	78 02 08 00 	mvhi r2,0x800
     b68:	58 82 00 00 	sw (r4+0),r2
     b6c:	e3 ff fe 07 	bi 388 <main+0x14c>
{
    int channel = buf[0];

    struct tdc_channel_state *ch = &channels[channel];

    if(buf[1]) {
     b70:	78 04 00 00 	mvhi r4,0x0
     b74:	38 84 19 a4 	ori r4,r4,0x19a4
     b78:	28 82 00 00 	lw r2,(r4+0)
}


static inline void ctl_chan_arm (int seq, uint32_t *buf)
{
    int channel = buf[0];
     b7c:	78 05 00 00 	mvhi r5,0x0
     b80:	38 a5 19 a8 	ori r5,r5,0x19a8

    struct tdc_channel_state *ch = &channels[channel];

    if(buf[1]) {
     b84:	28 43 00 00 	lw r3,(r2+0)
}


static inline void ctl_chan_arm (int seq, uint32_t *buf)
{
    int channel = buf[0];
     b88:	28 a2 00 00 	lw r2,(r5+0)
     b8c:	28 42 00 00 	lw r2,(r2+0)

    struct tdc_channel_state *ch = &channels[channel];

    if(buf[1]) {
     b90:	44 60 01 69 	be r3,r0,1134 <main+0xef8>
        ch->flags |= LIST_ARMED;
        ch->flags &= ~LIST_TRIGGERED;
     b94:	08 42 00 78 	muli r2,r2,120
     b98:	b6 c2 20 00 	add r4,r22,r2
    int channel = buf[0];

    struct tdc_channel_state *ch = &channels[channel];

    if(buf[1]) {
        ch->flags |= LIST_ARMED;
     b9c:	28 83 00 58 	lw r3,(r4+88)
        ch->flags &= ~LIST_TRIGGERED;
     ba0:	34 02 ff ef 	mvi r2,-17
    int channel = buf[0];

    struct tdc_channel_state *ch = &channels[channel];

    if(buf[1]) {
        ch->flags |= LIST_ARMED;
     ba4:	38 63 00 08 	ori r3,r3,0x8
        ch->flags &= ~LIST_TRIGGERED;
     ba8:	a0 62 10 00 	and r2,r3,r2
     bac:	58 82 00 58 	sw (r4+88),r2
     bb0:	e3 ff ff 9e 	bi a28 <main+0x7ec>
     bb4:	78 02 00 00 	mvhi r2,0x0
    {
        do_input();
        do_control();
    }

     bb8:	78 03 00 00 	mvhi r3,0x0
     bbc:	38 42 19 ac 	ori r2,r2,0x19ac
     bc0:	38 63 19 a8 	ori r3,r3,0x19a8
     bc4:	28 48 00 00 	lw r8,(r2+0)
     bc8:	28 62 00 00 	lw r2,(r3+0)
{
    int channel = buf[0];
    struct tdc_channel_state *st = &channels[channel];
    volatile uint32_t *obuf = ctl_claim_out();
  
    obuf[0] = ID_REP_STATE;
     bcc:	78 0f 00 00 	mvhi r15,0x0
     bd0:	39 ef 19 b0 	ori r15,r15,0x19b0
    {
        do_input();
        do_control();
    }

     bd4:	28 44 00 00 	lw r4,(r2+0)
{
    int channel = buf[0];
    struct tdc_channel_state *st = &channels[channel];
    volatile uint32_t *obuf = ctl_claim_out();
  
    obuf[0] = ID_REP_STATE;
     bd8:	29 e2 00 00 	lw r2,(r15+0)
     bdc:	78 05 01 00 	mvhi r5,0x100
    obuf[1] = seq;
     be0:	78 01 00 00 	mvhi r1,0x0
     be4:	59 05 00 00 	sw (r8+0),r5
    obuf[2] = channel;

    obuf[3] = st->id.system;
     be8:	3c 86 00 03 	sli r6,r4,3
     bec:	3c 85 00 07 	sli r5,r4,7
{
    int channel = buf[0];
    struct tdc_channel_state *st = &channels[channel];
    volatile uint32_t *obuf = ctl_claim_out();
  
    obuf[0] = ID_REP_STATE;
     bf0:	34 03 01 01 	mvi r3,257
    obuf[1] = seq;
     bf4:	38 21 19 b4 	ori r1,r1,0x19b4
{
    int channel = buf[0];
    struct tdc_channel_state *st = &channels[channel];
    volatile uint32_t *obuf = ctl_claim_out();
  
    obuf[0] = ID_REP_STATE;
     bf8:	58 43 00 00 	sw (r2+0),r3
    obuf[1] = seq;
     bfc:	28 22 00 00 	lw r2,(r1+0)
    obuf[2] = channel;

    obuf[3] = st->id.system;
     c00:	c8 a6 18 00 	sub r3,r5,r6
    struct tdc_channel_state *st = &channels[channel];
    volatile uint32_t *obuf = ctl_claim_out();
  
    obuf[0] = ID_REP_STATE;
    obuf[1] = seq;
    obuf[2] = channel;
     c04:	78 05 00 00 	mvhi r5,0x0
     c08:	38 a5 19 c4 	ori r5,r5,0x19c4
    int channel = buf[0];
    struct tdc_channel_state *st = &channels[channel];
    volatile uint32_t *obuf = ctl_claim_out();
  
    obuf[0] = ID_REP_STATE;
    obuf[1] = seq;
     c0c:	58 47 00 00 	sw (r2+0),r7
    obuf[2] = channel;
     c10:	28 a2 00 00 	lw r2,(r5+0)

    obuf[3] = st->id.system;
     c14:	78 06 00 00 	mvhi r6,0x0
     c18:	b6 c3 18 00 	add r3,r22,r3
    struct tdc_channel_state *st = &channels[channel];
    volatile uint32_t *obuf = ctl_claim_out();
  
    obuf[0] = ID_REP_STATE;
    obuf[1] = seq;
    obuf[2] = channel;
     c1c:	58 44 00 00 	sw (r2+0),r4

    obuf[3] = st->id.system;
     c20:	38 c6 19 c8 	ori r6,r6,0x19c8
     c24:	28 64 00 00 	lw r4,(r3+0)
     c28:	28 c2 00 00 	lw r2,(r6+0)
    obuf[4] = st->id.source_port;
     c2c:	78 0f 00 00 	mvhi r15,0x0
     c30:	39 ef 19 cc 	ori r15,r15,0x19cc
  
    obuf[0] = ID_REP_STATE;
    obuf[1] = seq;
    obuf[2] = channel;

    obuf[3] = st->id.system;
     c34:	58 44 00 00 	sw (r2+0),r4
    obuf[4] = st->id.source_port;
     c38:	28 64 00 04 	lw r4,(r3+4)
     c3c:	29 e2 00 00 	lw r2,(r15+0)
    obuf[5] = st->id.trigger;
     c40:	78 01 00 00 	mvhi r1,0x0
     c44:	38 21 19 d0 	ori r1,r1,0x19d0
    obuf[0] = ID_REP_STATE;
    obuf[1] = seq;
    obuf[2] = channel;

    obuf[3] = st->id.system;
    obuf[4] = st->id.source_port;
     c48:	58 44 00 00 	sw (r2+0),r4
    obuf[5] = st->id.trigger;
     c4c:	28 64 00 08 	lw r4,(r3+8)
     c50:	28 22 00 00 	lw r2,(r1+0)
    
    obuf[6] = st->delay.seconds;
     c54:	78 05 00 00 	mvhi r5,0x0
     c58:	38 a5 19 d4 	ori r5,r5,0x19d4
    obuf[1] = seq;
    obuf[2] = channel;

    obuf[3] = st->id.system;
    obuf[4] = st->id.source_port;
    obuf[5] = st->id.trigger;
     c5c:	58 44 00 00 	sw (r2+0),r4
    
    obuf[6] = st->delay.seconds;
     c60:	28 64 00 0c 	lw r4,(r3+12)
     c64:	28 a2 00 00 	lw r2,(r5+0)
    obuf[7] = st->delay.cycles;
     c68:	78 06 00 00 	mvhi r6,0x0
     c6c:	38 c6 19 d8 	ori r6,r6,0x19d8

    obuf[3] = st->id.system;
    obuf[4] = st->id.source_port;
    obuf[5] = st->id.trigger;
    
    obuf[6] = st->delay.seconds;
     c70:	58 44 00 00 	sw (r2+0),r4
    obuf[7] = st->delay.cycles;
     c74:	28 64 00 10 	lw r4,(r3+16)
     c78:	28 c2 00 00 	lw r2,(r6+0)
    obuf[8] = st->delay.frac;
     c7c:	78 0f 00 00 	mvhi r15,0x0
     c80:	39 ef 19 dc 	ori r15,r15,0x19dc
    obuf[3] = st->id.system;
    obuf[4] = st->id.source_port;
    obuf[5] = st->id.trigger;
    
    obuf[6] = st->delay.seconds;
    obuf[7] = st->delay.cycles;
     c84:	58 44 00 00 	sw (r2+0),r4
    obuf[8] = st->delay.frac;
     c88:	28 64 00 14 	lw r4,(r3+20)
     c8c:	29 e2 00 00 	lw r2,(r15+0)
    
    obuf[9] = st->timebase_offset.seconds;
     c90:	78 05 00 00 	mvhi r5,0x0
     c94:	38 a5 19 e0 	ori r5,r5,0x19e0
    obuf[4] = st->id.source_port;
    obuf[5] = st->id.trigger;
    
    obuf[6] = st->delay.seconds;
    obuf[7] = st->delay.cycles;
    obuf[8] = st->delay.frac;
     c98:	58 44 00 00 	sw (r2+0),r4
    
    obuf[9] = st->timebase_offset.seconds;
     c9c:	28 64 00 18 	lw r4,(r3+24)
     ca0:	28 a2 00 00 	lw r2,(r5+0)
    obuf[10] = st->timebase_offset.cycles;
     ca4:	78 06 00 00 	mvhi r6,0x0
    obuf[13] = st->last.cycles;
    obuf[14] = st->last.frac;
    
    obuf[15] = st->flags;
    obuf[16] = st->log_level;
    obuf[17] = st->mode;
     ca8:	28 61 00 60 	lw r1,(r3+96)
    obuf[6] = st->delay.seconds;
    obuf[7] = st->delay.cycles;
    obuf[8] = st->delay.frac;
    
    obuf[9] = st->timebase_offset.seconds;
    obuf[10] = st->timebase_offset.cycles;
     cac:	38 c6 19 e4 	ori r6,r6,0x19e4
    
    obuf[6] = st->delay.seconds;
    obuf[7] = st->delay.cycles;
    obuf[8] = st->delay.frac;
    
    obuf[9] = st->timebase_offset.seconds;
     cb0:	58 44 00 00 	sw (r2+0),r4
    obuf[10] = st->timebase_offset.cycles;
     cb4:	28 64 00 1c 	lw r4,(r3+28)
     cb8:	28 c2 00 00 	lw r2,(r6+0)
    obuf[11] = st->timebase_offset.frac;
     cbc:	78 0f 00 00 	mvhi r15,0x0
     cc0:	39 ef 19 e8 	ori r15,r15,0x19e8
    obuf[6] = st->delay.seconds;
    obuf[7] = st->delay.cycles;
    obuf[8] = st->delay.frac;
    
    obuf[9] = st->timebase_offset.seconds;
    obuf[10] = st->timebase_offset.cycles;
     cc4:	58 44 00 00 	sw (r2+0),r4
    obuf[11] = st->timebase_offset.frac;
     cc8:	28 64 00 20 	lw r4,(r3+32)
     ccc:	29 e2 00 00 	lw r2,(r15+0)
    
    obuf[12] = st->last.seconds;
     cd0:	78 05 00 00 	mvhi r5,0x0
     cd4:	38 a5 19 ec 	ori r5,r5,0x19ec
    obuf[7] = st->delay.cycles;
    obuf[8] = st->delay.frac;
    
    obuf[9] = st->timebase_offset.seconds;
    obuf[10] = st->timebase_offset.cycles;
    obuf[11] = st->timebase_offset.frac;
     cd8:	58 44 00 00 	sw (r2+0),r4
    
    obuf[12] = st->last.seconds;
     cdc:	28 64 00 24 	lw r4,(r3+36)
     ce0:	28 a2 00 00 	lw r2,(r5+0)
    obuf[13] = st->last.cycles;
     ce4:	78 06 00 00 	mvhi r6,0x0
     ce8:	38 c6 19 f0 	ori r6,r6,0x19f0
    
    obuf[9] = st->timebase_offset.seconds;
    obuf[10] = st->timebase_offset.cycles;
    obuf[11] = st->timebase_offset.frac;
    
    obuf[12] = st->last.seconds;
     cec:	58 44 00 00 	sw (r2+0),r4
    obuf[13] = st->last.cycles;
     cf0:	28 64 00 28 	lw r4,(r3+40)
     cf4:	28 c2 00 00 	lw r2,(r6+0)
    obuf[14] = st->last.frac;
     cf8:	78 0f 00 00 	mvhi r15,0x0
     cfc:	39 ef 19 f4 	ori r15,r15,0x19f4
    obuf[9] = st->timebase_offset.seconds;
    obuf[10] = st->timebase_offset.cycles;
    obuf[11] = st->timebase_offset.frac;
    
    obuf[12] = st->last.seconds;
    obuf[13] = st->last.cycles;
     d00:	58 44 00 00 	sw (r2+0),r4
    obuf[14] = st->last.frac;
     d04:	28 64 00 2c 	lw r4,(r3+44)
     d08:	29 e2 00 00 	lw r2,(r15+0)
    
    obuf[15] = st->flags;
     d0c:	78 05 00 00 	mvhi r5,0x0
     d10:	38 a5 19 f8 	ori r5,r5,0x19f8
    obuf[10] = st->timebase_offset.cycles;
    obuf[11] = st->timebase_offset.frac;
    
    obuf[12] = st->last.seconds;
    obuf[13] = st->last.cycles;
    obuf[14] = st->last.frac;
     d14:	58 44 00 00 	sw (r2+0),r4
    
    obuf[15] = st->flags;
     d18:	28 64 00 58 	lw r4,(r3+88)
     d1c:	28 a2 00 00 	lw r2,(r5+0)
    obuf[16] = st->log_level;
     d20:	78 06 00 00 	mvhi r6,0x0
     d24:	38 c6 19 fc 	ori r6,r6,0x19fc
    
    obuf[12] = st->last.seconds;
    obuf[13] = st->last.cycles;
    obuf[14] = st->last.frac;
    
    obuf[15] = st->flags;
     d28:	58 44 00 00 	sw (r2+0),r4
    obuf[16] = st->log_level;
     d2c:	28 64 00 5c 	lw r4,(r3+92)
     d30:	28 c2 00 00 	lw r2,(r6+0)
    obuf[17] = st->mode;
     d34:	78 0f 00 00 	mvhi r15,0x0
     d38:	39 ef 1a 00 	ori r15,r15,0x1a00
    obuf[12] = st->last.seconds;
    obuf[13] = st->last.cycles;
    obuf[14] = st->last.frac;
    
    obuf[15] = st->flags;
    obuf[16] = st->log_level;
     d3c:	58 44 00 00 	sw (r2+0),r4
    obuf[17] = st->mode;
     d40:	29 e2 00 00 	lw r2,(r15+0)

    obuf[18] = st->total_pulses;
     d44:	78 05 00 00 	mvhi r5,0x0
     d48:	38 a5 1a 04 	ori r5,r5,0x1a04
    obuf[13] = st->last.cycles;
    obuf[14] = st->last.frac;
    
    obuf[15] = st->flags;
    obuf[16] = st->log_level;
    obuf[17] = st->mode;
     d4c:	58 41 00 00 	sw (r2+0),r1
    obuf[23] = st->last_sent.ts.frac;
    obuf[24] = st->last_sent.id.system;
    obuf[25] = st->last_sent.id.source_port;
    obuf[26] = st->last_sent.id.trigger;
    obuf[27] = st->last_sent.seq;
    obuf[28] = sent_packets;
     d50:	2b 82 00 58 	lw r2,(sp+88)
    
    obuf[15] = st->flags;
    obuf[16] = st->log_level;
    obuf[17] = st->mode;

    obuf[18] = st->total_pulses;
     d54:	28 64 00 68 	lw r4,(r3+104)
    obuf[19] = st->sent_pulses;
     d58:	78 06 00 00 	mvhi r6,0x0
    obuf[23] = st->last_sent.ts.frac;
    obuf[24] = st->last_sent.id.system;
    obuf[25] = st->last_sent.id.source_port;
    obuf[26] = st->last_sent.id.trigger;
    obuf[27] = st->last_sent.seq;
    obuf[28] = sent_packets;
     d5c:	28 41 00 00 	lw r1,(r2+0)
    
    obuf[15] = st->flags;
    obuf[16] = st->log_level;
    obuf[17] = st->mode;

    obuf[18] = st->total_pulses;
     d60:	28 a2 00 00 	lw r2,(r5+0)
    obuf[19] = st->sent_pulses;
     d64:	38 c6 1a 08 	ori r6,r6,0x1a08
    obuf[20] = st->dead_time;
     d68:	78 0f 00 00 	mvhi r15,0x0
    
    obuf[15] = st->flags;
    obuf[16] = st->log_level;
    obuf[17] = st->mode;

    obuf[18] = st->total_pulses;
     d6c:	58 44 00 00 	sw (r2+0),r4
    obuf[19] = st->sent_pulses;
     d70:	28 64 00 6c 	lw r4,(r3+108)
     d74:	28 c2 00 00 	lw r2,(r6+0)
    obuf[20] = st->dead_time;
     d78:	39 ef 1a 0c 	ori r15,r15,0x1a0c

    obuf[21] = st->last_sent.ts.seconds;
     d7c:	78 05 00 00 	mvhi r5,0x0
    obuf[15] = st->flags;
    obuf[16] = st->log_level;
    obuf[17] = st->mode;

    obuf[18] = st->total_pulses;
    obuf[19] = st->sent_pulses;
     d80:	58 44 00 00 	sw (r2+0),r4
    obuf[20] = st->dead_time;
     d84:	28 64 00 74 	lw r4,(r3+116)
     d88:	29 e2 00 00 	lw r2,(r15+0)

    obuf[21] = st->last_sent.ts.seconds;
     d8c:	38 a5 1a 10 	ori r5,r5,0x1a10
    obuf[22] = st->last_sent.ts.cycles;
     d90:	78 06 00 00 	mvhi r6,0x0
    obuf[16] = st->log_level;
    obuf[17] = st->mode;

    obuf[18] = st->total_pulses;
    obuf[19] = st->sent_pulses;
    obuf[20] = st->dead_time;
     d94:	58 44 00 00 	sw (r2+0),r4

    obuf[21] = st->last_sent.ts.seconds;
     d98:	28 64 00 3c 	lw r4,(r3+60)
     d9c:	28 a2 00 00 	lw r2,(r5+0)
    obuf[22] = st->last_sent.ts.cycles;
     da0:	38 c6 1a 14 	ori r6,r6,0x1a14
    obuf[23] = st->last_sent.ts.frac;
     da4:	78 0f 00 00 	mvhi r15,0x0

    obuf[18] = st->total_pulses;
    obuf[19] = st->sent_pulses;
    obuf[20] = st->dead_time;

    obuf[21] = st->last_sent.ts.seconds;
     da8:	58 44 00 00 	sw (r2+0),r4
    obuf[22] = st->last_sent.ts.cycles;
     dac:	28 64 00 40 	lw r4,(r3+64)
     db0:	28 c2 00 00 	lw r2,(r6+0)
    obuf[23] = st->last_sent.ts.frac;
     db4:	39 ef 1a 18 	ori r15,r15,0x1a18
    obuf[24] = st->last_sent.id.system;
     db8:	78 05 00 00 	mvhi r5,0x0
    obuf[18] = st->total_pulses;
    obuf[19] = st->sent_pulses;
    obuf[20] = st->dead_time;

    obuf[21] = st->last_sent.ts.seconds;
    obuf[22] = st->last_sent.ts.cycles;
     dbc:	58 44 00 00 	sw (r2+0),r4
    obuf[23] = st->last_sent.ts.frac;
     dc0:	28 64 00 44 	lw r4,(r3+68)
     dc4:	29 e2 00 00 	lw r2,(r15+0)
    obuf[24] = st->last_sent.id.system;
     dc8:	38 a5 1a 1c 	ori r5,r5,0x1a1c
    obuf[25] = st->last_sent.id.source_port;
     dcc:	78 06 00 00 	mvhi r6,0x0
    obuf[19] = st->sent_pulses;
    obuf[20] = st->dead_time;

    obuf[21] = st->last_sent.ts.seconds;
    obuf[22] = st->last_sent.ts.cycles;
    obuf[23] = st->last_sent.ts.frac;
     dd0:	58 44 00 00 	sw (r2+0),r4
    obuf[24] = st->last_sent.id.system;
     dd4:	28 64 00 48 	lw r4,(r3+72)
     dd8:	28 a2 00 00 	lw r2,(r5+0)
    obuf[25] = st->last_sent.id.source_port;
     ddc:	38 c6 1a 20 	ori r6,r6,0x1a20
    obuf[26] = st->last_sent.id.trigger;
     de0:	78 0f 00 00 	mvhi r15,0x0
    obuf[20] = st->dead_time;

    obuf[21] = st->last_sent.ts.seconds;
    obuf[22] = st->last_sent.ts.cycles;
    obuf[23] = st->last_sent.ts.frac;
    obuf[24] = st->last_sent.id.system;
     de4:	58 44 00 00 	sw (r2+0),r4
    obuf[25] = st->last_sent.id.source_port;
     de8:	28 64 00 4c 	lw r4,(r3+76)
     dec:	28 c2 00 00 	lw r2,(r6+0)
    obuf[26] = st->last_sent.id.trigger;
     df0:	39 ef 1a 24 	ori r15,r15,0x1a24
    obuf[27] = st->last_sent.seq;
    obuf[28] = sent_packets;
     df4:	78 05 00 00 	mvhi r5,0x0

    obuf[21] = st->last_sent.ts.seconds;
    obuf[22] = st->last_sent.ts.cycles;
    obuf[23] = st->last_sent.ts.frac;
    obuf[24] = st->last_sent.id.system;
    obuf[25] = st->last_sent.id.source_port;
     df8:	58 44 00 00 	sw (r2+0),r4
    obuf[26] = st->last_sent.id.trigger;
     dfc:	28 64 00 50 	lw r4,(r3+80)
     e00:	29 e2 00 00 	lw r2,(r15+0)
    obuf[27] = st->last_sent.seq;
    obuf[28] = sent_packets;
     e04:	38 a5 1a 2c 	ori r5,r5,0x1a2c
     e08:	78 06 00 00 	mvhi r6,0x0
    obuf[21] = st->last_sent.ts.seconds;
    obuf[22] = st->last_sent.ts.cycles;
    obuf[23] = st->last_sent.ts.frac;
    obuf[24] = st->last_sent.id.system;
    obuf[25] = st->last_sent.id.source_port;
    obuf[26] = st->last_sent.id.trigger;
     e0c:	58 44 00 00 	sw (r2+0),r4
    obuf[27] = st->last_sent.seq;
     e10:	78 04 00 00 	mvhi r4,0x0
     e14:	38 84 1a 28 	ori r4,r4,0x1a28
     e18:	28 63 00 54 	lw r3,(r3+84)
     e1c:	28 82 00 00 	lw r2,(r4+0)
     e20:	38 c6 1a 30 	ori r6,r6,0x1a30
     e24:	78 04 40 01 	mvhi r4,0x4001
     e28:	58 43 00 00 	sw (r2+0),r3
    obuf[28] = sent_packets;
     e2c:	28 a2 00 00 	lw r2,(r5+0)
     e30:	38 84 40 00 	ori r4,r4,0x4000
     e34:	58 41 00 00 	sw (r2+0),r1
     e38:	28 c2 00 00 	lw r2,(r6+0)
     e3c:	59 02 00 00 	sw (r8+0),r2
     e40:	78 02 08 00 	mvhi r2,0x800
     e44:	58 82 00 00 	sw (r4+0),r2
     e48:	e3 ff fd 50 	bi 388 <main+0x14c>
}

static inline void ctl_chan_set_dead_time (int seq, uint32_t *buf)
{
    int i;
    dp_writel( buf[1], DR_REG_DEAD_TIME);
     e4c:	78 0f 00 00 	mvhi r15,0x0
     e50:	39 ef 19 a4 	ori r15,r15,0x19a4
     e54:	29 e2 00 00 	lw r2,(r15+0)
  return *(volatile uint32_t *) ( reg + CPU_DP_BASE );
}

static inline void dp_writel ( uint32_t value, uint32_t reg )
{
     *(volatile uint32_t *) ( reg + CPU_DP_BASE ) = value;
     e58:	78 01 00 00 	mvhi r1,0x0
     e5c:	38 21 19 88 	ori r1,r1,0x1988
     e60:	28 43 00 00 	lw r3,(r2+0)
     e64:	78 06 00 00 	mvhi r6,0x0
     e68:	28 22 00 00 	lw r2,(r1+0)
     e6c:	38 c6 19 ac 	ori r6,r6,0x19ac
     e70:	28 c4 00 00 	lw r4,(r6+0)
     e74:	58 43 00 00 	sw (r2+0),r3
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     e78:	78 05 00 00 	mvhi r5,0x0
static inline void ctl_chan_set_dead_time (int seq, uint32_t *buf)
{
    int i;
    dp_writel( buf[1], DR_REG_DEAD_TIME);
    for(i=0; i < TDC_NUM_CHANNELS; i++)
        channels[i].dead_time = buf[1];
     e7c:	5a c3 00 74 	sw (r22+116),r3
     e80:	5a c3 00 ec 	sw (r22+236),r3
     e84:	5a c3 01 64 	sw (r22+356),r3
     e88:	5a c3 01 dc 	sw (r22+476),r3
     e8c:	5a c3 02 54 	sw (r22+596),r3
     e90:	78 02 01 00 	mvhi r2,0x100
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     e94:	38 a5 19 b0 	ori r5,r5,0x19b0
     e98:	58 82 00 00 	sw (r4+0),r2
     e9c:	28 a2 00 00 	lw r2,(r5+0)
    buf[1] = seq;
     ea0:	78 06 00 00 	mvhi r6,0x0
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     ea4:	34 03 01 00 	mvi r3,256
    buf[1] = seq;
     ea8:	38 c6 19 b4 	ori r6,r6,0x19b4
}

static inline void ctl_ack( uint32_t seq )
{
    uint32_t *buf = ctl_claim_out();
    buf[0] = ID_REP_ACK;
     eac:	58 43 00 00 	sw (r2+0),r3
    buf[1] = seq;
     eb0:	28 c2 00 00 	lw r2,(r6+0)
     eb4:	78 0f 00 00 	mvhi r15,0x0
     eb8:	39 ef 19 b8 	ori r15,r15,0x19b8
     ebc:	58 47 00 00 	sw (r2+0),r7
     ec0:	29 e2 00 00 	lw r2,(r15+0)
     ec4:	58 82 00 00 	sw (r4+0),r2
     ec8:	78 04 40 01 	mvhi r4,0x4001
     ecc:	78 02 08 00 	mvhi r2,0x800
     ed0:	38 84 40 00 	ori r4,r4,0x4000
     ed4:	58 82 00 00 	sw (r4+0),r2
     ed8:	e3 ff fd 2c 	bi 388 <main+0x14c>
{
    uint32_t mask = dp_readl(DR_REG_CHAN_ENABLE);
    int channel = buf[0];
    struct tdc_channel_state *ch = &channels[channel];
    
    int enable = buf[1];
     edc:	78 04 00 00 	mvhi r4,0x0
     ee0:	38 84 19 a4 	ori r4,r4,0x19a4
     ee4:	28 82 00 00 	lw r2,(r4+0)
}

static inline void ctl_chan_enable (int seq, uint32_t *buf)
{
    uint32_t mask = dp_readl(DR_REG_CHAN_ENABLE);
    int channel = buf[0];
     ee8:	78 05 00 00 	mvhi r5,0x0
     eec:	38 a5 19 a8 	ori r5,r5,0x19a8
    struct tdc_channel_state *ch = &channels[channel];
    
    int enable = buf[1];

    if(enable)
     ef0:	28 44 00 00 	lw r4,(r2+0)

void rt_set_debug_slot(int slot);

static inline uint32_t dp_readl ( uint32_t reg )
{
  return *(volatile uint32_t *) ( reg + CPU_DP_BASE );
     ef4:	78 02 00 20 	mvhi r2,0x20
     ef8:	28 46 00 00 	lw r6,(r2+0)
}

static inline void ctl_chan_enable (int seq, uint32_t *buf)
{
    uint32_t mask = dp_readl(DR_REG_CHAN_ENABLE);
    int channel = buf[0];
     efc:	28 a2 00 00 	lw r2,(r5+0)
     f00:	28 43 00 00 	lw r3,(r2+0)
    struct tdc_channel_state *ch = &channels[channel];
    
    int enable = buf[1];

    if(enable)
     f04:	44 80 00 93 	be r4,r0,1150 <main+0xf14>
    {
	   mask |= (1<<channel);
       ch->flags |= LIST_ENABLED;
     f08:	08 64 00 78 	muli r4,r3,120
    
    int enable = buf[1];

    if(enable)
    {
	   mask |= (1<<channel);
     f0c:	34 02 00 01 	mvi r2,1
     f10:	bc 43 18 00 	sl r3,r2,r3
       ch->flags |= LIST_ENABLED;
     f14:	b6 c4 20 00 	add r4,r22,r4
     f18:	28 82 00 58 	lw r2,(r4+88)
    
    int enable = buf[1];

    if(enable)
    {
	   mask |= (1<<channel);
     f1c:	b8 66 18 00 	or r3,r3,r6
       ch->flags |= LIST_ENABLED;
     f20:	38 42 00 01 	ori r2,r2,0x1
     f24:	58 82 00 58 	sw (r4+88),r2
     f28:	78 06 00 00 	mvhi r6,0x0
     f2c:	38 c6 19 ac 	ori r6,r6,0x19ac
     f30:	28 c4 00 00 	lw r4,(r6+0)
}

static inline void dp_writel ( uint32_t value, uint32_t reg )
{
     *(volatile uint32_t *) ( reg + CPU_DP_BASE ) = value;
     f34:	78 02 00 20 	mvhi r2,0x20
     f38:	58 43 00 00 	sw (r2+0),r3
     f3c:	78 0f 01 00 	mvhi r15,0x100
     f40:	58 8f 00 00 	sw (r4+0),r15
     f44:	e3 ff fe 71 	bi 908 <main+0x6cc>
    ctl_ack(seq);
}

static inline void ctl_chan_set_delay (int seq, uint32_t *buf)
{
    int channel = buf[0];
     f48:	78 01 00 00 	mvhi r1,0x0
     f4c:	38 21 19 a8 	ori r1,r1,0x19a8
     f50:	28 22 00 00 	lw r2,(r1+0)
    struct tdc_channel_state *ch = &channels[channel];

    ch->delay.seconds = buf[1];
     f54:	78 04 00 00 	mvhi r4,0x0
     f58:	38 84 19 a4 	ori r4,r4,0x19a4
    ctl_ack(seq);
}

static inline void ctl_chan_set_delay (int seq, uint32_t *buf)
{
    int channel = buf[0];
     f5c:	28 43 00 00 	lw r3,(r2+0)
    struct tdc_channel_state *ch = &channels[channel];

    ch->delay.seconds = buf[1];
     f60:	28 82 00 00 	lw r2,(r4+0)
    ch->delay.cycles = buf[2];
     f64:	78 01 00 00 	mvhi r1,0x0
static inline void ctl_chan_set_delay (int seq, uint32_t *buf)
{
    int channel = buf[0];
    struct tdc_channel_state *ch = &channels[channel];

    ch->delay.seconds = buf[1];
     f68:	3c 64 00 03 	sli r4,r3,3
     f6c:	28 45 00 00 	lw r5,(r2+0)
     f70:	3c 62 00 07 	sli r2,r3,7
    ch->delay.cycles = buf[2];
     f74:	38 21 19 bc 	ori r1,r1,0x19bc
static inline void ctl_chan_set_delay (int seq, uint32_t *buf)
{
    int channel = buf[0];
    struct tdc_channel_state *ch = &channels[channel];

    ch->delay.seconds = buf[1];
     f78:	c8 44 10 00 	sub r2,r2,r4
     f7c:	b6 c2 20 00 	add r4,r22,r2
    ch->delay.cycles = buf[2];
     f80:	28 22 00 00 	lw r2,(r1+0)
static inline void ctl_chan_set_delay (int seq, uint32_t *buf)
{
    int channel = buf[0];
    struct tdc_channel_state *ch = &channels[channel];

    ch->delay.seconds = buf[1];
     f84:	58 85 00 0c 	sw (r4+12),r5
     f88:	78 0f 00 00 	mvhi r15,0x0
    ch->delay.cycles = buf[2];
     f8c:	28 43 00 00 	lw r3,(r2+0)
     f90:	39 ef 19 ac 	ori r15,r15,0x19ac
     f94:	29 e6 00 00 	lw r6,(r15+0)
     f98:	58 83 00 10 	sw (r4+16),r3
    ch->delay.frac = buf[3];
     f9c:	78 03 00 00 	mvhi r3,0x0
     fa0:	38 63 19 c0 	ori r3,r3,0x19c0
     fa4:	28 62 00 00 	lw r2,(r3+0)
     fa8:	28 43 00 00 	lw r3,(r2+0)
     fac:	58 83 00 14 	sw (r4+20),r3
     fb0:	e3 ff fd c6 	bi 6c8 <main+0x48c>
    a->seconds -= b->seconds;

    if(a->seconds == -1)
    {
      a->seconds = 0;
      a->cycles -= 125000000;
     fb4:	b5 a9 68 00 	add r13,r13,r9

    a->seconds -= b->seconds;

    if(a->seconds == -1)
    {
      a->seconds = 0;
     fb8:	34 0c 00 00 	mvi r12,0
     fbc:	e3 ff fd 31 	bi 480 <main+0x244>

    ch->prev = *ts;

    if( (ch->flags & LIST_TRIGGER_ASSIGNED ) && (ch->flags & LIST_ARMED) )
    {
    	ch->seq++;
     fc0:	28 ee 00 70 	lw r14,(r7+112)
    	ch->flags |= LIST_TRIGGERED;
     fc4:	28 e5 00 58 	lw r5,(r7+88)
    	if(ch->mode == LIST_TRIGGER_MODE_SINGLE )
     fc8:	28 e2 00 60 	lw r2,(r7+96)

    ch->prev = *ts;

    if( (ch->flags & LIST_TRIGGER_ASSIGNED ) && (ch->flags & LIST_ARMED) )
    {
    	ch->seq++;
     fcc:	35 ce 00 01 	addi r14,r14,1
    	ch->flags |= LIST_TRIGGERED;
     fd0:	38 a5 00 10 	ori r5,r5,0x10

    ch->prev = *ts;

    if( (ch->flags & LIST_TRIGGER_ASSIGNED ) && (ch->flags & LIST_ARMED) )
    {
    	ch->seq++;
     fd4:	58 ee 00 70 	sw (r7+112),r14
    	ch->flags |= LIST_TRIGGERED;
     fd8:	58 e5 00 58 	sw (r7+88),r5
    	if(ch->mode == LIST_TRIGGER_MODE_SINGLE )
     fdc:	44 53 00 49 	be r2,r19,1100 <main+0xec4>
    
static inline void send_trigger (struct tdc_channel_state *ch, struct list_timestamp *ts)
{
    volatile struct list_trigger_message *msg = mq_map_out_buffer(1, TDC_OUT_SLOT_REMOTE);

    msg->triggers[coalesce_count].id = ch->id;
     fe0:	2b 81 00 54 	lw r1,(sp+84)
    	ch->seq++;
    	ch->flags |= LIST_TRIGGERED;
    	if(ch->mode == LIST_TRIGGER_MODE_SINGLE )
    	    ch->flags &= ~LIST_ARMED;
    	
        ch->sent_pulses++;
     fe4:	b6 cf 28 00 	add r5,r22,r15
     fe8:	28 a7 00 6c 	lw r7,(r5+108)
    
static inline void send_trigger (struct tdc_channel_state *ch, struct list_timestamp *ts)
{
    volatile struct list_trigger_message *msg = mq_map_out_buffer(1, TDC_OUT_SLOT_REMOTE);

    msg->triggers[coalesce_count].id = ch->id;
     fec:	28 23 00 00 	lw r3,(r1+0)
     ff0:	28 a2 00 00 	lw r2,(r5+0)
    	ch->seq++;
    	ch->flags |= LIST_TRIGGERED;
    	if(ch->mode == LIST_TRIGGER_MODE_SINGLE )
    	    ch->flags &= ~LIST_ARMED;
    	
        ch->sent_pulses++;
     ff4:	34 e7 00 01 	addi r7,r7,1
    
static inline void send_trigger (struct tdc_channel_state *ch, struct list_timestamp *ts)
{
    volatile struct list_trigger_message *msg = mq_map_out_buffer(1, TDC_OUT_SLOT_REMOTE);

    msg->triggers[coalesce_count].id = ch->id;
     ff8:	3c 7d 00 02 	sli ra,r3,2
     ffc:	3c 6e 00 05 	sli r14,r3,5
    	ch->seq++;
    	ch->flags |= LIST_TRIGGERED;
    	if(ch->mode == LIST_TRIGGER_MODE_SINGLE )
    	    ch->flags &= ~LIST_ARMED;
    	
        ch->sent_pulses++;
    1000:	58 a7 00 6c 	sw (r5+108),r7
    
static inline void send_trigger (struct tdc_channel_state *ch, struct list_timestamp *ts)
{
    volatile struct list_trigger_message *msg = mq_map_out_buffer(1, TDC_OUT_SLOT_REMOTE);

    msg->triggers[coalesce_count].id = ch->id;
    1004:	c9 dd 58 00 	sub r11,r14,ra
    1008:	b7 0b 30 00 	add r6,r24,r11
    100c:	34 dd 00 20 	addi ra,r6,32
    1010:	5b a2 00 04 	sw (ra+4),r2
    1014:	28 a2 00 04 	lw r2,(r5+4)
    msg->triggers[coalesce_count].seq = ch->seq;
    msg->triggers[coalesce_count].ts = *ts; 
    1018:	34 c7 00 18 	addi r7,r6,24
    
    ch->last = *ts;
    coalesce_count++;
    101c:	34 a4 00 40 	addi r4,r5,64
    
static inline void send_trigger (struct tdc_channel_state *ch, struct list_timestamp *ts)
{
    volatile struct list_trigger_message *msg = mq_map_out_buffer(1, TDC_OUT_SLOT_REMOTE);

    msg->triggers[coalesce_count].id = ch->id;
    1020:	5b a2 00 08 	sw (ra+8),r2
    1024:	28 a2 00 08 	lw r2,(r5+8)
    msg->triggers[coalesce_count].seq = ch->seq;
    msg->triggers[coalesce_count].ts = *ts; 
    
    ch->last = *ts;
    coalesce_count++;
    1028:	34 63 00 01 	addi r3,r3,1
    
static inline void send_trigger (struct tdc_channel_state *ch, struct list_timestamp *ts)
{
    volatile struct list_trigger_message *msg = mq_map_out_buffer(1, TDC_OUT_SLOT_REMOTE);

    msg->triggers[coalesce_count].id = ch->id;
    102c:	5b a2 00 0c 	sw (ra+12),r2
    msg->triggers[coalesce_count].seq = ch->seq;
    1030:	28 ae 00 70 	lw r14,(r5+112)
    1034:	58 ce 00 30 	sw (r6+48),r14
    1038:	5b 8d 00 68 	sw (sp+104),r13
    103c:	5b 94 00 6c 	sw (sp+108),r20
    1040:	5b 8c 00 64 	sw (sp+100),r12
    msg->triggers[coalesce_count].ts = *ts; 
    1044:	58 cc 00 18 	sw (r6+24),r12
    1048:	2b 8e 00 68 	lw r14,(sp+104)
    	    ch->flags &= ~LIST_ARMED;
    	
        ch->sent_pulses++;
        send_trigger(ch, ts);
        ch->last_sent.ts  = *ts;
        ch->last_sent.id = ch->id;
    104c:	28 a2 00 00 	lw r2,(r5+0)
    1050:	28 a8 00 04 	lw r8,(r5+4)
{
    volatile struct list_trigger_message *msg = mq_map_out_buffer(1, TDC_OUT_SLOT_REMOTE);

    msg->triggers[coalesce_count].id = ch->id;
    msg->triggers[coalesce_count].seq = ch->seq;
    msg->triggers[coalesce_count].ts = *ts; 
    1054:	58 ee 00 04 	sw (r7+4),r14
    1058:	2b 8e 00 6c 	lw r14,(sp+108)
    	    ch->flags &= ~LIST_ARMED;
    	
        ch->sent_pulses++;
        send_trigger(ch, ts);
        ch->last_sent.ts  = *ts;
        ch->last_sent.id = ch->id;
    105c:	28 a6 00 08 	lw r6,(r5+8)
    1060:	58 82 00 08 	sw (r4+8),r2
{
    volatile struct list_trigger_message *msg = mq_map_out_buffer(1, TDC_OUT_SLOT_REMOTE);

    msg->triggers[coalesce_count].id = ch->id;
    msg->triggers[coalesce_count].seq = ch->seq;
    msg->triggers[coalesce_count].ts = *ts; 
    1064:	58 ee 00 08 	sw (r7+8),r14
    	
        ch->sent_pulses++;
        send_trigger(ch, ts);
        ch->last_sent.ts  = *ts;
        ch->last_sent.id = ch->id;
        ch->last_sent.seq = ch->seq;
    1068:	28 a2 00 70 	lw r2,(r5+112)
    msg->triggers[coalesce_count].id = ch->id;
    msg->triggers[coalesce_count].seq = ch->seq;
    msg->triggers[coalesce_count].ts = *ts; 
    
    ch->last = *ts;
    coalesce_count++;
    106c:	58 23 00 00 	sw (r1+0),r3
{
    volatile struct list_trigger_message *msg = mq_map_out_buffer(1, TDC_OUT_SLOT_REMOTE);

    msg->triggers[coalesce_count].id = ch->id;
    msg->triggers[coalesce_count].seq = ch->seq;
    msg->triggers[coalesce_count].ts = *ts; 
    1070:	58 ac 00 24 	sw (r5+36),r12
    1074:	58 ad 00 28 	sw (r5+40),r13
    1078:	58 b4 00 2c 	sw (r5+44),r20
    
    ch->last = *ts;
    coalesce_count++;
    107c:	58 ac 00 3c 	sw (r5+60),r12
    1080:	58 ad 00 40 	sw (r5+64),r13
    1084:	58 b4 00 44 	sw (r5+68),r20
    	    ch->flags &= ~LIST_ARMED;
    	
        ch->sent_pulses++;
        send_trigger(ch, ts);
        ch->last_sent.ts  = *ts;
        ch->last_sent.id = ch->id;
    1088:	58 88 00 0c 	sw (r4+12),r8
    108c:	58 86 00 10 	sw (r4+16),r6
        ch->last_sent.seq = ch->seq;
        ch->flags |= LIST_LAST_VALID;
    1090:	28 a3 00 58 	lw r3,(r5+88)
    	
        ch->sent_pulses++;
        send_trigger(ch, ts);
        ch->last_sent.ts  = *ts;
        ch->last_sent.id = ch->id;
        ch->last_sent.seq = ch->seq;
    1094:	58 a2 00 54 	sw (r5+84),r2
        ch->flags |= LIST_LAST_VALID;

	    if(ch->log_level & LIST_LOG_SENT)
    1098:	28 a2 00 5c 	lw r2,(r5+92)
        ch->sent_pulses++;
        send_trigger(ch, ts);
        ch->last_sent.ts  = *ts;
        ch->last_sent.id = ch->id;
        ch->last_sent.seq = ch->seq;
        ch->flags |= LIST_LAST_VALID;
    109c:	38 63 00 04 	ori r3,r3,0x4
    10a0:	58 a3 00 58 	sw (r5+88),r3

	    if(ch->log_level & LIST_LOG_SENT)
    10a4:	20 42 00 02 	andi r2,r2,0x2
    10a8:	44 40 fd 3a 	be r2,r0,590 <main+0x354>
    10ac:	78 02 01 00 	mvhi r2,0x100
    10b0:	5b 62 00 00 	sw (fp+0),r2
static inline void log_sent_trigger(struct tdc_channel_state *ch, struct list_timestamp *ts)
{
    volatile struct list_log_entry *msg =  mq_map_out_buffer(0, TDC_OUT_SLOT_LOGGING);
    
    mq_claim(0, TDC_OUT_SLOT_LOGGING);
    msg->type = ID_LOG_SENT_TRIGGER;
    10b4:	5a f5 00 00 	sw (r23+0),r21
    msg->seq = ch->seq;
    10b8:	28 a2 00 70 	lw r2,(r5+112)
    10bc:	78 03 04 00 	mvhi r3,0x400
    10c0:	38 63 00 09 	ori r3,r3,0x9
    10c4:	5a e2 00 04 	sw (r23+4),r2
    msg->id = ch->id;
    10c8:	28 a2 00 00 	lw r2,(r5+0)
    10cc:	5a e2 00 0c 	sw (r23+12),r2
    10d0:	28 a2 00 04 	lw r2,(r5+4)
    10d4:	5a e2 00 10 	sw (r23+16),r2
    10d8:	28 a2 00 08 	lw r2,(r5+8)
    10dc:	5a e2 00 14 	sw (r23+20),r2
    msg->ts = *ts;
    10e0:	2b 82 00 64 	lw r2,(sp+100)
    10e4:	5a e2 00 18 	sw (r23+24),r2
    10e8:	2b 82 00 68 	lw r2,(sp+104)
    10ec:	5a e2 00 1c 	sw (r23+28),r2
    10f0:	2b 82 00 6c 	lw r2,(sp+108)
    10f4:	5a e2 00 20 	sw (r23+32),r2
    10f8:	5b 63 00 00 	sw (fp+0),r3
    10fc:	e3 ff fd 25 	bi 590 <main+0x354>
    if( (ch->flags & LIST_TRIGGER_ASSIGNED ) && (ch->flags & LIST_ARMED) )
    {
    	ch->seq++;
    	ch->flags |= LIST_TRIGGERED;
    	if(ch->mode == LIST_TRIGGER_MODE_SINGLE )
    	    ch->flags &= ~LIST_ARMED;
    1100:	a0 aa 28 00 	and r5,r5,r10
    1104:	58 e5 00 58 	sw (r7+88),r5
    1108:	e3 ff ff b6 	bi fe0 <main+0xda4>
        ch->id.source_port = buf[3];
        ch->id.trigger = buf[4];
        ch->flags |= LIST_TRIGGER_ASSIGNED;
        ch->flags &= ~LIST_LAST_VALID;
    } else {
        ch->id.system = 0;
    110c:	08 42 00 78 	muli r2,r2,120
    1110:	b6 c2 18 00 	add r3,r22,r2
        ch->id.source_port = 0;
        ch->id.trigger = 0;
        ch->flags &= ~LIST_TRIGGER_ASSIGNED;
    1114:	28 64 00 58 	lw r4,(r3+88)
    1118:	34 02 ff fd 	mvi r2,-3
        ch->id.source_port = buf[3];
        ch->id.trigger = buf[4];
        ch->flags |= LIST_TRIGGER_ASSIGNED;
        ch->flags &= ~LIST_LAST_VALID;
    } else {
        ch->id.system = 0;
    111c:	58 60 00 00 	sw (r3+0),r0
        ch->id.source_port = 0;
        ch->id.trigger = 0;
        ch->flags &= ~LIST_TRIGGER_ASSIGNED;
    1120:	a0 82 10 00 	and r2,r4,r2
        ch->id.trigger = buf[4];
        ch->flags |= LIST_TRIGGER_ASSIGNED;
        ch->flags &= ~LIST_LAST_VALID;
    } else {
        ch->id.system = 0;
        ch->id.source_port = 0;
    1124:	58 60 00 04 	sw (r3+4),r0
        ch->id.trigger = 0;
    1128:	58 60 00 08 	sw (r3+8),r0
        ch->flags &= ~LIST_TRIGGER_ASSIGNED;
    112c:	58 62 00 58 	sw (r3+88),r2
    1130:	e3 ff fe 3e 	bi a28 <main+0x7ec>
    if(buf[1]) {
        ch->flags |= LIST_ARMED;
        ch->flags &= ~LIST_TRIGGERED;
    }
    else
        ch->flags &= ~(LIST_ARMED | LIST_TRIGGERED);
    1134:	08 42 00 78 	muli r2,r2,120
    1138:	b6 c2 18 00 	add r3,r22,r2
    113c:	28 64 00 58 	lw r4,(r3+88)
    1140:	34 02 ff e7 	mvi r2,-25
    1144:	a0 82 10 00 	and r2,r4,r2
    1148:	58 62 00 58 	sw (r3+88),r2
    114c:	e3 ff fe 37 	bi a28 <main+0x7ec>
    {
	   mask |= (1<<channel);
       ch->flags |= LIST_ENABLED;
    } else {
	   mask &= ~(1<<channel);
       ch->flags &= ~LIST_ENABLED | LIST_ARMED | LIST_TRIGGERED | LIST_LAST_VALID;
    1150:	08 64 00 78 	muli r4,r3,120
    if(enable)
    {
	   mask |= (1<<channel);
       ch->flags |= LIST_ENABLED;
    } else {
	   mask &= ~(1<<channel);
    1154:	34 02 00 01 	mvi r2,1
    1158:	bc 43 18 00 	sl r3,r2,r3
       ch->flags &= ~LIST_ENABLED | LIST_ARMED | LIST_TRIGGERED | LIST_LAST_VALID;
    115c:	b6 c4 20 00 	add r4,r22,r4
    1160:	28 85 00 58 	lw r5,(r4+88)
    1164:	34 02 ff fe 	mvi r2,-2
    if(enable)
    {
	   mask |= (1<<channel);
       ch->flags |= LIST_ENABLED;
    } else {
	   mask &= ~(1<<channel);
    1168:	a4 60 18 00 	not r3,r3
       ch->flags &= ~LIST_ENABLED | LIST_ARMED | LIST_TRIGGERED | LIST_LAST_VALID;
    116c:	a0 a2 10 00 	and r2,r5,r2
    if(enable)
    {
	   mask |= (1<<channel);
       ch->flags |= LIST_ENABLED;
    } else {
	   mask &= ~(1<<channel);
    1170:	a0 66 18 00 	and r3,r3,r6
       ch->flags &= ~LIST_ENABLED | LIST_ARMED | LIST_TRIGGERED | LIST_LAST_VALID;
    1174:	58 82 00 58 	sw (r4+88),r2
    1178:	e3 ff ff 6c 	bi f28 <main+0xcec>

0000117c <pp_vsprintf>:
		*(out++) = tmp[i++];
	return ret;
}

int pp_vsprintf(char *buf, const char *fmt, va_list args)
{
    117c:	37 9c ff a8 	addi sp,sp,-88
    1180:	5b 8b 00 44 	sw (sp+68),r11
    1184:	5b 8c 00 40 	sw (sp+64),r12
    1188:	5b 8d 00 3c 	sw (sp+60),r13
    118c:	5b 8e 00 38 	sw (sp+56),r14
    1190:	5b 8f 00 34 	sw (sp+52),r15
    1194:	5b 90 00 30 	sw (sp+48),r16
    1198:	5b 91 00 2c 	sw (sp+44),r17
    119c:	5b 92 00 28 	sw (sp+40),r18
    11a0:	5b 93 00 24 	sw (sp+36),r19
    11a4:	5b 94 00 20 	sw (sp+32),r20
    11a8:	5b 95 00 1c 	sw (sp+28),r21
    11ac:	5b 96 00 18 	sw (sp+24),r22
    11b0:	5b 97 00 14 	sw (sp+20),r23
    11b4:	5b 98 00 10 	sw (sp+16),r24
    11b8:	5b 99 00 0c 	sw (sp+12),r25
    11bc:	5b 9b 00 08 	sw (sp+8),fp
    11c0:	5b 9d 00 04 	sw (sp+4),ra
    11c4:	b8 40 70 00 	mv r14,r2
	char *s, *str = buf;
	int base, lead, wid;

	for (; *fmt ; ++fmt) {
    11c8:	40 42 00 00 	lbu r2,(r2+0)
		*(out++) = tmp[i++];
	return ret;
}

int pp_vsprintf(char *buf, const char *fmt, va_list args)
{
    11cc:	b8 20 c0 00 	mv r24,r1
	char *s, *str = buf;
    11d0:	b8 20 68 00 	mv r13,r1
		*(out++) = tmp[i++];
	return ret;
}

int pp_vsprintf(char *buf, const char *fmt, va_list args)
{
    11d4:	b8 60 98 00 	mv r19,r3
	char *s, *str = buf;
	int base, lead, wid;

	for (; *fmt ; ++fmt) {
    11d8:	34 01 00 00 	mvi r1,0
    11dc:	44 40 00 1b 	be r2,r0,1248 <pp_vsprintf+0xcc>
    11e0:	78 16 00 00 	mvhi r22,0x0
    11e4:	78 11 00 00 	mvhi r17,0x0
		if (*fmt != '%') {
    11e8:	34 14 00 25 	mvi r20,37
    11ec:	3a d6 1a 94 	ori r22,r22,0x1a94
		base = 10;
		lead = ' ';
		wid = 1;
	repeat:
		fmt++;		/* Skip '%' initially, other stuff later */
		switch(*fmt) {
    11f0:	34 19 00 78 	mvi r25,120
	while (i > 16 - wid + negative)
		tmp[--i] = lead;
	if (negative)
		tmp[--i] = '-';
	ret = 16 - i;
	while (i < 16)
    11f4:	34 12 00 0f 	mvi r18,15
		*(out++) = tmp[i++];
	return ret;
}

int pp_vsprintf(char *buf, const char *fmt, va_list args)
    11f8:	37 8f 00 4c 	addi r15,sp,76
    11fc:	3a 31 1a 80 	ori r17,r17,0x1a80
    1200:	e0 00 00 06 	bi 1218 <pp_vsprintf+0x9c>
	char *s, *str = buf;
	int base, lead, wid;

	for (; *fmt ; ++fmt) {
		if (*fmt != '%') {
			*str++ = *fmt;
    1204:	31 a2 00 00 	sb (r13+0),r2
    1208:	35 ad 00 01 	addi r13,r13,1
int pp_vsprintf(char *buf, const char *fmt, va_list args)
{
	char *s, *str = buf;
	int base, lead, wid;

	for (; *fmt ; ++fmt) {
    120c:	35 ce 00 01 	addi r14,r14,1
    1210:	41 c2 00 00 	lbu r2,(r14+0)
    1214:	44 40 00 0c 	be r2,r0,1244 <pp_vsprintf+0xc8>
		if (*fmt != '%') {
    1218:	34 1b 00 01 	mvi fp,1
    121c:	34 15 00 20 	mvi r21,32
    1220:	34 10 00 0a 	mvi r16,10
    1224:	5c 54 ff f8 	bne r2,r20,1204 <pp_vsprintf+0x88>

		base = 10;
		lead = ' ';
		wid = 1;
	repeat:
		fmt++;		/* Skip '%' initially, other stuff later */
    1228:	35 ce 00 01 	addi r14,r14,1
		switch(*fmt) {
    122c:	41 c2 00 00 	lbu r2,(r14+0)
    1230:	54 59 00 99 	bgu r2,r25,1494 <pp_vsprintf+0x318>
    1234:	3c 41 00 02 	sli r1,r2,2
    1238:	b6 c1 08 00 	add r1,r22,r1
    123c:	28 21 00 00 	lw r1,(r1+0)
    1240:	c0 20 00 00 	b r1
int pp_vsprintf(char *buf, const char *fmt, va_list args)
{
	char *s, *str = buf;
	int base, lead, wid;

	for (; *fmt ; ++fmt) {
    1244:	c9 b8 08 00 	sub r1,r13,r24
			str += number(str, va_arg(args, int), base, lead, wid);
			break;
		}
	}
 ret:
	*str = '\0';
    1248:	31 a0 00 00 	sb (r13+0),r0
	return str - buf;


}
    124c:	2b 9d 00 04 	lw ra,(sp+4)
    1250:	2b 8b 00 44 	lw r11,(sp+68)
    1254:	2b 8c 00 40 	lw r12,(sp+64)
    1258:	2b 8d 00 3c 	lw r13,(sp+60)
    125c:	2b 8e 00 38 	lw r14,(sp+56)
    1260:	2b 8f 00 34 	lw r15,(sp+52)
    1264:	2b 90 00 30 	lw r16,(sp+48)
    1268:	2b 91 00 2c 	lw r17,(sp+44)
    126c:	2b 92 00 28 	lw r18,(sp+40)
    1270:	2b 93 00 24 	lw r19,(sp+36)
    1274:	2b 94 00 20 	lw r20,(sp+32)
    1278:	2b 95 00 1c 	lw r21,(sp+28)
    127c:	2b 96 00 18 	lw r22,(sp+24)
    1280:	2b 97 00 14 	lw r23,(sp+20)
    1284:	2b 98 00 10 	lw r24,(sp+16)
    1288:	2b 99 00 0c 	lw r25,(sp+12)
    128c:	2b 9b 00 08 	lw fp,(sp+8)
    1290:	37 9c 00 58 	addi sp,sp,88
    1294:	c3 a0 00 00 	ret
			if (base == 10) /* yet unchaged */
				base = 8;
		case 'd':
		case 'i':
		case 'u':
			str += number(str, va_arg(args, int), base, lead, wid);
    1298:	2a 6c 00 00 	lw r12,(r19+0)
{
	char tmp[16];
	int i = 16, ret, negative = 0;

	/* No error checking at all: it is as ugly as possible */
	if ((signed)value < 0 && base == 10) {
    129c:	66 01 00 0a 	cmpei r1,r16,10
    12a0:	5b 80 00 48 	sw (sp+72),r0
    12a4:	01 82 00 1f 	srui r2,r12,31
			if (base == 10) /* yet unchaged */
				base = 8;
		case 'd':
		case 'i':
		case 'u':
			str += number(str, va_arg(args, int), base, lead, wid);
    12a8:	36 73 00 04 	addi r19,r19,4
{
	char tmp[16];
	int i = 16, ret, negative = 0;

	/* No error checking at all: it is as ugly as possible */
	if ((signed)value < 0 && base == 10) {
    12ac:	a0 41 08 00 	and r1,r2,r1
static const char hex[] = "0123456789abcdef";

static int number(char *out, unsigned value, int base, int lead, int wid)
{
	char tmp[16];
	int i = 16, ret, negative = 0;
    12b0:	34 17 00 00 	mvi r23,0

	/* No error checking at all: it is as ugly as possible */
	if ((signed)value < 0 && base == 10) {
    12b4:	44 20 00 06 	be r1,r0,12cc <pp_vsprintf+0x150>
		negative = 1;
		value = -value;
    12b8:	66 a3 00 20 	cmpei r3,r21,32
    12bc:	c8 0c 60 00 	sub r12,r0,r12
    12c0:	5b 83 00 48 	sw (sp+72),r3
    12c4:	34 10 00 0a 	mvi r16,10
	char tmp[16];
	int i = 16, ret, negative = 0;

	/* No error checking at all: it is as ugly as possible */
	if ((signed)value < 0 && base == 10) {
		negative = 1;
    12c8:	34 17 00 01 	mvi r23,1
		value = -value;
	}
	while (value && i) {
    12cc:	45 80 00 78 	be r12,r0,14ac <pp_vsprintf+0x330>
    12d0:	34 0b 00 10 	mvi r11,16
		tmp[--i] = hex[value % base];
    12d4:	b9 80 08 00 	mv r1,r12
    12d8:	ba 00 10 00 	mv r2,r16
    12dc:	f8 00 00 eb 	calli 1688 <__umodsi3>
    12e0:	b6 21 08 00 	add r1,r17,r1
    12e4:	40 24 00 00 	lbu r4,(r1+0)
    12e8:	35 6b ff ff 	addi r11,r11,-1
	while (i < 16)
		*(out++) = tmp[i++];
	return ret;
}

int pp_vsprintf(char *buf, const char *fmt, va_list args)
    12ec:	b5 eb 10 00 	add r2,r15,r11
	if ((signed)value < 0 && base == 10) {
		negative = 1;
		value = -value;
	}
	while (value && i) {
		tmp[--i] = hex[value % base];
    12f0:	30 44 00 00 	sb (r2+0),r4
		value /= base;
    12f4:	b9 80 08 00 	mv r1,r12
    12f8:	ba 00 10 00 	mv r2,r16
    12fc:	f8 00 00 d3 	calli 1648 <__udivsi3>
	/* No error checking at all: it is as ugly as possible */
	if ((signed)value < 0 && base == 10) {
		negative = 1;
		value = -value;
	}
	while (value && i) {
    1300:	7d 64 00 00 	cmpnei r4,r11,0
    1304:	7c 22 00 00 	cmpnei r2,r1,0
		tmp[--i] = hex[value % base];
		value /= base;
    1308:	b8 20 60 00 	mv r12,r1
	/* No error checking at all: it is as ugly as possible */
	if ((signed)value < 0 && base == 10) {
		negative = 1;
		value = -value;
	}
	while (value && i) {
    130c:	a0 82 10 00 	and r2,r4,r2
    1310:	5c 40 ff f1 	bne r2,r0,12d4 <pp_vsprintf+0x158>
		tmp[--i] = hex[value % base];
		value /= base;
	}
	if (i == 16)
		tmp[--i] = '0';
	if (negative && lead == ' ') {
    1314:	2b 82 00 48 	lw r2,(sp+72)
    1318:	44 40 00 06 	be r2,r0,1330 <pp_vsprintf+0x1b4>
		tmp[--i] = '-';
    131c:	35 6b ff ff 	addi r11,r11,-1
    1320:	b5 eb 08 00 	add r1,r15,r11
    1324:	34 03 00 2d 	mvi r3,45
    1328:	30 23 00 00 	sb (r1+0),r3
		negative = 0;
    132c:	34 17 00 00 	mvi r23,0
	}
	while (i > 16 - wid + negative)
    1330:	34 01 00 10 	mvi r1,16
    1334:	c8 3b 20 00 	sub r4,r1,fp
    1338:	b4 97 20 00 	add r4,r4,r23
    133c:	4c 8b 00 06 	bge r4,r11,1354 <pp_vsprintf+0x1d8>
    1340:	22 a3 00 ff 	andi r3,r21,0xff
		tmp[--i] = lead;
    1344:	35 6b ff ff 	addi r11,r11,-1
	while (i < 16)
		*(out++) = tmp[i++];
	return ret;
}

int pp_vsprintf(char *buf, const char *fmt, va_list args)
    1348:	b5 eb 10 00 	add r2,r15,r11
	if (negative && lead == ' ') {
		tmp[--i] = '-';
		negative = 0;
	}
	while (i > 16 - wid + negative)
		tmp[--i] = lead;
    134c:	30 43 00 00 	sb (r2+0),r3
		tmp[--i] = '0';
	if (negative && lead == ' ') {
		tmp[--i] = '-';
		negative = 0;
	}
	while (i > 16 - wid + negative)
    1350:	5d 64 ff fd 	bne r11,r4,1344 <pp_vsprintf+0x1c8>
		tmp[--i] = lead;
	if (negative)
    1354:	46 e0 00 05 	be r23,r0,1368 <pp_vsprintf+0x1ec>
		tmp[--i] = '-';
    1358:	35 6b ff ff 	addi r11,r11,-1
    135c:	b5 eb 08 00 	add r1,r15,r11
    1360:	34 02 00 2d 	mvi r2,45
    1364:	30 22 00 00 	sb (r1+0),r2
	ret = 16 - i;
    1368:	34 03 00 10 	mvi r3,16
    136c:	c8 6b 08 00 	sub r1,r3,r11
	while (i < 16)
    1370:	49 72 00 20 	bg r11,r18,13f0 <pp_vsprintf+0x274>
		*(out++) = tmp[i++];
	return ret;
}

int pp_vsprintf(char *buf, const char *fmt, va_list args)
    1374:	a5 60 48 00 	not r9,r11
    1378:	35 29 00 11 	addi r9,r9,17
    137c:	b5 eb 38 00 	add r7,r15,r11
    1380:	01 28 00 02 	srui r8,r9,2
    1384:	b8 ed 28 00 	or r5,r7,r13
    1388:	20 a5 00 03 	andi r5,r5,0x3
    138c:	3d 03 00 02 	sli r3,r8,2
    1390:	64 a5 00 00 	cmpei r5,r5,0
    1394:	75 22 00 03 	cmpgui r2,r9,0x3
    1398:	64 64 00 00 	cmpei r4,r3,0
    139c:	a0 a2 10 00 	and r2,r5,r2
    13a0:	18 42 00 01 	xori r2,r2,0x1
    13a4:	b8 82 10 00 	or r2,r4,r2
    13a8:	5c 40 00 45 	bne r2,r0,14bc <pp_vsprintf+0x340>
    13ac:	34 04 00 00 	mvi r4,0
    13b0:	b4 e4 28 00 	add r5,r7,r4
		tmp[--i] = lead;
	if (negative)
		tmp[--i] = '-';
	ret = 16 - i;
	while (i < 16)
		*(out++) = tmp[i++];
    13b4:	28 a6 00 00 	lw r6,(r5+0)
	return ret;
}

int pp_vsprintf(char *buf, const char *fmt, va_list args)
    13b8:	b5 a4 28 00 	add r5,r13,r4
		tmp[--i] = lead;
	if (negative)
		tmp[--i] = '-';
	ret = 16 - i;
	while (i < 16)
		*(out++) = tmp[i++];
    13bc:	34 42 00 01 	addi r2,r2,1
    13c0:	58 a6 00 00 	sw (r5+0),r6
    13c4:	34 84 00 04 	addi r4,r4,4
    13c8:	55 02 ff fa 	bgu r8,r2,13b0 <pp_vsprintf+0x234>
    13cc:	b5 63 58 00 	add r11,r11,r3
    13d0:	b5 a3 10 00 	add r2,r13,r3
    13d4:	44 69 00 07 	be r3,r9,13f0 <pp_vsprintf+0x274>
	return ret;
}

int pp_vsprintf(char *buf, const char *fmt, va_list args)
    13d8:	b5 eb 20 00 	add r4,r15,r11
		tmp[--i] = lead;
	if (negative)
		tmp[--i] = '-';
	ret = 16 - i;
	while (i < 16)
		*(out++) = tmp[i++];
    13dc:	40 83 00 00 	lbu r3,(r4+0)
    13e0:	35 6b 00 01 	addi r11,r11,1
    13e4:	30 43 00 00 	sb (r2+0),r3
    13e8:	34 42 00 01 	addi r2,r2,1
	while (i > 16 - wid + negative)
		tmp[--i] = lead;
	if (negative)
		tmp[--i] = '-';
	ret = 16 - i;
	while (i < 16)
    13ec:	4e 4b ff fb 	bge r18,r11,13d8 <pp_vsprintf+0x25c>
			if (base == 10) /* yet unchaged */
				base = 8;
		case 'd':
		case 'i':
		case 'u':
			str += number(str, va_arg(args, int), base, lead, wid);
    13f0:	b5 a1 68 00 	add r13,r13,r1
			break;
    13f4:	e3 ff ff 86 	bi 120c <pp_vsprintf+0x90>

		case 'c': /* char: supported */
			*str++ = (unsigned char) va_arg(args, int);
			break;
		case 's': /* string: supported */
			s = va_arg(args, char *);
    13f8:	2a 63 00 00 	lw r3,(r19+0)
    13fc:	36 73 00 04 	addi r19,r19,4
			while (*s)
    1400:	40 62 00 00 	lbu r2,(r3+0)
    1404:	44 40 ff 82 	be r2,r0,120c <pp_vsprintf+0x90>
				*str++ = *s++;
    1408:	31 a2 00 00 	sb (r13+0),r2
    140c:	34 63 00 01 	addi r3,r3,1
		case 'c': /* char: supported */
			*str++ = (unsigned char) va_arg(args, int);
			break;
		case 's': /* string: supported */
			s = va_arg(args, char *);
			while (*s)
    1410:	40 62 00 00 	lbu r2,(r3+0)
				*str++ = *s++;
    1414:	35 ad 00 01 	addi r13,r13,1
		case 'c': /* char: supported */
			*str++ = (unsigned char) va_arg(args, int);
			break;
		case 's': /* string: supported */
			s = va_arg(args, char *);
			while (*s)
    1418:	5c 40 ff fc 	bne r2,r0,1408 <pp_vsprintf+0x28c>
    141c:	e3 ff ff 7c 	bi 120c <pp_vsprintf+0x90>
		case 'p':
		case 'x':
		case 'X':
			base = 16;
		case 'o':
			if (base == 10) /* yet unchaged */
    1420:	34 02 00 0a 	mvi r2,10
		base = 10;
		lead = ' ';
		wid = 1;
	repeat:
		fmt++;		/* Skip '%' initially, other stuff later */
		switch(*fmt) {
    1424:	ba 60 08 00 	mv r1,r19
		case 'p':
		case 'x':
		case 'X':
			base = 16;
		case 'o':
			if (base == 10) /* yet unchaged */
    1428:	5e 02 ff 9c 	bne r16,r2,1298 <pp_vsprintf+0x11c>
				base = 8;
		case 'd':
		case 'i':
		case 'u':
			str += number(str, va_arg(args, int), base, lead, wid);
    142c:	28 2c 00 00 	lw r12,(r1+0)
    1430:	36 73 00 04 	addi r19,r19,4
    1434:	5b 80 00 48 	sw (sp+72),r0
		case 'x':
		case 'X':
			base = 16;
		case 'o':
			if (base == 10) /* yet unchaged */
				base = 8;
    1438:	34 10 00 08 	mvi r16,8
static const char hex[] = "0123456789abcdef";

static int number(char *out, unsigned value, int base, int lead, int wid)
{
	char tmp[16];
	int i = 16, ret, negative = 0;
    143c:	34 17 00 00 	mvi r23,0
    1440:	e3 ff ff a3 	bi 12cc <pp_vsprintf+0x150>
			goto repeat;

			/* Special cases for conversions */

		case 'c': /* char: supported */
			*str++ = (unsigned char) va_arg(args, int);
    1444:	2a 61 00 00 	lw r1,(r19+0)
    1448:	36 73 00 04 	addi r19,r19,4
    144c:	31 a1 00 00 	sb (r13+0),r1
    1450:	35 ad 00 01 	addi r13,r13,1
			break;
    1454:	e3 ff ff 6e 	bi 120c <pp_vsprintf+0x90>
			if (base == 10) /* yet unchaged */
				base = 8;
		case 'd':
		case 'i':
		case 'u':
			str += number(str, va_arg(args, int), base, lead, wid);
    1458:	2a 6c 00 00 	lw r12,(r19+0)

			/* integers are more or less printed */
		case 'p':
		case 'x':
		case 'X':
			base = 16;
    145c:	34 10 00 10 	mvi r16,16
			if (base == 10) /* yet unchaged */
				base = 8;
		case 'd':
		case 'i':
		case 'u':
			str += number(str, va_arg(args, int), base, lead, wid);
    1460:	5b 80 00 48 	sw (sp+72),r0
    1464:	36 73 00 04 	addi r19,r19,4
static const char hex[] = "0123456789abcdef";

static int number(char *out, unsigned value, int base, int lead, int wid)
{
	char tmp[16];
	int i = 16, ret, negative = 0;
    1468:	34 17 00 00 	mvi r23,0
    146c:	e3 ff ff 98 	bi 12cc <pp_vsprintf+0x150>
		fmt++;		/* Skip '%' initially, other stuff later */
		switch(*fmt) {
		case '\0':
			goto ret;
		case '0':
			lead = '0';
    1470:	34 15 00 30 	mvi r21,48
    1474:	e3 ff ff 6d 	bi 1228 <pp_vsprintf+0xac>
			goto repeat;

		case '*':
			/* should be precision, just eat it */
			base = va_arg(args, int);
    1478:	2a 70 00 00 	lw r16,(r19+0)
    147c:	36 73 00 04 	addi r19,r19,4
    1480:	e3 ff ff 6a 	bi 1228 <pp_vsprintf+0xac>
				*str++ = *s++;
			break;
		case 'n': /* number-thus-far: not supported */
			break;
		case '%': /* supported */
			*str++ = '%';
    1484:	34 01 00 25 	mvi r1,37
    1488:	31 a1 00 00 	sb (r13+0),r1
    148c:	35 ad 00 01 	addi r13,r13,1
			break;
    1490:	e3 ff ff 5f 	bi 120c <pp_vsprintf+0x90>
		case '*':
			/* should be precision, just eat it */
			base = va_arg(args, int);
			/* fall through: discard unknown stuff */
		default:
			if (*fmt >= '1' && *fmt <= '9')
    1494:	34 41 ff cf 	addi r1,r2,-49
    1498:	20 21 00 ff 	andi r1,r1,0xff
    149c:	34 03 00 08 	mvi r3,8
    14a0:	54 23 ff 62 	bgu r1,r3,1228 <pp_vsprintf+0xac>
				wid = *fmt - '0';
    14a4:	34 5b ff d0 	addi fp,r2,-48
    14a8:	e3 ff ff 60 	bi 1228 <pp_vsprintf+0xac>
	while (value && i) {
		tmp[--i] = hex[value % base];
		value /= base;
	}
	if (i == 16)
		tmp[--i] = '0';
    14ac:	34 01 00 30 	mvi r1,48
    14b0:	33 81 00 5b 	sb (sp+91),r1
    14b4:	34 0b 00 0f 	mvi r11,15
    14b8:	e3 ff ff 97 	bi 1314 <pp_vsprintf+0x198>
	while (i < 16)
		*(out++) = tmp[i++];
	return ret;
}

int pp_vsprintf(char *buf, const char *fmt, va_list args)
    14bc:	b9 a0 10 00 	mv r2,r13
    14c0:	e3 ff ff c6 	bi 13d8 <pp_vsprintf+0x25c>

000014c4 <pp_vprintf>:
#include <pp-printf.h>

static char print_buf[CONFIG_PRINT_BUFSIZE];

int pp_vprintf(const char *fmt, va_list args)
{
    14c4:	37 9c ff f4 	addi sp,sp,-12
    14c8:	5b 8b 00 0c 	sw (sp+12),r11
    14cc:	5b 8c 00 08 	sw (sp+8),r12
    14d0:	5b 9d 00 04 	sw (sp+4),ra
	int ret;

	ret = pp_vsprintf(print_buf, fmt, args);
    14d4:	78 0b 00 00 	mvhi r11,0x0
    14d8:	39 6b 1e e4 	ori r11,r11,0x1ee4
#include <pp-printf.h>

static char print_buf[CONFIG_PRINT_BUFSIZE];

int pp_vprintf(const char *fmt, va_list args)
{
    14dc:	b8 40 18 00 	mv r3,r2
	int ret;

	ret = pp_vsprintf(print_buf, fmt, args);
    14e0:	b8 20 10 00 	mv r2,r1
    14e4:	b9 60 08 00 	mv r1,r11
    14e8:	fb ff ff 25 	calli 117c <pp_vsprintf>
    14ec:	b8 20 60 00 	mv r12,r1
	puts(print_buf);
    14f0:	b9 60 08 00 	mv r1,r11
    14f4:	f8 00 00 34 	calli 15c4 <puts>
	return ret;
}
    14f8:	b9 80 08 00 	mv r1,r12
    14fc:	2b 9d 00 04 	lw ra,(sp+4)
    1500:	2b 8b 00 0c 	lw r11,(sp+12)
    1504:	2b 8c 00 08 	lw r12,(sp+8)
    1508:	37 9c 00 0c 	addi sp,sp,12
    150c:	c3 a0 00 00 	ret

00001510 <pp_sprintf>:

int pp_sprintf(char *s, const char *fmt, ...)
{
    1510:	37 9c ff e0 	addi sp,sp,-32
    1514:	5b 9d 00 04 	sw (sp+4),ra
    1518:	5b 83 00 0c 	sw (sp+12),r3
	va_list args;
	int ret;

	va_start(args, fmt);
	ret = pp_vsprintf(s, fmt, args);
    151c:	37 83 00 0c 	addi r3,sp,12
	puts(print_buf);
	return ret;
}

int pp_sprintf(char *s, const char *fmt, ...)
{
    1520:	5b 82 00 08 	sw (sp+8),r2
    1524:	5b 84 00 10 	sw (sp+16),r4
    1528:	5b 85 00 14 	sw (sp+20),r5
    152c:	5b 86 00 18 	sw (sp+24),r6
    1530:	5b 87 00 1c 	sw (sp+28),r7
    1534:	5b 88 00 20 	sw (sp+32),r8
	va_list args;
	int ret;

	va_start(args, fmt);
	ret = pp_vsprintf(s, fmt, args);
    1538:	fb ff ff 11 	calli 117c <pp_vsprintf>
	va_end(args);
	return ret;
}
    153c:	2b 9d 00 04 	lw ra,(sp+4)
    1540:	37 9c 00 20 	addi sp,sp,32
    1544:	c3 a0 00 00 	ret

00001548 <pp_printf>:


int pp_printf(const char *fmt, ...)
{
    1548:	37 9c ff d4 	addi sp,sp,-44
    154c:	5b 8b 00 0c 	sw (sp+12),r11
    1550:	5b 8c 00 08 	sw (sp+8),r12
    1554:	5b 9d 00 04 	sw (sp+4),ra

int pp_vprintf(const char *fmt, va_list args)
{
	int ret;

	ret = pp_vsprintf(print_buf, fmt, args);
    1558:	78 0b 00 00 	mvhi r11,0x0
    155c:	39 6b 1e e4 	ori r11,r11,0x1ee4
	return ret;
}


int pp_printf(const char *fmt, ...)
{
    1560:	5b 81 00 10 	sw (sp+16),r1
    1564:	5b 82 00 14 	sw (sp+20),r2
    1568:	5b 83 00 18 	sw (sp+24),r3

int pp_vprintf(const char *fmt, va_list args)
{
	int ret;

	ret = pp_vsprintf(print_buf, fmt, args);
    156c:	b8 20 10 00 	mv r2,r1
    1570:	37 83 00 14 	addi r3,sp,20
    1574:	b9 60 08 00 	mv r1,r11
	return ret;
}


int pp_printf(const char *fmt, ...)
{
    1578:	5b 84 00 1c 	sw (sp+28),r4
    157c:	5b 85 00 20 	sw (sp+32),r5
    1580:	5b 86 00 24 	sw (sp+36),r6
    1584:	5b 87 00 28 	sw (sp+40),r7
    1588:	5b 88 00 2c 	sw (sp+44),r8

int pp_vprintf(const char *fmt, va_list args)
{
	int ret;

	ret = pp_vsprintf(print_buf, fmt, args);
    158c:	fb ff fe fc 	calli 117c <pp_vsprintf>
    1590:	b8 20 60 00 	mv r12,r1
	puts(print_buf);
    1594:	b9 60 08 00 	mv r1,r11
    1598:	f8 00 00 0b 	calli 15c4 <puts>
	va_start(args, fmt);
	ret = pp_vprintf(fmt, args);
	va_end(args);

	return ret;
}
    159c:	b9 80 08 00 	mv r1,r12
    15a0:	2b 9d 00 04 	lw ra,(sp+4)
    15a4:	2b 8b 00 0c 	lw r11,(sp+12)
    15a8:	2b 8c 00 08 	lw r12,(sp+8)
    15ac:	37 9c 00 2c 	addi sp,sp,44
    15b0:	c3 a0 00 00 	ret

000015b4 <rt_set_debug_slot>:

static int debug_slot;

void rt_set_debug_slot(int slot)
{
    debug_slot = slot;
    15b4:	78 02 00 00 	mvhi r2,0x0
    15b8:	38 42 1f 64 	ori r2,r2,0x1f64
    15bc:	58 41 00 00 	sw (r2+0),r1
}
    15c0:	c3 a0 00 00 	ret

000015c4 <puts>:
}

static void *mq_map_out_buffer(int remote, int slot)
{
  uint32_t base = remote ? RMQ_BASE : HMQ_BASE;
  return (void *) (base + MQ_OUT (slot) + MQ_SLOT_DATA_START );
    15c4:	78 02 00 00 	mvhi r2,0x0
    15c8:	38 42 1f 64 	ori r2,r2,0x1f64
    15cc:	28 42 00 00 	lw r2,(r2+0)
    15d0:	78 06 40 01 	mvhi r6,0x4001
static inline void mq_writel( int remote, uint32_t val, uint32_t reg )
{
  if(remote)
    * (volatile uint32_t * ) (RMQ_BASE + reg) = val ;
  else
    * (volatile uint32_t * ) (HMQ_BASE + reg) = val ;
    15d4:	78 03 01 00 	mvhi r3,0x100
}

static void *mq_map_out_buffer(int remote, int slot)
{
  uint32_t base = remote ? RMQ_BASE : HMQ_BASE;
  return (void *) (base + MQ_OUT (slot) + MQ_SLOT_DATA_START );
    15d8:	34 42 00 20 	addi r2,r2,32
    15dc:	3c 42 00 0a 	sli r2,r2,10

int puts(const char *p)
{
    15e0:	b8 20 20 00 	mv r4,r1
    15e4:	b4 46 30 00 	add r6,r2,r6
static inline void mq_writel( int remote, uint32_t val, uint32_t reg )
{
  if(remote)
    * (volatile uint32_t * ) (RMQ_BASE + reg) = val ;
  else
    * (volatile uint32_t * ) (HMQ_BASE + reg) = val ;
    15e8:	58 c3 00 00 	sw (r6+0),r3
    int i;
    volatile uint32_t *buf = mq_map_out_buffer(0, debug_slot);
    
    mq_claim(0, debug_slot);
    
    buf[0] = 0xdeadbeef;
    15ec:	78 03 00 00 	mvhi r3,0x0
    15f0:	38 63 1c 78 	ori r3,r3,0x1c78
    15f4:	28 61 00 00 	lw r1,(r3+0)
    15f8:	78 03 00 00 	mvhi r3,0x0
    15fc:	38 63 1c 7c 	ori r3,r3,0x1c7c
    1600:	58 c1 00 08 	sw (r6+8),r1
    1604:	28 61 00 00 	lw r1,(r3+0)
    for(i=0;i<127;i++,p++)
    1608:	34 05 00 7f 	mvi r5,127
    int i;
    volatile uint32_t *buf = mq_map_out_buffer(0, debug_slot);
    
    mq_claim(0, debug_slot);
    
    buf[0] = 0xdeadbeef;
    160c:	b4 41 10 00 	add r2,r2,r1
    for(i=0;i<127;i++,p++)
    1610:	34 01 00 00 	mvi r1,0
    1614:	e0 00 00 05 	bi 1628 <puts+0x64>
      {
  	   if(*p)
	       buf[i+1] = *p;
    1618:	58 43 00 00 	sw (r2+0),r3
    volatile uint32_t *buf = mq_map_out_buffer(0, debug_slot);
    
    mq_claim(0, debug_slot);
    
    buf[0] = 0xdeadbeef;
    for(i=0;i<127;i++,p++)
    161c:	34 21 00 01 	addi r1,r1,1
    1620:	34 42 00 04 	addi r2,r2,4
    1624:	44 25 00 04 	be r1,r5,1634 <puts+0x70>
int	_EXFUN(getc, (FILE *));
int	_EXFUN(getchar, (void));
char *  _EXFUN(gets, (char *));
int	_EXFUN(putc, (int, FILE *));
int	_EXFUN(putchar, (int));
int	_EXFUN(puts, (const char *));
    1628:	b4 81 18 00 	add r3,r4,r1
      {
  	   if(*p)
    162c:	40 63 00 00 	lbu r3,(r3+0)
    1630:	5c 60 ff fa 	bne r3,r0,1618 <puts+0x54>
	       buf[i+1] = *p;
	     else
	       break;
      }

    mq_send(0, debug_slot, i + 1);
    1634:	34 23 00 01 	addi r3,r1,1
  mq_writel ( remote, MQ_CMD_CLAIM, MQ_OUT(slot) + MQ_SLOT_COMMAND );
}

static inline void mq_send( int remote, int slot, int count)
{
  mq_writel ( remote, MQ_CMD_READY | count, MQ_OUT(slot) + MQ_SLOT_COMMAND );
    1638:	78 02 04 00 	mvhi r2,0x400
    163c:	b8 62 10 00 	or r2,r3,r2
static inline void mq_writel( int remote, uint32_t val, uint32_t reg )
{
  if(remote)
    * (volatile uint32_t * ) (RMQ_BASE + reg) = val ;
  else
    * (volatile uint32_t * ) (HMQ_BASE + reg) = val ;
    1640:	58 c2 00 00 	sw (r6+0),r2
    return i;
}
    1644:	c3 a0 00 00 	ret

00001648 <__udivsi3>:
    1648:	37 9c ff fc 	addi sp,sp,-4
    164c:	5b 9d 00 04 	sw (sp+4),ra
    1650:	44 40 00 06 	be r2,r0,1668 <__udivsi3+0x20>
    1654:	34 03 00 00 	mvi r3,0
    1658:	f8 00 00 1c 	calli 16c8 <__udivmodsi4>
    165c:	2b 9d 00 04 	lw ra,(sp+4)
    1660:	37 9c 00 04 	addi sp,sp,4
    1664:	c3 a0 00 00 	ret
    1668:	90 00 08 00 	rcsr r1,IE
    166c:	20 21 00 01 	andi r1,r1,0x1
    1670:	b4 21 08 00 	add r1,r1,r1
    1674:	d0 01 00 00 	wcsr IE,r1
    1678:	90 e0 08 00 	rcsr r1,EBA
    167c:	bb a0 f0 00 	mv ea,ra
    1680:	34 21 00 a0 	addi r1,r1,160
    1684:	c0 20 00 00 	b r1

00001688 <__umodsi3>:
    1688:	37 9c ff fc 	addi sp,sp,-4
    168c:	5b 9d 00 04 	sw (sp+4),ra
    1690:	44 40 00 06 	be r2,r0,16a8 <__umodsi3+0x20>
    1694:	34 03 00 01 	mvi r3,1
    1698:	f8 00 00 0c 	calli 16c8 <__udivmodsi4>
    169c:	2b 9d 00 04 	lw ra,(sp+4)
    16a0:	37 9c 00 04 	addi sp,sp,4
    16a4:	c3 a0 00 00 	ret
    16a8:	90 00 08 00 	rcsr r1,IE
    16ac:	20 21 00 01 	andi r1,r1,0x1
    16b0:	b4 21 08 00 	add r1,r1,r1
    16b4:	d0 01 00 00 	wcsr IE,r1
    16b8:	90 e0 08 00 	rcsr r1,EBA
    16bc:	bb a0 f0 00 	mv ea,ra
    16c0:	34 21 00 a0 	addi r1,r1,160
    16c4:	c0 20 00 00 	b r1

000016c8 <__udivmodsi4>:
    16c8:	f4 22 20 00 	cmpgu r4,r1,r2
    16cc:	44 80 00 18 	be r4,r0,172c <__udivmodsi4+0x64>
    16d0:	34 04 00 01 	mvi r4,1
    16d4:	4c 40 00 0b 	bge r2,r0,1700 <__udivmodsi4+0x38>
    16d8:	34 05 00 00 	mvi r5,0
    16dc:	54 41 00 03 	bgu r2,r1,16e8 <__udivmodsi4+0x20>
    16e0:	c8 22 08 00 	sub r1,r1,r2
    16e4:	b8 a4 28 00 	or r5,r5,r4
    16e8:	00 84 00 01 	srui r4,r4,1
    16ec:	00 42 00 01 	srui r2,r2,1
    16f0:	5c 80 ff fb 	bne r4,r0,16dc <__udivmodsi4+0x14>
    16f4:	5c 60 00 02 	bne r3,r0,16fc <__udivmodsi4+0x34>
    16f8:	b8 a0 08 00 	mv r1,r5
    16fc:	c3 a0 00 00 	ret
    1700:	b4 42 10 00 	add r2,r2,r2
    1704:	b4 84 20 00 	add r4,r4,r4
    1708:	7c 86 00 00 	cmpnei r6,r4,0
    170c:	f4 22 28 00 	cmpgu r5,r1,r2
    1710:	a0 c5 28 00 	and r5,r6,r5
    1714:	44 a0 00 02 	be r5,r0,171c <__udivmodsi4+0x54>
    1718:	4c 40 ff fa 	bge r2,r0,1700 <__udivmodsi4+0x38>
    171c:	34 05 00 00 	mvi r5,0
    1720:	44 80 ff f5 	be r4,r0,16f4 <__udivmodsi4+0x2c>
    1724:	34 05 00 00 	mvi r5,0
    1728:	e3 ff ff ed 	bi 16dc <__udivmodsi4+0x14>
    172c:	34 04 00 01 	mvi r4,1
    1730:	34 05 00 00 	mvi r5,0
    1734:	e3 ff ff ea 	bi 16dc <__udivmodsi4+0x14>

00001738 <memset>:
    1738:	37 9c ff e8 	addi sp,sp,-24
    173c:	5b 8b 00 14 	sw (sp+20),r11
    1740:	5b 8c 00 10 	sw (sp+16),r12
    1744:	5b 8d 00 0c 	sw (sp+12),r13
    1748:	5b 8e 00 08 	sw (sp+8),r14
    174c:	5b 9d 00 04 	sw (sp+4),ra
    1750:	b8 20 60 00 	mv r12,r1
    1754:	20 21 00 03 	andi r1,r1,0x3
    1758:	b8 40 68 00 	mv r13,r2
    175c:	b9 80 58 00 	mv r11,r12
    1760:	44 20 00 0b 	be r1,r0,178c <memset+0x54>
    1764:	44 60 00 39 	be r3,r0,1848 <memset+0x110>
    1768:	34 63 ff ff 	addi r3,r3,-1
    176c:	20 45 00 ff 	andi r5,r2,0xff
    1770:	e0 00 00 03 	bi 177c <memset+0x44>
    1774:	44 60 00 35 	be r3,r0,1848 <memset+0x110>
    1778:	34 63 ff ff 	addi r3,r3,-1
    177c:	31 65 00 00 	sb (r11+0),r5
    1780:	35 6b 00 01 	addi r11,r11,1
    1784:	21 64 00 03 	andi r4,r11,0x3
    1788:	5c 80 ff fb 	bne r4,r0,1774 <memset+0x3c>
    178c:	34 01 00 03 	mvi r1,3
    1790:	50 23 00 27 	bgeu r1,r3,182c <memset+0xf4>
    1794:	21 a5 00 ff 	andi r5,r13,0xff
    1798:	b4 a5 20 00 	add r4,r5,r5
    179c:	b4 84 20 00 	add r4,r4,r4
    17a0:	b4 84 20 00 	add r4,r4,r4
    17a4:	b4 84 20 00 	add r4,r4,r4
    17a8:	b4 84 20 00 	add r4,r4,r4
    17ac:	b4 84 20 00 	add r4,r4,r4
    17b0:	b4 84 20 00 	add r4,r4,r4
    17b4:	b4 84 20 00 	add r4,r4,r4
    17b8:	b8 85 70 00 	or r14,r4,r5
    17bc:	34 02 00 10 	mvi r2,16
    17c0:	b9 c0 08 00 	mv r1,r14
    17c4:	5b 83 00 18 	sw (sp+24),r3
    17c8:	f8 00 00 3f 	calli 18c4 <__ashlsi3>
    17cc:	2b 83 00 18 	lw r3,(sp+24)
    17d0:	34 02 00 0f 	mvi r2,15
    17d4:	b8 2e 28 00 	or r5,r1,r14
    17d8:	b9 60 38 00 	mv r7,r11
    17dc:	b9 60 20 00 	mv r4,r11
    17e0:	b8 60 30 00 	mv r6,r3
    17e4:	34 01 00 0f 	mvi r1,15
    17e8:	54 62 00 20 	bgu r3,r2,1868 <memset+0x130>
    17ec:	34 04 00 00 	mvi r4,0
    17f0:	34 02 00 03 	mvi r2,3
    17f4:	b4 e4 08 00 	add r1,r7,r4
    17f8:	34 84 00 04 	addi r4,r4,4
    17fc:	58 25 00 00 	sw (r1+0),r5
    1800:	c8 64 08 00 	sub r1,r3,r4
    1804:	54 22 ff fc 	bgu r1,r2,17f4 <memset+0xbc>
    1808:	34 63 ff fc 	addi r3,r3,-4
    180c:	00 61 00 01 	srui r1,r3,1
    1810:	20 63 00 03 	andi r3,r3,0x3
    1814:	00 21 00 01 	srui r1,r1,1
    1818:	34 21 00 01 	addi r1,r1,1
    181c:	b4 21 08 00 	add r1,r1,r1
    1820:	b4 21 08 00 	add r1,r1,r1
    1824:	b4 e1 38 00 	add r7,r7,r1
    1828:	b8 e0 58 00 	mv r11,r7
    182c:	44 60 00 07 	be r3,r0,1848 <memset+0x110>
    1830:	21 a2 00 ff 	andi r2,r13,0xff
    1834:	34 04 00 00 	mvi r4,0
    1838:	b5 64 28 00 	add r5,r11,r4
    183c:	30 a2 00 00 	sb (r5+0),r2
    1840:	34 84 00 01 	addi r4,r4,1
    1844:	5c 64 ff fd 	bne r3,r4,1838 <memset+0x100>
    1848:	b9 80 08 00 	mv r1,r12
    184c:	2b 9d 00 04 	lw ra,(sp+4)
    1850:	2b 8b 00 14 	lw r11,(sp+20)
    1854:	2b 8c 00 10 	lw r12,(sp+16)
    1858:	2b 8d 00 0c 	lw r13,(sp+12)
    185c:	2b 8e 00 08 	lw r14,(sp+8)
    1860:	37 9c 00 18 	addi sp,sp,24
    1864:	c3 a0 00 00 	ret
    1868:	58 85 00 00 	sw (r4+0),r5
    186c:	58 85 00 04 	sw (r4+4),r5
    1870:	58 85 00 08 	sw (r4+8),r5
    1874:	58 85 00 0c 	sw (r4+12),r5
    1878:	34 c6 ff f0 	addi r6,r6,-16
    187c:	34 84 00 10 	addi r4,r4,16
    1880:	54 c1 ff fa 	bgu r6,r1,1868 <memset+0x130>
    1884:	34 63 ff f0 	addi r3,r3,-16
    1888:	00 67 00 01 	srui r7,r3,1
    188c:	34 01 00 03 	mvi r1,3
    1890:	00 e7 00 01 	srui r7,r7,1
    1894:	20 63 00 0f 	andi r3,r3,0xf
    1898:	00 e7 00 01 	srui r7,r7,1
    189c:	00 e7 00 01 	srui r7,r7,1
    18a0:	34 e7 00 01 	addi r7,r7,1
    18a4:	b4 e7 38 00 	add r7,r7,r7
    18a8:	b4 e7 38 00 	add r7,r7,r7
    18ac:	b4 e7 38 00 	add r7,r7,r7
    18b0:	b4 e7 38 00 	add r7,r7,r7
    18b4:	b5 67 38 00 	add r7,r11,r7
    18b8:	54 61 ff cd 	bgu r3,r1,17ec <memset+0xb4>
    18bc:	b8 e0 58 00 	mv r11,r7
    18c0:	e3 ff ff db 	bi 182c <memset+0xf4>

000018c4 <__ashlsi3>:
    18c4:	20 42 00 1f 	andi r2,r2,0x1f
    18c8:	78 03 00 00 	mvhi r3,0x0
    18cc:	38 63 19 5c 	ori r3,r3,0x195c
    18d0:	b4 42 10 00 	add r2,r2,r2
    18d4:	b4 42 10 00 	add r2,r2,r2
    18d8:	c8 62 18 00 	sub r3,r3,r2
    18dc:	c0 60 00 00 	b r3

000018e0 <__ashlsi3_31>:
    18e0:	b4 21 08 00 	add r1,r1,r1

000018e4 <__ashlsi3_30>:
    18e4:	b4 21 08 00 	add r1,r1,r1

000018e8 <__ashlsi3_29>:
    18e8:	b4 21 08 00 	add r1,r1,r1

000018ec <__ashlsi3_28>:
    18ec:	b4 21 08 00 	add r1,r1,r1

000018f0 <__ashlsi3_27>:
    18f0:	b4 21 08 00 	add r1,r1,r1

000018f4 <__ashlsi3_26>:
    18f4:	b4 21 08 00 	add r1,r1,r1

000018f8 <__ashlsi3_25>:
    18f8:	b4 21 08 00 	add r1,r1,r1

000018fc <__ashlsi3_24>:
    18fc:	b4 21 08 00 	add r1,r1,r1

00001900 <__ashlsi3_23>:
    1900:	b4 21 08 00 	add r1,r1,r1

00001904 <__ashlsi3_22>:
    1904:	b4 21 08 00 	add r1,r1,r1

00001908 <__ashlsi3_21>:
    1908:	b4 21 08 00 	add r1,r1,r1

0000190c <__ashlsi3_20>:
    190c:	b4 21 08 00 	add r1,r1,r1

00001910 <__ashlsi3_19>:
    1910:	b4 21 08 00 	add r1,r1,r1

00001914 <__ashlsi3_18>:
    1914:	b4 21 08 00 	add r1,r1,r1

00001918 <__ashlsi3_17>:
    1918:	b4 21 08 00 	add r1,r1,r1

0000191c <__ashlsi3_16>:
    191c:	b4 21 08 00 	add r1,r1,r1

00001920 <__ashlsi3_15>:
    1920:	b4 21 08 00 	add r1,r1,r1

00001924 <__ashlsi3_14>:
    1924:	b4 21 08 00 	add r1,r1,r1

00001928 <__ashlsi3_13>:
    1928:	b4 21 08 00 	add r1,r1,r1

0000192c <__ashlsi3_12>:
    192c:	b4 21 08 00 	add r1,r1,r1

00001930 <__ashlsi3_11>:
    1930:	b4 21 08 00 	add r1,r1,r1

00001934 <__ashlsi3_10>:
    1934:	b4 21 08 00 	add r1,r1,r1

00001938 <__ashlsi3_9>:
    1938:	b4 21 08 00 	add r1,r1,r1

0000193c <__ashlsi3_8>:
    193c:	b4 21 08 00 	add r1,r1,r1

00001940 <__ashlsi3_7>:
    1940:	b4 21 08 00 	add r1,r1,r1

00001944 <__ashlsi3_6>:
    1944:	b4 21 08 00 	add r1,r1,r1

00001948 <__ashlsi3_5>:
    1948:	b4 21 08 00 	add r1,r1,r1

0000194c <__ashlsi3_4>:
    194c:	b4 21 08 00 	add r1,r1,r1

00001950 <__ashlsi3_3>:
    1950:	b4 21 08 00 	add r1,r1,r1

00001954 <__ashlsi3_2>:
    1954:	b4 21 08 00 	add r1,r1,r1

00001958 <__ashlsi3_1>:
    1958:	b4 21 08 00 	add r1,r1,r1

0000195c <__ashlsi3_0>:
    195c:	c3 a0 00 00 	ret
