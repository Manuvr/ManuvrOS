# Build all libraries
C_STANDARD = c99

# Oh man.... this will be very messy.

INCLUDES    = -iquote. -I./paho.mqtt.embedded-c/MQTTPacket/src
INCLUDES   += -I./wakaama -I./wakaama/core

SOURCES_MQTT_C  = paho.mqtt.embedded-c/MQTTPacket/src/MQTTConnectServer.c
SOURCES_MQTT_C += paho.mqtt.embedded-c/MQTTPacket/src/MQTTDeserializePublish.c
SOURCES_MQTT_C += paho.mqtt.embedded-c/MQTTPacket/src/MQTTFormat.c
SOURCES_MQTT_C += paho.mqtt.embedded-c/MQTTPacket/src/MQTTPacket.c
SOURCES_MQTT_C += paho.mqtt.embedded-c/MQTTPacket/src/MQTTSerializePublish.c
SOURCES_MQTT_C += paho.mqtt.embedded-c/MQTTPacket/src/MQTTSubscribeClient.c
SOURCES_MQTT_C += paho.mqtt.embedded-c/MQTTPacket/src/MQTTSubscribeServer.c
SOURCES_MQTT_C += paho.mqtt.embedded-c/MQTTPacket/src/MQTTUnsubscribeClient.c
SOURCES_MQTT_C += paho.mqtt.embedded-c/MQTTPacket/src/MQTTUnsubscribeServer.c

SOURCES_COAP_C  = wakaama/core/er-coap-13/er-coap-13.c

SOURCES_C  = $(SOURCES_MQTT_C) $(SOURCES_COAP_C)

#FLAGS_LIBCOAP  = -DWITH_POSIX -DHAVE_CONFIG_H -DHAVE_NETINET_IN_H -D_GNU_SOURCE
FLAGS_LIBMQTT  =

#LOCAL_C_FLAGS = $(FLAGS_LIBCOAP)
LOCAL_C_FLAGS = -nostdlib $(FLAGS_LIBMQTT)


OBJS  = $(SOURCES_C:.c=.o)
LIBNAME = extraprotocols.a


all: $(LIBNAME)
#	$(MAKE) -C telehash-c

%.o : %.c
	$(CC) $(INCLUDES) $(CFLAGS) $(LOCAL_C_FLAGS) -std=$(C_STANDARD) -nostdlib -c $< -o $@


$(LIBNAME): $(OBJS)
	$(AR) -rvs $(OUTPUT_PATH)/$@ $(OBJS)


clean:
	rm -f $(OBJS)
	rm -f $(OUTPUT_PATH)/$(LIBNAME)
