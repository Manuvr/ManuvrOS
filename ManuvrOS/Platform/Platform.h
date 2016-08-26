/*
File:   Platform.h
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


This file is meant to contain a set of common functions that are
  typically platform-dependent. The goal is to make a class instance
  that is pre-processor-selectable to reflect the platform with an API
  that is consistent, thereby giving the kernel the ability to...
    * Access the realtime clock (if applicatble)
    * Get definitions for GPIO pins.
    * Access a true RNG (if it exists)
    * Persist and retrieve data across runtimes.
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

#include <CommonConstants.h>
#include <DataStructures/StringBuilder.h>

// Conditional inclusion for different threading models...
#if defined(__MANUVR_LINUX)
  #include <pthread.h>
  #include <signal.h>
  #include <sys/time.h>
#elif defined(__MANUVR_FREERTOS)

#endif


class ManuvrRunnable;
class Kernel;

/**
* Platform init is only considered complete if all of the following
*   things have been accomplished without errors (in no particular order)
* 1) RNG init and priming
* 2) Storage init and default config load.
* 3) RTC init'd and datetime validity known.
*/

//#define MANUVR_BOOT_STAGE_PLATFORM_INIT
//#define MANUVR_BOOT_STAGE_KERNEL_UP
//#define MANUVR_BOOT_STAGE_READY
//#define MANUVR_BOOT_STAGE_SHUTDOWN


#define MANUVR_PLAT_FLAG_BOOT_STAGE_MASK  0x00000007  // Lowest 3-bits for boot-state.
#define MANUVR_PLAT_FLAG_HAS_LOCATION     0x08000000  // Platform is locus-aware.
#define MANUVR_PLAT_FLAG_HAS_CRYPTO       0x10000000
#define MANUVR_PLAT_FLAG_INNATE_DATETIME  0x20000000
#define MANUVR_PLAT_FLAG_HAS_THREADS      0x40000000
#define MANUVR_PLAT_FLAG_HAS_STORAGE      0x80000000


/*
* Notes regarding hooking into the boot sequence...
* All of these functions will be invoked in the specified order (if they exist).
*
* User code wishing to hook into these calls can do so by modifying this stuct.
* See main_template.cpp for an example.
*/
typedef struct _init_hooks_struct {
  FunctionPointer rng_init        = nullptr;
  FunctionPointer rtc_init        = nullptr;
  FunctionPointer gpio_init       = nullptr;
  FunctionPointer sec_init        = nullptr;
  FunctionPointer storage_init    = nullptr;
  FunctionPointer plat_pre_init   = nullptr;
  FunctionPointer plat_post_init  = nullptr;
  FunctionPointer reboot          = nullptr;
  FunctionPointer shutdown        = nullptr;
  FunctionPointer bootloader      = nullptr;
} PlatformInitFxns;


/**
* This class should form the single reference via which all platform-specific
*   functions and features are accessed. This will allow us a more natural
*   means of extension to other platforms while retaining some linguistic
*   checks and validations.
*/
class ManuvrPlatform {
  public:
    inline bool hasLocation() {     return _check_flags(MANUVR_PLAT_FLAG_HAS_LOCATION);  };
    inline bool hasCryptography() { return _check_flags(MANUVR_PLAT_FLAG_HAS_CRYPTO);  };
    inline bool hasTimeAndData() {  return _check_flags(MANUVR_PLAT_FLAG_INNATE_DATETIME); };
    inline bool hasThreads() {      return _check_flags(MANUVR_PLAT_FLAG_HAS_THREADS); };
    inline bool hasStorage() {      return _check_flags(MANUVR_PLAT_FLAG_HAS_STORAGE); };

    /* These are bootstrap checkpoints. */
    int8_t bootstrap();
    inline void booted(bool en) { _alter_flags(en, MANUVR_PLAT_FLAG_BOOT_STAGE_MASK);  };
    inline bool booted() { return _check_flags(MANUVR_PLAT_FLAG_BOOT_STAGE_MASK == bootStage());  };

    inline void setKernel(Kernel* k) {   if (nullptr == _kernel) _kernel = k;  };
    inline Kernel* getKernel() {         return _kernel;  };

    inline uint8_t bootStage() {
      return ((uint8_t) _pflags & MANUVR_PLAT_FLAG_BOOT_STAGE_MASK);
    };

    /* These are safe function proxies for external callers. */
    //inline void idleHook() { if (_idle_hook) _idle_hook(); };
    void idleHook();

    void setIdleHook(FunctionPointer nu);

    void printDebug(StringBuilder* out);
    void printDebug();


  protected:

  private:
    uint32_t        _pflags    = 0;
    Kernel*         _kernel    = nullptr;
    FunctionPointer _idle_hook = nullptr;

    /* Inlines for altering and reading the flags. */
    inline void _alter_flags(bool en, uint32_t mask) {
      _pflags = (en) ? (_pflags | mask) : (_pflags & ~mask);
    };
    inline bool _check_flags(uint32_t mask) {
      return (_pflags == (_pflags & mask));
    };
};

extern ManuvrPlatform platform;

#if defined (ARDUINO)
  #include <Arduino.h>
