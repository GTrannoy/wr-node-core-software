# If it exists includes Makefile.specific. In this Makefile, you should put
# specific Makefile code that you want to run before this. For example,
# build a particular environment.
-include Makefile.specific

WRNC ?= ../../../

LIB = libdemo.a
LOBJ := libdemo.o

CFLAGS += -Wall -ggdb -O2 -I. -I../include -I$(WRNC)/lib -I$(WRNC)/kernel $(EXTRACFLAGS)
LDLIBS += -L. -ldemo


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
