# If it exists includes Makefile.specific. In this Makefile, you should put
# specific Makefile code that you want to run before this. For example,
# build a particular environment.
-include Makefile.specific

TRTL ?= ../../../

CFLAGS += -Wall -ggdb -O0
CFLAGS += -I. -I../include -I$(TRTL)/include -I$(TRTL)/lib -I../lib
CFLAGS += $(EXTRACFLAGS)
CFLAGS += $(EXTRACFLAGS)
LDLIBS += -Wl,-Bstatic -L../lib -lwrtd -L$(TRTL)/lib -lmockturtle
LDLIBS += -Wl,-Bdynamic

PROG := wrtd-ut
SRCS = CuTest.c wrtd-ut-common.c wrtd-ut-input.c wrtd-ut-output.c wrtd-ut-op.c

all: $(PROG)
clean:
	rm -f $(PROG) *.o *~

.PHONY: all, clean

%: %.c $(SRCS) ../lib/libwrtd.a
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@
