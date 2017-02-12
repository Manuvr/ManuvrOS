#
# Digitabulum-r2 via ESP32
#

#COMPONENT_ADD_INCLUDEDIRS := include
#COMPONENT_PRIV_INCLUDEDIRS := include/freertos

#CFLAGS +=

COMPONENT_ADD_INCLUDEDIRS := $(PROJECT_PATH)/lib

CFLAGS += -D__MANUVR_ESP32
CPPFLAGS += -D__MANUVR_ESP32

COMPONENT_SRCDIRS := DataStructures Drivers ManuvrMsg Types XenoSession Transports Platform Platform/Cryptographic Platform/Identity Platform/Targets/ESP32 Platform/Peripherals
