
.PHONY: all clean modules install modules_install clean_all
.PHONY: gitmodules prereq prereq_install prereq_install_warn prereq_clean

DIRS = wrtd

all clean modules install modules_install doc: gitmodules
	for d in $(DIRS); do $(MAKE) -C $$d $@ || exit 1; done
