#!/bin/bash

SPEC_MASTER_DEV=/dev/spec-0b00
SPEC_SLAVE_DEV=/dev/spec-0500

SPEC_MASTER_BUS=0xb
SPEC_SLAVE_BUS=0x5

rmmod wr-node-core
rmmod spec
rmmod fmc

insmod ../../../../spec-sw/fmc-bus/kernel/fmc.ko
insmod ../../../../spec-sw/kernel/spec.ko

#dd if=/home/user/spec-wr-node-demo-20150727.bin of=/dev/spec-0b00 bs=5000000 count=1
dd if=spec_top.bin of=$SPEC_MASTER_DEV bs=5000000 count=1
dd if=spec_top.bin of=$SPEC_SLAVE_DEV bs=5000000 count=1
../../../../spec-sw/tools/spec-cl -b $SPEC_MASTER_BUS -c 0x40000 wr-core.bin
../../../../spec-sw/tools/spec-cl -b $SPEC_SLAVE_BUS -c 0x40000 wr-core.bin

sleep 1
insmod ../../../../kernel/wr-node-core.ko

../../../../tools/wrnc-messages -Q