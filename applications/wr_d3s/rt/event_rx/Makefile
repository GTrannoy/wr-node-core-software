OBJS = rt-event-rx.o ../common/shared_vars.o
OUTPUT = rt-event-rx

include ../common/wrnode.mk

load:
	wrnc-loader -D 0x302 -i 1 -f rt-event-rx.bin 
	wrnc-cpu-restart -D 0x302 -i 1
	wrnc-loader -D 0x102 -i 1 -f rt-event-rx.bin 
	wrnc-cpu-restart -D 0x102 -i 1
