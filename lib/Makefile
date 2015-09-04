# If it exists includes Makefile.specific. In this Makefile, you should put
# specific Makefile code that you want to run before this. For example,
# build a particular environment.
-include Makefile.specific

WRNC ?= ../

LIBS = libwrnc.so
LIB = libwrnc.a
LOBJ := libwrnc.o
LOBJ += libwrnc-rt-msg.o

CFLAGS += -Wall -ggdb -fPIC -I. -I$(WRNC)/include $(EXTRACFLAGS)
LDFLAGS = -L. -lwrnc


modules all: $(LIB) $(LIBS)


%: %.c $(LIB)
	$(CC) $(CFLAGS) $*.c $(LDFLAGS) -o $@

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
