###########################################################################
# ManuvrOS platform Makefile
# Author: J. Ian Lindsay
#
# This is where we conditionally compile platform-support code.
###########################################################################

CXX_STANDARD       = gnu++11

# Includes
INCLUDES = -I../ -iquote.

# Libraries
LIBS   =

# Driver-specific flags.
LOCAL_FLAGS  =  -std=$(CXX_STANDARD) -nostdlib

# Sources
SRCS   = Platform.cpp


# This Makefile heeds two separate exports:
ifeq ($(MANUVR_PLATFORM),LINUX)
SRCS   += Linux/LinuxStorage.cpp
SRCS   += Linux/Linux.cpp

ifeq ($(MANUVR_BOARD),RASPI)
SRCS   += Raspi/DieThermometer/DieThermometer.cpp
SRCS   += Linux/LinuxStorage.cpp
SRCS   += Raspi/Raspi.cpp
endif  # linux
endif  # raspi

ifeq ($(MANUVR_PLATFORM),TEENSY3)
SRCS   += Teensy3/Teensy3.cpp
else
endif


all:
	@echo ''
	@echo ''
	@echo '======================================================'
	@echo 'Platform source files:'
	@echo $(SRCS)
	@echo '======================================================'
	$(CXX) -r $(INCLUDES) $(LOCAL_FLAGS) $(CPP_FLAGS) $(LIBS) $(SRCS) -o Platform.o


clean:
	rm -f Platform.o
