###########################################################################
# Makefile for libraries
# Author: J. Ian Lindsay
###########################################################################

# Build all libraries
C_STANDARD   = gnu99
CPP_STANDARD = gnu++11

SOURCES_C      =
SOURCES_CPP    =
LIBS_TO_BUILD  =
LIBS_TO_COPY   = mbedtls/library/*.a
INCLUDES       = -iquote.

# Oh man.... this will be very messy.

INCLUDES   += -I./paho.mqtt.embedded-c/MQTTPacket
INCLUDES   += -I./jansson/src

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
SOURCES_JSON += jansson/src/strconv.c
SOURCES_JSON += jansson/src/error.c
SOURCES_JSON += jansson/src/hashtable.c
SOURCES_JSON += jansson/src/hashtable_seed.c
SOURCES_JSON += jansson/src/load.c
SOURCES_JSON += jansson/src/memory.c
SOURCES_JSON += jansson/src/pack_unpack.c
SOURCES_JSON += jansson/src/utf.c
SOURCES_JSON += jansson/src/value.c
SOURCES_JSON += jansson/src/dump.c


# Jansson private config won't be used unless we do this...
JANNSON_FLAGS = -DHAVE_CONFIG_H -I$(shell pwd)


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
	FLAGS_IOTIVITY += -DOC_SECURITY ${DTLSFLAGS}
	VPATH += ../../security/:../../deps/tinydtls/:../../deps/tinydtls/aes/:../../deps/tinydtls/sha2/:../../deps/tinydtls/ecc/:
	IOTIVITY_SRC += $(DTLS_SRC) $(wildcard iotivity/security/*.c)
	INCLUDES   += -I./iotivity/deps/tinydtls
endif



###########################################################################
# Parameter unification and make targets
###########################################################################
SOURCES_C  += $(SOURCES_MQTT_C)
SOURCES_C  += $(IOTIVITY_SRC)
#SOURCES_C  += $(SOURCES_JSON)

LIBS_TO_BUILD += mbedtls

LOCAL_C_FLAGS   = -nostdlib $(CFLAGS) $(FLAGS_LIBMQTT) $(FLAGS_IOTIVITY)
LOCAL_CXX_FLAGS = -nostdlib $(CPP_FLAGS)

OBJS  = $(SOURCES_C:.c=.o) $(SOURCES_CPP:.cpp=.o)
LIBNAME = libextras.a


all: $(LIBNAME)


%.o : %.c
	$(CC) $(INCLUDES) $(LOCAL_C_FLAGS) -std=$(C_STANDARD) -c $< -o $@

%.o : %.cpp
	$(CXX) $(INCLUDES) $(LOCAL_CXX_FLAGS) -std=$(CPP_STANDARD) -c $< -o $@

$(LIBNAME): externals $(OBJS)
	$(AR) -rvs $(OUTPUT_PATH)/$@ $(OBJS)
	cp -RP $(LIBS_TO_COPY) $(OUTPUT_PATH)


externals:
	$(MAKE) -C $(LIBS_TO_BUILD)
	#$(MAKE) -C telehash-c

clean:
	rm -f $(OBJS)
	rm -f $(OUTPUT_PATH)/$(LIBNAME)
	$(MAKE) clean -C mbedtls
