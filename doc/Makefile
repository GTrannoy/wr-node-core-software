GIT_VERSION = $(shell cd $(src); git describe --dirty --long --tags)

O ?= doxy-wrnc

# Build the documentation with or without the internals details
ifdef SHOW_INTERNALS
	EXCLUDE_FILES = ""
	BRIEF = "API Documentation - With Internals"
else
	EXCLUDE_FILES = "../lib/libwrnc-internal.h"
	BRIEF = "API Documentation"
endif

all: doxygen

doxygen:
	GIT_VERSION=$(GIT_VERSION) EXCLUDE_FILES=$(EXCLUDE_FILES) BRIEF=$(BRIEF) \
	OUTPUT=$(O) doxygen ./doxygen-wrnc-config

clean:
	rm -rf $(O)
# Latex stuff
	rm -f *.dvi *.aux *.out *.log *.backup
