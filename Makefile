###########################################################################
# Makefile for ManuvrOS.
# Author: J. Ian Lindsay
# Date:   2014.08.13
#
###########################################################################

# Used for internal functionality macros. Feel free to rename. Need to
#   replace this with an autoconf script which I haven't yet learned how to
#   write.

OPTIMIZATION       = -O0 -g
C_STANDARD         = gnu99
CPP_STANDARD       = gnu++11


# Environmental awareness...
###########################################################################
SHELL          = /bin/sh
WHO_I_AM       = $(shell whoami)
WHERE_I_AM     = $(shell pwd)
HOME_DIRECTORY = /home/$(WHO_I_AM)

export CPP         = $(shell which g++)
export CC          = $(shell which gcc)

export OUTPUT_PATH = $(WHERE_I_AM)/build/


###########################################################################
# Source files, includes, and linker directives...
###########################################################################
INCLUDES    = -I$(WHERE_I_AM)/.
INCLUDES   += -I$(WHERE_I_AM)/ManuvrOS
INCLUDES   += -I$(WHERE_I_AM)/lib


# Libraries to link
LIBS = -L$(OUTPUT_PATH) -L$(WHERE_I_AM)/lib -lstdc++ -lm -lmanuvr

# Wrap the include paths into the flags...
CFLAGS  = $(OPTIMIZATION) -Wall $(INCLUDES)
CFLAGS += -fsingle-precision-constant -Wdouble-promotion


###########################################################################
# Are we on a 64-bit system? If so, we'll need to specify
#   that we want a 32-bit build...
#
# Eventually, the Argument class and the aparatus surrounding it will need
#   to be validated on a 64-bit build, but for now, we don't want to have
#   to worry about it.
#
# Thanks, estabroo...
# http://www.linuxquestions.org/questions/programming-9/how-can-make-makefile-detect-64-bit-os-679513/
###########################################################################
LBITS = $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
  CFLAGS += -m32
else
  TARGET_WIDTH =
endif


###########################################################################
# Source file definitions...
###########################################################################

CPP_SRCS  = main.cpp

SRCS   = $(CPP_SRCS)

# TODO: I badly need to learn to write autoconf scripts....
#   I've at least tried to modularize to make the invariable transition less-painful...
MANUVR_OPTIONS  = -DMANUVR_SUPPORT_SERIAL
#MANUVR_OPTIONS += -DMANUVR_SUPPORT_TCPSOCKET
MANUVR_OPTIONS += -D__MANUVR_DEBUG

# Options that build for certain threading models (if any).
#MANUVR_OPTIONS += -D__MANUVR_FREERTOS
MANUVR_OPTIONS += -D__MANUVR_LINUX
MANUVR_OPTIONS += -DMANUVR_SUPPORT_COAP

LIBS += -lpthread 

CFLAGS += $(MANUVR_OPTIONS)


export CFLAGS
export CPP_FLAGS = $(CFLAGS)


###########################################################################
# Rules for building the firmware follow...
#
# 'make raspi' will build a sample firmware for the original Raspberry Pi.
#    The idea is to be able to quickly iterate on a design idea with the
#    aid of valgrind and gdb.
###########################################################################

.PHONY: all


all: clean libs
	export __MANUVR_LINUX
	make -C ManuvrOS/
	$(CPP) -static -g -o manuvr $(SRCS) $(CFLAGS) -std=$(CPP_STANDARD) $(LIBS) -D_GNU_SOURCE -O2

raspi: clean libs
	export RASPI
	export __MANUVR_LINUX
	make -C ManuvrOS/
	$(CPP) -static -g -o manuvr $(SRCS) $(CFLAGS) -std=$(CPP_STANDARD) $(LIBS) -DRASPI -D_GNU_SOURCE -O2

debug:
	$(CPP) -static -g -o manuvr main.cpp $(SRCS) $(GENERIC_DRIVERS) $(CFLAGS) -std=$(CPP_STANDARD) $(LIBS) -D__MANUVR_DEBUG -D_GNU_SOURCE -O0 -fstack-usage
# Options configured such that you can then...
# valgrind --tool=callgrind ./manuvr
# gprof2dot --format=callgrind --output=out.dot callgrind.out.16562
# dot  -Tpng out.dot -o graph.png

manuvrtests:
	mkdir $(OUTPUT_PATH)
	$(CPP) -o $(OUTPUT_PATH)/dstest tests/TestDataStructures.cpp DataStructures/*.cpp -I. -lstdc++ -lc -lm

builddir:
	mkdir -p $(OUTPUT_PATH)

libs: builddir
#	make -C lib/

clean:
	make clean -C ManuvrOS/
	rm -f *.o *.su *~ testbench manuvr
	rm -rf $(OUTPUT_PATH)

fullclean: clean
	rm -rf doc/doxygen


docs:
	doxygen Doxyfile

stats:
	find ./ManuvrOS ./DataStructures ./StringBuilder -type f \( -name \*.cpp -o -name \*.h \) -exec wc -l {} +
