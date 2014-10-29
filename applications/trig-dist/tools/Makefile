# If it exists includes Makefile.specific. In this Makefile, you should put
# specific Makefile code that you want to run before this. For example,
# build a particular environment.
-include Makefile.specific

DESTDIR ?= /usr/local

CFLAGS += -Wall -ggdb -I../common -I$(WRNC)/lib -I$(WRNC)/kernel -I../lib $(EXTRACFLAGS)
LDLIBS += -L../lib -lwrtd -L$(WRNC)/lib -lwrnc

PROGS := wrtd-boot
PROGS += wrtd-tdc-config

all: $(PROGS)

install:
	install -d $(DESTDIR)/bin
	install -D $(PROGS) $(DESTDIR)/bin

%: %.c ../lib/libwrtd.a
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

# make nothing for modules_install, but avoid errors
modules_install:

clean:
	rm -f $(PROGS) *.o *~

.PHONY: all, clean
