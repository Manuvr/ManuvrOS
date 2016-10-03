###########################################################################
# ManuvrOS platform Makefile
# Author: J. Ian Lindsay
#
# This is where we conditionally compile platform-support code.
#
# Cryptographic support is also handled by this Makefile.
#
# TODO:
# It was intended that this be open-ended enough to allow others to easilly
#   implement other arbitrary platforms. It might be better to ultimately
#   re-work this to use platformIO.
###########################################################################

C_STANDARD         = gnu99
CXX_STANDARD       = gnu++11

# Includes
INCLUDES = -I../ -iquote.

# Libraries
LIBS   =

# Driver-specific flags.
LOCAL_FLAGS  =  -std=$(CXX_STANDARD) -nostdlib

# Sources
C_SRCS     =
CPP_SRCS   = Platform.cpp

ifeq ($(SECURE),1)
CPP_SRCS   += Cryptographic/Cryptographic.cpp
C_SRCS     += Cryptographic/MbedTLS.c
C_SRCS     += Cryptographic/OpenSSL.c
endif

# This Makefile heeds two separate exports:
ifeq ($(MANUVR_PLATFORM),LINUX)
CPP_SRCS   += Linux/LinuxStorage.cpp
CPP_SRCS   += Linux/Linux.cpp
ifeq ($(MANUVR_BOARD),RASPI)
CPP_SRCS   += Raspi/DieThermometer/DieThermometer.cpp
CPP_SRCS   += Raspi/Raspi.cpp
else  # raspi
CPP_SRCS   += Linux/Unsupported.cpp
endif  # Board selection
endif  # linux

ifeq ($(MANUVR_PLATFORM),TEENSY3)
CPP_SRCS   += Teensy3/Teensy3.cpp
CPP_SRCS   += Teensy3/TeensyStorage.cpp
endif

ifeq ($(MANUVR_PLATFORM),STM32F7)
CPP_SRCS   += STM32F7/STM32F7.cpp
#SRCS   += STM32F7/STM32F7Storage.cpp
endif

all:
	@echo ''
	@echo ''
	@echo '======================================================'
	@echo 'Platform source files:'
	@echo $(C_SRCS)
	@echo $(CPP_SRCS)
	@echo '======================================================'
	$(CXX) -r $(INCLUDES) $(LOCAL_FLAGS) $(CPP_FLAGS) $(LIBS) $(C_SRCS) $(CPP_SRCS) -o Platform.o


clean:
	rm -f Platform.o
