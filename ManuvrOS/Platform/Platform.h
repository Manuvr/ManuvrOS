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
#include <DataStructures/uuid.h>

#if defined(MANUVR_STORAGE)
  #include <Storage/Storage.h>
#endif

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
* These are just lables. We don't really ever care about the *actual* integers
*   being defined here. Only their consistency.
*/
#define MANUVR_PLAT_FLAG_P_STATE_MASK     0x00000007  // Bits 0-2: platform-state.

#define MANUVR_PLAT_FLAG_PRIOR_BOOT       0x00000100  // Do we have memory of a prior boot?
#define MANUVR_PLAT_FLAG_RNG_READY        0x00000200  // RNG ready?
#define MANUVR_PLAT_FLAG_RTC_READY        0x00000400  // RTC ready?
#define MANUVR_PLAT_FLAG_RTC_SET          0x00000800  // RTC trust-worthy?
#define MANUVR_PLAT_FLAG_BIG_ENDIAN       0x00001000  // Big-endian?
#define MANUVR_PLAT_FLAG_ALU_WIDTH_MASK   0x00006000  // Bits 13-14: ALU width.

#define MANUVR_PLAT_FLAG_HAS_IDENTITY     0x04000000  // Do we know who we are?
#define MANUVR_PLAT_FLAG_HAS_LOCATION     0x08000000  // Hardware is locus-aware.
#define MANUVR_PLAT_FLAG_HAS_CRYPTO       0x10000000  // Platform supports cryptography.
#define MANUVR_PLAT_FLAG_INNATE_DATETIME  0x20000000  // Can the hardware remember the datetime?
#define MANUVR_PLAT_FLAG_HAS_THREADS      0x40000000  // Compute carved-up via threads?
#define MANUVR_PLAT_FLAG_HAS_STORAGE      0x80000000

/**
* Flags that reflect the state of the platform.
* Platform init is only considered complete if all of the following
*   things have been accomplished without errors (in no particular order)
* 1) RNG init and priming
* 2) Storage init and default config load.
* 3) RTC init'd and datetime validity known.
*/
#define MANUVR_INIT_STATE_UNINITIALIZED   0
#define MANUVR_INIT_STATE_RESERVED_0      1
#define MANUVR_INIT_STATE_PREINIT         2
#define MANUVR_INIT_STATE_KERNEL_BOOTING  3
#define MANUVR_INIT_STATE_POST_INIT       4
#define MANUVR_INIT_STATE_NOMINAL         5
#define MANUVR_INIT_STATE_SHUTDOWN        6
#define MANUVR_INIT_STATE_HALTED          7


/**
* GPIO is a platform issue. Here is a hasty attempt to stack more generality
*   into the Arduino conventions.
*/
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

/* This is how we conceptualize a GPIO pin. */
// TODO: I'm fairly sure this sucks. It's too needlessly memory heavy to
//         define 60 pins this way. Can't add to a list of const's, so it can't
//         be both run-time definable AND const.
typedef struct __platform_gpio_def {
  ManuvrRunnable* event;
  FunctionPointer fxn;
  uint8_t         pin;
  uint8_t         flags;
  uint16_t        mode;  // Strictly more than needed. Padding structure...
} PlatformGPIODef;



/**
* This class should form the single reference via which all platform-specific
*   functions and features are accessed. This will allow us a more natural
*   means of extension to other platforms while retaining some linguistic
*   checks and validations.
*
* TODO: First stage of re-org in-progress. Ultimately, this ought to be made
*         easy to compose platforms. IE:  (Base --> Linux --> Raspi)
*       This will not be hard to achieve. But: baby-steps.
*/
class ManuvrPlatform {
  public:
    /*
    * Notes regarding hooking into init functions...
    * All of these functions will be invoked in the specified order (if they exist).
    *
    * User code wishing to hook into these calls can do so by modifying this stuct.
    * See main_template.cpp for an example.
    */
    // TODO: These should be protected?
    virtual int8_t platformPreInit();
    virtual int8_t platformPostInit();

    /* Platform state-reset functions. */
    virtual void seppuku();           // Simple process termination. Reboots if not implemented.
    virtual void reboot();
    virtual void hardwareShutdown();
    virtual void jumpToBootloader();

    /* Accessors for platform capability discovery. */
    inline bool hasLocation() {     return _check_flags(MANUVR_PLAT_FLAG_HAS_LOCATION);    };
    inline bool hasTimeAndDate() {  return _check_flags(MANUVR_PLAT_FLAG_INNATE_DATETIME); };
    inline bool hasStorage() {      return _check_flags(MANUVR_PLAT_FLAG_HAS_STORAGE);     };
    inline bool hasThreads() {      return _check_flags(MANUVR_PLAT_FLAG_HAS_THREADS);     };
    inline bool hasCryptography() { return _check_flags(MANUVR_PLAT_FLAG_HAS_CRYPTO);      };
    inline bool booted() {          return (MANUVR_INIT_STATE_NOMINAL == platformState()); };
    inline bool bigEndian() {       return _check_flags(MANUVR_PLAT_FLAG_BIG_ENDIAN);      };
    inline uint8_t aluWidth() {
      // TODO: This is possible to do without the magic number 13... Figure out how.
      return (8 << ((_pflags & MANUVR_PLAT_FLAG_ALU_WIDTH_MASK) >> 13));
    };

