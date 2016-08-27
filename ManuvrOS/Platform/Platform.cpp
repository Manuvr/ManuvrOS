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
#include <Kernel.h>

#include <DataStructures/uuid.h>


// TODO: I know this is horrid. I'm sick of screwing with the build system today...
#if defined(RASPI) | defined(RASPI2) | defined(RASPI3)
  #include "./PlatformRaspi.cpp"
  #include "./Raspi/DieThermometer.cpp"
  ManuvrPlatform platform;
#elif defined(__MK20DX256__) | defined(__MK20DX128__)
  #include "./Teensy3/Teensy3.cpp"
  ManuvrPlatform platform;
#elif defined(STM32F4XX)
  #include "./STM32F4/STM32F4.cpp"
  ManuvrPlatform platform;
#elif defined(STM32F7XX) | defined(STM32F746xx)
  #include "./STM32F7/STM32F7.h"
  #include "./STM32F7/STM32F7.cpp"
  ManuvrPlatform platform;
#elif defined(ARDUINO)
  #include "./Arduino/Arduino.cpp"
  ManuvrPlatform platform;
#elif defined(__MANUVR_PHOTON)
  #include "./Particle/Photon.cpp"
  ManuvrPlatform platform;
#elif defined(__MANUVR_LINUX)
  //#include "./Linux/Linux.h"
  #include "./Linux/Linux.cpp"
  ManuvrPlatform platform;
#else
  // Unsupportage.
  //#include "PlatformUnsupported.cpp"
#endif



extern "C" {

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
* Issue a human-readable string representing the condition that causes an
*   IRQ to fire.
*/
const char* getIRQConditionString(int con_code) {
  switch (con_code) {
    case RISING:   return "RISING";
    case FALLING:  return "FALLING";
    case CHANGE:   return "CHANGE";
    default:       return "<UNDEF>";
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
* Misc                                                                                              *
****************************************************************************************************/
/**
* Sometimes we question the size of the stack.
*
* @return the stack pointer at call time.
*/
volatile uintptr_t getStackPointer() {
  uintptr_t test;  // Important to not do assignment here.
  test = (uintptr_t) &test;  // Store the pointer.
  return test;
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
    TaskHandle_t taskHandle;
    portBASE_TYPE ret = xTaskCreate((TaskFunction_t) _fxn, "_t", 2048, (void*)_args, 1, &taskHandle);
    if (pdPASS == ret) {
      *_thread_id = (unsigned long) taskHandle;
      return 0;
    }
  #endif
  Kernel::log("Failed to create thread.\n");
  return -1;
}

int deleteThread(unsigned long* _thread_id) {
  #if defined(__MANUVR_LINUX)
  #elif defined(__MANUVR_FREERTOS)
  //vTaskDelete(&_thread_id);
  #endif
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


}  // extern "C"


/**
* This is called by user code to initialize the platform.
*/
int8_t ManuvrPlatform::bootstrap() {
  _kernel = Kernel::getInstance();

  /* Follow your shadow. */
  ManuvrRunnable *boot_completed_ev = Kernel::returnEvent(MANUVR_MSG_SYS_BOOT_COMPLETED);
  boot_completed_ev->priority = EVENT_PRIORITY_HIGHEST;
  Kernel::staticRaiseEvent(boot_completed_ev);

  while (0 < _kernel->procIdleFlags()) {
    // TODO: Safety! Need to be able to diagnose infinte loops.
  }

  return 0;
}



/**
* Prints platform information without necessitating the caller
*   provide a buffer.
*/
void ManuvrPlatform::printDebug() {
  StringBuilder output;
  printDebug(&output);
  Kernel::log(&output);
}
