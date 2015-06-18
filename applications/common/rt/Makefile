# and don't touch the rest unless you know what you're doing.
CROSS_COMPILE_TARGET ?= lm32-elf-
INSTALL_PREFIX ?= .
PATH_COMMON_RT ?= .
PATH_COMMON_H ?= ../include

CC =		$(CROSS_COMPILE_TARGET)gcc
LD =		$(CROSS_COMPILE_TARGET)ld
OBJDUMP =	$(CROSS_COMPILE_TARGET)objdump
OBJCOPY =	$(CROSS_COMPILE_TARGET)objcopy
SIZE =		$(CROSS_COMPILE_TARGET)size

CFLAGS += -DWRNODE_RT -g -O3 -mmultiply-enabled -mbarrel-shift-enabled
CFLAGS += -I.
CFLAGS += -I$(PATH_COMMON)/rt
CFLAGS += -I$(PATH_COMMON)/include
CFLAGS += $(EXTRA_CFLAGS)


OBJS += $(PATH_COMMON)/rt/wrn-crt0.o
OBJS += $(PATH_COMMON)/rt/vsprintf-xint.o
OBJS += $(PATH_COMMON)/rt/printf.o
OBJS += $(PATH_COMMON)/rt/rt-common.o

LDSCRIPT = $(PATH_COMMON)/rt/wrnode.ld

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
