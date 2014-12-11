# If it exists includes Makefile.specific. In this Makefile, you should put
# specific Makefile code that you want to run before this. For example,
# build a particular environment.
-include Makefile.specific

DESTDIR ?= /usr/local

LIBTDC ?= ../fmc-tdc-sw/lib
LIBFD ?= ../fine-delay-sw/lib
WRNC ?= ../../../
CFLAGS += -Wall -ggdb -I. -I../common -I$(WRNC)/lib -I$(WRNC)/kernel -I../lib $(EXTRACFLAGS)
LDLIBS += -L$(LIBFD) -lfdelay -L$(LIBTDC) -lfmctdc -L../lib -lwrtd -L$(WRNC)/lib -lwrnc

PROGS := wrtd-boot
PROGS += wrtd-in-config
PROGS += wrtd-out-config

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
