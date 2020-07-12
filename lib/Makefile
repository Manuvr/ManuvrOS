###########################################################################
# Makefile for libraries
# Author: J. Ian Lindsay
###########################################################################
C_STANDARD   = gnu99
CPP_STANDARD = gnu++11

SOURCES_C      =
SOURCES_CPP    =
LIBS_TO_BUILD  = libextras.a
INCLUDES       = -iquote.

# Oh man.... this will be very messy.

INCLUDES   += -I./paho.mqtt.embedded-c/MQTTPacket

# TODO: More makefile fixes required to avoid this...
INCLUDES   += -I.
INCLUDES   += -I./iotivity
INCLUDES   += -I./iotivity/include


###########################################################################
# Paho source and configuration.
SOURCES_MQTT_C  = paho.mqtt.embedded-c/MQTTPacket/src/MQTTConnectClient.c
SOURCES_MQTT_C += paho.mqtt.embedded-c/MQTTPacket/src/MQTTDeserializePublish.c
SOURCES_MQTT_C += paho.mqtt.embedded-c/MQTTPacket/src/MQTTFormat.c
SOURCES_MQTT_C += paho.mqtt.embedded-c/MQTTPacket/src/MQTTPacket.c
SOURCES_MQTT_C += paho.mqtt.embedded-c/MQTTPacket/src/MQTTSerializePublish.c
SOURCES_MQTT_C += paho.mqtt.embedded-c/MQTTPacket/src/MQTTSubscribeClient.c
SOURCES_MQTT_C += paho.mqtt.embedded-c/MQTTPacket/src/MQTTSubscribeServer.c
SOURCES_MQTT_C += paho.mqtt.embedded-c/MQTTPacket/src/MQTTUnsubscribeClient.c
SOURCES_MQTT_C += paho.mqtt.embedded-c/MQTTPacket/src/MQTTUnsubscribeServer.c
#SOURCES_MQTT_C += paho.mqtt.embedded-c/MQTTPacket/src/MQTTConnectServer.c

FLAGS_LIBMQTT  =


###########################################################################
# Jansson source and configuration.
SOURCES_JSON  = jansson/src/strbuffer.c
SOURCES_JSON += jansson/src/dump.c
SOURCES_JSON += jansson/src/error.c
SOURCES_JSON += jansson/src/strconv.c
SOURCES_JSON += jansson/src/hashtable.c
SOURCES_JSON += jansson/src/memory.c
SOURCES_JSON += jansson/src/pack_unpack.c
SOURCES_JSON += jansson/src/value.c
SOURCES_JSON += jansson/src/utf.c
SOURCES_JSON += jansson/src/load.c
SOURCES_JSON += jansson/src/hashtable_seed.c


###########################################################################
# CppPotpourri source and configuration.
SOURCES_CPPP  = CppPotpourri/src/BusQueue.cpp
SOURCES_CPPP += CppPotpourri/src/I2CAdapter.cpp
SOURCES_CPPP += CppPotpourri/src/I2CDevice.cpp
SOURCES_CPPP += CppPotpourri/src/Quaternion.cpp
SOURCES_CPPP += CppPotpourri/src/SensorFilter.cpp
SOURCES_CPPP += CppPotpourri/src/SensorWrapper.cpp
SOURCES_CPPP += CppPotpourri/src/SPIBusOp.cpp
SOURCES_CPPP += CppPotpourri/src/Storage.cpp
SOURCES_CPPP += CppPotpourri/src/TripleAxisCompass.cpp
SOURCES_CPPP += CppPotpourri/src/UARTAdapter.cpp
SOURCES_CPPP += CppPotpourri/src/WakeLock.cpp
SOURCES_CPPP += CppPotpourri/src/GPSWrapper.cpp
SOURCES_CPPP += CppPotpourri/src/I2CBusOp.cpp
SOURCES_CPPP += CppPotpourri/src/ParsingConsole.cpp
SOURCES_CPPP += CppPotpourri/src/SensorDatum.cpp
SOURCES_CPPP += CppPotpourri/src/SensorManager.cpp
SOURCES_CPPP += CppPotpourri/src/SPIAdapter.cpp
SOURCES_CPPP += CppPotpourri/src/StopWatch.cpp
SOURCES_CPPP += CppPotpourri/src/StringBuilder.cpp
SOURCES_CPPP += CppPotpourri/src/TripleAxisPipe.cpp
SOURCES_CPPP += CppPotpourri/src/uuid.cpp
SOURCES_CPPP += CppPotpourri/src/cbor-cpp/cbor.cpp
SOURCES_CPPP += CppPotpourri/src/Image/Image.cpp

