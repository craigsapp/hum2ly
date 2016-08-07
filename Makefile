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

# targets which don't actually refer to files
.PHONY: external
.SUFFIXES:

SRCDIR    = .
TARGDIR   = .
SRCS      = hum2ly.cpp
TARGET    = hum2ly
INCDIRS   = -Iexternal/humlib/include 
LIBDIRS   = -Lexternal/humlib/lib
HUMLIB    = humlib
COMPILER  = g++
PREFLAGS  = -O3 -Wall $(INCDIRS)
POSTFLAGS = $(LIBDIRS) -l$(HUMLIB)

# Location of guild header and lib:
INCDIRS  += -I/usr/local/include
LIBDIRS  += -L/usr/local/lib

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

clean:
	(cd external && $(MAKE) clean)
	-rm -f hum2ly



