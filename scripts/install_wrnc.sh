#!/bin/bash

WRNC_SW=/user/dcobas/cage/dds/wr-node-core-sw
WRNC_SW_KERNEL=$WRNC_SW/kernel
FMC_SW=$WRNC_SW/fmc-bus
FMC_SW_KERNEL=$FMC_SW/kernel
SVEC_SW=$WRNC_SW/svec-sw
SVEC_SW_KERNEL=$SVEC_SW/kernel

# remove modules if present
grep -q '^wr_node_core ' /proc/modules && /sbin/rmmod wr-node-core
grep -q '^svec ' /proc/modules && /sbin/rmmod svec
grep -q '^fmc ' /proc/modules && /sbin/rmmod fmc

# install our own fmc-bus
grep -q '^fmc-bus ' /proc/modules ||
	/sbin/insmod $FMC_SW_KERNEL/fmc.ko

# install standard SVEC driver, then load demo app firmware
grep -q '^svec ' /proc/modules ||
    sh $(dirname "$0")/install_svec.sh -d $SVEC_SW_KERNEL/svec

# loading firmware to the most recently created SVEC
SVEC_DEV=$(ls -rt /dev/svec.* | tail -n 1 )
dd if=/lib/firmware/fmc/test/svec-wr-node-demo-20150527.bin of=$SVEC_DEV obs=10000000

# install node core driver
grep -q '^wr_node_core ' /proc/modules || 
	  /sbin/insmod $WRNC_SW_KERNEL/wr-node-core.ko

# FIXME: do we need this for svec-wrc-loader?
grep -q '/sys/kernel/debug' /proc/mounts ||
	mount -t debugfs none /sys/kernel/debug/

# /usr/local/bin/svec-wrc-loader -a /lib/firmware/fmc/wr-core.bin
