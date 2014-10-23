# If it exists includes Makefile.specific. In this Makefile, you should put
# specific Makefile code that you want to run before this. For example,
# build a particular environment.
-include Makefile.specific

DESTDIR ?= /usr/local

CFLAGS += -Wall -g -ggdb -I$(LIBWRNC) $(EXTRACFLAGS)
LDFLAGS += -L$(LIBWRNC) -lwrnc

PROGS := wrnc-count
PROGS += lswrnc
PROGS += wrnc-loader

all: $(PROGS)

install:
	install -d $(DESTDIR)/bin
	install -D $(PROGS) $(DESTDIR)/bin

%: %.c $(LIBWRNC)/libwrnc.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# make nothing for modules_install, but avoid errors
modules_install:

clean:
	rm -f $(PROGS) *.o *~

.PHONY: all, clean
