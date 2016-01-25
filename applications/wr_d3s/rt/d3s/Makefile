OBJS = rt-d3s.o ad95xx.o adf4002.o ../common/shared_vars.o
OUTPUT = rt-d3s

include ../common/wrnode.mk

load:
	wrnc-loader -D 0x282 -i 0 -f rt-d3s.bin 
	wrnc-cpu-restart -D 0x302 -i 0
	wrnc-loader -D 0x282 -i 0 -f rt-d3s.bin 
	wrnc-cpu-restart -D 0x302 -i 0