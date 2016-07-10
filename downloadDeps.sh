#!/bin/bash
#
# This script is meant to go fetch the most recent versions of various libraries that
#   ManuvrOS has been written against. None of this is strictly required for a basic build,
#   but most real-world applications will want at least one of them.

# Make the lib directory...
mkdir lib

# JSON support via ArduinoJson...
rm -rf lib/ArduinoJson
git clone https://github.com/bblanchon/ArduinoJson.git lib/ArduinoJson

# FreeRTOS...
rm -rf lib/FreeRTOS_Arduino
git clone https://github.com/greiman/FreeRTOS-Arduino lib/FreeRTOS_Arduino

# CoAP, if desired.
rm -rf lib/wakaama
git clone https://github.com/eclipse/wakaama.git lib/wakaama

# MQTT, if desired.
# Note that we do special-handling here to make the build-process smoother...
rm -rf lib/paho.mqtt.embedded-c
git clone https://github.com/eclipse/paho.mqtt.embedded-c.git lib/paho.mqtt.embedded-c
cp lib/paho.mqtt.embedded-c/MQTTPacket/src/* lib/paho.mqtt.embedded-c/

# TinyCBOR...
rm -rf lib/tinycbor
git clone https://github.com/01org/tinycbor.git lib/tinycbor

# mbedTLS...
rm -rf lib/mbedtls
git clone https://github.com/ARMmbed/mbedtls.git lib/mbedtls

# Telehash
rm -rf lib/telehash-c
git clone https://github.com/telehash/telehash-c.git lib/telehash-c

# Elliptic curve crypto...
#rm -rf lib/uECC
#git clone https://github.com/kmackay/micro-ecc.git lib/uECC

# Arduino libraries...

# Teensy loader...

# Teensyduino libraries...

# Return...
cd ..
