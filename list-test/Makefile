CC=/acc/sys/L865/cdk/gcc -m32
CXX=/acc/sys/L865/cdk/g++ -m32
CFLAGS = -I. -g
CXXFLAGS = -I. -g
OBJS = wrn-lib.o fmc-lib.o list-lib.o

all:	lib list-boot test-rx

lib:	$(OBJS)
	ar rc libwrn.a $(OBJS)

list-boot:	list-boot.o $(OBJS)
	${CXX} -o list-boot list-boot.o $(OBJS) -L. -lwrn -lpthread

test-rx:	test-rx.o $(OBJS)
	${CXX} -o test-rx test-rx.o $(OBJS) -L. -lwrn -lpthread

clean:
	rm -f $(OBJS) libwrn.a test-rx
	
%.o:	%.cpp
	${CXX} -o $@ -c $^ $(CXXFLAGS)