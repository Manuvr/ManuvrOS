vpath %.cpp src

# Includes
INCLUDES = -I../ -iquote.

# Libraries
#LIBS   = -lm

# Sources
SRCS   = Platform.cpp
#SRCS  += PlatformRaspi.cpp
#SRCS  += PlatformFubarino.cpp
#SRCS  += STM32F4.cpp
#SRCS  += PlatformTeensy3.cpp

ifeq ($(STM32F746xx),)
	SRCS  += STM32F7/STM32F7.cpp
	LIBS  += -lstdperiph
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
	$(CC) $(CPP_FLAGS) $(INCLUDES) -L$(OUTPUT_PATH) $(LIBS) -nostdlib $(SRCS) -o Platform.o
