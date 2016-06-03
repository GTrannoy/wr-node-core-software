OBJS = rt-demo.o
OUTPUT = rt-demo
TRTL = ../../../../

EXTRA_CFLAGS += -I../../include
EXTRA_CFLAGS += -I../common
EXTRA_CFLAGS += -DRTPERFORMANCE
EXTRA_CFLAGS += -DLIBRT_DEBUG -DLIBRT_ERROR
RT_USE_LIBRT := 1

include $(TRTL)/applications/common/rt/Makefile
