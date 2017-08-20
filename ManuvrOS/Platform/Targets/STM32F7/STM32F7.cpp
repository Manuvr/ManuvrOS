/*
File:   STM32F7.cpp
Author: J. Ian Lindsay
Date:   2015.12.02

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


This file is meant to contain a set of common functions that are typically platform-dependent.
  The goal is to make a class instance that is pre-processor-selectable for instantiation by the
  kernel, thereby giving the kernel the ability to...
    * Access the realtime clock (if applicatble)
    * Get definitions for GPIO pins.
    * Access a true RNG (if it exists)
*/

#include <Platform/Platform.h>
#include "STM32F7.h"

#if defined(MANUVR_STORAGE)
// No storage support yet.
// RTC backup ram?
#endif

#include <unistd.h>

#if defined (__BUILD_HAS_FREERTOS)
  #include <FreeRTOS.h>
#endif


/****************************************************************************************************
* The code under this block is special on this platform, and will not be available elsewhere.       *
****************************************************************************************************/
RTC_HandleTypeDef rtc;


/*
* For the STM32 parts, we will treat pins as being the linear sequence of ports
*   and pins. This incurs a slight performance hit as we need to shift and divide
*   each time we call a pin by its 8-bit identifier. But this is the cost of
*   uniformity across platforms.
*/
static GPIO_TypeDef* _port_assoc[] = {
  GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF,
  GPIOG, GPIOH, GPIOI, GPIOJ, GPIOK
};

inline GPIO_TypeDef* _associated_port(uint8_t _pin) {
  return _port_assoc[_pin >> 4];
};
inline uint16_t _associated_pin(uint8_t _pin) {
  return (1 << (_pin % 16));
};


volatile uint32_t     randomness_pool[PLATFORM_RNG_CARRY_CAPACITY];
volatile unsigned int _random_pool_r_ptr = 0;
volatile unsigned int _random_pool_w_ptr = 0;

volatile uint32_t     millis_since_reset = 1;   // Start at one because WWDG.
volatile uint8_t      watchdog_mark      = 42;

volatile static PlatformEXTIDef __ext_line_bindings[16];

/* Takes a pin and returns the IRQ channel. */
IRQn_Type _get_stm32f7_irq_channel(uint8_t _pin) {
  switch (_pin % 16) {
    case 0:  return EXTI0_IRQn;
    case 1:  return EXTI1_IRQn;
    case 2:  return EXTI2_IRQn;
    case 3:  return EXTI3_IRQn;
    case 4:  return EXTI4_IRQn;
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:  return EXTI9_5_IRQn;
    default: return EXTI15_10_IRQn;
  }
}


void printEXTIDef(uint8_t _pin, StringBuilder* output) {
  uint16_t pin_idx   = (_pin % 16);
  const char* _msg_name = "<NONE>";
  if (nullptr != __ext_line_bindings[pin_idx].event) {
    _msg_name = ManuvrMsg::getMsgTypeString(__ext_line_bindings[pin_idx].event->eventCode());
  }
  output->concatf("\t---< EXTI Def %d >----------\n", pin_idx);
  output->concatf("\tPin         %d\n", __ext_line_bindings[pin_idx].pin);
  output->concatf("\tCondition   %d\n", ManuvrPlatform::getIRQConditionString(__ext_line_bindings[pin_idx].condition));
  output->concatf("\tFXN         %p\n", __ext_line_bindings[pin_idx].fxn);
  output->concatf("\tEvent       %s\n\n", _msg_name);
}


/*
* Trivial, since the period of systick is 1ms, and we are already
*   keeping track of how many times it has rolled over. Simply return
*   that value.
* Returns the number of milliseconds since reset.
*/
unsigned long millis(void) {  return millis_since_reset;  }


