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
FIRMWARE_NAME      = manuvr

# Environmental awareness...
###########################################################################
SHELL          = /bin/sh
WHO_I_AM       = $(shell whoami)
WHERE_I_AM     = $(shell pwd)
HOME_DIRECTORY = /home/$(WHO_I_AM)

export CXX     = $(shell which g++)
export CC      = $(shell which gcc)
export SZ      = $(shell which size)
export MAKE    = make

export OUTPUT_PATH = $(WHERE_I_AM)/build/


###########################################################################
# Source files, includes, and linker directives...
###########################################################################
INCLUDES    = -I$(WHERE_I_AM)/.
INCLUDES   += -I$(WHERE_I_AM)/ManuvrOS
INCLUDES   += -I$(WHERE_I_AM)/lib

INCLUDES   += -I$(WHERE_I_AM)/lib/paho.mqtt.embedded-c/
INCLUDES   += -I$(WHERE_I_AM)/lib/mbedtls/include/

# Libraries to link
LIBS = -L$(OUTPUT_PATH) -L$(WHERE_I_AM)/lib -lstdc++ -lm -lmanuvr

# Wrap the include paths into the flags...
CFLAGS  = $(OPTIMIZATION) -Wall $(INCLUDES)
CFLAGS += -fsingle-precision-constant -Wdouble-promotion
CFLAGS += -fno-rtti -fno-exceptions


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
MANUVR_OPTIONS += -D__MANUVR_DEBUG
MANUVR_OPTIONS += -D__MANUVR_PIPE_DEBUG
# Options that build for certain threading models (if any).
#MANUVR_OPTIONS += -D__MANUVR_FREERTOS
MANUVR_OPTIONS += -D__MANUVR_LINUX

#MANUVR_OPTIONS += -DRASPI
#MANUVR_OPTIONS += -DMANUVR_SUPPORT_OSC
MANUVR_OPTIONS += -DMANUVR_SUPPORT_COAP
#MANUVR_OPTIONS += -DMANUVR_OVER_THE_WIRE
#MANUVR_OPTIONS += -DMANUVR_SUPPORT_MQTT
MANUVR_OPTIONS += -DMANUVR_SUPPORT_UDP
#MANUVR_OPTIONS += -DMANUVR_SUPPORT_TCPSOCKET

MANUVR_OPTIONS += -D__MANUVR_MBEDTLS
MBEDTLS_CONFIG_FILE = $(WHERE_I_AM)/mbedTLS_conf.h

LIBS += -lpthread $(OUTPUT_PATH)/extraprotocols.a
LIBS += $(OUTPUT_PATH)/libmbedtls.a
LIBS += $(OUTPUT_PATH)/libmbedx509.a
LIBS += $(OUTPUT_PATH)/libmbedcrypto.a

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
	$(CXX) -static -g -o $(FIRMWARE_NAME) $(SRCS) $(CFLAGS) -std=$(CPP_STANDARD) $(LIBS) -D_GNU_SOURCE -O2
	$(SZ) $(FIRMWARE_NAME)

raspi: clean libs
	export RASPI
	export __MANUVR_LINUX
	make -C ManuvrOS/
	$(CXX) -static -g -o $(FIRMWARE_NAME) $(SRCS) $(CFLAGS) -std=$(CPP_STANDARD) $(LIBS) -DRASPI -D_GNU_SOURCE -O2
	$(SZ) $(FIRMWARE_NAME)

debug: clean libs
	$(CXX) -static -g -o $(FIRMWARE_NAME) $(SRCS) $(CFLAGS) -std=$(CPP_STANDARD) $(LIBS) -D__MANUVR_DEBUG -D_GNU_SOURCE -O0
	$(SZ) $(FIRMWARE_NAME)
# Options configured such that you can then...
# valgrind --tool=callgrind ./manuvr
# gprof2dot --format=callgrind --output=out.dot callgrind.out.16562
# dot  -Tpng out.dot -o graph.png

tests: libs
	export __MANUVR_LINUX
	make -C ManuvrOS/
	$(CXX) -static -g -o dstest tests/TestDataStructures.cpp $(CFLAGS) -std=$(CPP_STANDARD) $(LIBS) -D_GNU_SOURCE -O2
	$(CXX) -static -g -o bptest tests/BufferPipeTest.cpp $(CFLAGS) -std=$(CPP_STANDARD) $(LIBS) -D_GNU_SOURCE -O2

builddir:
	mkdir -p $(OUTPUT_PATH)

libs: builddir
	make -C lib/

clean:
	make clean -C ManuvrOS/
	rm -f *.o *.su *~ testbench $(FIRMWARE_NAME)
	rm -rf $(OUTPUT_PATH)

fullclean: clean
	rm -rf doc/doxygen


docs:
	doxygen Doxyfile

stats:
	find ./ManuvrOS -type f \( -name \*.cpp -o -name \*.h \) -exec wc -l {} +
