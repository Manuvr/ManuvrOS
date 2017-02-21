#
# Digitabulum-r2 via ESP32
#

#COMPONENT_ADD_INCLUDEDIRS := include
#COMPONENT_PRIV_INCLUDEDIRS := include/freertos

#CFLAGS +=

COMPONENT_ADD_INCLUDEDIRS := $(PROJECT_PATH)/lib

CFLAGS += -D__MANUVR_ESP32
CXXFLAGS += -D__MANUVR_ESP32

# These are components that are known to work on ESP32.
LOCAL_ESP_COMPS  = . DataStructures Drivers/ADP8866
LOCAL_ESP_COMPS += Drivers/ADP8866 Drivers/BusQueue ManuvrMsg Types
LOCAL_ESP_COMPS += Platform/Targets/ESP32 Platform/Peripherals
LOCAL_ESP_COMPS += Platform Platform/Cryptographic Platform/Identity
LOCAL_ESP_COMPS += Platform/Peripherals/SPI Platform/Peripherals/I2C
LOCAL_ESP_COMPS += XenoSession XenoSession/Console
LOCAL_ESP_COMPS += Transports Transports/StandardIO

#COMPONENT_ADD_LDFLAGS := -L$(OUTPUT_PATH)/ManuvrOS

COMPONENT_SRCDIRS := $(LOCAL_ESP_COMPS)
