###########################################################################
# Makefile for ManuvrOS.
# Author: J. Ian Lindsasy
#
# Note... This is supposed to be called from an upstream Makefile where
#   platform-specific build options are defined.
###########################################################################
CPP_STANDARD       = gnu++11

###########################################################################
# Source files, includes, and linker directives...
###########################################################################
INCLUDES    = -I. -iquote.

# Libraries to link
LIBS =  -lm

CPP_SRCS   = DataStructures/StringBuilder.cpp
CPP_SRCS  += DataStructures/Quaternion.cpp
CPP_SRCS  += ManuvrRunnable.cpp
CPP_SRCS  += EnumeratedTypeCodes.cpp
CPP_SRCS  += Kernel.cpp
CPP_SRCS  += EventReceiver.cpp
CPP_SRCS  += TaskProfilerData.cpp
CPP_SRCS  += XenoSession/XenoMessage.cpp
CPP_SRCS  += XenoSession/XenoSession.cpp
CPP_SRCS  += Transports/ManuvrXport.cpp
CPP_SRCS  += Transports/ManuvrSerial/ManuvrSerial.cpp

# TODO: Case-off for socket/TCP/Pipe support...
#CPP_SRCS  += Transports/ManuvrPipe/ManuvrPipe.cpp
#CPP_SRCS  += Transports/ManuvrTelehash/ManuvrTelehash.cpp
CPP_SRCS  += Transports/ManuvrSocket/ManuvrTCP.cpp


CPP_SRCS  += ManuvrMsg/Argument.cpp
CPP_SRCS  += ManuvrMsg/ManuvrMsg.cpp

# TODO: Bring up to speed WRT Runnable.
#CPP_SRCS  += Drivers/LightSensor/LightSensor.cpp
#CPP_SRCS  += Drivers/AudioRouter/AudioRouter.cpp

# TODO: Use GPio abstractions!
#CPP_SRCS  += Drivers/MGC3130/MGC3130.cpp
#CPP_SRCS  += Drivers/MCP73833/MCP73833.cpp
#CPP_SRCS  += Drivers/ADP8866/ADP8866.cpp

# TODO: No STM32F7 support yet. Make no-platform fail the build.
#CPP_SRCS  += Drivers/i2c-adapter/I2CDevice.cpp
#CPP_SRCS  += Drivers/i2c-adapter/i2c-adapter.cpp
#CPP_SRCS  += Drivers/i2c-adapter/I2CDeviceWithRegisters.cpp
#CPP_SRCS  += Drivers/i2c-adapter/I2CQueuedOperation.cpp

# TODO: Only for Teensyduino libs...
#CPP_SRCS  += Drivers/ManuvrAudio/ManuvrAudio.cpp

# TODO: Bring up to speed WRT scheduler removal.
#CPP_SRCS  += Drivers/ADCScanner/ADCScanner.cpp

# TODO: Full of fail. Arduino abstraction, GPIO, runnable, scheduler, et al...
#CPP_SRCS  += Drivers/ManuvrableNeoPixel/ManuvrableNeoPixel.cpp

CPP_SRCS  += Drivers/ExampleDriver/ExampleDriver.cpp
CPP_SRCS  += Drivers/ManuvrCoAP/ManuvrCoAP.cpp
CPP_SRCS  += Drivers/ISL23345/ISL23345.cpp
CPP_SRCS  += Drivers/Bluetooth/RN4020/RN4020.cpp
CPP_SRCS  += Drivers/ADG2128/ADG2128.cpp
CPP_SRCS  += Drivers/INA219/INA219.cpp
CPP_SRCS  += Drivers/TMP006/TMP006.cpp
CPP_SRCS  += Drivers/ISL29033/ISL29033.cpp
#CPP_SRCS  += Drivers/LSMD9S1/LSMD9S1.cpp
CPP_SRCS  += Drivers/SensorWrapper/SensorWrapper.cpp
CPP_SRCS  += Drivers/DeviceWithRegisters/DeviceWithRegisters.cpp
CPP_SRCS  += Drivers/DeviceWithRegisters/DeviceRegister.cpp
CPP_SRCS  += Drivers/ManuvrableGPIO/ManuvrableGPIO.cpp



OBJS  = $(CPP_SRCS:.cpp=.o)
LIBNAME = libmanuvr.a


all: setenvars $(LIBNAME)


%.o : %.cpp
	$(CPP) $(INCLUDES) $(CPP_FLAGS) -std=$(CPP_STANDARD) -nostdlib -c $< -o $@

$(LIBNAME): $(OBJS)
	$(MAKE) -C Platform/
	$(AR) -rvs $(OUTPUT_PATH)/$@ $(OBJS) Platform/*.o

clean:
	$(MAKE) clean -C Platform/
	rm -f $(OBJS)
	rm -f $(OUTPUT_PATH)/$(LIBNAME)


setenvars:
	@echo '======================================================'
	@echo ''
	@echo '======================================================'


fullclean: clean
	rm -rf doc/doxygen

docs:
	doxygen Doxyfile
