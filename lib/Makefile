# Build all libraries
C_STANDARD = c99

# Oh man.... this will be very messy.

INCLUDES    = -iquote. -I./paho.mqtt.embedded-c/MQTTPacket

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

SOURCES_CBOR_C  = tinycbor/src/cborerrorstrings.c
SOURCES_CBOR_C += tinycbor/src/cborencoder.c
SOURCES_CBOR_C += tinycbor/src/cborencoder_close_container_checked.c
SOURCES_CBOR_C += tinycbor/src/cborparser.c
SOURCES_CBOR_C += tinycbor/src/cborparser_dup_string.c
SOURCES_CBOR_C += tinycbor/src/cborpretty.c
SOURCES_CBOR_C += tinycbor/src/cbortojson.c

SOURCES_C  = $(SOURCES_MQTT_C) $(SOURCES_CBOR_C)
SOURCES_C += $(SOURCES_UECC_C)

FLAGS_LIBMQTT  =

LOCAL_C_FLAGS = -nostdlib $(FLAGS_LIBMQTT)


OBJS  = $(SOURCES_C:.c=.o)
LIBNAME = extraprotocols.a


all: $(LIBNAME)
	$(MAKE) -C mbedtls
	cp -RP mbedtls/library/libmbedtls.*    $(OUTPUT_PATH)
	cp -RP mbedtls/library/libmbedx509.*   $(OUTPUT_PATH)
	cp -RP mbedtls/library/libmbedcrypto.* $(OUTPUT_PATH)
#	$(MAKE) -C telehash-c

%.o : %.c
	$(CC) $(INCLUDES) $(CFLAGS) $(LOCAL_C_FLAGS) -std=$(C_STANDARD) -nostdlib -c $< -o $@


$(LIBNAME): $(OBJS)
	$(AR) -rvs $(OUTPUT_PATH)/$@ $(OBJS)


clean:
	rm -f $(OBJS)
	rm -f $(OUTPUT_PATH)/$(LIBNAME)
	$(MAKE) clean -C mbedtls
