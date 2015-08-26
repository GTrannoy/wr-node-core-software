# If it exists includes Makefile.specific. In this Makefile, you should put
# specific Makefile code that you want to run before this. For example,
# build a particular environment.
-include Makefile.specific

WRNC ?= ../../../
LIBTDC ?= ../fmc-tdc-sw/lib
LIBFD ?= ../fine-delay-sw/lib

LIBS = libwrtd.so
LIB = libwrtd.a
LOBJ := libwrtd-internal.o
LOBJ += libwrtd-common.o
LOBJ += libwrtd-logging.o
LOBJ += libwrtd-input.o
LOBJ += libwrtd-output.o

CFLAGS += -Wall -ggdb -O2 -fPIC -I. -I../include -I$(WRNC)/include -I$(WRNC)/lib -I$(LIBFD) -I$(LIBTDC) -I$(LIBFD)/../kernel
CFLAGS += $(EXTRACFLAGS)
LDLIBS += -L. -lwrtd


modules all: $(LIB) $(LIBS)


%: %.c $(LIB)
	$(CC) $(CFLAGS) $(LDFLAGS) $*.c $(LDLIBS) -o $@

$(LIB): $(LOBJ)
	ar r $@ $^

$(LIBS): $(LIB)
	$(CC) -shared  -o $@ -Wl,--whole-archive,-soname,$@ $^ -Wl,--no-whole-archive

clean:
	rm -f $(LIB) .depend *.o *~

.depend: Makefile $(wildcard *.c *.h)
	$(CC) $(CFLAGS) -M $(LOBJ:.o=.c) -o $@

install modules_install:

-include .depend
