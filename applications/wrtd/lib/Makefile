# If it exists includes Makefile.specific. In this Makefile, you should put
# specific Makefile code that you want to run before this. For example,
# build a particular environment.
-include Makefile.specific

WRNC ?= ../../../
LIBTDC ?= ../fmc-tdc-sw/lib
LIBFD ?= ../fine-delay-sw/lib

LIB = libwrtd.a
LOBJ := libwrtd-internal.o
LOBJ += libwrtd-common.o
LOBJ += libwrtd-logging.o
LOBJ += libwrtd-input.o
LOBJ += libwrtd-output.o

CFLAGS += -Wall -ggdb -O2 -I. -I../include -I$(WRNC)/lib -I$(WRNC)/kernel -I$(LIBFD) -I$(LIBTDC) -I$(LIBFD)/../kernel $(EXTRACFLAGS)
LDLIBS += -L. -lwrtd


modules all: $(LIB)


%: %.c $(LIB)
	$(CC) $(CFLAGS) $(LDFLAGS) $*.c $(LDLIBS) -o $@

$(LIB): $(LOBJ)
	ar r $@ $^

clean:
	rm -f $(LIB) .depend *.o *~

.depend: Makefile $(wildcard *.c *.h)
	$(CC) $(CFLAGS) -M $(LOBJ:.o=.c) -o $@

install modules_install:

-include .depend
