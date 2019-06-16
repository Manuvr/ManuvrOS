#
# Digitabulum-r2 via ESP32
#

#COMPONENT_ADD_INCLUDEDIRS := include
#COMPONENT_PRIV_INCLUDEDIRS := include/freertos

#CFLAGS +=

COMPONENT_ADD_INCLUDEDIRS := $(PROJECT_PATH)/lib . ../..

CFLAGS += -D__MANUVR_ESP32
CXXFLAGS += -D__MANUVR_ESP32

# These are components that are known to work on ESP32.
LOCAL_ESP_COMPS  = . DataStructures
LOCAL_ESP_COMPS += Types
LOCAL_ESP_COMPS += Types/cbor-cpp
LOCAL_ESP_COMPS += Drivers/ADP8866 Drivers/BusQueue ManuvrMsg Types
LOCAL_ESP_COMPS += Drivers/ATECC508
LOCAL_ESP_COMPS += Drivers/ISL23345
LOCAL_ESP_COMPS += Drivers/AudioRouter
LOCAL_ESP_COMPS += Drivers/ADG2128
LOCAL_ESP_COMPS += Drivers/DeviceWithRegisters
LOCAL_ESP_COMPS += Drivers/TestDriver
LOCAL_ESP_COMPS += Drivers/PMIC/BQ24155
LOCAL_ESP_COMPS += Drivers/PMIC/LTC294x
LOCAL_ESP_COMPS += Drivers/SSD1331
LOCAL_ESP_COMPS += Drivers/Sensors
LOCAL_ESP_COMPS += Drivers/Sensors/BMP280
LOCAL_ESP_COMPS += Drivers/Sensors/TMP102
LOCAL_ESP_COMPS += Drivers/Sensors/IMU/ADXL345

LOCAL_ESP_COMPS += Platform/Targets/ESP32
LOCAL_ESP_COMPS += Platform/Targets/ESP32/I2C
LOCAL_ESP_COMPS += Platform/Peripherals/I2C
LOCAL_ESP_COMPS += Platform/Peripherals/SPI
LOCAL_ESP_COMPS += Platform/Peripherals
LOCAL_ESP_COMPS += Platform
LOCAL_ESP_COMPS += Platform/Cryptographic Platform/Identity

LOCAL_ESP_COMPS += XenoSession XenoSession/Console
LOCAL_ESP_COMPS += Transports Transports/StandardIO
LOCAL_ESP_COMPS += Transports Transports/ManuvrSocket

#COMPONENT_ADD_LDFLAGS := -L$(OUTPUT_PATH)/ManuvrOS

COMPONENT_SRCDIRS := $(LOCAL_ESP_COMPS)
