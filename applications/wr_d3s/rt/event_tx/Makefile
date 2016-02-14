OBJS = rt-event-tx.o ../common/shared_vars.o
OUTPUT = rt-event-tx

include ../common/wrnode.mk

load:
	wrnc-loader -D 0x1c2 -i 1 -f rt-event-tx.bin 
	wrnc-cpu-restart -D 0x1c2 -i 1
