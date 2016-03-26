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
#include <Kernel.h>



/****************************************************************************************************
* Functions that convert from #define codes to something readable by a human...                     *
****************************************************************************************************/
const char* getRTCStateString(uint32_t code) {
  switch (code) {
    case MANUVR_RTC_STARTUP_UNKNOWN:     return "RTC_UNKNOWN";
    case MANUVR_RTC_OSC_FAILURE:         return "RTC_OSC_FAIL";
    case MANUVR_RTC_STARTUP_GOOD_UNSET:  return "RTC_GOOD_UNSET";
    case MANUVR_RTC_STARTUP_GOOD_SET:    return "RTC_GOOD_SET";
    default:                             return "RTC_UNDEFINED";
  }
}


/*
* Platform-abstracted function to enable or suspend interrupts. Whatever
*   that might mean on a given platform.
*/
void maskableInterrupts(bool enable) {
  if (enable) {
    globalIRQEnable();
  }
  else {
    globalIRQDisable();
  }
}


/****************************************************************************************************
* Threading                                                                                         *
****************************************************************************************************/
/**
* On linux, we support pthreads. On microcontrollers, we support FreeRTOS.
* This is the wrapper to create a new thread.
*
* @return The thread's return value.
*/
int createThread(unsigned long* _thread_id, void* _something, ThreadFxnPtr _fxn, void* _args) {
  #if defined(__MANUVR_LINUX)
    return pthread_create(_thread_id, (const pthread_attr_t*) _something, _fxn, _args);
  #elif defined(__MANUVR_FREERTOS)
    // TODO: Make the task parameters 1-to-1 with pthreads.
    //xTaskCreate((TaskFunction_t) _fxn, "nThread", 2000, (void*)Kernel::getInstance(), 1, (TaskHandle_t) &_thread_id);
    return (uint32_t) _thread_id;
  #endif
  return -1;
}


int deleteThread(unsigned long _thread_id) {
  return -1;
}


/**
* Wrapper for causing threads to sleep. This is NOT intended to be used as a delay
*   mechanism, although that use-case will work. It is more for the sake of not
*   burning CPU needlessly in polling-loops where it might be better-used elsewhere.
*
* If you are interested in delaying without suspending the entire thread, you should
*   probably use interrupts instead. 
*/
void sleep_millis(unsigned long millis) {
  #if defined(__MANUVR_LINUX)
    struct timespec t = {(long) (millis / 1000), (long) ((millis % 1000) * 1000000UL)}; 
    nanosleep(&t, &t);
  #elif defined(__MANUVR_FREERTOS)
    vTaskDelay(millis / portTICK_PERIOD_MS);
  #endif
}

