CPP_STANDARD       = gnu++11

# Includes
INCLUDES = -I../ -iquote.

# Libraries
LIBS   =

# Driver-specific flags.
LOCAL_FLAGS  =  -std=$(CPP_STANDARD) -nostdlib

# Sources
SRCS   = Platform.cpp
#SRCS  += PlatformRaspi.cpp
#SRCS  += PlatformFubarino.cpp
#SRCS  += STM32F4.cpp
#SRCS  += PlatformTeensy3.cpp


all: setenvars
	@echo ''
	@echo ''
	@echo '======================================================'
	@echo 'Platform compile'
	@echo $(SRCS)
	@echo $(LIBS)
	@echo '======================================================'
	$(CPP) -r $(INCLUDES) $(LOCAL_FLAGS) $(CPP_FLAGS) $(LIBS) $(SRCS) -o Platform.o

setenvars:
ifeq ($(STM32F746xx),)
#LIBS  += -lstdperiph
#SRCS  += STM32F7.cpp
#else ifeq ($STM32F4XX,)
  #SRCS  += STM32F4.cpp
endif
#else ifeq ($(__MK20DX256__),) | ifeq ($(__MK20DX128__),)
  #SRCS  += PlatformTeensy3.cpp
ifeq ($(__MANUVR_LINUX),)
ifeq ($(RASPI),)
LIBS   += -lpthread
SRCS  += PlatformRaspi.cpp
else
#LIBS   += -lpthread
#SRCS  += PlatformUnsupported.cpp
endif
endif


clean:
	rm -f Platform.o
