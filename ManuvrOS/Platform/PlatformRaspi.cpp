/*
File:   Platform.cpp
Author: J. Ian Lindsay
Date:   2015.11.01


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

#include "Platform.h"

#include <sys/time.h>
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
  srand(time(NULL));          // Seed the PRNG...
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
  if (target != NULL) {
  }
}


/*
* Not provided elsewhere on a linux platform.
*/
uint32_t millis() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000L);
}


/*
* Not provided elsewhere on a linux platform.
*/
uint32_t micros() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (ts.tv_sec * 1000000L + ts.tv_nsec / 1000L);
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
* Misc                                                                                              *
****************************************************************************************************/
volatile void jumpToBootloader() {
  // TODO: Restart the program.
}

volatile void reboot() {
  // TODO: Actually reboot the system.
}

// Ze interrupts! Zhey do nuhsing!
// TODO: Perhaps raise the nice value?
void globalIRQEnable() {    }
void globalIRQDisable() {   }

/*
* Call this with a boolean to enable or disable maskable interrupts globally.
* NOTE: This includes USB and SysTick. So no host communication, and no scheduler.
*       Events ought to still work, however.
*/
void maskableInterrupts(bool enable) {
}


void platformInit() {
  start_time_micros = micros();
  init_RNG();
}


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

