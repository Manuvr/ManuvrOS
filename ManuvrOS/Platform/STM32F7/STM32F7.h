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

#define MANUVR_RTC_STARTUP_GOOD_UNSET  0xA40C131B
#define MANUVR_RTC_STARTUP_GOOD_SET    0x1529578F

/*
* External interrupt actions are stored this way in an array, with indecies
*   corresponding to the EXTI source.
* If both values are non-NULL, they will both be proc'd, with the function call
*   preceeding Runnable insertion to the Kernel's ISR queue.
*/
typedef struct __platform_exti_def {
  ManuvrRunnable* event;
  FunctionPointer fxn;
  uint8_t         pin;
  uint8_t         condition;
} PlatformEXTIDef;


/*
* For the STM32 parts, we will treat pins as being the linear sequence of ports
*   and pins. This incurs a slight performance hit as we need to shift and divide
*   each time we call a pin by its 8-bit identifier. But this is the cost of
*   uniformity across platforms.
*/
GPIO_TypeDef* _port_assoc[] = {
  GPIOA,  GPIOB,  GPIOC,  GPIOD,  GPIOE,
  GPIOF,  GPIOG,  GPIOH,  GPIOI,  GPIOJ,
  GPIOK
};

inline GPIO_TypeDef* _associated_port(uint8_t _pin) {
  return _port_assoc[_pin >> 4];
}

inline uint16_t _associated_pin(uint8_t _pin) {
  return (1 << (_pin % 16));
}



class STM32F7Platform : public ManuvrPlatform {
  public:
    virtual int8_t platformPreInit();
    virtual int8_t platformPostInit();

    /* Platform state-reset functions. */
    virtual void seppuku();           // Simple process termination. Reboots if not implemented.
    virtual void reboot();
    virtual void hardwareShutdown();
    virtual void jumpToBootloader();

    virtual void printDebug(StringBuilder* out);


  protected:
};


#endif  // __PLATFORM_STM32F7_H__
