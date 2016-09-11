###########################################################################
# ManuvrOS platform Makefile
# Author: J. Ian Lindsay
#
# This is where we conditionally compile platform-support code.
#
# TODO:
# It was intended that this be open-ended enough to allow others to easilly
#   implement other arbitrary platforms. It might be better to ultimately
#   re-work this to use platformIO.
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
SRCS   += Raspi/Raspi.cpp
else  # raspi
SRCS   += Linux/Unsupported.cpp
endif  # Board selection
endif  # linux

ifeq ($(MANUVR_PLATFORM),TEENSY3)
SRCS   += Teensy3/Teensy3.cpp
SRCS   += Teensy3/TeensyStorage.cpp
endif

ifeq ($(MANUVR_PLATFORM),STM32F7)
SRCS   += STM32F7/STM32F7.cpp
#SRCS   += STM32F7/STM32F7Storage.cpp
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
