/*
File:   STM32F7.h
Author: J. Ian Lindsay
Date:   2016.05.26

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

#ifndef __PLATFORM_STM32F7_H__
#define __PLATFORM_STM32F7_H__

#include <stm32f7xx_hal.h>
#include <stm32f7xx_hal_rcc.h>
#include <stm32f7xx_hal_rtc.h>
#include <stm32f7xx_hal_rtc_ex.h>
#include <stm32f7xx_hal_gpio.h>
#include <stm32f7xx_hal_gpio_ex.h>


#define PLATFORM_GPIO_PIN_COUNT   48

  #define HIGH               GPIO_PIN_SET
  #define LOW                GPIO_PIN_RESET

  #define CHANGE             0xFC
  #define FALLING            0xFB
  #define RISING             0xFA
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

enum class GPIOMode {
  INPUT           = GPIO_MODE_INPUT,
  OUTPUT          = GPIO_MODE_OUTPUT_PP,
  OUTPUT_OD       = GPIO_MODE_OUTPUT_OD,
  BIDIR_OD,
  BIDIR_OD_PULLUP,
  INPUT_PULLUP,
  INPUT_PULLDOWN,
  ANALOG_OUT,
  ANALOG_IN,
  UNINIT
};


#define MANUVR_RTC_STARTUP_GOOD_UNSET  0xA40C131B
#define MANUVR_RTC_STARTUP_GOOD_SET    0x1529578F

/*
* External interrupt actions are stored this way in an array, with indecies
*   corresponding to the EXTI source.
* If both values are non-NULL, they will both be proc'd, with the function call
*   preceeding Msg insertion to the Kernel's ISR queue.
*/
typedef struct __platform_exti_def {
  ManuvrMsg* event;
  FxnPointer fxn;
  uint8_t         pin;
  uint8_t         condition;
} PlatformEXTIDef;



class STM32F7Platform : public ManuvrPlatform {
  public:
    inline  int8_t platformPreInit() {   return platformPreInit(nullptr); };
    virtual int8_t platformPreInit(Argument*);
    virtual void   printDebug(StringBuilder* out);

    /* Platform state-reset functions. */
    virtual void seppuku();           // Simple process termination. Reboots if not implemented.
    virtual void reboot();
    virtual void hardwareShutdown();
    virtual void jumpToBootloader();


  protected:
    virtual int8_t platformPostInit();

};


#endif  // __PLATFORM_STM32F7_H__
