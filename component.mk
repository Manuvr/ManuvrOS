#
# Digitabulum-r2 via ESP32
#

#COMPONENT_ADD_INCLUDEDIRS := include
#COMPONENT_PRIV_INCLUDEDIRS := include/freertos

#CFLAGS +=

CXXFLAGS += -nostdlib


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

ifeq ($(MANUVR_PLATFORM),ESP32)
CPP_SRCS   += Targets/ESP32/ESP32.cpp
COMPONENT_OBJS += Targets/ESP32/ESP32.cpp
#CPP_SRCS   += Targets/ESP32/Storage/ESP32Storage.cpp
#CPP_SRCS   += Targets/ESP32/I2C/I2CAdapter.cpp
#CPP_SRCS   += Targets/ESP32/SPI/SPIAdapter.cpp
#CPP_SRCS   += Targets/ESP32/WiFi/WiFi.cpp
#CPP_SRCS   += Targets/ESP32/Bluetooth/Bluetooth.cpp
endif
