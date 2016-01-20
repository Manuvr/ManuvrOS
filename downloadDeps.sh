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
https://github.com/greiman/FreeRTOS-Arduino lib/FreeRTOS_Arduino

# Arduino libraries...

# Teensy loader...

# Teensyduino libraries...
