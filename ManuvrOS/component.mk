#
# Digitabulum-r2 via ESP32
#

#COMPONENT_ADD_INCLUDEDIRS := include
#COMPONENT_PRIV_INCLUDEDIRS := include/freertos

#CFLAGS +=

#COMPONENT_ADD_INCLUDEDIRS := $(PROJECT_PATH)/lib $(PROJECT_PATH)/lib/CppPotpourri/src . ../..
COMPONENT_ADD_INCLUDEDIRS := $(PROJECT_PATH)/lib $(PROJECT_PATH)/lib/CppPotpourri/src . ../..

CFLAGS += -D__MANUVR_ESP32
CXXFLAGS += -D__MANUVR_ESP32

# These are components that are known to work on ESP32.
LOCAL_ESP_COMPS  = . DataStructures
LOCAL_ESP_COMPS += Types
LOCAL_ESP_COMPS += Drivers/ADP8866 Drivers/BusQueue ManuvrMsg Types
LOCAL_ESP_COMPS += Drivers/ATECC508
LOCAL_ESP_COMPS += Drivers/ISL23345
LOCAL_ESP_COMPS += Drivers/DS1881
LOCAL_ESP_COMPS += Drivers/AudioRouter
LOCAL_ESP_COMPS += Drivers/ADG2128
LOCAL_ESP_COMPS += Drivers/DeviceWithRegisters
LOCAL_ESP_COMPS += Drivers/TestDriver
LOCAL_ESP_COMPS += Drivers/PMIC/BQ24155
LOCAL_ESP_COMPS += Drivers/PMIC/LTC294x
LOCAL_ESP_COMPS += Drivers/Display/SSD-OLED
LOCAL_ESP_COMPS += Drivers/Sensors
LOCAL_ESP_COMPS += Drivers/Sensors/BMP280
LOCAL_ESP_COMPS += Drivers/Sensors/INA219
LOCAL_ESP_COMPS += Drivers/Sensors/TMP102
LOCAL_ESP_COMPS += Drivers/Sensors/LightSensor
LOCAL_ESP_COMPS += Drivers/Sensors/HMC5883
LOCAL_ESP_COMPS += Drivers/Sensors/IMU/ADXL345
LOCAL_ESP_COMPS += Drivers/Sensors/IMU/LSM9DS1
LOCAL_ESP_COMPS += Drivers/KridaDimmer
LOCAL_ESP_COMPS += Drivers/Modems/LORA/SX1276
LOCAL_ESP_COMPS += Drivers/SX8634
LOCAL_ESP_COMPS += Drivers/SX1503

LOCAL_ESP_COMPS += ../lib/CppPotpourri/src

LOCAL_ESP_COMPS += Platform/Targets/ESP32
LOCAL_ESP_COMPS += Platform/Targets/ESP32/I2C
LOCAL_ESP_COMPS += Platform/Targets/ESP32/SPI
LOCAL_ESP_COMPS += Platform/Targets/ESP32/UART
LOCAL_ESP_COMPS += Platform/Peripherals/I2C
LOCAL_ESP_COMPS += Platform/Peripherals/SPI
LOCAL_ESP_COMPS += Platform/Peripherals/UART
LOCAL_ESP_COMPS += Platform/Peripherals
LOCAL_ESP_COMPS += Platform
LOCAL_ESP_COMPS += Platform/Cryptographic Platform/Identity

LOCAL_ESP_COMPS += XenoSession XenoSession/Console
LOCAL_ESP_COMPS += Transports Transports/StandardIO
LOCAL_ESP_COMPS += Transports/ManuvrSocket
LOCAL_ESP_COMPS += Transports/BufferPipes/ManuvrGPS

#COMPONENT_ADD_LDFLAGS := -L$(OUTPUT_PATH)/ManuvrOS

COMPONENT_SRCDIRS := $(LOCAL_ESP_COMPS)
