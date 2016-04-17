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

#include "Platform/Platform.h"

#include <unistd.h>

#define PLATFORM_GPIO_PIN_COUNT   33



/****************************************************************************************************
* The code under this block is special on this platform, and will not be available elsewhere.       *
****************************************************************************************************/
#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_rcc.h"

//#if defined(ENABLE_USB_VCP)
  #include "tm_stm32_usb_device.h"
  #include "tm_stm32_usb_device_cdc.h"
//#endif

volatile uint32_t millis_since_reset = 1;   // Start at one because WWDG.
volatile uint8_t  watchdog_mark      = 42;
unsigned long     start_time_micros  = 0;


/*
* Trivial, since the period of systick is 1ms, and we are already
*   keeping track of how many times it has rolled over. Simply return
*   that value.
* Returns the number of milliseconds since reset.
*/
unsigned long millis(void) {  return millis_since_reset;  }

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
    //return c0*8000-v1/(MCK/8000000UL);
    return (unsigned long) millis_since_reset/1000000;
  }
  else { // systick rollover, use last count value
    //return c1*8000-v1/(MCK/8000000UL);
    return (unsigned long) millis_since_reset/1000000;
  }
  return 0;
}




/****************************************************************************************************
* Randomness                                                                                        *
****************************************************************************************************/
volatile uint32_t next_random_int[PLATFORM_RNG_CARRY_CAPACITY];

/**
* Dead-simple interface to the RNG. Despite the fact that it is interrupt-driven, we may resort
*   to polling if random demand exceeds random supply. So this may block until a random number
*   is actually availible (next_random_int != 0).
*
* @return   A 32-bit unsigned random number. This can be cast as needed.
*/
uint32_t randomInt() {
  while (!(RNG->SR & (RNG_SR_DRDY)));
  uint32_t return_value = RNG->DR;
  return return_value;
}

/**
* Called by the RNG ISR to provide new random numbers.
*
* @param    nu_rnd The supplied random number.
* @return   True if the RNG should continue supplying us, false if it should take a break until we need more.
*/
volatile bool provide_random_int(uint32_t nu_rnd) {
  for (uint8_t i = 0; i < PLATFORM_RNG_CARRY_CAPACITY; i++) {
    if (next_random_int[i] == 0) {
      next_random_int[i] = nu_rnd;
      return (i == PLATFORM_RNG_CARRY_CAPACITY-1) ? false : true;
    }
  }
  return false;
}

/*
* Init the RNG. Short and sweet.
*/
void init_RNG() {
  for (uint8_t i = 0; i < PLATFORM_RNG_CARRY_CAPACITY; i++) next_random_int[i] = 0;
  __HAL_RCC_RNG_CLK_ENABLE();
  RNG->CR |= RNG_CR_RNGEN;
}


/****************************************************************************************************
* Identity and serial number                                                                        *
****************************************************************************************************/
int platformSerialNumberSize() { return 12; }

int getSerialNumber(uint8_t* buf) {
  *((uint32_t*)buf + 0) = *((uint32_t*) 0x1FF0F420);
  *((uint32_t*)buf + 4) = *((uint32_t*) 0x1FF0F424);
  *((uint32_t*)buf + 8) = *((uint32_t*) 0x1FF0F428);
  return 12;
}


/****************************************************************************************************
* Time and date                                                                                     *
****************************************************************************************************/
uint32_t rtc_startup_state = MANUVR_RTC_STARTUP_UNINITED;


/*
*
*/
bool initPlatformRTC() {
  return true;
}

/*
* Given an RFC2822 datetime string, decompose it and set the time and date.
* We would prefer RFC2822, but we should try and cope with things like missing
*   time or timezone.
* Returns false if the date failed to set. True if it did.
*/
bool setTimeAndDate(char* nu_date_time) {
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
  if (target != NULL) {
  }
}



/****************************************************************************************************
* GPIO and change-notice                                                                            *
****************************************************************************************************/

/*
*
*/
volatile PlatformGPIODef gpio_pins[PLATFORM_GPIO_PIN_COUNT];

void pin_isr_pitch_event() {
}


