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


This file is meant to contain a set of common functions that are typically platform-dependent.
  The goal is to make a class instance that is pre-processor-selectable for instantiation by the
  kernel, thereby giving the kernel the ability to...
    * Access the realtime clock (if applicatble)
    * Get definitions for GPIO pins.
    * Access a true RNG (if it exists)

This file forms the catch-all for linux platforms that have no support.
*/

#include "Platform.h"
#include <Kernel.h>


/****************************************************************************************************
* The code under this block is special on this platform, and will not be available elsewhere.       *
****************************************************************************************************/
volatile Kernel* __kernel = NULL;


/****************************************************************************************************
* Watchdog                                                                                          *
****************************************************************************************************/
volatile uint32_t millis_since_reset = 1;   // Start at one because WWDG.
volatile uint8_t  watchdog_mark      = 42;
unsigned long     start_time_micros  = 0;


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
  uint32_t return_value = rand();
  return return_value;
}

/**
* Called by the RNG ISR to provide new random numbers.
*
* @param    nu_rnd The supplied random number.
* @return   True if the RNG should continue supplying us, false if it should take a break until we need more.
*/
volatile bool provide_random_int(uint32_t nu_rnd) {
  return false;
}

/*
* Init the RNG. Short and sweet.
*/
void init_RNG() {
  srand(time(NULL));          // Seed the PRNG...
}


/****************************************************************************************************
* Identity and serial number                                                                        *
****************************************************************************************************/
/**
* We sometimes need to know the length of the platform's unique identifier (if any). If this platform
*   is not serialized, this function will return zero.
*
* @return   The length of the serial number on this platform, in terms of bytes.
*/
int platformSerialNumberSize() {
  return 0;
}

/**
* Writes the serial number to the indicated buffer.
*
* @param    A pointer to the target buffer.
* @return   The number of bytes written.
*/
int getSerialNumber(uint8_t *buf) {
  return 0;
}



/****************************************************************************************************
* Data persistence                                                                                  *
****************************************************************************************************/
// Returns true if this platform can store data locally.
bool persistCapable() {
  return false;
}

// Returns the number of bytes availible to store data.
unsigned long persistFree() {
  return -1L;
}


/****************************************************************************************************
* Time and date                                                                                     *
****************************************************************************************************/
uint32_t rtc_startup_state = MANUVR_RTC_STARTUP_UNINITED;


/*
*
*/
bool initPlatformRTC() {
  return false;
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
uint32_t currentTimestamp(void) {
  return 0;
}


/*
* Writes a human-readable datetime to the argument.
* Returns ISO 8601 datetime string.
* 2004-02-12T15:19:21+00:00
*/
void currentDateTime(StringBuilder* target) {
  if (target != NULL) {
    target->concat("No time support on this platform.\n");
  }
}


/*
* Not provided elsewhere on a linux platform.
*/
unsigned long millis() {
  return 0L;
}


/*
* Not provided elsewhere on a linux platform.
*/
unsigned long micros() {
  return 0L;
}


/****************************************************************************************************
* GPIO and change-notice                                                                            *
****************************************************************************************************/
/*
* This fxn should be called once on boot to setup the CPU pins that are not claimed
*   by other classes. GPIO pins at the command of this-or-that class should be setup
*   in the class that deals with them.
* Pending peripheral-level init of pins, we should just enable everything and let
*   individual classes work out their own requirements.
*/
void gpioSetup() {
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
void globalIRQEnable() {}
void globalIRQDisable() {}


/****************************************************************************************************
* Process control                                                                                   *
****************************************************************************************************/
/*
* Terminate this running process, along with any children it may have forked() off.
*/
volatile void seppuku() {}

/*
* Jump to the bootloader.
*/
volatile void jumpToBootloader() {}


/****************************************************************************************************
* Underlying system control.                                                                        *
****************************************************************************************************/
volatile void hardwareShutdown() {}
volatile void reboot() {}



/****************************************************************************************************
* Platform initialization.                                                                          *
****************************************************************************************************/

/*
* Init that needs to happen prior to kernel bootstrap().
* This is the final function called by the kernel constructor.
*/
void platformPreInit() {
  __kernel = (volatile Kernel*) Kernel::getInstance();
  gpioSetup();
}


/*
* Called as a result of kernels bootstrap() fxn.
*/
void platformInit() {
  start_time_micros = micros();
  init_RNG();
  initPlatformRTC();
}
