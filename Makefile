
.PHONY: all clean modules install modules_install clean_all
.PHONY: gitmodules prereq prereq_install prereq_install_warn prereq_clean

DIRS = kernel lib tools applications

all clean modules install modules_install: gitmodules
	@if echo $@ | grep -q install; then $(MAKE) prereq_install_warn; fi
	for d in $(DIRS); do $(MAKE) -C $$d $@ || exit 1; done
doc:
	$(MAKE) -C doc clean
	$(MAKE) -C doc all

all modules: prereq

clean_all: clean prereq_clean

#### The following targets are used to manage prerequisite repositories
#### only for THIS repository
gitmodules:
	@test -d fmc-bus/doc || echo "Checking out submodules"
	@test -d fmc-bus/doc || git submodule update --init

# The user can override, using environment variables, the place for our
# three submodules. Note that svec-sw is not built, as it uses cern-internal
# pathnames, and thus won't build elsewhere. We have it as a submodule to
# find needed headers to build kernel code.
#
# Use the absolute path so it can be used by submodule
CURDIR ?= $(shell pwd)
FMC_BUS ?= $(CURDIR)/fmc-bus
export FMC_BUS
SVEC_SW ?= $(CURDIR)/svec-sw
export SVEC_SW
SUBMOD = $(FMC_BUS) $(SVEC_SW)
LIBWRNC= $(CURDIR)/lib
export LIBWRNC
WRNC=$(CURDIR)
export WRNC

prereq:
	for d in $(SUBMOD); do $(MAKE) -C $$d || exit 1; done

prereq_install_warn:
	@test -f .prereq_installed || \
		echo -e "\n\n\tWARNING: Consider \"make prereq_install\"\n"

prereq_install:
	for d in $(SUBMOD); do $(MAKE) -C $$d modules_install || exit 1; done
	touch .prereq_installed

prereq_clean:
	for d in $(SUBMOD); do $(MAKE) -C $$d clean || exit 1; done
