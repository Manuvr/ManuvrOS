# Build all libraries
C_STANDARD   = c99
CPP_STANDARD = gnu++11

SOURCES_C      =
SOURCES_CPP    =
LIBS_TO_BUILD  =
LIBS_TO_COPY   = mbedtls/library/*.a
INCLUDES       = -iquote.

# Oh man.... this will be very messy.

INCLUDES   += -I./paho.mqtt.embedded-c/MQTTPacket
INCLUDES   += -I./jansson/src

#SOURCES_MQTT_C  = paho.mqtt.embedded-c/MQTTPacket/src/MQTTConnectServer.c
SOURCES_MQTT_C  = paho.mqtt.embedded-c/MQTTPacket/src/MQTTConnectClient.c
SOURCES_MQTT_C += paho.mqtt.embedded-c/MQTTPacket/src/MQTTDeserializePublish.c
SOURCES_MQTT_C += paho.mqtt.embedded-c/MQTTPacket/src/MQTTFormat.c
SOURCES_MQTT_C += paho.mqtt.embedded-c/MQTTPacket/src/MQTTPacket.c
SOURCES_MQTT_C += paho.mqtt.embedded-c/MQTTPacket/src/MQTTSerializePublish.c
SOURCES_MQTT_C += paho.mqtt.embedded-c/MQTTPacket/src/MQTTSubscribeClient.c
SOURCES_MQTT_C += paho.mqtt.embedded-c/MQTTPacket/src/MQTTSubscribeServer.c
SOURCES_MQTT_C += paho.mqtt.embedded-c/MQTTPacket/src/MQTTUnsubscribeClient.c
SOURCES_MQTT_C += paho.mqtt.embedded-c/MQTTPacket/src/MQTTUnsubscribeServer.c

SOURCES_CBOR  = cbor-cpp/src/encoder.cpp
SOURCES_CBOR += cbor-cpp/src/decoder.cpp
SOURCES_CBOR += cbor-cpp/src/input.cpp
SOURCES_CBOR += cbor-cpp/src/listener_debug.cpp
SOURCES_CBOR += cbor-cpp/src/output_dynamic.cpp
SOURCES_CBOR += cbor-cpp/src/output_static.cpp

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

SOURCES_C  += $(SOURCES_MQTT_C)
#SOURCES_C  += $(SOURCES_JSON)


#ifeq ($(SECURE),1)
LIBS_TO_BUILD += mbedtls
#LIBS_TO_COPY  += mbedtls/library/*.a
#endif

#ifdef $(MANUVR_OPENINTERCONNECT)
#LIBS_TO_BUILD += iotivity/port/linux
#endif

#ifdef $(MANUVR_CBOR)
SOURCES_CPP += $(SOURCES_CBOR)
#endif

FLAGS_LIBMQTT  =

LOCAL_C_FLAGS   = -nostdlib $(FLAGS_LIBMQTT) # $(JANNSON_FLAGS)
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
	$(MAKE) client -C iotivity/port/linux
	$(MAKE) -C $(LIBS_TO_BUILD)
	#$(MAKE) -C telehash-c

clean:
	rm -f $(OBJS)
	rm -f $(OUTPUT_PATH)/$(LIBNAME)
	$(MAKE) clean -C mbedtls
	$(MAKE) clean -C iotivity/port/linux
