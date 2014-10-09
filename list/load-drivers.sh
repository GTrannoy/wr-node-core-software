#!/bin/bash

/sbin/rmmod fmc_adc_100m14b
/sbin/rmmod fmc-fine-delay
/sbin/rmmod fmc-tdc
/sbin/rmmod svec
/sbin/rmmod zio
/sbin/rmmod fmc

/sbin/rmmod fmc_adc_100m14b
/sbin/rmmod fmc-fine-delay
/sbin/rmmod fmc-tdc
/sbin/rmmod svec
/sbin/rmmod zio
/sbin/rmmod fmc

/sbin/insmod /user/twlostow/repos/fmc-bus/kernel/fmc.ko
/sbin/insmod /user/twlostow/repos/zio/zio.ko

/sbin/insmod /user/twlostow/repos/fmc-tdc-sw/kernel/fmc-tdc.ko show_sdb=1 verbose=1 gateware=fmc/test/svec-list-tdc-fd-y.bin
/sbin/insmod /user/twlostow/repos/fine-delay-sw/kernel/fmc-fine-delay.ko show_sdb=1 verbose=1 gateware=fmc/test/svec-list-tdc-fd-y.bin

/sbin/insmod /user/twlostow/repos/svec-sw/kernel/svec.ko slot=5,8,12 lun=0,1,2 verbose=1 #fw_name=fmc/test/svec-list-test.bin verbose=1
/user/twlostow/repos/svec-sw/tools/svec-config -u 0 -b 0xa00000 -w 0x100000 -v 0x86 -f 1
/user/twlostow/repos/svec-sw/tools/svec-config -u 1 -b 0xb00000 -w 0x100000 -v 0x88 -f 1
/user/twlostow/repos/svec-sw/tools/svec-config -u 2 -b 0xc00000 -w 0x100000 -v 0x90 -f 1

#/sbin/i

#./svec-fwloader /lib/firmware/fmc/test/svec-list-test.bin
./svec-wrc-loader -a /lib/firmware/fmc/wr-core-current.bin

sleep 2

/usr/local/bin/fmc-fdelay-pulse -i 2 -o 1 -T 89u+3n
/usr/local/bin/fmc-fdelay-pulse -i 2 -o 2 -T 89u+4n

/usr/local/bin/fmc-fdelay-board-time -i 0 wr
/usr/local/bin/fmc-fdelay-board-time -i 1 wr
../../fmc-tdc-sw/tools/fmc-tdc-time 0140 wr
../../fmc-tdc-sw/tools/fmc-tdc-time 0200 wr
/usr/local/bin/fmc-fdelay-pulse -i 0 -o 1 -p
/usr/local/bin/fmc-fdelay-pulse -i 1 -o 1 -p
