vpath %.cpp src

# Includes
INCLUDES  = -I../Core/cmsis -I../Core/stm32 -I../../../Drivers/CMSIS/Device/ST/STM32F7xx/Include
INCLUDES += -Isrc -I../src/drivers -I../../../Inc -I../../../Drivers/STM32F7xx_HAL_Driver/Inc
INCLUDES += -I../../../Drivers/CMSIS/Include -I../FreeRTOS/Source/CMSIS_RTOS -I../FreeRTOS/Source/include
INCLUDES += -I../FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1/
INCLUDES += -I../ -iquote.

# Sources
SRCS   = Platform.cpp
#SRCS  += PlatformRaspi.cpp
#SRCS  += PlatformFubarino.cpp
#SRCS  += STM32F4.cpp
#SRCS  += PlatformTeensy3.cpp

ifeq ($(STM32F746xx),)
  SRCS  += STM32F7.cpp
#else ifneq ($(strip $(STM32F746xx)),)
#else ifneq ($(strip $(STM32F746xx)),)
else ifeq ($(STM32F4XX),)
  SRCS  += STM32F4.cpp
else ifeq ($(__MK20DX256__),) | ifeq ($(__MK20DX128__),)
  SRCS  += PlatformTeensy3.cpp
else ifeq ($(LINUX),)
  SRCS  += PlatformRaspi.cpp
else
  SRCS  += PlatformUnsupported.cpp
endif


all:
	$(CC) $(CPP_FLAGS) $(INCLUDES) -nostdlib $(SRCS) -o Platform.o
