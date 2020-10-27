#
#
#
message(STATUS "==============================================")
message(STATUS "Manuvr")
message(STATUS "==============================================")

# idf_component_set_property(Manuvr COMPONENT_NAME "Manuvr")
# idf_component_set_property(Manuvr COMPONENT_TYPE LIBRARY)

# These are components that are known to work on ESP32.
idf_component_register(
  SRC_DIRS "." "DataStructures" "Types" "ManuvrMsg"
    "Platform/Targets/ESP32" "Platform/Targets/ESP32/I2C"
    "Platform/Targets/ESP32/SPI" "Platform/Targets/ESP32/UART"
    "Platform/Peripherals/I2C" "Platform/Peripherals/UART" "Platform/Peripherals"
    "Platform" "Platform/Cryptographic" "Platform/Identity"
    "XenoSession" "XenoSession/Console"
    "Transports" "Transports/StandardIO" "Transports/ManuvrSocket"
  INCLUDE_DIRS "."
  REQUIRES CppPotpourri nvs_flash
)
