# and don't touch the rest unless you know what you're doing.
CROSS_COMPILE_TARGET ?= lm32-elf-
INSTALL_PREFIX ?= .
PATH_COMMON_RT ?= .
PATH_COMMON_H ?= ../include
WRNC ?= ../../

CC =		$(CROSS_COMPILE_TARGET)gcc
LD =		$(CROSS_COMPILE_TARGET)ld
OBJDUMP =	$(CROSS_COMPILE_TARGET)objdump
OBJCOPY =	$(CROSS_COMPILE_TARGET)objcopy
SIZE =		$(CROSS_COMPILE_TARGET)size

RT_GIT_VERSION = 0x$(shell cd ../../..; git rev-parse --short=8 HEAD)

CFLAGS += -Wall -DWRNODE_RT -g -O3 -mmultiply-enabled -mbarrel-shift-enabled
CFLAGS += -I.
CFLAGS += -I$(WRNC)/applications/common/rt
CFLAGS += -I$(WRNC)/applications/common/include
CFLAGS += -I$(WRNC)/include
CFLAGS += -DGIT_VERSION=$(RT_GIT_VERSION)
CFLAGS += $(EXTRA_CFLAGS)


OBJS += $(WRNC)/applications/common/rt/wrn-crt0.o
OBJS += $(WRNC)/applications/common/rt/vsprintf-xint.o
OBJS += $(WRNC)/applications/common/rt/printf.o
OBJS += $(WRNC)/applications/common/rt/rt-common.o
ifdef RT_USE_LIBRT
OBJS += $(WRNC)/applications/common/rt/librt.o
endif

LDSCRIPT = $(WRNC)/applications/common/rt/wrnode.ld

all: clean $(OUTPUT)

$(OUTPUT): $(LDSCRIPT) $(OBJS)
	${CC} -o $(OUTPUT).elf -nostartfiles $(OBJS) -T $(LDSCRIPT) -lgcc -lc
	${OBJCOPY} --remove-section .smem -O binary $(OUTPUT).elf $(OUTPUT).bin
	${OBJDUMP} -S $(OUTPUT).elf  > disasm.S
	$(SIZE) $(OUTPUT).elf

clean:
	rm -f $(OBJS) $(OUTPUT).bin

install:
	cp $(OUTPUT).bin $(INSTALL_PREFIX)
