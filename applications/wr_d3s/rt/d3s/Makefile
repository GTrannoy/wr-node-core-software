OBJS = rt-d3s.o ad95xx.o adf4002.o
OUTPUT = rt-d3s

include ../common/wrnode.mk

load:
	../../../../tools/wrnc-loader -D 0x302 -i 0 -f rt-d3s.bin 
	../../../../tools/wrnc-cpu-restart -D 0x302 -i 0
	../../../../tools/wrnc-loader -D 0x1c2 -i 0 -f rt-d3s.bin 
	../../../../tools/wrnc-cpu-restart -D 0x1c2 -i 0