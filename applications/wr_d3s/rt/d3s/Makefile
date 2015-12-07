OBJS = ad95xx.o adf4002.o rt-d3s.o #main.o master_loop.o wr.o
OUTPUT = rt-d3s

include ../common/wrnode.mk

load:
	wrnc-loader -D 0xb01 -i 0 -f rt-d3s.bin 
	wrnc-cpu-restart -D 0xb01 -i 0
	wrnc-loader -D 0x501 -i 0 -f rt-d3s.bin 
	wrnc-cpu-restart -D 0x501 -i 0