#define MCK 16000000
/**
* Written by Magnus Lundin.
* https://github.com/mlu/arduino-stm32/blob/master/hardware/cores/stm32/wiring.c
*
* @return  unsigned long  The number of microseconds since reset.
*/
unsigned long micros(void) {
  long v0 = SysTick->VAL;      // Glitch free clock
  long c0 = millis_since_reset;
  long v1 = SysTick->VAL;
  long c1 = millis_since_reset;
  if (v1 < v0) {             // Downcounting, no systick rollover
    return c0*8000-v1/(MCK/8000000UL);
    //return (unsigned long) millis_since_reset/1000000;
  }
  else { // systick rollover, use last count value
    return c1*8000-v1/(MCK/8000000UL);
    //return (unsigned long) millis_since_reset/1000000;
  }
  return 0;
}




/*******************************************************************************
* Randomness                                                                   *
*******************************************************************************/
/**
* Dead-simple interface to the RNG. Despite the fact that it is interrupt-driven, we may resort
*   to polling if random demand exceeds random supply. So this may block until a random number
*   is actually availible (randomness_pool != 0).
*
* @return   A 32-bit unsigned random number. This can be cast as needed.
*/
uint32_t randomInt() {
  uint32_t enablement_mask = (RNG_CR_RNGEN | RNG_CR_IE);
  // Preferably, we'd shunt to a PRNG at this point. For now we block.
  while (_random_pool_w_ptr <= _random_pool_r_ptr) {
    if ((RNG->CR & enablement_mask) != enablement_mask) {
      // As long as we are going to block, prevent lock-ups due to possible
      //   concurrency mis-alignments.
      RNG->CR |= enablement_mask;
    }
    if (RNG->SR & RNG_SR_DRDY) {
      // And just-in-case we have interrupts disabled for some reason.
      return RNG->DR;
    }
  }
  return randomness_pool[_random_pool_r_ptr++ % PLATFORM_RNG_CARRY_CAPACITY];
}

/**
* Called by the RNG ISR to provide new random numbers.
*
* @param    nu_rnd The supplied random number.
* @return   True if the RNG should continue supplying us, false if it should take a break until we need more.
*/
volatile bool provide_random_int(uint32_t nu_rnd) {
  for (uint8_t i = 0; i < PLATFORM_RNG_CARRY_CAPACITY; i++) {
    if (randomness_pool[i] == 0) {
      randomness_pool[i] = nu_rnd;
      return (i == PLATFORM_RNG_CARRY_CAPACITY-1) ? false : true;
    }
  }
  return false;
}

/*
* Init the RNG. Short and sweet.
*/
void init_RNG() {
  for (uint8_t i = 0; i < PLATFORM_RNG_CARRY_CAPACITY; i++) randomness_pool[i] = 0;
  __HAL_RCC_RNG_CLK_ENABLE();
  RNG->CR |= (RNG_CR_RNGEN | RNG_CR_IE);
  HAL_NVIC_SetPriority(RNG_IRQn, 1, 0);  // IRQ channel, priority, pin idex.
  HAL_NVIC_EnableIRQ(RNG_IRQn);
}


/****************************************************************************************************
* Identity and serial number                                                                        *
****************************************************************************************************/
void STM32F7Platform::printDebug(StringBuilder* output) {
  output->concatf("==< STM32F7 [%s] >=================================\n", getPlatformStateStr(platformState()));
  ManuvrPlatform::printDebug(output);
  // TODO: GPIO
}




/**
* We sometimes need to know the length of the platform's unique identifier (if any). If this platform
*   is not serialized, this function will return zero.
*
* @return   The length of the serial number on this platform, in terms of bytes.
*/
int platformSerialNumberSize() { return 12; }


/**
* Writes the serial number to the indicated buffer.
*
* @param    A pointer to the target buffer.
* @return   The number of bytes written.
*/
int getSerialNumber(uint8_t* buf) {
  *((uint32_t*)buf + 0) = *((uint32_t*) 0x1FF0F420);
  *((uint32_t*)buf + 4) = *((uint32_t*) 0x1FF0F424);
  *((uint32_t*)buf + 8) = *((uint32_t*) 0x1FF0F428);
  return 12;
}


