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
    * Access a common logging API.
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

  #define LOG_EMERG   0    /* system is unusable */
  #define LOG_ALERT   1    /* action must be taken immediately */
  #define LOG_CRIT    2    /* critical conditions */
  #define LOG_ERR     3    /* error conditions */
  #define LOG_WARNING 4    /* warning conditions */
  #define LOG_NOTICE  5    /* normal but significant condition */
  #define LOG_INFO    6    /* informational */
  #define LOG_DEBUG   7    /* debug-level messages */

  #include "StringBuilder/StringBuilder.h"

#ifdef __cplusplus
 extern "C" {
#endif 


/*
* These are constants where we care about the number.
*/
#define PLATFORM_RNG_CARRY_CAPACITY           10     // How many random numbers should be cached?


/*
* This is the actual class...
*/
class Platform {
  public:
    volatile static uint32_t millis_since_reset;
    volatile static uint8_t  watchdog_mark;

    static StringBuilder log_buffer;
    
    ManuvrEvent static_test_event;
    ManuvrEvent static_test_event_idem;
    
    StaticHub(void);
    static StaticHub* getInstance(void);
    int8_t bootstrap(void);
    
    // These are functions that should be reachable from everywhere in the application.
    volatile static void log(const char *fxn_name, int severity, const char *str, ...);  // Pass-through to the logger class, whatever that happens to be.
    volatile static void log(int severity, const char *str);                             // Pass-through to the logger class, whatever that happens to be.
    volatile static void log(const char *str);                                           // Pass-through to the logger class, whatever that happens to be.
    volatile static void log(char *str);                                           // Pass-through to the logger class, whatever that happens to be.
    volatile static void log(StringBuilder *str);

    /*
    * Nice utility functions.
    */
    static uint32_t randomInt(void);                                // Fetches one of the stored randoms and blocks until one is available.
    static volatile bool provide_random_int(uint32_t);              // Provides a new random to StaticHub from the RNG ISR.
    static volatile uint32_t getStackPointer(void);                 // Returns the value of the stack pointer and prints some data.
    
        
    bool setTimeAndDate(char*);   // Takes a string of the form given by RFC-2822: "Mon, 15 Aug 2005 15:52:01 +0000"   https://www.ietf.org/rfc/rfc2822.txt
    uint32_t currentTimestamp(void);         // Returns an integer representing the current datetime.
    void currentTimestamp(StringBuilder*);   // Same, but writes a string representation to the argument.
    void currentDateTime(StringBuilder*);    // Writes a human-readable datetime to the argument.
    

    /*
    * These are global resource accessor functions. They are called once from each class that
    *   requires them. That class can technically call this accessor for each use, but this should
    *   be discouraged, as the instances fetched by these functions should never change.
    */
    // Services...
    EventManager* fetchEventManager(void);
    Scheduler* fetchScheduler(void);

    void disableLogCallback();

    // Volatile statics that serve as ISRs...
    volatile static void advanceScheduler(void);
    

  protected:

    
  private:
    volatile static StaticHub* INSTANCE;
    static volatile uint32_t next_random_int[PLATFORM_RNG_CARRY_CAPACITY];  // Stores the last 10 random numbers.
    
    bool     usb_string_waiting   = false;
    StringBuilder usb_rx_buffer;
    StringBuilder last_user_input;

    // Scheduler PIDs that will be heavilly used...
    uint32_t pid_log_moderator   = 0;  // Moderate the logs running into the USB line.
    uint32_t pid_profiler_report = 0;  // Internal testing. Allows periodic profiler dumping.

    // Global system resource handles...
    EventManager event_manager;            // This is our asynchronous message queue. 
    Scheduler __scheduler;


    void print_type_sizes(void);

    // These functions handle various stages of bootstrap...
    void gpioSetup(void) volatile;        // We call this once on bootstrap. Sets up GPIO not covered by other classes.
    void init_RNG(void) volatile;         // Fire up the random number generator.
    
    void procDirectDebugInstruction(StringBuilder*);
};

#ifdef __cplusplus
}
#endif 

#endif

