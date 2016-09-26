/*
File:   PlatformArduino.cpp
Author: J. Ian Lindsay
Date:   2016.04.10

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


Fallback to Arduino support...
*/

#include "Platform.h"

#include <wiring.h>
#include <Time/Time.h>
#include <unistd.h>

#define PLATFORM_GPIO_PIN_COUNT   16



/****************************************************************************************************
* The code under this block is special on this platform, and will not be available elsewhere.       *
****************************************************************************************************/


/****************************************************************************************************
* Watchdog                                                                                          *
****************************************************************************************************/
volatile uint32_t millis_since_reset = 1;   // Start at one because WWDG.

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


/*
* Init the RNG. Short and sweet.
*/
void init_RNG() {
  srand(Teensy3Clock.get());          // Seed the PRNG...
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
  return 16;
}


/**
* Writes the serial number to the indicated buffer.
* Note that for the Teensy3.x we use the Freescale unique ID. NOT the
*   independent serial number installed by PJRC. See this thread:
*   https://forum.pjrc.com/threads/25522-Serial-Number-of-Teensy-3-1
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
uint32_t rtc_startup_state = MANUVR_RTC_STARTUP_UNINITED;


/*
*
*/
bool initPlatformRTC() {
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
  if (target != nullptr) {
    target->concatf("%04d-%02d-%02dT", year(), month(), day());
    target->concatf("%02d:%02d:%02d+00:00", hour(), minute(), second());
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
}


int8_t gpioDefine(uint8_t pin, int mode) {
  pinMode(pin, mode);
  return 0;
}


void unsetPinIRQ(uint8_t pin) {
  detachInterrupt(pin);
}


int8_t setPinEvent(uint8_t pin, uint8_t condition, ManuvrRunnable* isr_event) {
  return 0;
}


/*
* Pass the function pointer
*/
int8_t setPinFxn(uint8_t pin, uint8_t condition, FxnPointer fxn) {
  attachInterrupt(pin, fxn, condition);
  return 0;
}


int8_t setPin(uint8_t pin, bool val) {
  digitalWrite(pin, val);
  return 0;
}


int8_t readPin(uint8_t pin) {
  return digitalRead(pin);
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
* Interrupt-masking                                                                                 *
****************************************************************************************************/

#if defined (__MANUVR_FREERTOS)
  void globalIRQEnable() {     taskENABLE_INTERRUPTS();    }
  void globalIRQDisable() {    taskDISABLE_INTERRUPTS();   }
#else
  void globalIRQEnable() {     sei();    }
  void globalIRQDisable() {    cli();    }
#endif


/****************************************************************************************************
* Process control                                                                                   *
****************************************************************************************************/

/*
* Terminate this running process, along with any children it may have forked() off.
* Never returns.
*/
volatile void seppuku() {
  // This means "Halt" on a base-metal build.
  cli();
  while(true);
}


/*
* Jump to the bootloader.
* Never returns.
*/
volatile void jumpToBootloader() {
  cli();
  while(true);
}


/****************************************************************************************************
* Underlying system control.                                                                        *
****************************************************************************************************/

/*
* This means "Halt" on a base-metal build.
* Never returns.
*/
volatile void hardwareShutdown() {
  cli();
  while(true);
}

/*
* Causes immediate reboot.
* Never returns.
*/
volatile void reboot() {
  cli();
  while(true);
}



/****************************************************************************************************
* Platform initialization.                                                                          *
****************************************************************************************************/

/*
* Init that needs to happen prior to kernel bootstrap().
* This is the final function called by the kernel constructor.
*/
void platformPreInit() {
  ManuvrPlatform::platformPreInit(root_config);
  gpioSetup();
  init_RNG();
}
