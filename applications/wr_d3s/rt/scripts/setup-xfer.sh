#!/bin/bash

#desired frequency (in Hz)
FREQ=352e6

../../../../tools/wrnc-loader -D 0xb01 -i 0 -f ../d3s/rt-d3s.bin
../../../../tools/wrnc-cpu-restart -D 0xb01 -i 0
../../../../tools/wrnc-loader -D 0x501 -i 0 -f ../d3s/rt-d3s.bin
../../../../tools/wrnc-cpu-restart -D 0x501 -i 0

sleep 1

../../tools/wr-d3s-ctl -D 0xb01 -C stream master 143 $FREQ
../../tools/wr-d3s-ctl -D 0x501 -C stream slave 143 0
