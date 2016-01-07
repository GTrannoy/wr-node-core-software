-include Makefile.specific

OBJS := wrtd-rt-fd.o
OBJS += ../common/loop-queue.o
OUTPUT = wrtd-rt-fd
WRNC = ../../../../

EXTRA_CFLAGS += -I../../include
EXTRA_CFLAGS += -I../common
EXTRA_CFLAGS += -DLIBRT_ERROR
EXTRA_CFLAGS += -DLIBRT_DEBUG
RT_USE_LIBRT := 1
include $(WRNC)/applications/common/rt/Makefile
