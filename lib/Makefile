# If it exists includes Makefile.specific. In this Makefile, you should put
# specific Makefile code that you want to run before this. For example,
# build a particular environment.
-include Makefile.specific

# include parent_common.mk for buildsystem's defines
REPO_PARENT ?= ../..
-include $(REPO_PARENT)/parent_common.mk


TRTL ?= ../

LIBS = libmockturtle.so
LIB = libmockturtle.a
LOBJ := libmockturtle.o
LOBJ += libmockturtle-rt-msg.o

CFLAGS += -Wall -Werror -ggdb -fPIC
CFLAGS += -I. -I$(TRTL)/include $(EXTRACFLAGS)
LDFLAGS = -L. -lmockturtle


modules all: $(LIB) $(LIBS)


%: %.c $(LIB)
	$(CC) $(CFLAGS) $*.c $(LDFLAGS) -o $@

$(LIB): $(LOBJ)
	ar r $@ $^

$(LIBS): $(LIB)
	$(CC) -shared  -o $@ -Wl,--whole-archive,-soname,$@ $^ -Wl,--no-whole-archive

clean:
	rm -f $(LIB) $(LIBS) .depend *.o *~

.depend: Makefile $(wildcard *.c *.h)
	$(CC) $(CFLAGS) -M $(LOBJ:.o=.c) -o $@

install modules_install:

-include .depend
