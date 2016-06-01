OBJS = autodemo.o ../common/demo-smem-code.o
OUTPUT = autodemo
TRTL = ../../../../

EXTRA_CFLAGS += -I../../include
EXTRA_CFLAGS += -I../common
EXTRA_CFLAGS += -DFPGA_APPLICATION_ID=0xd330d330
EXTRA_CFLAGS += -DRT_APPLICATION_ID=0x1236

include $(TRTL)/applications/common/rt/Makefile
