
LINUX ?= /lib/modules/$(shell uname -r)/build
FMC_BUS ?= $(src)/../fmc-bus

KBUILD_EXTRA_SYMBOLS := \
	$(ZIO)/Module.symvers \
	$(FMC_BUS)/kernel/Module.symvers

GIT_VERSION = $(shell cd $(src); git describe --always --dirty --long --tags)

# For this CSM_VERSION, please see ohwr.org/csm documentation
ifdef CONFIG_CSM_VERSION
  ccflags-y += -D"CERN_SUPER_MODULE=MODULE_VERSION(\"$(CONFIG_CSM_VERSION)\")"
else
  ccflags-y += -DCERN_SUPER_MODULE=""
endif

# The library includes <sdb.h>, so point -I directtly there
# include our header before to avoid conflicts with the kernel
LINUXINCLUDE := -I$(FMC_BUS)/kernel/include $(LINUXINCLUDE)

ccflags-y += -DGIT_VERSION=\"$(GIT_VERSION)\" \
	-I$(src) \
	-I$(src)/../include


ccflags-$(CONFIG_TRTL_DEBUG) += -DDEBUG


subdirs-ccflags-y = $(ccflags-y)

obj-m := mockturtle.o
mockturtle-y :=  mockturtle-core.o
mockturtle-y +=  mockturtle-cpu.o
mockturtle-y +=  mockturtle-hmq.o
mockturtle-y +=  mockturtle-dbg.o

all modules:
	$(MAKE) -C $(LINUX) M=$(shell /bin/pwd) modules

install modules_install:
	$(MAKE) -C $(LINUX) M=$(shell /bin/pwd) modules_install

# be able to run the "clean" rule even if $(LINUX) is not valid
clean:
	rm -rf *.o *~  .*.cmd *.ko *.mod.c .tmp_versions Module.symvers \
		Module.markers modules.order

coccicheck:
	$(MAKE) -C $(LINUX) M=$(shell /bin/pwd) coccicheck
