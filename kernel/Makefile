
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

ccflags-y += -DGIT_VERSION=\"$(GIT_VERSION)\" \
	-I$(FMC_BUS)/kernel/include \
	-I$(src)

ccflags-$(CONFIG_WRNC_DEBUG) += -DDEBUG


subdirs-ccflags-y = $(ccflags-y)

obj-m := wr-node-core.o
wr-node-core-y :=  wrnc-core.o
wr-node-core-y +=  wrnc-cpu.o
wr-node-core-y +=  wrnc-hmq.o

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
