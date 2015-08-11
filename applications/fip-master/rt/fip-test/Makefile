CROSS_COMPILE=/opt/gcc-lm32/bin/lm32-elf-

OBJS = fip-test.o
OUTPUT = fip-test
PATH_COMMON = ../../../common/

EXTRA_CFLAGS += -I../../include
EXTRA_CFLAGS += -DRTPERFORMANCE
include $(PATH_COMMON)/rt/Makefile

load:	all
	../../../../tools/wrnc-loader -D 0xb01 -i 0 -f fip-test.bin
	../../../../tools/wrnc-cpu-restart -D 0xb01 -i 0
	../../../../tools/wrnc-messages -Q