/*
File:   PlatformUnsupported.cpp
Author: J. Ian Lindsay
Date:   2015.11.01

Copyright 2016 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


This file forms the catch-all for linux platforms that have no support.
Fill this file with do-nothing stubs for functions such as GPIO.
*/

#include <Platform/Platform.h>

extern "C" {
  /*
  * Not provided elsewhere on a linux platform.
  */
  uint32_t micros() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000000L + ts.tv_nsec / 1000L);
  }

  void unsetPinIRQ(uint8_t pin) {}
  int8_t setPinEvent(uint8_t, uint8_t, ManuvrMsg*) {  return 0;  }
  int8_t setPinAnalog(uint8_t pin, int val) {              return 0;  }
  int readPinAnalog(uint8_t pin) {                         return -1; }

}   // extern "C"
