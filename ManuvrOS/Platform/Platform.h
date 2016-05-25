/*
File:   Platform.h
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


This file is meant to contain a set of common functions that are typically platform-dependent.
  The goal is to make a class instance that is pre-processor-selectable for instantiation by the
  kernel, thereby giving the kernel the ability to...
    * Access the realtime clock (if applicatble)
    * Get definitions for GPIO pins.
    * Access a true RNG (if it exists)
*/


#ifndef __PLATFORM_SUPPORT_H__
#define __PLATFORM_SUPPORT_H__

#include "FirmwareDefs.h"

// System-level includes.
#include <inttypes.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#include <CommonConstants.h>
#include <DataStructures/StringBuilder.h>

// Conditional inclusion for different threading models...
#if defined(__MANUVR_LINUX)
  #include <pthread.h>
  #include <signal.h>
  #include <sys/time.h>
#elif defined(__MANUVR_FREERTOS)

#endif


class ManuvrRunnable;


#if defined (ARDUINO)
  #include <Arduino.h>
#elif defined(STM32F7XX) | defined(STM32F746xx)
  #include <stm32f7xx_hal.h>
  
  #define HIGH         1
  #define LOW          0
  #define INPUT        GPIO_MODE_INPUT
  #define OUTPUT       1
  #define INPUT_PULLUP 2
  #define CHANGE       GPIO_MODE_IT_RISING_FALLING
  #define FALLING      GPIO_MODE_IT_FALLING
  #define RISING       GPIO_MODE_IT_RISING
  extern "C" {
    unsigned long millis();
    unsigned long micros();
  }
#else
  // We adopt Arduino GPIO access conventions.
  #define HIGH         1
  #define LOW          0
  #define INPUT        0
  #define OUTPUT       1
  #define INPUT_PULLUP 2
  #define CHANGE       4
  #define FALLING      2
  #define RISING       3
  extern "C" {
    unsigned long millis();
    unsigned long micros();
  }
#endif


#ifdef __cplusplus
 extern "C" {
#endif



typedef struct __platform_gpio_def {
  ManuvrRunnable* event;
  FunctionPointer fxn;
  uint8_t         pin;
  uint8_t         flags;
  uint16_t        mode;  // Strictly more than needed. Padding structure...
} PlatformGPIODef;

/*
* Time and date
*/
bool initPlatformRTC();       // We call this once on bootstrap. Sets up the RTC.
bool setTimeAndDate(char*);   // Takes a string of the form given by RFC-2822: "Mon, 15 Aug 2005 15:52:01 +0000"   https://www.ietf.org/rfc/rfc2822.txt
uint32_t epochTime();         // Returns an integer representing the current datetime.
void currentDateTime(StringBuilder*);    // Writes a human-readable datetime to the argument.

// Performs string conversion for integer type-code, and is only useful for logging.
const char* getRTCStateString(uint32_t code);

/*
* Watchdog timer.
*/

/*
* Interrupt-masking
*/
void globalIRQEnable();
void globalIRQDisable();
void maskableInterrupts(bool);

/*
* Process control
*/
volatile void jumpToBootloader();
volatile void seppuku();

/*
* Underlying system control.
*/
volatile void reboot();
volatile void hardwareShutdown();

/*
* Threading
*/
int createThread(unsigned long*, void*, ThreadFxnPtr, void*);
int deleteThread(unsigned long);
void sleep_millis(unsigned long millis);

/*
* Misc
*/
volatile uint32_t getStackPointer();   // Returns the value of the stack pointer.

/*
* Data-persistence functions. This is the API used by anything that wants to write
*   formless data to a place on the device to be recalled on a different runtime.
*/
//int8_t persistData(const char* store_name, uint8_t* data, int length);
bool persistCapable();        // Returns true if this platform can store data locally.
unsigned long persistFree();  // Returns the number of bytes availible to store data.


/*
* Randomness
*/
uint32_t randomInt();                        // Fetches one of the stored randoms and blocks until one is available.
volatile bool provide_random_int(uint32_t);  // Provides a new random to the pool from the RNG ISR.
void init_RNG();                             // Fire up the random number generator.


/*
* Identity and serial number.
* Do be careful exposing this stuff, as any outside knowledge of chip
*   serial numbers may compromise crypto that directly-relies on them.
* For crypto, it would be best to never use an invariant number like this directly,
*   and instead use it as a PRNG seed or some other such approach. Please see the
*   security documentation for a deeper discussion of the issues at hand.
*/
int platformSerialNumberSize();   // Returns the length of the serial number on this platform (in bytes).
int getSerialNumber(uint8_t*);    // Writes the serial number to the indicated buffer.


/*
* GPIO and change-notice.
*/
void   gpioSetup();        // We call this once on bootstrap. Sets up GPIO not covered by other classes.
int8_t gpioDefine(uint8_t pin, int mode);
void   unsetPinIRQ(uint8_t pin);
int8_t setPinEvent(uint8_t pin, uint8_t condition, ManuvrRunnable* isr_event);
int8_t setPinFxn(uint8_t pin, uint8_t condition, FunctionPointer fxn);
int8_t setPin(uint8_t pin, bool high);
int8_t readPin(uint8_t pin);
int8_t setPinAnalog(uint8_t pin, int);
int    readPinAnalog(uint8_t pin);


/*
* Call this once on system init to configure the basics of the platform.
*/
void platformPreInit();
void platformInit();



#ifdef __cplusplus
}
#endif

#endif
