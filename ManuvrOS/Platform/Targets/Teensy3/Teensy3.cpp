/*
File:   Teensy3.cpp
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


#include <CommonConstants.h>
#include <Platform/Platform.h>

#if defined(CONFIG_MANUVR_STORAGE)
#include <Platform/Targets/Teensy3/TeensyStorage.h>
#endif

#include <wiring.h>
#include <Time/Time.h>

#if !defined (__BUILD_HAS_FREERTOS)
  #include "IntervalTimer.h"

  IntervalTimer timer0;  // Scheduler
  void timerCallbackScheduler() {  platform.advanceScheduler();  }
#endif


/*******************************************************************************
* The code under this block is special on this platform,                       *
*******************************************************************************/
time_t getTeensy3Time() {   return Teensy3Clock.get();   }

#if defined(CONFIG_MANUVR_STORAGE)
TeensyStorage _t_storage(nullptr);
#endif




/*******************************************************************************
* Watchdog                                                                     *
*******************************************************************************/
volatile uint32_t millis_since_reset = 1;   // Start at one because WWDG.
volatile uint8_t  watchdog_mark      = 42;


/*******************************************************************************
* Randomness                                                                   *
*******************************************************************************/
volatile uint32_t next_random_int[PLATFORM_RNG_CARRY_CAPACITY];

/**
* Dead-simple interface to the RNG. Despite the fact that it is interrupt-driven, we may resort
*   to polling if random demand exceeds random supply. So this may block until a random number
*   is actually availible (next_random_int != 0).
*
* @return   A 32-bit unsigned random number. This can be cast as needed.
*/
uint32_t randomUInt32() {
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
void init_rng() {
  for (uint8_t i = 0; i < PLATFORM_RNG_CARRY_CAPACITY; i++) next_random_int[i] = 0;
  srand(Teensy3Clock.get());          // Seed the PRNG...
}


/*******************************************************************************
*  ___   _           _      ___
* (  _`\(_ )        ( )_  /'___)
* | |_) )| |    _ _ | ,_)| (__   _    _ __   ___ ___
* | ,__/'| |  /'_` )| |  | ,__)/'_`\ ( '__)/' _ ` _ `\
* | |    | | ( (_| || |_ | |  ( (_) )| |   | ( ) ( ) |
* (_)   (___)`\__,_)`\__)(_)  `\___/'(_)   (_) (_) (_)
* These are overrides and additions to the platform class.
*******************************************************************************/
void Teensy3::printDebug(StringBuilder* output) {
  output->concatf("==< Teensy3 [%s] >================================\n", getPlatformStateStr(platformState()));
  ManuvrPlatform::printDebug(output);
}



/*******************************************************************************
* Identity and serial number                                                   *
*******************************************************************************/
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
  *((uint32_t*) buf + 0)  = *((uint32_t*) 0x40048054);
  *((uint32_t*) buf + 4)  = *((uint32_t*) 0x40048058);
  *((uint32_t*) buf + 8)  = *((uint32_t*) 0x4004805C);
  *((uint32_t*) buf + 12) = *((uint32_t*) 0x40048060);
  return 16;
}



/*******************************************************************************
* Time and date                                                                *
*******************************************************************************/

/*
*
*/
bool init_rtc() {
  setSyncProvider(getTeensy3Time);
  setSyncInterval(60);  // Re-sync the internal clock every minute.
  switch (timeStatus()) {
    case timeSet:
      return true;
    case timeNotSet:
    case timeNeedsSync:
    default:
      return false;
  }
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
*/
bool setTimeAndDate(uint8_t y, uint8_t m, uint8_t d, uint8_t wd, uint8_t h, uint8_t mi, uint8_t s) {
  TimeElements tm;
  tm.Second = s;
  tm.Minute = mi;
  tm.Hour   = h;
  tm.Wday   = wd;
  tm.Day    = d;
  tm.Month  = m;
  tm.Year   = y;
  time_t t = makeTime(tm);
  setTime(t);
  return false;
}


/*
* Returns an integer representing the current datetime.
*/
uint32_t epochTime(void) {
  return getTeensy3Time();
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



/*******************************************************************************
* GPIO and change-notice                                                       *
*******************************************************************************/
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
    gpio_pins[i].mode  = (int8_t) GPIOMode::INPUT;  // All pins begin as inputs.
    gpio_pins[i].pin   = i;      // The pin number.
  }
}


