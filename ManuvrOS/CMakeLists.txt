#
#
#

cmake_minimum_required(VERSION 3.5)
message(STATUS "==============================================")
message(STATUS "Manuvr")
message(STATUS "==============================================")
# include($ENV{IDF_PATH}/tools/cmake/idf.cmake)

# idf_component_set_property(Manuvr COMPONENT_NAME "Manuvr")
# idf_component_set_property(Manuvr COMPONENT_TYPE LIBRARY)

# These are components that are known to work on ESP32.
idf_component_register(
  SRC_DIRS "." "DataStructures" "Types" "Drivers/ADP8866" "ManuvrMsg" "Types" "Drivers/ATECC508" "Drivers/ISL23345" "Drivers/DS1881" "Drivers/AudioRouter" "Drivers/ADG2128" "Drivers/DeviceWithRegisters" "Drivers/TestDriver" "Drivers/PMIC/BQ24155" "Drivers/PMIC/LTC294x" "Drivers/Display/SSD-OLED" "Drivers/Sensors" "Drivers/Sensors/BMP280" "Drivers/Sensors/INA219" "Drivers/Sensors/TMP102" "Drivers/Sensors/LightSensor" "Drivers/Sensors/HMC5883" "Drivers/Sensors/IMU/ADXL345" "Drivers/Sensors/IMU/LSM9DS1" "Drivers/KridaDimmer" "Drivers/Modems/LORA/SX1276" "Drivers/SX8634" "Drivers/SX1503" "Platform/Targets/ESP32" "Platform/Targets/ESP32/I2C" "Platform/Targets/ESP32/SPI" "Platform/Targets/ESP32/UART" "Platform/Peripherals/I2C" "Platform/Peripherals/UART" "Platform/Peripherals" "Platform" "Platform/Cryptographic" "Platform/Identity" "XenoSession" "XenoSession/Console" "Transports" "Transports/StandardIO" "Transports/ManuvrSocket" "Transports/BufferPipes/ManuvrGPS" 

  INCLUDE_DIRS "."
  REQUIRES CppPotpourri nvs_flash
)
