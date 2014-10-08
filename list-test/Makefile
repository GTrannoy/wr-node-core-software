CC=/acc/sys/L865/cdk/gcc -m32
CXX=/acc/sys/L865/cdk/g++ -m32
CFLAGS = -I. -g
CXXFLAGS = -I. -g
OBJS = wrn-lib.o fmc-lib.o list-lib.o

all:	lib list-boot list-input-test test-rx test-hash  list-output-test list-dbg

lib:	$(OBJS)
	ar rc libwrn.a $(OBJS)

list-boot:	list-boot.o $(OBJS)
	${CXX} -o list-boot list-boot.o $(OBJS) -L. -lwrn -lpthread

list-dbg:	list-dbg.o $(OBJS)
	${CXX} -o list-dbg list-dbg.o $(OBJS) -L. -lwrn -lpthread

test-rx:	test-rx.o $(OBJS)
	${CXX} -o test-rx test-rx.o $(OBJS) -L. -lwrn -lpthread

test-hash:	test-hash.o $(OBJS)
	${CXX} -o test-hash test-hash.o $(OBJS) -L. -lwrn -lpthread

list-input-test: list-input-test.o $(OBJS)
	${CXX} -o list-input-test list-input-test.o $(OBJS) -L. -lwrn -lpthread

list-output-test: list-output-test.o $(OBJS)
	${CXX} -o list-output-test list-output-test.o $(OBJS) -L. -lwrn -lpthread

clean:
	rm -f $(OBJS) libwrn.a test-rx test-hash list-boot
	
%.o:	%.cpp
	${CXX} -o $@ -c $^ $(CXXFLAGS)