/*******************************************************************************
* Time and date                                                                *
*******************************************************************************/
/*
* Setup the realtime clock module.
* Informed by code here:
* http://autoquad.googlecode.com/svn/trunk/onboard/rtc.c
*/
bool initPlatformRTC() {
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();        /* Allow access to BKP Domain */

    RCC_OscInitTypeDef RCC_OscInitStruct;
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInitStruct.RTCClockSelection    = RCC_RTCCLKSOURCE_LSE;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_NONE;
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE;
    RCC_OscInitStruct.LSEState       = RCC_LSE_ON;

    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
    __HAL_RCC_RTC_ENABLE();     /* Enable the RTC Clock */

    /* Configure the RTC data register and RTC prescaler */
    /* ck_spre(1Hz) = RTCCLK(LSE) /(uwAsynchPrediv + 1)*(uwSynchPrediv + 1)*/
    rtc.Init.AsynchPrediv   = 0x1F;
    rtc.Init.SynchPrediv    = 0x03FF;
    rtc.Init.HourFormat     = RTC_HOURFORMAT_24;
    rtc.Init.OutPut         = RTC_OUTPUT_DISABLE;
    rtc.Init.OutPutType     = RTC_OUTPUT_TYPE_PUSHPULL;
    rtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;

    RTC_DateTypeDef RTC_DateStructure;
    RTC_TimeTypeDef RTC_TimeStructure;
    uint32_t rtc_startup_state = HAL_RTCEx_BKUPRead(&rtc, RTC_BKP_DR31);

    switch (rtc_startup_state) {
      case MANUVR_RTC_STARTUP_GOOD_SET:
      case MANUVR_RTC_STARTUP_GOOD_UNSET:
        /* Wait for RTC APB register's sync. */
        if (HAL_OK == HAL_RTC_WaitForSynchro(&rtc)) {

        }
        HAL_RTC_GetTime(&rtc, &RTC_TimeStructure, RTC_FORMAT_BCD);
        HAL_RTC_GetDate(&rtc, &RTC_DateStructure, RTC_FORMAT_BCD);
        __HAL_RCC_CLEAR_RESET_FLAGS();
        break;
      default:
        /* Set the initial date, */
        // TODO: Do this from build datetime.
        RTC_DateStructure.Year    = 16;
        RTC_DateStructure.Month   = RTC_MONTH_JUNE;
        RTC_DateStructure.Date    = 8;
        RTC_DateStructure.WeekDay = RTC_WEEKDAY_WEDNESDAY;
        HAL_RTC_SetDate(&rtc, &RTC_DateStructure, RTC_FORMAT_BCD);
        /* Set the initial time. */
        RTC_TimeStructure.TimeFormat = RTC_HOURFORMAT12_AM;
        RTC_TimeStructure.Hours      = 0;
        RTC_TimeStructure.Minutes    = 0;
        RTC_TimeStructure.Seconds    = 0;
        HAL_RTC_SetTime(&rtc, &RTC_TimeStructure, RTC_FORMAT_BCD);
        HAL_RTC_Init(&rtc);
        HAL_RTCEx_BKUPWrite(&rtc, RTC_BKP_DR31, MANUVR_RTC_STARTUP_GOOD_UNSET);
        break;
    }
  return true;
}

/*
*/
bool setTimeAndDate(uint8_t y, uint8_t m, uint8_t d, uint8_t wd, uint8_t h, uint8_t mi, uint8_t s) {
  //RTC_WriteProtectionCmd(DISABLE);
  RTC_EnterInitMode(&rtc);
  RTC_DateTypeDef RTC_DateStructure;
  RTC_TimeTypeDef RTC_TimeStructure;

  /* Set the date: Friday January 11th 2013 */
  RTC_DateStructure.Year    = y;
  RTC_DateStructure.Month   = m;
  RTC_DateStructure.Date    = d;
  RTC_DateStructure.WeekDay = wd;
  HAL_RTC_SetDate(&rtc, &RTC_DateStructure, RTC_FORMAT_BCD);

  /* Set the time to 05h 20mn 00s AM */
  RTC_TimeStructure.TimeFormat = RTC_HOURFORMAT12_AM;
  RTC_TimeStructure.Hours      = h;
  RTC_TimeStructure.Minutes    = mi;
  RTC_TimeStructure.Seconds    = s;
  HAL_RTC_SetTime(&rtc, &RTC_TimeStructure, RTC_FORMAT_BCD);

  /* Let's us determine that we've already setup the peripheral on next run... */
  //RTC_WriteBackupRegister(RTC_BKP_DR0, MANUVR_RTC_STARTUP_GOOD_SET);
  //RTC_WriteProtectionCmd(ENABLE);
  //RTC_ExitInitMode();
  return true;
}