/*
* This fxn should be called once on boot to setup the CPU pins that are not claimed
*   by other classes. GPIO pins at the command of this-or-that class should be setup
*   in the class that deals with them.
* Pending peripheral-level init of pins, we should just enable everything and let
*   individual classes work out their own requirements.
*/
void gpioSetup() {
  // Null-out all the pin definitions in preparation for assignment.
  for (uint8_t i = 0; i < PLATFORM_GPIO_PIN_COUNT; i++) {
    gpio_pins[i].event = 0;      // No event assigned.
    gpio_pins[i].fxn   = 0;      // No function pointer.
    gpio_pins[i].mode  = 1;  // All pins begin as inputs.
    gpio_pins[i].pin   = i;      // The pin number.
  }

  /* GPIO Ports Clock Enable */
  __GPIOE_CLK_ENABLE();
  __GPIOB_CLK_ENABLE();
  __GPIOG_CLK_ENABLE();
  __GPIOD_CLK_ENABLE();
  __GPIOC_CLK_ENABLE();
  __GPIOA_CLK_ENABLE();
  __GPIOI_CLK_ENABLE();
  __GPIOH_CLK_ENABLE();
  __GPIOF_CLK_ENABLE();
}




int8_t gpioDefine(uint8_t pin, int mode) {
  GPIO_InitTypeDef GPIO_InitStruct;

  GPIO_InitStruct.Pin    = GPIO_PIN_3|GPIO_PIN_2|GPIO_PIN_1|GPIO_PIN_0;
  GPIO_InitStruct.Mode   = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull   = GPIO_NOPULL;
  GPIO_InitStruct.Speed  = GPIO_SPEED_LOW;

  //HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
  return 0;
}


void unsetPinIRQ(uint8_t pin) {
}


int8_t setPinEvent(uint8_t pin, uint8_t condition, ManuvrRunnable* isr_event) {
  return 0;
}


/*
* Pass the function pointer
*/
int8_t setPinFxn(uint8_t pin, uint8_t condition, FunctionPointer fxn) {
  return 0;
}


int8_t setPin(uint8_t pin, bool val) {
  return 0;
}


int8_t readPin(uint8_t pin) {
  return 0;
}


int8_t setPinAnalog(uint8_t pin, int val) {
  return 0;
}

int readPinAnalog(uint8_t pin) {
  return -1;
}


/****************************************************************************************************
* Persistent configuration                                                                          *
****************************************************************************************************/




/****************************************************************************************************
* Interrupt-masking                                                                                 *
****************************************************************************************************/

void globalIRQEnable() {     asm volatile ("cpsid i");    }
void globalIRQDisable() {    asm volatile ("cpsie i");    }



/****************************************************************************************************
* Process control                                                                                   *
****************************************************************************************************/

/*
* Terminate this running process, along with any children it may have forked() off.
* Never returns.
*/
volatile void seppuku() {
  // This means "Halt" on a base-metal build.
  globalIRQDisable();
  while(true);
}


/*
* Jump to the bootloader.
* Never returns.
*/
volatile void jumpToBootloader() {
  globalIRQDisable();
  __set_MSP(0x20001000);      // Set the main stack pointer to default value for the F417...

  // Per clive1's post, set some sort of key value just below the initial stack pointer.
  // We don't really care if we clobber something, because this fxn will reboot us. But
  // when the reset handler is executed, it will look for this value. If it finds it, it
  // will branch to the Bootloader code.
  *((unsigned long *)0x2000FFF0) = 0xb00710ad;
  NVIC_SystemReset();
}


/****************************************************************************************************
* Underlying system control.                                                                        *
****************************************************************************************************/

/*
* This means "Halt" on a base-metal build.
* Never returns.
*/
volatile void hardwareShutdown() {
  globalIRQDisable();
  while(true);
}

/*
* Causes immediate reboot.
* Never returns.
*/
volatile void reboot() {
  globalIRQDisable();
  //HAL_RCC_DeInit();                   // Switch to HSI, no PLL
  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL  = 0;

  __set_MSP(0x20001000);      // Set the main stack pointer to default value for the F417...

  // Per clive1's post, set some sort of key value just below the initial stack pointer.
  // We don't really care if we clobber something, because this fxn will reboot us. But
  // when the reset handler is executed, it will look for this value. If it finds it, it
  // will branch to the Bootloader code.
  *((unsigned long *)0x2000FFF0) = 0xb00710ad;
  NVIC_SystemReset();
}



/****************************************************************************************************
* Platform initialization.                                                                          *
****************************************************************************************************/

/*
* Init that needs to happen prior to kernel bootstrap().
* This is the final function called by the kernel constructor.
*/
void platformPreInit() {
  gpioSetup();
}


/*
* Called as a result of kernels bootstrap() fxn.
*/
void platformInit() {
  start_time_micros = micros();
  init_RNG();
  #if defined(ENABLE_USB_VCP)
    TM_USB_Init();    /* Init USB peripheral */
    TM_USBD_CDC_Init(TM_USB_FS);
    TM_USBD_Start(TM_USB_FS);
  #endif
}
