OBJS := wrtd-rt-tdc.o
OBJS += ../common/loop-queue.o
OUTPUT = wrtd-rt-tdc
WRNC = ../../../../

EXTRA_CFLAGS += -I../../include
EXTRA_CFLAGS += -I../common
EXTRA_CFLAGS += -D__MAX_ACTION_RECV=1
EXTRA_CFLAGS += -D__MAX_ACTION_SEND=0
EXTRA_CFLAGS += -DLIBRT_ERROR
RT_USE_LIBRT := 1
include $(WRNC)/applications/common/rt/Makefile