/*
* Given an RFC2822 datetime string, decompose it and set the time and date.
* We would prefer RFC2822, but we should try and cope with things like missing
*   time or timezone.
* Returns false if the date failed to set. True if it did.
*/
bool setTimeAndDateStr(char* nu_date_time) {
  return false;
}


/*
* Returns an integer representing the current datetime.
*/
uint32_t epochTime(void) {
  return 0;
}


/*
* Writes a human-readable datetime to the argument.
* Returns ISO 8601 datetime string.
* 2004-02-12T15:19:21+00:00
*/
void currentDateTime(StringBuilder* target) {
  if (target != nullptr) {
    RTC_TimeTypeDef RTC_TimeStructure;
    RTC_DateTypeDef RTC_DateStructure;

    HAL_RTC_GetTime(&rtc, &RTC_TimeStructure, RTC_FORMAT_BCD);
    HAL_RTC_GetDate(&rtc, &RTC_DateStructure, RTC_FORMAT_BCD);

    target->concatf("%d-%d-%dT", RTC_DateStructure.Year, RTC_DateStructure.Month, RTC_DateStructure.Date);
    target->concatf("%d:%d:%d+00:00", RTC_TimeStructure.Hours, RTC_TimeStructure.Minutes, RTC_TimeStructure.Seconds);
  }
}



/*******************************************************************************
* GPIO and change-notice                                                       *
*******************************************************************************/

/*
* This fxn should be called once on boot to setup the CPU pins that are not claimed
*   by other classes. GPIO pins at the command of this-or-that class should be setup
*   in the class that deals with them.
* Pending peripheral-level init of pins, we should just enable everything and let
*   individual classes work out their own requirements.
*/
void gpioSetup() {
  // Null-out all the pin definitions in preparation for assignment.
  for (uint8_t i = 0; i < 16; i++) {
    /* Zero all the EXTI definitions */
    __ext_line_bindings[i].event     = 0;
    __ext_line_bindings[i].fxn       = 0;
    __ext_line_bindings[i].condition = 0;
    __ext_line_bindings[i].pin       = 0;
  }

  /* GPIO Ports Clock Enable */
  //__GPIOE_CLK_ENABLE();
  //__GPIOB_CLK_ENABLE();
  //__GPIOG_CLK_ENABLE();
  //__GPIOD_CLK_ENABLE();
  //__GPIOC_CLK_ENABLE();
  //__GPIOA_CLK_ENABLE();
  //__GPIOI_CLK_ENABLE();
  //__GPIOH_CLK_ENABLE();
  //__GPIOF_CLK_ENABLE();
}



/**
* On STM32 parts, we construe an 8-bit pin number to be a linear reference to
*   successive 16-bit ports. See STM32F7.h for clarification.
*/
int8_t gpioDefine(uint8_t pin, int mode) {
  //if (pin < PLATFORM_GPIO_PIN_COUNT) {
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin    = _associated_pin(pin);
    GPIO_InitStruct.Speed  = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Pull   = GPIO_NOPULL;
    GPIO_InitStruct.Mode   = GPIO_MODE_INPUT;

    switch (mode) {
      case INPUT_PULLUP:
        GPIO_InitStruct.Pull   = GPIO_PULLUP;
        break;
      case INPUT_PULLDOWN:
        GPIO_InitStruct.Pull   = GPIO_PULLDOWN;
        break;
      case OUTPUT:
        GPIO_InitStruct.Mode   = GPIO_MODE_OUTPUT_PP;
        break;
      case OUTPUT_OD:
        GPIO_InitStruct.Mode   = GPIO_MODE_OUTPUT_OD;
        break;
      case INPUT:
        // Nothing needs to happen. This is the default we set above.
        break;
      default:
        Kernel::log("Tried to define a GPIO pin with a mode that we don't handle.\n");
        GPIO_InitStruct.Mode   = GPIO_MODE_ANALOG;
        break;
    }

    HAL_GPIO_Init(_associated_port(pin), &GPIO_InitStruct);
  //}
  //else {
    //Kernel::log("Tried to define a GPIO pin that was out-of-range for this platform.\n");
  //}
  return 0;
}


