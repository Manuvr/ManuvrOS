###########################################################################
# Makefile for ManuvrOS.
# Author: J. Ian Lindsay
# Date:   2014.08.13
#
# This is a Makefile for an example project. New projects should either
#   use this Makefile as a starting-point for their own, or (if using a
#   toolchain other than automake) recreate it's logic as appropriate.
###########################################################################

# Used for internal functionality macros. Feel free to rename. Need to
#   replace this with an autoconf script which I haven't yet learned how to
#   write.
OPTIMIZATION       = -O2
C_STANDARD         = gnu99
CPP_STANDARD       = gnu++11
FIRMWARE_NAME      = manuvr
CFLAGS             = -Wall
MANUVR_OPTIONS     =

###########################################################################
# Environmental awareness...
###########################################################################
SHELL          = /bin/sh
WHERE_I_AM     = $(shell pwd)

export CXX     = $(shell which g++)
export CC      = $(shell which gcc)
export SZ      = $(shell which size)
export MAKE    = $(shell which make)

export OUTPUT_PATH = $(WHERE_I_AM)/build/


###########################################################################
# Source files, includes, and linker directives...
###########################################################################
INCLUDES    = -I$(WHERE_I_AM)/.
INCLUDES   += -I$(WHERE_I_AM)/ManuvrOS
INCLUDES   += -I$(WHERE_I_AM)/lib

INCLUDES   += -I$(WHERE_I_AM)/lib/paho.mqtt.embedded-c/
INCLUDES   += -I$(WHERE_I_AM)/lib/mbedtls/include/
INCLUDES   += -I$(WHERE_I_AM)/lib/iotivity
INCLUDES   += -I$(WHERE_I_AM)/lib/iotivity/include

# Libraries to link
# We should be gradually phasing out C++ standard library on linux builds.
# TODO: Advance this goal.
LIBS = -L$(OUTPUT_PATH) -L$(WHERE_I_AM)/lib -lstdc++ -lm -lmanuvr

# Wrap the include paths into the flags...
CFLAGS += $(OPTIMIZATION) $(INCLUDES)
CFLAGS += -fsingle-precision-constant -Wdouble-promotion


###########################################################################
# Are we on a 64-bit system? If so, we'll need to specify
#   that we want a 32-bit build...
#
# Eventually, the Argument class and the aparatus surrounding it will need
#   to be validated on a 64-bit build, but for now, we don't want to have
#   to worry about it.
# This is probably just a matter of having a platform-support macro to cast
#   type-sizes to integers, but will also have implications for
#   memory-management surrounding (say) 64-bit integers.
#
# Thanks, estabroo...
# http://www.linuxquestions.org/questions/programming-9/how-can-make-makefile-detect-64-bit-os-679513/
###########################################################################
LBITS = $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
	# This is no longer required on 64-bit platforms. But it is being retained in
	#   case 32-bit problems need to be debugged.
  CFLAGS += -m32
endif



###########################################################################
# Source file and options definitions...
# TODO: I badly need to learn to write autoconf scripts....
#   I've at least tried to modularize to make the invariable transition less-painful.
###########################################################################
# This project has a single source file.
CPP_SRCS  = main.cpp

# Supported transports...
MANUVR_OPTIONS += -DMANUVR_STDIO
MANUVR_OPTIONS += -DMANUVR_SUPPORT_SERIAL
MANUVR_OPTIONS += -DMANUVR_SUPPORT_UDP
MANUVR_OPTIONS += -DMANUVR_SUPPORT_TCPSOCKET

# Options that build for certain threading models (if any).
#MANUVR_OPTIONS += -D__MANUVR_FREERTOS
MANUVR_OPTIONS += -D__MANUVR_LINUX
MANUVR_OPTIONS += -DMANUVR_OPENINTERCONNECT

# Wire and session protocols...
#MANUVR_OPTIONS += -DMANUVR_SUPPORT_OSC
MANUVR_OPTIONS += -DMANUVR_OVER_THE_WIRE
#MANUVR_OPTIONS += -DMANUVR_SUPPORT_MQTT
MANUVR_OPTIONS += -DMANUVR_SUPPORT_COAP
MANUVR_OPTIONS += -D__MANUVR_CONSOLE_SUPPORT
MANUVR_OPTIONS += -DMANUVR_GPS_PIPE

