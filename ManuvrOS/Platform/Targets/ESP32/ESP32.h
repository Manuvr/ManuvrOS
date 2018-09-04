/*
File:   ESP32.h
Author: J. Ian Lindsay
Date:   2016.08.31

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

*/

#ifndef __PLATFORM_ESP32_H__
#define __PLATFORM_ESP32_H__

#include "driver/gpio.h"
#include "esp_system.h"

#define PLATFORM_GPIO_PIN_COUNT GPIO_PIN_COUNT

enum class GPIOMode : uint8_t {
  INPUT           = GPIO_MODE_INPUT,
  OUTPUT          = GPIO_MODE_OUTPUT,
  OUTPUT_OD       = GPIO_MODE_OUTPUT_OD,
  BIDIR_OD        = GPIO_MODE_INPUT_OUTPUT_OD,
  BIDIR_OD_PULLUP,
  INPUT_PULLUP,
  INPUT_PULLDOWN,
  ANALOG_OUT,
  ANALOG_IN,
  UNINIT
};

#define HIGH               1
#define LOW                0

#define CHANGE             GPIO_INTR_ANYEDGE
#define FALLING            GPIO_INTR_NEGEDGE
#define RISING             GPIO_INTR_POSEDGE
#define CHANGE_PULL_UP     0xF9
#define FALLING_PULL_UP    0xF8
#define RISING_PULL_UP     0xF7
#define CHANGE_PULL_DOWN   0xF6
#define FALLING_PULL_DOWN  0xF5
#define RISING_PULL_DOWN   0xF4
extern "C" {
  unsigned long millis();
  unsigned long micros();
}



class ESP32Platform : public ManuvrPlatform {
  public:
    ESP32Platform() : ManuvrPlatform("ESP32") {};

    inline  int8_t platformPreInit() {   return platformPreInit(nullptr); };
    virtual int8_t platformPreInit(Argument*);
    virtual void   printDebug(StringBuilder* out);

    /* Platform state-reset functions. */
    void seppuku();           // Simple process termination. Reboots if not implemented.
    void reboot();
    void hardwareShutdown();
    void jumpToBootloader();

    uint32_t cpu_freq() {
      return (CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ * 1000000);
    };


  protected:
    virtual int8_t platformPostInit();
    #if defined(MANUVR_STORAGE)
      // Called during boot to load configuration.
      int8_t _load_config();
    #endif
};

#endif  // __PLATFORM_ESP32_H__