    /* These are bootstrap checkpoints. */
    int8_t bootstrap();
    inline uint8_t platformState() {   return (_pflags & MANUVR_PLAT_FLAG_P_STATE_MASK);  };

    inline void setKernel(Kernel* k) {  if (nullptr == _kernel) _kernel = k;  };
    inline Kernel* getKernel() {        return _kernel;  };

    /* These are storage-realted members. */
    Storage* fetchStorage(const char*);
    int8_t   offerStorage(const char*, Storage*);

    /* These are safe function proxies for external callers. */
    void setIdleHook(FunctionPointer nu);
    void idleHook();
    void setWakeHook(FunctionPointer nu);
    void wakeHook();

    /* Writes a platform information string to the provided buffer. */
    const char* getRTCStateString();
    const char* getPlatformStateStr();

    void printDebug(StringBuilder* out);
    void printDebug();

    /*
    * TODO: This belongs in "Identity".
    * Identity and serial number.
    * Do be careful exposing this stuff, as any outside knowledge of chip
    *   serial numbers may compromise crypto that directly-relies on them.
    * For crypto, it would be best to never use an invariant number like this directly,
    *   and instead use it as a PRNG seed or some other such approach. Please see the
    *   security documentation for a deeper discussion of the issues at hand.
    */
    int platformSerialNumberSize();    // Returns the length of the serial number on this platform (in bytes).
    int getSerialNumber(uint8_t*);     // Writes the serial number to the indicated buffer.

    /*
    * Performs string conversions for integer codes. Only useful for logging.
    */
    static const char* getIRQConditionString(int);
    static void printPlatformBasics(StringBuilder*);  // TODO: on the copping-block.



  private:
    Kernel*         _kernel    = nullptr;
    #if defined(MANUVR_STORAGE)
      Storage* _storage_device = nullptr;
    #endif
    uint32_t        _pflags    = 0;
    FunctionPointer _idle_hook = nullptr;
    FunctionPointer _wake_hook = nullptr;

    UUID instance_serial_number;  // TODO: This belongs in "Identity".

    /* Inlines for altering and reading the flags. */
    inline void _alter_flags(bool en, uint32_t mask) {
      _pflags = (en) ? (_pflags | mask) : (_pflags & ~mask);
    };
    inline bool _check_flags(uint32_t mask) {
      return (mask == (_pflags & mask));
    };
    inline void _set_init_state(uint8_t s) {
      _pflags = ((_pflags & ~MANUVR_PLAT_FLAG_P_STATE_MASK) | s);
    };


    void _discoverALUParams();
};


extern ManuvrPlatform platform;  // TODO: Awful.


#ifdef __cplusplus
 extern "C" {
#endif

/*
* Time and date
*/
bool setTimeAndDateStr(char*);   // Takes a string of the form given by RFC-2822: "Mon, 15 Aug 2005 15:52:01 +0000"   https://www.ietf.org/rfc/rfc2822.txt
bool setTimeAndDate(uint8_t y, uint8_t m, uint8_t d, uint8_t wd, uint8_t h, uint8_t mi, uint8_t s);
uint32_t epochTime();            // Returns an integer representing the current datetime.
void currentDateTime(StringBuilder*);    // Writes a human-readable datetime to the argument.

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
* Threading
*/
int createThread(unsigned long*, void*, ThreadFxnPtr, void*);
int deleteThread(unsigned long*);
void sleep_millis(unsigned long millis);


/*
* Data-persistence functions. This is the API used by anything that wants to write
*   formless data to a place on the device to be recalled on a different runtime.
*/
//int8_t persistData(const char* store_name, uint8_t* data, int length);
unsigned long persistFree();  // Returns the number of bytes availible to store data.


/*
* Randomness
*/
uint32_t randomInt();                        // Fetches one of the stored randoms and blocks until one is available.
volatile bool provide_random_int(uint32_t);  // Provides a new random to the pool from the RNG ISR.


/*
* GPIO and change-notice.
*/
int8_t gpioDefine(uint8_t pin, int mode);
void   unsetPinIRQ(uint8_t pin);
int8_t setPinEvent(uint8_t pin, uint8_t condition, ManuvrRunnable* isr_event);
int8_t setPinFxn(uint8_t pin, uint8_t condition, FunctionPointer fxn);
int8_t setPin(uint8_t pin, bool high);
int8_t readPin(uint8_t pin);
int8_t setPinAnalog(uint8_t pin, int);
int    readPinAnalog(uint8_t pin);


#ifdef __cplusplus
}
#endif

#endif  // __PLATFORM_SUPPORT_H__
