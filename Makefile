## hum2ly GNU makefile 
##
## Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
## Creation Date: Sat Aug  6 10:57:54 CEST 2016
## Last Modified: Sat Aug  6 15:17:29 CEST 2016
## Filename:      hum2ly/Makefile
##
## Description: This Makefile can create the hum2ly program.
##
## To run this makefile, type (without quotes) "make" (or
## "gmake library" on FreeBSD computers).
##

# targets which don't actually refer to files:
.PHONY: external tests
.SUFFIXES:

SRCDIR    = .
INCDIR    = .
TARGDIR   = .
SRCS      = hum2ly.cpp main.cpp
TARGET    = hum2ly
INCDIRS   = -I$(INCDIR) -Iexternal/humlib/include 
LIBDIRS   = -Lexternal/humlib/lib
HUMLIB    = humlib
COMPILER  = g++
PREFLAGS  = -O3 -Wall $(INCDIRS)
PUGIXML   = pugixml
POSTFLAGS = $(LIBDIRS) -l$(HUMLIB) -l$(PUGIXML)

# Humlib needs C++11:
PREFLAGS += -std=c++11

all: external targetdir
	$(COMPILER) $(PREFLAGS) -o $(TARGDIR)/$(TARGET) $(SRCS) $(POSTFLAGS) \
		&& strip $(TARGDIR)/$(TARGET)


targetdir:
	mkdir -p $(TARGDIR)
	

external:
ifeq ($(wildcard external/humlib/lib/libhumlib.a),)
	(cd external && $(MAKE))
endif


tests:
	for i in tests/*.krn; do ./hum2ly $$i > tests/`basename $$i .krn`.ly; done
	(cd tests && for i in *.ly; do lilypond $$i; done)


clean:
	(cd external && $(MAKE) clean)
	-rm -f hum2ly


