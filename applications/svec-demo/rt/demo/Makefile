OBJS = rt-demo.o
OUTPUT = rt-demo
WRNC = ../../../../

EXTRA_CFLAGS += -I../../include
EXTRA_CFLAGS += -I../common
EXTRA_CFLAGS += -D__MAX_ACTION_RECV=0
EXTRA_CFLAGS += -D__MAX_ACTION_SEND=0
EXTRA_CFLAGS += -DRTPERFORMANCE
EXTRA_CFLAGS += -DLIBRT_DEBUG -DLIBRT_ERROR
RT_USE_LIBRT := 1

include $(WRNC)/applications/common/rt/Makefile
