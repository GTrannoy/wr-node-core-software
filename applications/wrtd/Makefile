
.PHONY: all clean modules install modules_install clean_all
.PHONY: gitmodules prereq prereq_install prereq_install_warn prereq_clean

DIRS = fmc-tdc-sw fine-delay-sw lib tools

all clean modules install modules_install: gitmodules
	for d in $(DIRS); do $(MAKE) -C $$d $@ || exit 1; done
