#!/bin/bash

make load
#/wr-d3s-ctl -D 0x1c2 -C stream master 143 184.76e6
#./wr-d3s-ctl -D 0x1c2 -C stream master 143 184.62e6
#./wr-d3s-ctl -D 0x1c2 -C stream master 143 184.71e6
./wr-d3s-ctl -D 0xb01 -C stream master 143 $1
#sleep 2
./wr-d3s-ctl -D 0x501 -C stream slave 143 0