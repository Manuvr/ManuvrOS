/*
File:   STM32F7.cpp
Author: J. Ian Lindsay
Date:   2015.12.02


Copyright (C) 2014 J. Ian Lindsay
All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


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

#ifdef __cplusplus
 extern "C" {
#endif


/****************************************************************************************************
* The code under this block is special on this platform, and will not be available elsewhere.       *
****************************************************************************************************/
#include "stm32f7xx_hal.h"

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
}

int8_t gpioDefine(uint8_t pin, int mode) {
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


/****************************************************************************************************
* Persistent configuration                                                                          *
****************************************************************************************************/


/****************************************************************************************************
* Misc                                                                                              *
****************************************************************************************************/
/**
* Sometimes we question the size of the stack.
*
* @return the stack pointer at call time.
*/
volatile uint32_t getStackPointer() {
  uint32_t test;  // Important to not do assignment here.
  test = (uint32_t) &test;  // Store the pointer.
  return test;
}


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
  //_reboot_Teensyduino_();
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
  *((uint32_t *)0xE000ED0C) = 0x5FA0004;
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
}



#ifdef __cplusplus
 }
#endif
