CC=/acc/sys/L865/cdk/gcc -m32
CXX=/acc/sys/L865/cdk/g++ -m32
CFLAGS = -I. -g -I../include
CXXFLAGS = -I. -g  -I../include
OBJS = wrn-lib.o fmc-lib.o list-lib.o

all:	lib

lib:	$(OBJS)
	ar rc liblist.a $(OBJS)

clean:
	rm -f $(OBJS) liblist.a
	
%.o:	%.cpp
	${CXX} -o $@ -c $^ $(CXXFLAGS)