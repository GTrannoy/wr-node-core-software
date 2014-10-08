CC=/acc/sys/L865/cdk/gcc -m32
CXX=/acc/sys/L865/cdk/g++ -m32
CFLAGS = -I. -g -I../lib -I../include
CXXFLAGS = -I. -g
LDFLAGS = -L../lib -llist -lpthread

all:	list-boot list-input-test list-output-test list-dbg

	
list-boot:	list-boot.o $(OBJS)
	${CXX} -o list-boot list-boot.o $(OBJS) -L. $(LDFLAGS)

list-dbg:	list-dbg.o $(OBJS)
	${CXX} -o list-dbg list-dbg.o $(OBJS) -L. $(LDFLAGS)

list-input-test: list-input-test.o $(OBJS)
	${CXX} -o list-input-test list-input-test.o $(OBJS) -L. $(LDFLAGS)

list-output-test: list-output-test.o $(OBJS)
	${CXX} -o list-output-test list-output-test.o $(OBJS) -L. $(LDFLAGS)

clean:
	rm -f list-boot list-input-test list-output-test list-dbg
