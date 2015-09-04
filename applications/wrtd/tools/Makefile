# If it exists includes Makefile.specific. In this Makefile, you should put
# specific Makefile code that you want to run before this. For example,
# build a particular environment.
-include Makefile.specific

DESTDIR ?= /usr/local

WRNC ?= ../../../

CFLAGS += -Wall -ggdb -I. -I../include -I$(WRNC)/include -I$(WRNC)/lib -I../lib
CFLAGS += $(EXTRACFLAGS)
CFLAGS += $(EXTRACFLAGS)
LDLIBS += -Wl,-Bstatic -L../lib -lwrtd -L$(WRNC)/lib -lwrnc
LDLIBS += -Wl,-Bdynamic -lpthread

PROGS := wrtd-boot
PROGS += wrtd-in-config
PROGS += wrtd-out-config
PROGS += wrtd-logging
PROGS += wrtd-ping

all: $(PROGS)

install:
	install -d $(DESTDIR)/bin
	install -D $(PROGS) $(DESTDIR)/bin

%: %.c ../lib/libwrtd.a wrtd-inout-common.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@
wrtd-inout-common.o: wrtd-inout-common.c
	$(CC) -c $(CFLAGS) $^ -o $@

# make nothing for modules_install, but avoid errors
modules_install:

clean:
	rm -f $(PROGS) *.o *~

.PHONY: all, clean
