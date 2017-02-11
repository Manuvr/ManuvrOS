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
OPTIMIZATION       = -O1
C_STANDARD         = gnu99
CPP_STANDARD       = gnu++11
FIRMWARE_NAME      = manuvr
CFLAGS             = -Wall
MANUVR_OPTIONS     =

###########################################################################
# Environmental awareness...
###########################################################################
SHELL          = /bin/sh

export CC      = $(shell which gcc)
export CXX     = $(shell which g++)
export AR      = $(shell which ar)
export SZ      = $(shell which size)
export MAKE    = $(shell which make)

# This is where we will store compiled libs and the final output.
export BUILD_ROOT  = $(shell pwd)
export OUTPUT_PATH = $(BUILD_ROOT)/build/


###########################################################################
# Source files, includes, and linker directives...
###########################################################################
INCLUDES    = -I$(BUILD_ROOT)/.
INCLUDES   += -I$(BUILD_ROOT)/ManuvrOS
INCLUDES   += -I$(BUILD_ROOT)/lib

INCLUDES   += -I$(BUILD_ROOT)/lib/paho.mqtt.embedded-c/
INCLUDES   += -I$(BUILD_ROOT)/lib/mbedtls/include/
INCLUDES   += -I$(BUILD_ROOT)/lib/iotivity
INCLUDES   += -I$(BUILD_ROOT)/lib/iotivity/include

# Libraries to link
# We should be gradually phasing out C++ standard library on linux builds.
# TODO: Advance this goal.
LIBS = -L$(OUTPUT_PATH) -L$(BUILD_ROOT)/lib -lstdc++ -lm

# Wrap the include paths into the flags...
CFLAGS += $(INCLUDES)
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

# Options that build for certain threading models (if any).
#MANUVR_OPTIONS += -D__MANUVR_FREERTOS
MANUVR_OPTIONS += -D__MANUVR_LINUX

# Wire and session protocols...
#MANUVR_OPTIONS += -DMANUVR_SUPPORT_OSC

# Since we are building on linux, we will have threading support via
# pthreads.
LIBS +=  -lmanuvr -lextras -lpthread


# Framework selections, if any are desired.
ifeq ($(OIC_SERVER),1)
	MANUVR_OPTIONS += -DMANUVR_OPENINTERCONNECT -DOC_SERVER
	MANUVR_OPTIONS += -DOC_SECURITY
else ifeq ($(OIC_CLIENT),1)
	MANUVR_OPTIONS += -DMANUVR_OPENINTERCONNECT -DOC_CLIENT
	MANUVR_OPTIONS += -DOC_SECURITY
endif

# Options for various security features.
ifeq ($(SECURE),1)
# The remaining lines are to prod header files in libraries.
LIBS += $(OUTPUT_PATH)/libmbedtls.a
LIBS += $(OUTPUT_PATH)/libmbedx509.a
LIBS += $(OUTPUT_PATH)/libmbedcrypto.a
# mbedTLS will require this in order to use our chosen options.
MANUVR_OPTIONS += -DMBEDTLS_CONFIG_FILE='<mbedTLS_conf.h>'
export SECURE=1
endif

# Support for specific SBC hardware on linux.
ifeq ($(BOARD),RASPI)
MANUVR_OPTIONS += -DRASPI
export MANUVR_BOARD = RASPI
endif

# Debugging options...
ifeq ($(DEBUG),1)
#MANUVR_OPTIONS += -D__MANUVR_PIPE_DEBUG
#OPTIMIZATION    = -O0 -g
# Options configured such that you can then...
# valgrind --tool=callgrind ./manuvr
# gprof2dot --format=callgrind --output=out.dot callgrind.out.16562
# dot  -Tpng out.dot -o graph.png
endif

###########################################################################
# Rules for building the firmware follow...
###########################################################################
# Merge our choices and export them to the downstream Makefiles...
CFLAGS += $(MANUVR_OPTIONS) $(OPTIMIZATION)
ANALYZER_FLAGS  = $(MANUVR_OPTIONS) $(INCLUDES)
ANALYZER_FLAGS +=  --std=c++11 --report-progress --force -j6

export MANUVR_PLATFORM = LINUX
export ANALYZER_FLAGS

export CFLAGS
export CPP_FLAGS    = $(CFLAGS) -fno-rtti -fno-exceptions

export JSON=1

.PHONY: all

all: tests
	$(CXX) -Wl,--gc-sections -static -o $(FIRMWARE_NAME) $(CPP_SRCS) $(CPP_FLAGS) -std=$(CPP_STANDARD) $(LIBS) -D_GNU_SOURCE
	$(SZ) $(FIRMWARE_NAME)

tests: libs
	$(MAKE) -C tests/

menuconfig:
	@echo '======================================================'

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
	$(MAKE) clean -C tests/
	rm -f *.o *.su *~ testbench $(FIRMWARE_NAME)

fullclean: clean
	rm -rf $(OUTPUT_PATH)
	export SECURE=1
	$(MAKE) clean -C lib/
	$(MAKE) clean -C tests/
	rm -rf doc/doxygen

docs:
	doxygen Doxyfile
	mkdir -p doc/doxygen/html/doc/
	ln -s ../../../3d-logo.png doc/doxygen/html/doc/3d-logo.png

check:
	$(MAKE) check -C ManuvrOS/
