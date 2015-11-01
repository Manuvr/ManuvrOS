###########################################################################
# Makefile for ManuvrOS.
# Author: J. Ian Lindsay
# Date:   2014.08.13
#
###########################################################################

# Used for internal functionality macros. Feel free to rename. Need to
#   replace this with an autoconf script which I haven't yet learned how to
#   write.
FIRMWARE_NAME      = ManuvrOS
OUTPUT_PATH        = build/

OPTIMIZATION       = -O0 -g
C_STANDARD         = gnu99
CPP_STANDARD       = gnu++11


###########################################################################
# Environmental awareness...
###########################################################################
SHELL          = /bin/sh
WHO_I_AM       = $(shell whoami)
WHERE_I_AM     = $(shell pwd)
HOME_DIRECTORY = /home/$(WHO_I_AM)

CPP            = $(shell which g++)


###########################################################################
# Source files, includes, and linker directives...
###########################################################################
INCLUDES    = -iquote. -iquote -I.


# Libraries to link
LIBS =  -lstdc++ -lm
                                                                                              
# Wrap the include paths into the flags...
CFLAGS = $(OPTIMIZATION) -Wall
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
MANUVROS_SRCS = StringBuilder/*.cpp DataStructures/*.cpp ManuvrOS/*.cpp ManuvrOS/XenoSession/*.cpp ManuvrOS/ManuvrMsg/*.cpp
SENSOR_SRCS   = ManuvrOS/Drivers/SensorWrapper/*.cpp 
I2C_DRIVERS   = ManuvrOS/Drivers/i2c-adapter/*.cpp ManuvrOS/Drivers/DeviceWithRegisters/DeviceRegister.cpp ManuvrOS/Drivers/DeviceWithRegisters/DeviceWithRegisters.cpp
COM_DRIVERS   = ManuvrOS/Transports/*.cpp ManuvrOS/Transports/ManuvrComPort/*.cpp

CPP_SRCS  = $(MANUVROS_SRCS)
CPP_SRCS += $(I2C_DRIVERS) $(SENSOR_SRCS) $(COM_DRIVERS)

SRCS   = $(CPP_SRCS)
             
MANuVR_OPTIONS = -D__ENABLE_MSG_SEMANTICS

CFLAGS += $(MANuVR_OPTIONS)

###########################################################################
# Rules for building the firmware follow...
#
# 'make raspi' will build a sample firmware for the original Raspberry Pi.  
#    The idea is to be able to quickly iterate on a design idea with the
#    aid of valgrind and gdb.
#
# 'make testbench' will build a crude debug tool. It is mostly useless.
###########################################################################

.PHONY: all


all: clean
	$(CPP) -static -g -o manuvr raspiMain.cpp StaticHub/StaticHub.cpp ManuvrOS/Drivers/ManuvrableGPIO/*.cpp $(SRCS) $(CFLAGS) -std=$(CPP_STANDARD) $(TARGET_WIDTH) $(LIBS) $(INCLUDES) -Idemo/ -DTEST_BENCH -D_GNU_SOURCE -O0

raspi: clean
	$(CPP) -static -g -o manuvr raspiMain.cpp StaticHub/StaticHub.cpp ManuvrOS/Drivers/ManuvrableGPIO/*.cpp $(SRCS) $(CFLAGS) -std=$(CPP_STANDARD) $(TARGET_WIDTH) $(LIBS) $(INCLUDES) -DRASPI -D_GNU_SOURCE -O0

testbench:
	$(CPP) -static -g -o testbench demo/test-bench.cpp demo/StaticHub.cpp $(SRCS) $(CFLAGS) -std=$(CPP_STANDARD) $(TARGET_WIDTH) $(LIBS) $(INCLUDES) -Idemo/ -DTEST_BENCH -D_GNU_SOURCE -O0 -fstack-usage
# valgrind --tool=callgrind ./testbench
# gprof2dot --format=callgrind --output=out.dot callgrind.out.16562
# dot  -Tpng out.dot -o graph.png


clean:
	rm -f *.o *.su *~ testbench manuvr
	rm -rf $(OUTPUT_PATH)

fullclean: clean
	rm -rf doc/doxygen


docs:
	doxygen Doxyfile

stats:
	find ./src -type f \( -name \*.cpp -o -name \*.h \) -exec wc -l {} +

checkin: fullclean
	git push

checkout:
	git pull