void unsetPinIRQ(uint8_t pin) {
}


int8_t setPinEvent(uint8_t _pin, uint8_t condition, ManuvrMsg* isr_event) {
  uint16_t pin_idx   = (_pin % 16);

  if (0 == __ext_line_bindings[pin_idx].condition) {
    /* There is not presently an interrupt condition on this pin.
       Proceed with impunity.  */
    __ext_line_bindings[pin_idx].pin       = _pin;
    __ext_line_bindings[pin_idx].condition = condition;
    __ext_line_bindings[pin_idx].event     = isr_event;

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin    = _associated_pin(_pin);
    GPIO_InitStruct.Speed  = GPIO_SPEED_LOW;
    GPIO_InitStruct.Pull   = GPIO_NOPULL;

    switch (condition) {
      case RISING:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
        break;
      case FALLING:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
        break;
      case CHANGE:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
        break;
      case RISING_PULL_UP:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        break;
      case FALLING_PULL_UP:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        break;
      case CHANGE_PULL_UP:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        break;
      case RISING_PULL_DOWN:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        break;
      case FALLING_PULL_DOWN:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        break;
      case CHANGE_PULL_DOWN:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        break;
      default:
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        break;
    }
    HAL_GPIO_Init(_associated_port(_pin), &GPIO_InitStruct);
    IRQn_Type chan = _get_stm32f7_irq_channel(_pin);
    HAL_NVIC_SetPriority(chan, 0, pin_idx);  // IRQ channel, priority, pin idex.
    HAL_NVIC_EnableIRQ(chan);
  }
  else if (nullptr == __ext_line_bindings[pin_idx].event) {
    /* No event already occupies the slot, but the def is active. Probably
         already has a FXN assigned. Ignore the supplied condition and add this
         event to the list of things to do. */
    __ext_line_bindings[pin_idx].event     = isr_event;
  }
  else if (__ext_line_bindings[pin_idx].event != isr_event) {
    StringBuilder local_log("Tried to clobber an existing IRQ event association...\n");
    printEXTIDef(_pin, &local_log);
    Kernel::log(&local_log);
    return -1;
  }
  return 0;
}


/*
* Pass the function pointer
*/
int8_t setPinFxn(uint8_t _pin, uint8_t condition, FxnPointer fxn) {
  uint16_t pin_idx   = (_pin % 16);

  if (0 == __ext_line_bindings[pin_idx].condition) {
    /* There is not presently an interrupt condition on this pin.
       Proceed with impunity.  */
    __ext_line_bindings[pin_idx].pin       = _pin;
    __ext_line_bindings[pin_idx].condition = condition;
    __ext_line_bindings[pin_idx].fxn       = fxn;

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin    = _associated_pin(_pin);
    GPIO_InitStruct.Speed  = GPIO_SPEED_LOW;
    GPIO_InitStruct.Pull   = GPIO_NOPULL;

    switch (condition) {
      case RISING:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
        break;
      case FALLING:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
        break;
      case CHANGE:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
        break;
      case RISING_PULL_UP:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        break;
      case FALLING_PULL_UP:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        break;
      case CHANGE_PULL_UP:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        break;
      case RISING_PULL_DOWN:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        break;
      case FALLING_PULL_DOWN:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        break;
      case CHANGE_PULL_DOWN:
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        break;
      default:
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        break;
    }
    HAL_GPIO_Init(_associated_port(_pin), &GPIO_InitStruct);
    IRQn_Type chan = _get_stm32f7_irq_channel(_pin);
    HAL_NVIC_SetPriority(chan, 1, pin_idx);  // IRQ channel, priority, pin idex.
    HAL_NVIC_EnableIRQ(chan);
  }
  else if (nullptr == __ext_line_bindings[pin_idx].fxn) {
    /* No fxn already occupies the slot, but the def is active. Probably
         already has an event assigned. Ignore the supplied condition and add this
         fxn to the list of things to do. */
    __ext_line_bindings[pin_idx].fxn = fxn;
  }
  else if (__ext_line_bindings[pin_idx].fxn != fxn) {
    StringBuilder local_log("Tried to clobber an existing IRQ event association...\n");
    printEXTIDef(_pin, &local_log);
    Kernel::log(&local_log);
    return -1;
  }
  return 0;
}


