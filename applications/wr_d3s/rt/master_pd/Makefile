OBJS = main.o master_loop.o ../common/ad95xx.o ../common/adf4002.o ../common/shared_vars.o ../common/wr.o
OUTPUT = rt-d3s

include ../common/wrnode.mk

load:
	wrnc-loader -D 0x182 -i 0 -f rt-d3s.bin 
	wrnc-cpu-restart -D 0x182 -i 0
#	wrnc-loader -D 0x282 -i 0 -f rt-d3s.bin 
#	wrnc-cpu-restart -D 0x282 -i 0