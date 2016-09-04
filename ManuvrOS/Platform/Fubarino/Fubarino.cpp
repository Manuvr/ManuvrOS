/*
File:   Platform.cpp
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

#include "Platform.h"

#include <wiring.h>
#include <Time/Time.h>
#include <unistd.h>



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
  srand(time(nullptr));          // Seed the PRNG...
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
* Time and date                                                                                     *
****************************************************************************************************/
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
  uint32_t return_value = 0;
  return return_value;
}

/*
* Same, but writes a string representation to the argument.
*/
void currentTimestamp(StringBuilder* target) {
}

/*
* Writes a human-readable datetime to the argument.
* Returns ISO 8601 datetime string.
* 2004-02-12T15:19:21+00:00
*/
void currentDateTime(StringBuilder* target) {
  if (target != nullptr) {
  }
}



/****************************************************************************************************
* GPIO and change-notice                                                                            *
****************************************************************************************************/
/*
* This structure allows us to keep track of which pins are at our discretion to read/write/set ISRs on.
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
    gpio_pins[i].mode  = INPUT;  // All pins begin as inputs.
    gpio_pins[i].pin   = i;      // The pin number.
  }
}


int8_t gpioDefine(uint8_t pin, uint8_t mode) {
  pinMode(pin, mode);
  return 0;
}


void unsetPinIRQ(uint8_t pin) {
  detachInterrupt(pin);
}


void setPinEvent(uint8_t pin, uint8_t condition, ManuvrRunnable* isr_event) {
}


/*
* Pass the function pointer
*/
void setPinFxn(uint8_t pin, uint8_t condition, FunctionPointer fxn) {
  attachInterrupt(pin, condition, fxn);
}


int8_t setPin(uint8_t pin, bool val) {
  return digitalWrite(pin, val);
}


int8_t readPin(uint8_t pin) {
  return readPin(pin);
}


int8_t setPinAnalog(uint8_t pin, int val) {
  analogWrite(pin, val);
  return 0;
}

int readPinAnalog(uint8_t pin) {
  return analogRead(pin);
}



/****************************************************************************************************
* Persistent configuration                                                                          *
****************************************************************************************************/


/****************************************************************************************************
* Misc                                                                                              *
****************************************************************************************************/

void platformInit() {
  start_time_micros = micros();
  init_RNG();
}

volatile void jumpToBootloader(void) {
  executeSoftReset(0);
}


volatile void reboot(void) {
  executeSoftReset(ENTER_BOOTLOADER_ON_BOOT);
}


uint32_t timerCallbackScheduler(uint32_t currentTime) {
  platform.advanceScheduler();
  return (currentTime + (CORE_TICK_RATE * 1));
}


/*******************************************************************************
* Platform initialization.                                                     *
*******************************************************************************/
#define  DEFAULT_PLATFORM_FLAGS ( \
              MANUVR_PLAT_FLAG_HAS_IDENTITY)

/**
* Init that needs to happen prior to kernel bootstrap().
* This is the final function called by the kernel constructor.
*/
int8_t ManuvrPlatform::platformPreInit() {
  // TODO: Should we really be setting capabilities this late?
  uint32_t default_flags = DEFAULT_PLATFORM_FLAGS;

  #if defined(__MANUVR_MBEDTLS)
    default_flags |= MANUVR_PLAT_FLAG_HAS_CRYPTO;
  #endif

  #if defined(MANUVR_GPS_PIPE)
    default_flags |= MANUVR_PLAT_FLAG_HAS_LOCATION;
  #endif

  _alter_flags(true, default_flags);
  _discoverALUParams();

  start_time_micros = micros();
  init_rng();
  _alter_flags(true, MANUVR_PLAT_FLAG_RNG_READY);

  return 0;
}


/*
* Called before kernel instantiation. So do the minimum required to ensure
*   internal system sanity.
*/
int8_t ManuvrPlatform::platformPostInit() {
  if (nullptr == _identity) {
    _identity = new IdentityUUID(IDENTITY_STRING);
  }
  attachCoreTimerService(timerCallbackScheduler);
  globalIRQEnable();
  return 0;
}
