/*
File:   Platform.h
Author: J. Ian Lindsay
Date:   2015.11.01


Copyright (C) 2015 Manuvr
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

#include "StringBuilder/StringBuilder.h"

#define LOG_EMERG   0    /* system is unusable */
#define LOG_ALERT   1    /* action must be taken immediately */
#define LOG_CRIT    2    /* critical conditions */
#define LOG_ERR     3    /* error conditions */
#define LOG_WARNING 4    /* warning conditions */
#define LOG_NOTICE  5    /* normal but significant condition */
#define LOG_INFO    6    /* informational */
#define LOG_DEBUG   7    /* debug-level messages */

/*
* These are just lables. We don't really ever care about the *actual* integers being defined here. Only
*   their consistency.
*/
#define MANUVR_RTC_STARTUP_UNINITED       0x00000000
#define MANUVR_RTC_STARTUP_UNKNOWN        0x23196400
#define MANUVR_RTC_OSC_FAILURE            0x23196401
#define MANUVR_RTC_STARTUP_GOOD_UNSET     0x23196402
#define MANUVR_RTC_STARTUP_GOOD_SET       0x23196403

/*
* These are constants where we care about the number.
*/
#define PLATFORM_RNG_CARRY_CAPACITY           10     // How many random numbers should be cached? 

class ManuvrEvent;

// Function-pointer definitions
typedef void (*FunctionPointer) ();


typedef struct __platform_gpio_def {
  ManuvrEvent*    event;
  FunctionPointer fxn;
  uint8_t         pin;
  uint8_t         flags;
  uint16_t        mode;  // Strictly more than needed. Padding structure...
} PlatformGPIODef;


#ifdef __cplusplus
 extern "C" {
#endif 

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
* Misc
*/
volatile uint32_t getStackPointer();   // Returns the value of the stack pointer.
volatile void jumpToBootloader();
volatile void reboot();
void globalIRQEnable();
void globalIRQDisable();
void maskableInterrupts(bool);

/*
* Randomness
*/
uint32_t randomInt();                        // Fetches one of the stored randoms and blocks until one is available.
volatile bool provide_random_int(uint32_t);  // Provides a new random to the pool from the RNG ISR.
void init_RNG();                             // Fire up the random number generator.


/*
* GPIO and change-notice.
*/
void gpioSetup();        // We call this once on bootstrap. Sets up GPIO not covered by other classes.
int8_t gpioDefine(uint8_t pin, int mode);
void unsetPinIRQ(uint8_t pin);
void setPinEvent(uint8_t pin, ManuvrEvent* isr_event);
void setPinFxn(uint8_t pin, FunctionPointer fxn);

/*
* Call this once on system init to configure the basics of the platform.
*/
void platformInit();


#ifdef __cplusplus
}
#endif 

#endif

