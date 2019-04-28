###########################################################################
# ManuvrOS platform Makefile
# Author: J. Ian Lindsay
#
# This is where we conditionally compile platform-support code.
#
# Cryptographic support is also handled by this Makefile.
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
C_SRCS     = Peripherals/RNG/pcg_basic/pcg_basic.c
CPP_SRCS   = Peripherals/Peripheral.cpp
CPP_SRCS  += Platform.cpp

CPP_SRCS   += Cryptographic/Cryptographic.cpp
CPP_SRCS   += Cryptographic/MbedTLS.cpp
CPP_SRCS   += Cryptographic/OpenSSL.cpp
CPP_SRCS   += Cryptographic/Blind.cpp

# This Makefile heeds two separate exports:
ifeq ($(MANUVR_PLATFORM),LINUX)
CPP_SRCS   += Targets/Linux/LinuxStorage.cpp
CPP_SRCS   += Targets/Linux/Linux.cpp
CPP_SRCS   += Targets/Linux/I2C/I2CAdapter.cpp
ifeq ($(MANUVR_BOARD),RASPI)
CPP_SRCS   += Targets/Raspi/DieThermometer/DieThermometer.cpp
CPP_SRCS   += Targets/Raspi/Raspi.cpp
else  # raspi
CPP_SRCS   += Targets/Linux/Unsupported.cpp
endif  # Board selection
endif  # linux

ifeq ($(MANUVR_PLATFORM),TEENSY3)
CPP_SRCS   += Targets/Teensy3/Teensy3.cpp
CPP_SRCS   += Targets/Teensy3/TeensyStorage.cpp
CPP_SRCS   += Targets/Teensy3/I2C/I2CAdapter.cpp
CPP_SRCS   += Targets/Teensy3/SPI/SPIAdapter.cpp
endif

ifeq ($(MANUVR_PLATFORM),ESP32)
CPP_SRCS   += Targets/ESP32/ESP32.cpp
#CPP_SRCS   += Targets/ESP32/Storage/ESP32Storage.cpp
#CPP_SRCS   += Targets/ESP32/I2C/I2CAdapter.cpp
#CPP_SRCS   += Targets/ESP32/SPI/SPIAdapter.cpp
#CPP_SRCS   += Targets/ESP32/WiFi/WiFi.cpp
#CPP_SRCS   += Targets/ESP32/Bluetooth/Bluetooth.cpp
endif

ifeq ($(MANUVR_PLATFORM),ARDUINO)
CPP_SRCS   += Targets/Arduino/Arduino.cpp
CPP_SRCS   += Targets/Arduino/I2C/I2CAdapter.cpp
endif

ifeq ($(MANUVR_PLATFORM),STM32F7)
CPP_SRCS   += Targets/STM32F7/STM32F7.cpp
CPP_SRCS   += Targets/STM32F7/I2C/I2CAdapter.cpp
#SRCS   += Targets/STM32F7/STM32F7Storage.cpp
endif

ifeq ($(MANUVR_PLATFORM),STM32F4)
CPP_SRCS   += Targets/STM32F4/STM32F4.cpp
CPP_SRCS   += Targets/STM32F4/I2C/I2CAdapter.cpp
#SRCS   += Targets/STM32F4/STM32F4Storage.cpp
endif


COV_FILES = $(CPP_SRCS:.cpp=.gcda) $(CPP_SRCS:.cpp=.gcno) $(C_SRCS:.c=.gcno) $(C_SRCS:.c=.gcno)

all:
	@echo ''
	@echo ''
	@echo '======================================================'
	@echo 'Platform source files:'
	@echo $(C_SRCS)
	@echo $(CPP_SRCS)
	@echo '======================================================'
	$(CXX) -r $(INCLUDES) $(LOCAL_FLAGS) $(CXXFLAGS) $(LIBS) $(C_SRCS) $(CPP_SRCS) -o Platform.o


clean:
	rm -f Platform.o $(COV_FILES) *.gcno *.gcda
