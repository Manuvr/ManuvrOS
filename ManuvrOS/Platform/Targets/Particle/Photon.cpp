/*
File:   ParticlePhoton.cpp
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


Particle Photon support...
*/

#include "Platform.h"
#include <Kernel.h>


/****************************************************************************************************
* The code under this block is special on this platform, and will not be available elsewhere.       *
****************************************************************************************************/

/****************************************************************************************************
* Watchdog                                                                                          *
****************************************************************************************************/
volatile uint32_t millis_since_reset = 1;   // Start at one because WWDG.
volatile uint8_t  watchdog_mark      = 42;


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
  rtc_startup_state = MANUVR_RTC_STARTUP_GOOD_UNSET;
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
uint32_t currentTimestamp(void) {
  uint32_t return_value = 0;
  return return_value;
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


/*
* Not provided elsewhere on a linux platform.
*/
unsigned long millis() {
}


/*
* Not provided elsewhere on a linux platform.
*/
unsigned long micros() {
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


/****************************************************************************************************
* Persistent configuration                                                                          *
****************************************************************************************************/



/****************************************************************************************************
* Interrupt-masking                                                                                 *
****************************************************************************************************/

// Ze interrupts! Zhey do nuhsing!
// TODO: Perhaps raise the nice value?
// At minimum, turn off the periodic timer, since this is what would happen on
//   other platforms.
void globalIRQEnable() {
  // TODO: Need to stack the time remaining.
  //set_linux_interval_timer();
}

void globalIRQDisable() {
  // TODO: Need to unstack the time remaining and fire any schedules.
  //unset_linux_interval_timer();
}



/****************************************************************************************************
* Process control                                                                                   *
****************************************************************************************************/

/*
* Terminate this running process, along with any children it may have forked() off.
*/
volatile void seppuku() {
  // Whatever the kernel cared to clean up, it better have done so by this point,
  //   as no other platforms return from this function.
  if (Kernel::log_buffer.length() > 0) {
    printf("\n\njumpToBootloader(): About to exit(). Remaining log follows...\n%s", Kernel::log_buffer.string());
  }
  printf("\n\n");
}


/*
* On linux, we take this to mean: scheule a program restart with the OS,
*   and then terminate this one.
*/
volatile void jumpToBootloader() {
  // TODO: Schedule a program restart.
  seppuku();
}


/****************************************************************************************************
* Underlying system control.                                                                        *
****************************************************************************************************/

volatile void hardwareShutdown() {
  // TODO: Actually shutdown the system.
}

volatile void reboot() {
  // TODO: Actually reboot the system.
}



/****************************************************************************************************
* Platform initialization.                                                                          *
****************************************************************************************************/

/*
* Init that needs to happen prior to kernel bootstrap().
* This is the final function called by the kernel constructor.
*/
int8_t platformPreInit(Argument* root_opts) {
  ManuvrPlatform::platformPreInit(root_config);
  gpioSetup();
  init_RNG();
  initPlatformRTC();
  return 0;
}


/*
* Called as a result of kernels bootstrap() fxn.
*/
int8_t platformPostInit() {
  return 0;
}
