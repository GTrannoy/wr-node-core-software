#!/usr/bin/python
def calc_tune(freq,sample_rate):
    tune = int( float(1<<42) * (freq / sample_rate) * 8.0 );
    print("HI=0x%x" % ((tune >> 32) & 0xffffffff))
    print("LO=0x%x" % ((tune >> 0) & 0xffffffff))

calc_tune(10e3, 500e6) 


