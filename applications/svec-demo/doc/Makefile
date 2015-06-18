GIT_VERSION = $(shell cd $(src); git describe --dirty --long --tags)

O ?= doxy-demo

EXCLUDE_FILES = ""
BRIEF = "API Documentation - With Internals"


all: doxygen

doxygen:
	GIT_VERSION=$(GIT_VERSION) EXCLUDE_FILES=$(EXCLUDE_FILES) BRIEF=$(BRIEF) \
	OUTPUT=$(O) doxygen ./doxygen-demo-config

clean:
	rm -rf $(O)
# Latex stuff
	rm -f *.dvi *.aux *.out *.log *.backup