# Options for various platform features.
MANUVR_OPTIONS += -DMANUVR_STORAGE
MANUVR_OPTIONS += -DMANUVR_CBOR
#MANUVR_OPTIONS += -DMANUVR_JSON

# Framework selections, if any are desired.
MANUVR_OPTIONS += -DMANUVR_OPENINTERCONNECT

# Since we are building on linux, we will have threading support via
# pthreads.
LIBS += -lpthread $(OUTPUT_PATH)/extraprotocols.a

# Options for various security features.
ifeq ($(SECURE),1)
# mbedTLS will require this in order to use our chosen options.
MBEDTLS_CONFIG_FILE = $(WHERE_I_AM)/mbedTLS_conf.h
LIBS += $(OUTPUT_PATH)/libmbedtls.a
LIBS += $(OUTPUT_PATH)/libmbedx509.a
LIBS += $(OUTPUT_PATH)/libmbedcrypto.a
MANUVR_OPTIONS += -D__MANUVR_MBEDTLS
# The remaining lines are to prod header files in libraries.
MANUVR_OPTIONS += -DOC_SECURITY -DOC_CLIENT
export SECURE=1
endif

# Support for specific SBC hardware on linux.
ifeq ($(BOARD),RASPI)
MANUVR_OPTIONS += -DRASPI
export MANUVR_BOARD = RASPI
endif

# Debugging options...
ifeq ($(DEBUG),1)
MANUVR_OPTIONS += -D__MANUVR_DEBUG
MANUVR_OPTIONS += -D__MANUVR_PIPE_DEBUG
OPTIMIZATION    = -O0 -g
# Options configured such that you can then...
# valgrind --tool=callgrind ./manuvr
# gprof2dot --format=callgrind --output=out.dot callgrind.out.16562
# dot  -Tpng out.dot -o graph.png
endif
MANUVR_OPTIONS += -D__MANUVR_EVENT_PROFILER

###########################################################################
# Rules for building the firmware follow...
#
# 'make raspi' will build a sample firmware for the original Raspberry Pi.
#    The idea is to be able to quickly iterate on a design idea with the
#    aid of valgrind and gdb.
###########################################################################
# Merge our choices and export them to the downstream Makefiles...
CFLAGS += $(MANUVR_OPTIONS) $(OPTIMIZATION)

export MANUVR_PLATFORM = LINUX
export CFLAGS
export CPP_FLAGS    = $(CFLAGS) -fno-rtti -fno-exceptions


.PHONY: all


all: clean libs
	$(MAKE) -C ManuvrOS/
	$(CXX) -static -o $(FIRMWARE_NAME) $(CPP_SRCS) $(CFLAGS) -std=$(CPP_STANDARD) $(LIBS) -D_GNU_SOURCE
	$(SZ) $(FIRMWARE_NAME)

tests: libs
	$(CXX) -static -o dstest tests/TestDataStructures.cpp $(CPP_FLAGS) -std=$(CPP_STANDARD) $(LIBS) -D_GNU_SOURCE

examples: libs
	$(CXX) -static -o barebones examples/main_template.cpp $(CPP_FLAGS) -std=$(CPP_STANDARD) $(LIBS) -D_GNU_SOURCE
	$(CXX) -static -o gpstest   examples/tcp-gps.cpp $(CPP_FLAGS) -std=$(CPP_STANDARD) $(LIBS) -D_GNU_SOURCE

builddir:
	mkdir -p $(OUTPUT_PATH)

libs: builddir
	$(MAKE) -C lib/
	$(MAKE) -C ManuvrOS/

clean:
	$(MAKE) clean -C ManuvrOS/
	rm -f *.o *.su *~ testbench $(FIRMWARE_NAME)
	rm -rf $(OUTPUT_PATH)

fullclean: clean
	$(MAKE) clean -C lib/
	rm -rf doc/doxygen


docs:
	doxygen Doxyfile

stats:
	find ./ManuvrOS -type f \( -name \*.cpp -o -name \*.h \) -exec wc -l {} +
