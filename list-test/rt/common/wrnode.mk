# and don't touch the rest unless you know what you're doing.
CROSS_COMPILE ?= /user/twlostow/apps/gcc-lm32/bin/lm32-elf-

CC =		$(CROSS_COMPILE)gcc
LD =		$(CROSS_COMPILE)ld
OBJDUMP =	$(CROSS_COMPILE)objdump
OBJCOPY =	$(CROSS_COMPILE)objcopy
SIZE =		$(CROSS_COMPILE)size

CFLAGS = -g -Os -I. -I../common
OBJS += ../common/wrn-crt0.o ../common/vsprintf-mini.o ../common/printf.o
LDSCRIPT = ../common/wrnode.ld

$(OUTPUT): $(LDSCRIPT) $(OBJS)
	${CC} -o $(OUTPUT).elf -nostartfiles $(OBJS) -T $(LDSCRIPT) -lgcc -lc
	${OBJCOPY} -O binary $(OUTPUT).elf $(OUTPUT).bin
	${OBJDUMP} -S $(OUTPUT).elf  > disasm.S
	$(SIZE) $(OUTPUT).elf
#	../genramvhd -p wrc_simulation_firmware $(OUTPUT).bin >  wrc_simulation_firmware_pkg.vhd
#	../common/genraminit $(OUTPUT).bin > $(OUTPUT).ram

clean:
	rm -f $(OBJS) $(OUTPUT).bin