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

all: doxygen latex

doxygen:
	GIT_VERSION=$(GIT_VERSION) EXCLUDE_FILES=$(EXCLUDE_FILES) BRIEF=$(BRIEF) \
	OUTPUT=$(O) doxygen ./doxygen-wrnc-config

latex:
	latex wrnc.tex
	latex wrnc.tex

	@which dvipdf > /dev/null;					\
	if [ $$? -eq  0 ]; then						\
		dvipdf wrnc.dvi;					\
	else								\
		@which dvipdf > /dev/null;				\
		if [ $$? -eq 0 ]; then					\
			dvipdfm wrnc.dvi;				\
		else							\
			echo "Cannot convert to pdf. dvipdf/dvipdfm missing"; \
		fi 							\
	fi 								\

clean:
	rm -rf $(O)
# Latex stuff
	rm -f *.dvi *.aux *.out *.log *.backup *.pdf