int8_t setPin(uint8_t pin, bool val) {
  HAL_GPIO_WritePin(_associated_port(pin), _associated_pin(pin), (val?GPIO_PIN_SET:GPIO_PIN_RESET));
  return 0;
}


int8_t readPin(uint8_t pin) {
  return (GPIO_PIN_SET == HAL_GPIO_ReadPin(_associated_port(pin), _associated_pin(pin)));
}


int8_t setPinAnalog(uint8_t pin, int val) {
  return 0;
}

int readPinAnalog(uint8_t pin) {
  return -1;
}


/*******************************************************************************
* Persistent configuration                                                     *
*******************************************************************************/

/*******************************************************************************
* Interrupt-masking                                                            *
*******************************************************************************/
void globalIRQEnable() {     asm volatile ("cpsid i");    }
void globalIRQDisable() {    asm volatile ("cpsie i");    }


/*******************************************************************************
* Process control                                                              *
*******************************************************************************/

/*
* Terminate this running process, along with any children it may have forked() off.
* Never returns.
*/
void STM32F7Platform::seppuku() {
  // This means "Halt" on a base-metal build.
  globalIRQDisable();
  HAL_RCC_DeInit();               // Switch to HSI, no PLL
  while(true);
}


/*
* Jump to the bootloader.
* Never returns.
*/
void STM32F7Platform::jumpToBootloader() {
  globalIRQDisable();
  __set_MSP(0x20001000);      // Set the main stack pointer...
  HAL_RCC_DeInit();               // Switch to HSI, no PLL

  // Per clive1's post, set some sort of key value just below the initial stack pointer.
  // We don't really care if we clobber something, because this fxn will reboot us. But
  // when the reset handler is executed, it will look for this value. If it finds it, it
  // will branch to the Bootloader code.
  *((unsigned long *)0x20001FF0) = 0xb00710ad;
  NVIC_SystemReset();
}


/*******************************************************************************
* Underlying system control.                                                   *
*******************************************************************************/

/*
* This means "Halt" on a base-metal build.
* Never returns.
*/
void STM32F7Platform::hardwareShutdown() {
  globalIRQDisable();
  NVIC_SystemReset();
}

/*
* Causes immediate reboot.
* Never returns.
*/
void STM32F7Platform::reboot() {
  globalIRQDisable();
  __set_MSP(0x20001000);      // Set the main stack pointer...
  HAL_RCC_DeInit();               // Switch to HSI, no PLL

  // We do not want to enter the bootloader....
  *((unsigned long *)0x2000FFF0) = 0;
  NVIC_SystemReset();
}



/*******************************************************************************
* Platform initialization.                                                     *
*******************************************************************************/
#define  DEFAULT_PLATFORM_FLAGS ( \
              MANUVR_PLAT_FLAG_INNATE_DATETIME | \
              MANUVR_PLAT_FLAG_HAS_IDENTITY)

/*
* Init that needs to happen prior to kernel bootstrap().
* This is the final function called by the kernel constructor.
*/
int8_t STM32F7Platform::platformPreInit(Argument* root_config) {
  ManuvrPlatform::platformPreInit(root_config);
  _alter_flags(true, DEFAULT_PLATFORM_FLAGS);
  init_RNG();
  gpioSetup();
  //initPlatformRTC();
  return 0;
}


/*
* Called before kernel instantiation. So do the minimum required to ensure
*   internal system sanity.
*/
int8_t STM32F7Platform::platformPostInit() {
  return 0;
}