int8_t pinMode(uint8_t pin, GPIOMode mode) {
  pinMode(pin, (int) mode);
  return 0;
}


void unsetPinIRQ(uint8_t pin) {
  detachInterrupt(pin);
}


int8_t setPinEvent(uint8_t pin, uint8_t condition, ManuvrMsg* isr_event) {
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


/*******************************************************************************
* Persistent configuration                                                     *
*******************************************************************************/
#if defined(CONFIG_MANUVR_STORAGE)
  // Called during boot to load configuration.
  int8_t Teensy3::_load_config() {
    if (_storage_device) {
      if (_storage_device->isMounted()) {
        uint8_t raw[2044];
        int len = _storage_device->persistentRead(NULL, raw, 2044, 0);
        _config = Argument::decodeFromCBOR(raw, len);
        if (_config) {
          return 0;
        }
      }
    }
    return -1;
  }
#endif



/*******************************************************************************
* Interrupt-masking                                                            *
*******************************************************************************/

#if defined (__BUILD_HAS_FREERTOS)
  void globalIRQEnable() {     taskENABLE_INTERRUPTS();    }
  void globalIRQDisable() {    taskDISABLE_INTERRUPTS();   }
  //void globalIRQEnable() {         }
  //void globalIRQDisable() {        }
#else
  void globalIRQEnable() {     sei();    }
  void globalIRQDisable() {    cli();    }
#endif


/*******************************************************************************
* Process control                                                              *
*******************************************************************************/

/*
* Terminate this running process, along with any children it may have forked() off.
* Never returns.
*/
void Teensy3::seppuku() {
  reboot();
}


/*
* Jump to the bootloader.
* Never returns.
*/
void Teensy3::jumpToBootloader() {
  cli();
  _reboot_Teensyduino_();
}


/*******************************************************************************
* Underlying system control.                                                   *
*******************************************************************************/

/*
* This means "Halt" on a base-metal build.
* Never returns.
*/
void Teensy3::hardwareShutdown() {
  cli();
  while(true);
}

/*
* Causes immediate reboot.
* Never returns.
*/
void Teensy3::reboot() {
  cli();
  *((uint32_t *)0xE000ED0C) = 0x5FA0004;
}



/*******************************************************************************
* Platform initialization.                                                     *
*******************************************************************************/
#define  DEFAULT_PLATFORM_FLAGS ( \
              MANUVR_PLAT_FLAG_INNATE_DATETIME | \
              MANUVR_PLAT_FLAG_SERIALED | \
              MANUVR_PLAT_FLAG_HAS_IDENTITY)

/*
* Init that needs to happen prior to kernel bootstrap().
* This is the final function called by the kernel constructor.
*/
int8_t Teensy3::platformPreInit(Argument* root_config) {
  ManuvrPlatform::platformPreInit(root_config);
  _alter_flags(true, DEFAULT_PLATFORM_FLAGS);

  init_rng();
  _alter_flags(true, MANUVR_PLAT_FLAG_RNG_READY);

  if (init_rtc()) {
    _alter_flags(true, MANUVR_PLAT_FLAG_RTC_SET);
  }
  _alter_flags(true, MANUVR_PLAT_FLAG_RTC_READY);
  gpioSetup();

  if (root_config) {
  }

  #if defined(CONFIG_MANUVR_STORAGE)
    _storage_device = (Storage*) &_t_storage;
    _kernel.subscribe((EventReceiver*) &_t_storage);
  #endif
  return 0;
}


/*
* Called as a result of kernels bootstrap() fxn.
*/
int8_t Teensy3::platformPostInit() {
  #if defined (__BUILD_HAS_FREERTOS)
  #else
  // No threads. We are responsible for pinging our own scheduler.
  // Turn on the periodic interrupts...
  timer0.begin(timerCallbackScheduler, 1000*MANUVR_PLATFORM_TIMER_PERIOD_MS);
  #endif
  return 0;
}
