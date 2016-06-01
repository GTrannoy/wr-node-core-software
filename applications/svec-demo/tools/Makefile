# If it exists includes Makefile.specific. In this Makefile, you should put
# specific Makefile code that you want to run before this. For example,
# build a particular environment.
-include Makefile.specific

DESTDIR ?= /usr/local

TRTL ?= ../../../

CFLAGS += -Wall -ggdb -I. -I../include -I$(TRTL)/include -I$(TRTL)/lib -I../lib
CFLAGS += $(EXTRACFLAGS)

LDLIBS += -Wl,-Bstatic -L../lib -ldemo -L$(TRTL)/lib -lmockturtle
LDLIBS += -Wl,-Bdynamic

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
