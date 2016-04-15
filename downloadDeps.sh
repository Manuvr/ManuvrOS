#!/bin/bash
#
# This script is meant to go fetch the most recent versions of various libraries that
#   ManuvrOS has been written against. None of this is strictly required for a basic build,
#   but most real-world applications will want at least one of them.

# Make the lib directory...
mkdir lib

# JSON support via ArduinoJson...
git clone https://github.com/bblanchon/ArduinoJson.git lib/ArduinoJson

# FreeRTOS...
git clone https://github.com/greiman/FreeRTOS-Arduino lib/FreeRTOS_Arduino

# CoAP, if desired.
git clone https://github.com/eclipse/wakaama.git lib/wakaama

# MQTT, if desired.
git clone https://github.com/eclipse/paho.mqtt.embedded-c.git lib/paho.mqtt.embedded-c

# Telehash
git clone https://github.com/telehash/telehash-c.git lib/telehash-c

# Arduino libraries...

# Teensy loader...

# Teensyduino libraries...

# Return...
cd ..
