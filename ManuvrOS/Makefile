###########################################################################
# Makefile for ManuvrOS.
# Author: J. Ian Lindsasy
#
# Note... This is supposed to be called from an upstream Makefile where
#   platform-specific build options are defined.
#
# TODO: I've written a lot of code. Use makefile defines to trim down the
#         compile. It's getting large.
###########################################################################
CPP_STANDARD       = gnu++11

###########################################################################
# Source files, includes, and linker directives...
###########################################################################
INCLUDES    = -I. -iquote.

# Libraries to link
LIBS =  -lm

# Datastructures
CPP_SRCS   = DataStructures/StringBuilder.cpp
CPP_SRCS  += DataStructures/BufferPipe.cpp
CPP_SRCS  += DataStructures/Quaternion.cpp
CPP_SRCS  += DataStructures/InertialMeasurement.cpp
CPP_SRCS  += DataStructures/uuid.cpp
CPP_SRCS  += DataStructures/Argument.cpp

# Types and encodings
SOURCES_CBOR  = Types/cbor-cpp/encoder.cpp
SOURCES_CBOR += Types/cbor-cpp/decoder.cpp
SOURCES_CBOR += Types/cbor-cpp/input.cpp
SOURCES_CBOR += Types/cbor-cpp/output_dynamic.cpp
SOURCES_CBOR += Types/cbor-cpp/output_static.cpp

CPP_SRCS  += $(SOURCES_CBOR)
CPP_SRCS  += Types/TypeTranscriber.cpp
CPP_SRCS  += Types/Avro/Avro.cpp

# Manuvr core
CPP_SRCS  += EnumeratedTypeCodes.cpp
CPP_SRCS  += Kernel.cpp
CPP_SRCS  += EventReceiver.cpp
CPP_SRCS  += TaskProfilerData.cpp
CPP_SRCS  += Utilities.cpp
CPP_SRCS  += ManuvrMsg/ManuvrMsg.cpp

# Concepts of "session" according to various standards...
CPP_SRCS  += XenoSession/XenoMessage.cpp
CPP_SRCS  += XenoSession/XenoSession.cpp
CPP_SRCS  += XenoSession/Console/ManuvrConsole.cpp
CPP_SRCS  += XenoSession/Manuvr/ManuvrSession.cpp
CPP_SRCS  += XenoSession/Manuvr/XenoManuvrMessage.cpp
CPP_SRCS  += XenoSession/CoAP/CoAPSession.cpp
CPP_SRCS  += XenoSession/CoAP/CoAPMessage.cpp
CPP_SRCS  += XenoSession/MQTT/MQTTSession.cpp
CPP_SRCS  += XenoSession/MQTT/MQTTMessage.cpp
CPP_SRCS  += XenoSession/OSC/OSCSession.cpp
CPP_SRCS  += XenoSession/OSC/OSCMessage.cpp

# Specialized BufferPipes
CPP_SRCS  += Transports/BufferPipes/ZooKeeper/ZooKeeper.cpp
CPP_SRCS  += Transports/BufferPipes/ManuvrGPS/ManuvrGPS.cpp
CPP_SRCS  += Transports/BufferPipes/ManuvrTLS/ManuvrTLS.cpp
CPP_SRCS  += Transports/BufferPipes/ManuvrTLS/ManuvrTLSServer.cpp
CPP_SRCS  += Transports/BufferPipes/ManuvrTLS/ManuvrTLSClient.cpp
CPP_SRCS  += Transports/BufferPipes/XportBridge/XportBridge.cpp

# Transports (Drivers)
CPP_SRCS  += Transports/ManuvrXport.cpp
CPP_SRCS  += Transports/StandardIO/StandardIO.cpp
CPP_SRCS  += Transports/ManuvrSerial/ManuvrSerial.cpp
CPP_SRCS  += Transports/ManuvrSocket/ManuvrSocket.cpp
CPP_SRCS  += Transports/ManuvrSocket/ManuvrTCP.cpp
CPP_SRCS  += Transports/ManuvrSocket/ManuvrUDP.cpp
CPP_SRCS  += Transports/ManuvrSocket/UDPPipe.cpp
# TODO: Case-off for socket/TCP/Pipe support...
#CPP_SRCS  += Transports/ManuvrTelehash/ManuvrTelehash.cpp

# "Layer-7" Frameworks
CPP_SRCS  += Frameworks/OIC/ManuvrOIC.cpp

# i2c apparatus...
CPP_SRCS  += Drivers/BusQueue/BusQueue.cpp
CPP_SRCS  += Platform/Peripherals/I2C/I2CDevice.cpp
CPP_SRCS  += Platform/Peripherals/I2C/I2CAdapter.cpp
CPP_SRCS  += Platform/Peripherals/I2C/I2CDeviceWithRegisters.cpp
CPP_SRCS  += Platform/Peripherals/I2C/I2CBusOp.cpp

