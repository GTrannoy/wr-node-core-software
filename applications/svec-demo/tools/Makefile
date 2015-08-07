# If it exists includes Makefile.specific. In this Makefile, you should put
# specific Makefile code that you want to run before this. For example,
# build a particular environment.
-include Makefile.specific

DESTDIR ?= /usr/local

WRNC ?= ../../../

CFLAGS += -Wall -ggdb -I. -I../include -I$(WRNC)/include -I$(WRNC)/lib -I../lib
CFLAGS += $(EXTRACFLAGS)
LDLIBS-WRNC := -L$(WRNC)/lib -lwrnc
LDLIBS += -L../lib -ldemo $(LDLIBS-WRNC)

PROGS := demo

all: $(PROGS)

install:
	install -d $(DESTDIR)/bin
	install -D $(PROGS) $(DESTDIR)/bin

demo: demo.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

# make nothing for modules_install, but avoid errors
modules_install:

clean:
	rm -f $(PROGS) *.o *~

.PHONY: all, clean
