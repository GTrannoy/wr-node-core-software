# If it exists includes Makefile.specific. In this Makefile, you should put
# specific Makefile code that you want to run before this. For example,
# build a particular environment.
-include Makefile.specific

DESTDIR ?= /usr/local
TRTL ?= ../

CFLAGS += -Wall -Werror -ggdb -I$(TRTL)/lib
CFLAGS += -I$(TRTL)/include
CFLAGS += $(EXTRACFLAGS)
LDLIBS += -Wl,-Bstatic -L$(TRTL)/lib -lmockturtle
LDLIBS += -Wl,-Bdynamic -lpthread
PROGS := mockturtle-count
PROGS += lsmockturtle
PROGS += mockturtle-loader
PROGS += mockturtle-messages
PROGS += mockturtle-cpu-restart
PROGS += mockturtle-smem

all: $(PROGS)

install:
	install -d $(DESTDIR)/bin
	install -D $(PROGS) $(DESTDIR)/bin

%: %.c $(TRTL)/libmockturtle.a
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

# make nothing for modules_install, but avoid errors
modules_install:

clean:
	rm -f $(PROGS) *.o *~

.PHONY: all, clean