#elif defined(STM32F7XX) | defined(STM32F746xx)
  #include <stm32f7xx_hal.h>

  #define HIGH           GPIO_PIN_SET
  #define LOW            GPIO_PIN_RESET

  #define INPUT          GPIO_MODE_INPUT
  #define OUTPUT         GPIO_MODE_OUTPUT_PP
  #define OUTPUT_OD      GPIO_MODE_OUTPUT_OD
  #define INPUT_PULLUP   0xFE
  #define INPUT_PULLDOWN 0xFD

  #define CHANGE             0xFC
  #define FALLING            0xFB
  #define RISING             0xFA
  #define CHANGE_PULL_UP     0xF9
  #define FALLING_PULL_UP    0xF8
  #define RISING_PULL_UP     0xF7
  #define CHANGE_PULL_DOWN   0xF6
  #define FALLING_PULL_DOWN  0xF5
  #define RISING_PULL_DOWN   0xF4
  extern "C" {
    unsigned long millis();
    unsigned long micros();
  }
#else
  // We adopt and extend Arduino GPIO access conventions.
  #define HIGH         1
  #define LOW          0

  #define INPUT        0
  #define OUTPUT       1
  #define INPUT_PULLUP 2

  #define CHANGE       4
  #define FALLING      2
  #define RISING       3
  extern "C" {
    unsigned long millis();
    unsigned long micros();
  }
#endif


#ifdef __cplusplus
 extern "C" {
#endif



typedef struct __platform_gpio_def {
  ManuvrRunnable* event;
  FunctionPointer fxn;
  uint8_t         pin;
  uint8_t         flags;
  uint16_t        mode;  // Strictly more than needed. Padding structure...
} PlatformGPIODef;

/*
* Internal utility function prototypes
*/
const char* getIRQConditionString(int);


/*
* Time and date
*/
bool initPlatformRTC();          // We call this once on bootstrap. Sets up the RTC.
bool setTimeAndDateStr(char*);   // Takes a string of the form given by RFC-2822: "Mon, 15 Aug 2005 15:52:01 +0000"   https://www.ietf.org/rfc/rfc2822.txt
bool setTimeAndDate(uint8_t y, uint8_t m, uint8_t d, uint8_t wd, uint8_t h, uint8_t mi, uint8_t s);
uint32_t epochTime();            // Returns an integer representing the current datetime.
void currentDateTime(StringBuilder*);    // Writes a human-readable datetime to the argument.

// Performs string conversion for integer type-code, and is only useful for logging.
const char* getRTCStateString(uint32_t code);

/*
* Watchdog timer.
*/

/*
* Interrupt-masking
*/
void globalIRQEnable();
void globalIRQDisable();
void maskableInterrupts(bool);

/*
* Process control
*/
volatile void jumpToBootloader();
volatile void seppuku();

/*
* Underlying system control.
*/
volatile void reboot();
volatile void hardwareShutdown();

/*
* Threading
*/
int createThread(unsigned long*, void*, ThreadFxnPtr, void*);
int deleteThread(unsigned long*);
void sleep_millis(unsigned long millis);

/*
* Misc
*/
volatile uintptr_t getStackPointer();   // Returns the value of the stack pointer.

/*
* Data-persistence functions. This is the API used by anything that wants to write
*   formless data to a place on the device to be recalled on a different runtime.
*/
//int8_t persistData(const char* store_name, uint8_t* data, int length);
bool persistCapable();        // Returns true if this platform can store data locally.
unsigned long persistFree();  // Returns the number of bytes availible to store data.


/*
* Randomness
*/
uint32_t randomInt();                        // Fetches one of the stored randoms and blocks until one is available.
volatile bool provide_random_int(uint32_t);  // Provides a new random to the pool from the RNG ISR.
void init_RNG();                             // Fire up the random number generator.


/*
* Identity and serial number.
* Do be careful exposing this stuff, as any outside knowledge of chip
*   serial numbers may compromise crypto that directly-relies on them.
* For crypto, it would be best to never use an invariant number like this directly,
*   and instead use it as a PRNG seed or some other such approach. Please see the
*   security documentation for a deeper discussion of the issues at hand.
*/
int platformSerialNumberSize();    // Returns the length of the serial number on this platform (in bytes).
int getSerialNumber(uint8_t*);     // Writes the serial number to the indicated buffer.

/* Writes a platform information string to the provided buffer. */
void manuvrPlatformInfo(StringBuilder*);

/*
* GPIO and change-notice.
*/
void   gpioSetup();        // We call this once on bootstrap. Sets up GPIO not covered by other classes.
int8_t gpioDefine(uint8_t pin, int mode);
void   unsetPinIRQ(uint8_t pin);
int8_t setPinEvent(uint8_t pin, uint8_t condition, ManuvrRunnable* isr_event);
int8_t setPinFxn(uint8_t pin, uint8_t condition, FunctionPointer fxn);
int8_t setPin(uint8_t pin, bool high);
int8_t readPin(uint8_t pin);
int8_t setPinAnalog(uint8_t pin, int);
int    readPinAnalog(uint8_t pin);


/*
* Call this once on system init to configure the basics of the platform.
*/
void platformPreInit();
void platformInit();



#ifdef __cplusplus
}
#endif

#endif
