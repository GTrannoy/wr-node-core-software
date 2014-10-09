#!/bin/bash

./test/list-boot 0
./test/list-boot 1


./test/list-input-test -l 0 1 enable
./test/list-input-test -l 0 1 assign 0:0:1
./test/list-input-test -l 0 1 mode auto
./test/list-input-test -l 0 1 arm

./test/list-input-test -l 1 2 enable
./test/list-input-test -l 1 2 assign 0:0:2
./test/list-input-test -l 1 2 mode auto
./test/list-input-test -l 1 2 arm

./test/list-output-test -l 1 1 assign 0:0:1
./test/list-output-test -l 1 1 delay 0 100u

./test/list-output-test -l 0 3 assign 0:0:1
./test/list-output-test -l 0 3 delay 0 80u 