###########################################################################
# iotivity-constrained.
# TODO: Much redundant. Refactor to use our native CBOR and MBED.
DTLS_SRC      = iotivity/deps/tinydtls/ccm.c
DTLS_SRC     += iotivity/deps/tinydtls/hmac.c
DTLS_SRC     += iotivity/deps/tinydtls/netq.c
DTLS_SRC     += iotivity/deps/tinydtls/peer.c
DTLS_SRC     += iotivity/deps/tinydtls/dtls_time.c
DTLS_SRC     += iotivity/deps/tinydtls/session.c
DTLS_SRC     += iotivity/deps/tinydtls/sha2/sha2.c
DTLS_SRC     += iotivity/deps/tinydtls/ecc/ecc.c
DTLS_SRC     += iotivity/deps/tinydtls/aes/rijndael.c
DTLS_SRC     += iotivity/deps/tinydtls/crypto.c
DTLS_SRC     += iotivity/deps/tinydtls/dtls.c

CBOR_SRC      = iotivity/deps/tinycbor/src/cborencoder.c
CBOR_SRC     += iotivity/deps/tinycbor/src/cborencoder_close_container_checked.c
CBOR_SRC     += iotivity/deps/tinycbor/src/cborparser.c

DTLSFLAGS = -DDTLSV12 -DWITH_SHA256 -DDTLS_CHECK_CONTENTTYPE -DWITH_OCF -I../../deps/tinydtls -DNDEBUG

IOTIVITY_SRC  = $(wildcard iotivity/util/*.c) ${CBOR_SRC}
IOTIVITY_SRC += $(wildcard iotivity/messaging/coap/*.c iotivity/api/*.c)
IOTIVITY_SRC += iotivity/port/linux/ipadapter.c
VPATH = iotivity/messaging/coap/:iotivity/util/:iotivity/api/:iotivity/deps/tinycbor/src/:iotivity/include/


###########################################################################
# Option conditionals
###########################################################################
ifeq ($(SECURE),1)
	ifeq ($(CONFIG_WITH_IOTIVITY),1)
		FLAGS_IOTIVITY += -DOC_SECURITY ${DTLSFLAGS}
		VPATH += ../../security/:../../deps/tinydtls/:../../deps/tinydtls/aes/:../../deps/tinydtls/sha2/:../../deps/tinydtls/ecc/:
		IOTIVITY_SRC += $(DTLS_SRC) $(wildcard iotivity/security/*.c)
		INCLUDES   += -I./iotivity/deps/tinydtls
		SOURCES_C  += $(IOTIVITY_SRC)
	endif
	LIBS_TO_BUILD += mbedtls.a
endif

ifeq ($(JSON),1)
# Jansson private config won't be used unless we do this...
	INCLUDES      += -I./jansson/src
	LOCAL_C_FLAGS += -DHAVE_CONFIG_H
	SOURCES_C     += $(SOURCES_JSON)
endif


###########################################################################
# Parameter unification and make targets
###########################################################################
SOURCES_C  += $(SOURCES_MQTT_C) $(SOURCES_CPPP)

LOCAL_C_FLAGS   += $(INCLUDES) -nostdlib $(CFLAGS) $(FLAGS_LIBMQTT) $(FLAGS_IOTIVITY)
LOCAL_CXX_FLAGS += $(INCLUDES) $(CXXFLAGS)

OBJS  = $(SOURCES_C:.c=.o) $(SOURCES_CPP:.cpp=.o)


all: $(LIBS_TO_BUILD)

%.o : %.c
	$(CC) $(LOCAL_C_FLAGS) -std=$(C_STANDARD) -c $< -o $@

%.o : %.cpp
	$(CXX) $(LOCAL_CXX_FLAGS) -std=$(CPP_STANDARD) -c $< -o $@

libextras.a: $(OBJS)
	$(AR) -rvs $(OUTPUT_PATH)/libextras.a $(OBJS)

mbedtls.a:
	$(MAKE) lib -C mbedtls/
	cp -RP mbedtls/library/*.a $(OUTPUT_PATH)

clean:
	rm -f $(OBJS) $(OUTPUT_PATH)/libextras.a
	$(MAKE) clean -C mbedtls/