/*******************************************************************************
* .-. .----..----.    .-.     .--.  .-. .-..----.
* | |{ {__  | {}  }   | |    / {} \ |  `| || {}  \
* | |.-._} }| .-. \   | `--./  /\  \| |\  ||     /
* `-'`----' `-' `-'   `----'`-'  `-'`-' `-'`----'
*
* Interrupt service routine support functions. Everything in this block
*   executes under an ISR. Keep it brief...
*******************************************************************************/
extern "C" {
  /* The ISR for the hardware RNG subsystem. Adds to entropy buffer. */
  void RNG_IRQHandler() {
    uint32_t sr = RNG->SR;
    if (sr & (RNG_SR_SEIS | RNG_SR_CEIS)) {
      // RNG fault? Clear the flags. Log?
      RNG->SR &= ~(RNG_SR_SEIS | RNG_SR_CEIS);
    }

    if (sr & RNG_SR_DRDY) {
      unsigned int _w_ptr = _random_pool_w_ptr % PLATFORM_RNG_CARRY_CAPACITY;
      randomness_pool[_w_ptr] = RNG->DR;
      _random_pool_w_ptr++;   // Concurrency...

      if ((_random_pool_w_ptr - _random_pool_r_ptr) >= PLATFORM_RNG_CARRY_CAPACITY) {
        // We have filled our entropy pool. Turn off the interrupts...
        RNG->CR &= ~(RNG_CR_IE);
        // ...and the RNG.
        RNG->CR &= ~(RNG_CR_RNGEN);
      }
    }
  }

  /*
  *
  */
  void EXTI0_IRQHandler(void) {
    if (EXTI->PR & (EXTI_PR_PR0)) {
      EXTI->PR = EXTI_PR_PR0;   // Clear service bit.
      //Kernel::log("EXTI 0\n");
      if (nullptr != __ext_line_bindings[0].fxn) {
        __ext_line_bindings[0].fxn();
      }
      if (nullptr != __ext_line_bindings[0].event) {
        Kernel::isrRaiseEvent(__ext_line_bindings[0].event);
      }
    }
  }

  /*
  *
  */
  void EXTI1_IRQHandler(void) {
    if (EXTI->PR & (EXTI_PR_PR1)) {
      EXTI->PR = EXTI_PR_PR1;   // Clear service bit.
      if (nullptr != __ext_line_bindings[1].fxn) {
        __ext_line_bindings[1].fxn();
      }
      if (nullptr != __ext_line_bindings[1].event) {
        Kernel::isrRaiseEvent(__ext_line_bindings[1].event);
      }
    }
  }

  /*
  *
  */
  void EXTI2_IRQHandler(void) {
    if (EXTI->PR & (EXTI_PR_PR2)) {
      EXTI->PR = EXTI_PR_PR2;   // Clear service bit.
      if (nullptr != __ext_line_bindings[2].fxn) {
        __ext_line_bindings[2].fxn();
      }
      if (nullptr != __ext_line_bindings[2].event) {
        Kernel::isrRaiseEvent(__ext_line_bindings[2].event);
      }
    }
  }

  /*
  *
  */
  void EXTI3_IRQHandler(void) {
    if (EXTI->PR & (EXTI_PR_PR3)) {
      EXTI->PR = EXTI_PR_PR3;   // Clear service bit.
      if (nullptr != __ext_line_bindings[3].fxn) {
        __ext_line_bindings[3].fxn();
      }
      if (nullptr != __ext_line_bindings[3].event) {
        Kernel::isrRaiseEvent(__ext_line_bindings[3].event);
      }
    }
  }

  /*
  *
  */
  void EXTI4_IRQHandler(void) {
    if (EXTI->PR & (EXTI_PR_PR4)) {
      EXTI->PR = EXTI_PR_PR4;   // Clear service bit.
      if (nullptr != __ext_line_bindings[4].fxn) {
        __ext_line_bindings[4].fxn();
      }
      if (nullptr != __ext_line_bindings[4].event) {
        Kernel::isrRaiseEvent(__ext_line_bindings[4].event);
      }
    }
  }

  /*
  *
  */
  void EXTI9_5_IRQHandler(void) {
    if (EXTI->PR & (EXTI_PR_PR5)) {
      EXTI->PR = EXTI_PR_PR5;   // Clear service bit.
      if (nullptr != __ext_line_bindings[5].fxn) {
        __ext_line_bindings[5].fxn();
      }
      if (nullptr != __ext_line_bindings[5].event) {
        Kernel::isrRaiseEvent(__ext_line_bindings[5].event);
      }
    }
    if (EXTI->PR & (EXTI_PR_PR6)) {
      EXTI->PR = EXTI_PR_PR6;   // Clear service bit.
      if (nullptr != __ext_line_bindings[6].fxn) {
        __ext_line_bindings[6].fxn();
      }
      if (nullptr != __ext_line_bindings[6].event) {
        Kernel::isrRaiseEvent(__ext_line_bindings[6].event);
      }
    }
    if (EXTI->PR & (EXTI_PR_PR7)) {
      EXTI->PR = EXTI_PR_PR7;   // Clear service bit.
      if (nullptr != __ext_line_bindings[7].fxn) {
        __ext_line_bindings[7].fxn();
      }
      if (nullptr != __ext_line_bindings[7].event) {
        Kernel::isrRaiseEvent(__ext_line_bindings[7].event);
      }
    }
    if (EXTI->PR & (EXTI_PR_PR8)) {
      EXTI->PR = EXTI_PR_PR8;   // Clear service bit.
      if (nullptr != __ext_line_bindings[8].fxn) {
        __ext_line_bindings[8].fxn();
      }
      if (nullptr != __ext_line_bindings[8].event) {
        Kernel::isrRaiseEvent(__ext_line_bindings[8].event);
      }
    }
    if (EXTI->PR & (EXTI_PR_PR9)) {
      EXTI->PR = EXTI_PR_PR9;   // Clear service bit.
      if (nullptr != __ext_line_bindings[9].fxn) {
        __ext_line_bindings[9].fxn();
      }
      if (nullptr != __ext_line_bindings[9].event) {
        Kernel::isrRaiseEvent(__ext_line_bindings[9].event);
      }
    }
  }

  /*
  *
  */
  void EXTI15_10_IRQHandler(void) {
    if (EXTI->PR & (EXTI_PR_PR11)) {
      EXTI->PR = EXTI_PR_PR11;   // Clear service bit.
      if (nullptr != __ext_line_bindings[11].fxn) {
        __ext_line_bindings[11].fxn();
      }
      if (nullptr != __ext_line_bindings[11].event) {
        Kernel::isrRaiseEvent(__ext_line_bindings[11].event);
      }
    }
    if (EXTI->PR & (EXTI_PR_PR12)) {
      EXTI->PR = EXTI_PR_PR12;   // Clear service bit.
      if (nullptr != __ext_line_bindings[12].fxn) {
        __ext_line_bindings[12].fxn();
      }
      if (nullptr != __ext_line_bindings[12].event) {
        Kernel::isrRaiseEvent(__ext_line_bindings[12].event);
      }
    }
    if (EXTI->PR & (EXTI_PR_PR13)) {
      EXTI->PR = EXTI_PR_PR13;   // Clear service bit.
      if (nullptr != __ext_line_bindings[13].fxn) {
        __ext_line_bindings[13].fxn();
      }
      if (nullptr != __ext_line_bindings[13].event) {
        Kernel::isrRaiseEvent(__ext_line_bindings[13].event);
      }
    }
    if (EXTI->PR & (EXTI_PR_PR14)) {
      EXTI->PR = EXTI_PR_PR14;   // Clear service bit.
      if (nullptr != __ext_line_bindings[14].fxn) {
        __ext_line_bindings[14].fxn();
      }
      if (nullptr != __ext_line_bindings[14].event) {
        Kernel::isrRaiseEvent(__ext_line_bindings[14].event);
      }
    }
    if (EXTI->PR & (EXTI_PR_PR15)) {
      EXTI->PR = EXTI_PR_PR15;   // Clear service bit.
      if (nullptr != __ext_line_bindings[15].fxn) {
        __ext_line_bindings[15].fxn();
      }
      if (nullptr != __ext_line_bindings[15].event) {
        Kernel::isrRaiseEvent(__ext_line_bindings[15].event);
      }
    }
  }


}  // extern "C"