# SPI apparatus...
CPP_SRCS  += Platform/Peripherals/SPI/SPIAdapter.cpp
CPP_SRCS  += Platform/Peripherals/SPI/SPIBusOp.cpp


CPP_SRCS  += Drivers/LightSensor/LightSensor.cpp
CPP_SRCS  += Drivers/AudioRouter/AudioRouter.cpp

CPP_SRCS  += Drivers/MCP73833/MCP73833.cpp
CPP_SRCS  += Drivers/ADP8866/ADP8866.cpp

# TODO: Only for Teensyduino libs...
#CPP_SRCS  += Drivers/ManuvrAudio/ManuvrAudio.cpp

CPP_SRCS  += Drivers/ADCScanner/ADCScanner.cpp

CPP_SRCS  += Drivers/ManuvrableNeoPixel/ManuvrableNeoPixel.cpp

CPP_SRCS  += Drivers/ExampleDriver/ExampleDriver.cpp
CPP_SRCS  += Drivers/ISL23345/ISL23345.cpp
CPP_SRCS  += Drivers/Bluetooth/RN4020/RN4020.cpp
CPP_SRCS  += Drivers/ADG2128/ADG2128.cpp

CPP_SRCS  += Drivers/Sensors/SensorWrapper.cpp
CPP_SRCS  += Drivers/Sensors/SensorDatum.cpp
CPP_SRCS  += Drivers/Sensors/Si7021/Si7021.cpp
CPP_SRCS  += Drivers/Sensors/INA219/INA219.cpp
CPP_SRCS  += Drivers/Sensors/VEML6070/VEML6070.cpp
CPP_SRCS  += Drivers/Sensors/TMP006/TMP006.cpp
CPP_SRCS  += Drivers/Sensors/TMP102/TMP102.cpp
CPP_SRCS  += Drivers/Sensors/ISL29033/ISL29033.cpp
CPP_SRCS  += Drivers/Sensors/LPS331/LPS331.cpp
CPP_SRCS  += Drivers/Sensors/TSL2561/TSL2561.cpp
CPP_SRCS  += Drivers/Sensors/MGC3130/MGC3130.cpp
CPP_SRCS  += Drivers/Sensors/BMP085/BMP085.cpp

CPP_SRCS  += Drivers/TLC5947/TLC5947.cpp
#CPP_SRCS  += Drivers/LSMD9S1/LSMD9S1.cpp
CPP_SRCS  += Drivers/DeviceWithRegisters/DeviceWithRegisters.cpp
CPP_SRCS  += Drivers/DeviceWithRegisters/DeviceRegister.cpp
CPP_SRCS  += Drivers/ManuvrableGPIO/ManuvrableGPIO.cpp

# Power-management ICs and modules...
#CPP_SRCS  += Drivers/PMIC/LDS8160/LDS8160.cpp

CPP_SRCS  += Platform/Identity.cpp
CPP_SRCS  += Platform/Identity/IdentityUUID.cpp
CPP_SRCS  += Platform/Identity/IdentityCrypto.cpp
CPP_SRCS  += Platform/Identity/IdentityOIC.cpp
CPP_SRCS  += Platform/Storage.cpp


OBJS  = $(CPP_SRCS:.cpp=.o)
LIBNAME = libmanuvr.a


all: $(LIBNAME)

%.o : %.cpp
	$(CXX) $(INCLUDES) $(CPP_FLAGS) -std=$(CPP_STANDARD) -nostdlib -c $< -o $@

$(LIBNAME): setenvars $(OBJS)
	$(MAKE) -C Platform/
	$(AR) -rvs $(OUTPUT_PATH)/$@ $(OBJS) Platform/*.o

clean:
	$(MAKE) clean -C Platform/
	rm -f $(OBJS)
	rm -f $(OUTPUT_PATH)/$(LIBNAME)


setenvars:
	@echo '======================================================'
	@echo '    __  ___                             ____  _____'
	@echo	'   /  |/  /___ _____  __  ___   _______/ __ \/ ___/'
	@echo	'  / /|_/ / __ `/ __ \/ / / / | / / ___/ / / /\__ \'
	@echo	' / /  / / /_/ / / / / /_/ /| |/ / /  / /_/ /___/ /'
	@echo	'/_/  /_/\__,_/_/ /_/\__,_/ |___/_/   \____//____/ '
	@echo ''
	@echo	'======================================================'


fullclean: clean

# If cppcheck is installed, we can do some simple static analysis. It is worth
#   noting that MANUVR_OPTIONS and ROOT_INCLUDES are not normally exported. They
#   are only valid here.
check:
	find ./ -type f \( -name \*.cpp -o -name \*.h \) -exec wc -l {} +
	cppcheck $(CPP_SRCS) $(ANALYZER_FLAGS)
