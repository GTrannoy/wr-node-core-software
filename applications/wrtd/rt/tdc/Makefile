include Makefile.specific

OBJS := wrtd-rt-tdc.o
OBJS += ../common/loop-queue.o
OUTPUT = wrtd-rt-tdc
WRNC = ../../../../

EXTRA_CFLAGS += -I../../include
EXTRA_CFLAGS += -I../common
EXTRA_CFLAGS += -DLIBRT_ERROR
RT_USE_LIBRT := 1
include $(WRNC)/applications/common/rt/Makefile
