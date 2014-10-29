
.PHONY: all clean modules install modules_install clean_all
.PHONY: gitmodules prereq prereq_install prereq_install_warn prereq_clean

DIRS = lib tools

WRTD=$(shell pwd)
export WRTD
export WRNC

all clean modules install modules_install: gitmodules
	for d in $(DIRS); do $(MAKE) -C $$d $@ || exit 1; done
