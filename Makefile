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

CPP            = $(shell which g++)

OUTPUT_PATH    = build/


###########################################################################
# Source files, includes, and linker directives...
###########################################################################
INCLUDES    = -I.


# Libraries to link
LIBS =  -lstdc++ -lm

# Wrap the include paths into the flags...
CFLAGS = $(OPTIMIZATION) -Wall $(INCLUDES)
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
  TARGET_WIDTH = -m32
else
  TARGET_WIDTH =
endif
                      
CFLAGS += $(CPP_FLAGS)


###########################################################################
# Source file definitions...
###########################################################################
MANUVROS_SRCS   = DataStructures/*.cpp ManuvrOS/*.cpp ManuvrOS/XenoSession/*.cpp ManuvrOS/ManuvrMsg/*.cpp

SENSOR_SRCS     = ManuvrOS/Platform/Platform.cpp ManuvrOS/Drivers/SensorWrapper/*.cpp 
I2C_DRIVERS     = ManuvrOS/Drivers/i2c-adapter/*.cpp ManuvrOS/Drivers/DeviceWithRegisters/DeviceRegister.cpp ManuvrOS/Drivers/DeviceWithRegisters/DeviceWithRegisters.cpp

COM_DRIVERS     = ManuvrOS/Transports/*.cpp
COM_DRIVERS    += ManuvrOS/Transports/ManuvrSocket/ManuvrTCP.cpp
COM_DRIVERS    += ManuvrOS/Transports/ManuvrSerial/ManuvrSerial.cpp

# Because this Makefile technically supports two platforms.
# TODO: Make a single linux platform driver, and case-off Raspi stuff within it.
RASPI_DRIVERS   = ManuvrOS/Drivers/ManuvrableGPIO/*.cpp ManuvrOS/Platform/PlatformRaspi.cpp


GENERIC_DRIVERS = ManuvrOS/Platform/PlatformUnsupported.cpp


CPP_SRCS  = $(MANUVROS_SRCS) 
CPP_SRCS += $(I2C_DRIVERS) $(SENSOR_SRCS) $(COM_DRIVERS)

SRCS   = $(CPP_SRCS)
             
# TODO: I badly need to learn to write autoconf scripts....
#   I've at least tried to modularize to make the invariable transition less-painful...
MANUVR_OPTIONS  = -DMANUVR_SUPPORT_SERIAL
MANUVR_OPTIONS += -DMANUVR_SUPPORT_TCPSOCKET
MANUVR_OPTIONS += -D__MANUVR_DEBUG

# Options that build for certain threading models (if any).
#MANUVR_OPTIONS += -D__MANUVR_FREERTOS
MANUVR_OPTIONS += -D__MANUVR_LINUX


CFLAGS += $(MANUVR_OPTIONS) 

###########################################################################
# Rules for building the firmware follow...
#
# 'make raspi' will build a sample firmware for the original Raspberry Pi.  
#    The idea is to be able to quickly iterate on a design idea with the
#    aid of valgrind and gdb.
###########################################################################

.PHONY: all


all: clean
	$(CPP) -static -g -o manuvr main.cpp $(SRCS) $(GENERIC_DRIVERS) $(CFLAGS) -std=$(CPP_STANDARD) $(TARGET_WIDTH) $(LIBS) -D_GNU_SOURCE -O2

raspi: clean
	$(CPP) -static -g -o manuvr main.cpp $(SRCS) $(RASPI_DRIVERS) $(CFLAGS) -std=$(CPP_STANDARD) $(TARGET_WIDTH) $(LIBS) -DRASPI -D_GNU_SOURCE -O2

debug:
	$(CPP) -static -g -o manuvr main.cpp $(SRCS) $(GENERIC_DRIVERS) $(CFLAGS) -std=$(CPP_STANDARD) $(TARGET_WIDTH) $(LIBS) -D__MANUVR_DEBUG -D_GNU_SOURCE -O0 -fstack-usage
# Options configured such that you can then...
# valgrind --tool=callgrind ./manuvr
# gprof2dot --format=callgrind --output=out.dot callgrind.out.16562
# dot  -Tpng out.dot -o graph.png

manuvrtests:
	mkdir $(OUTPUT_PATH)
	$(CPP) -o $(OUTPUT_PATH)/dstest tests/TestDataStructures.cpp DataStructures/*.cpp -I. -lstdc++ -lc -lm

clean:
	rm -f *.o *.su *~ testbench manuvr
	rm -rf $(OUTPUT_PATH)

fullclean: clean
	rm -rf doc/doxygen


docs:
	doxygen Doxyfile

stats:
	find ./ManuvrOS ./DataStructures ./StringBuilder -type f \( -name \*.cpp -o -name \*.h \) -exec wc -l {} +

checkin: fullclean
	git push

checkout:
	git pull

