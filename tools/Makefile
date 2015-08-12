# If it exists includes Makefile.specific. In this Makefile, you should put
# specific Makefile code that you want to run before this. For example,
# build a particular environment.
-include Makefile.specific

DESTDIR ?= /usr/local
WRNC ?= ../

CFLAGS += -Wall -ggdb -I$(WRNC)/lib
CFLAGS += -I$(WRNC)/include
CFLAGS += $(EXTRACFLAGS)
LDLIBS += -L$(WRNC)/lib -lwrnc -lpthread
PROGS := wrnc-count
PROGS += lswrnc
PROGS += wrnc-loader
PROGS += wrnc-messages
PROGS += wrnc-cpu-restart
PROGS += wrnc-smem

all: $(PROGS)

install:
	install -d $(DESTDIR)/bin
	install -D $(PROGS) $(DESTDIR)/bin

%: %.c $(WRNC)/libwrnc.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

# make nothing for modules_install, but avoid errors
modules_install:

clean:
	rm -f $(PROGS) *.o *~

.PHONY: all, clean
