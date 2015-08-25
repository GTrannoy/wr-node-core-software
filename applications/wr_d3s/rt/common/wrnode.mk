# and don't touch the rest unless you know what you're doing.
CROSS_COMPILE ?= /user/twlostow/apps/gcc-lm32/bin/lm32-elf-
INSTALL_PREFIX ?= .

CC =		$(CROSS_COMPILE)gcc
LD =		$(CROSS_COMPILE)ld
OBJDUMP =	$(CROSS_COMPILE)objdump
OBJCOPY =	$(CROSS_COMPILE)objcopy
SIZE =		$(CROSS_COMPILE)size

CFLAGS = -DWRNODE_RT -g -O3 -I. -I../common -I../../include -mmultiply-enabled -mbarrel-shift-enabled
OBJS += ../common/wrn-crt0.o ../common/vsprintf-xint.o ../common/printf.o ../common/rt-common.o 
LDSCRIPT = ../common/wrnode.ld

all:	$(OUTPUT)

$(OUTPUT): $(LDSCRIPT) $(OBJS)
	${CC} -o $(OUTPUT).elf -nostartfiles $(OBJS) -T $(LDSCRIPT) -lgcc -lc
	${OBJCOPY} --remove-section .smem -O binary $(OUTPUT).elf $(OUTPUT).bin
	${OBJDUMP} -S $(OUTPUT).elf  > disasm.S
	$(SIZE) $(OUTPUT).elf

clean:
	rm -f $(OBJS) $(OUTPUT).bin
	
install:
	cp $(OUTPUT).bin $(INSTALL_PREFIX)