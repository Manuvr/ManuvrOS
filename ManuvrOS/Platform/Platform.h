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
    * Access peripherals (true RNG, I2C/SPI/UART, GPIO, RTC, etc)
    * Provide a uniform interface around cryptographic implementations
    * Maintain a notion of Identity
    * Persist and retrieve data across runtimes
    * Setup other frameworks
    * Manage threading
*/


#ifndef __PLATFORM_SUPPORT_H__
#define __PLATFORM_SUPPORT_H__

// System-level includes.
#include <inttypes.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#include <Rationalizer.h>
#include <CommonConstants.h>
#include <DataStructures/uuid.h>

#if defined(__BUILD_HAS_PTHREADS)
  #include <pthread.h>
#elif defined(__BUILD_HAS_FREERTOS)
  extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
  }
#endif

// TODO: Split OCF off into it's own concern. Right now, it will depend on Linux.
#if defined(MANUVR_OPENINTERCONNECT)
extern "C" {
  // We intend on overlaying iotivity-constrained platform calls into our own.
  #include "iotivity/include/oc_api.h"
  #include "iotivity/port/oc_assert.h"
  #include "iotivity/port/oc_clock.h"
  #include "iotivity/port/oc_connectivity.h"
  #include "iotivity/port/oc_storage.h"
  #include "iotivity/port/oc_random.h"
  #include "iotivity/port/oc_network_events_mutex.h"
  #include "iotivity/port/oc_log.h"

  int example_main();
}
#endif   // MANUVR_OPENINTERCONNECT

#include <Kernel.h>

class Argument;
class Identity;
class Storage;


/**
* These are _pflags definitions.
*/
#define MANUVR_PLAT_FLAG_P_STATE_MASK     0x00000007  // Bits 0-2: platform-state.

#define MANUVR_PLAT_FLAG_PRIOR_BOOT       0x00000100  // Do we have memory of a prior boot?
#define MANUVR_PLAT_FLAG_RNG_READY        0x00000200  // RNG ready?
#define MANUVR_PLAT_FLAG_RTC_READY        0x00000400  // RTC ready?
#define MANUVR_PLAT_FLAG_RTC_SET          0x00000800  // RTC trust-worthy?
#define MANUVR_PLAT_FLAG_BIG_ENDIAN       0x00001000  // Big-endian?
#define MANUVR_PLAT_FLAG_ALU_WIDTH_MASK   0x00006000  // Bits 13-14: ALU width.
#define MANUVR_PLAT_FLAG_RESERVED_0       0x00008000  //

#define MANUVR_PLAT_FLAG_SERIALED         0x02000000  // Do we have a serial number?
#define MANUVR_PLAT_FLAG_HAS_IDENTITY     0x04000000  // Do we know who we are?
#define MANUVR_PLAT_FLAG_HAS_LOCATION     0x08000000  // Hardware is locus-aware.
#define MANUVR_PLAT_FLAG_RESERVED_1       0x10000000  //
#define MANUVR_PLAT_FLAG_INNATE_DATETIME  0x20000000  // Can the hardware remember the datetime?
#define MANUVR_PLAT_FLAG_RESERVED_2       0x40000000  //
#define MANUVR_PLAT_FLAG_HAS_STORAGE      0x80000000  //

/**
* Flags that reflect the state of the platform.
* Platform init is only considered complete if all of the following
*   things have been accomplished without errors (in no particular order)
* 1) RNG init and priming
* 2) Storage init and default config load.
* 3) RTC init'd and datetime validity known.
*
* Any correspondence with *nix init levels is a co-incidence, but serves
*   a similar function.
*/
#define MANUVR_INIT_STATE_UNINITIALIZED   0
#define MANUVR_INIT_STATE_RESERVED_0      1
#define MANUVR_INIT_STATE_PREINIT         2
#define MANUVR_INIT_STATE_KERNEL_BOOTING  3
#define MANUVR_INIT_STATE_POST_INIT       4
#define MANUVR_INIT_STATE_NOMINAL         5
#define MANUVR_INIT_STATE_SHUTDOWN        6
#define MANUVR_INIT_STATE_HALTED          7


enum class GPIOMode;

/* This is how we conceptualize a GPIO pin. */
// TODO: I'm fairly sure this sucks. It's too needlessly memory heavy to
//         define 60 pins this way. Can't add to a list of const's, so it can't
//         be both run-time definable AND const.
typedef struct __platform_gpio_def {
  ManuvrMsg* event;
  FxnPointer fxn;
  uint8_t         pin;
  uint8_t         flags;
  uint16_t        mode;  // Strictly more than needed. Padding structure...
} PlatformGPIODef;

/*
* When we wrap platform-provided peripherals, they will carry
* one of these type-codes.
*/
enum class PeripheralTypes {
  PERIPH_I2C,
  PERIPH_SPI,
  PERIPH_UART,
  PERIPH_BLUETOOTH,
  PERIPH_WIFI,
  PERIPH_RTC,
  PERIPH_RNG,
  PERIPH_STORAGE
};


/**
* This class should form the single reference via which all platform-specific
*   functions and features are accessed. This will allow us a more natural
*   means of extension to other platforms while retaining some linguistic
*   checks and validations.
*
* This is base platform support. It is a pure virtual, so only funtions that
*   are infrequently-called ought to be called directly as class members. Otherwise,
*   performance will suffer.
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
    inline  int8_t platformPreInit() {   return platformPreInit(nullptr); };
    virtual int8_t platformPreInit(Argument*);

    /* Platform state-reset functions. */
    virtual void seppuku() =0;    // Simple process termination. Reboots if not implemented.
    virtual void reboot()  =0;    // Simple reboot. Should be possible on any platform.
    virtual void hardwareShutdown() =0;  // For platforms that support it. Reboot if not.
    virtual void jumpToBootloader() =0;  // For platforms that support it. Reboot if not.

    /* Accessors for platform capability discovery. */
    #if defined(__BUILD_HAS_THREADS)
      inline bool hasThreads() {        return true;     };
    #else
      inline bool hasThreads() {        return false;    };
    #endif
    #if defined(__HAS_CRYPT_WRAPPER)
      inline bool hasCryptography() {   return true;     };
    #else
      inline bool hasCryptography() {   return false;    };
    #endif
    inline bool hasSerialNumber() { return _check_flags(MANUVR_PLAT_FLAG_SERIALED);        };
    inline bool hasLocation() {     return _check_flags(MANUVR_PLAT_FLAG_HAS_LOCATION);    };
    inline bool hasTimeAndDate() {  return _check_flags(MANUVR_PLAT_FLAG_INNATE_DATETIME); };
    inline bool hasStorage() {      return _check_flags(MANUVR_PLAT_FLAG_HAS_STORAGE);     };
    inline bool booted() {          return (MANUVR_INIT_STATE_NOMINAL == platformState()); };  // TODO: Painful name. Cut.
    inline bool nominalState() {    return (MANUVR_INIT_STATE_NOMINAL == platformState()); };
    inline bool bigEndian() {       return _check_flags(MANUVR_PLAT_FLAG_BIG_ENDIAN);      };
    inline uint8_t aluWidth() {
      // TODO: This is possible to do without the magic number 13... Figure out how.
      return (8 << ((_pflags & MANUVR_PLAT_FLAG_ALU_WIDTH_MASK) >> 13));
    };

    /* These are bootstrap checkpoints. */
    int8_t bootstrap();
    inline uint8_t platformState() {   return (_pflags & MANUVR_PLAT_FLAG_P_STATE_MASK);  };

    inline void advanceScheduler() {  _kernel.advanceScheduler();  };

    /* This cannot possibly return NULL. */
    inline Kernel* kernel() {          return &_kernel;  };

    /* These are storage-related members. */
    #if defined(MANUVR_STORAGE)
      Storage* fetchStorage(const char*);
      int8_t   offerStorage(const char*, Storage*);
    #endif

    /*
    * Systems that want support for dynamic runtime configurations
    *   can aggregate that config in platform.
    */
    Argument* getConfKey(const char* key);
    int8_t storeConf(Argument*);
    inline int8_t configLoaded() {   return (nullptr == _config) ? 0 : 1;  };

    inline Identity* selfIdentity() {   return _self;  };

    /* These are safe function proxies for external callers. */
    void setIdleHook(FxnPointer nu);
    void idleHook();
    void setWakeHook(FxnPointer nu);
    void wakeHook();

    /**
    * This is given as a convenience function. The programmer of an application could
    *   call this as a last-act in the main function. This function never returns.
    */
    inline void forsakeMain() {
      if (!nominalState()) {  bootstrap(); }     // Bootstrap if necessary.
      while (1) {  _kernel.procIdleFlags();  }   // Run forever.
    };

    void printCryptoOverview(StringBuilder*);
    void printConfig(StringBuilder* out);
    virtual void printDebug(StringBuilder* out);
    void printDebug();

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

    /*
    * Performs string conversions for integer codes. Only useful for logging.
    */
    const char* getRTCStateString();

    static const char* getIRQConditionString(int);
    static const char* getPlatformStateStr(int);



  protected:
    Kernel      _kernel;
    const char* _board_name;
    Argument*   _config    = nullptr;
    Identity*   _self      = nullptr;
    Identity*   _identity  = nullptr;
    #if defined(MANUVR_STORAGE)
      Storage* _storage_device = nullptr;
      // TODO: Ultimately, there will be a similar object for the crypto module.
    #endif

    ManuvrPlatform(const char* n) : _board_name(n) {
    };


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

    virtual int8_t platformPostInit() =0;

    #if defined(MANUVR_STORAGE)
      // Called during boot to load configuration.
      virtual int8_t _load_config() =0;
    #endif

    static unsigned long _start_micros;
    static unsigned long _boot_micros;


  private:
    uint32_t   _pflags    = 0;
    FxnPointer _idle_hook = nullptr;
    FxnPointer _wake_hook = nullptr;

    void _discoverALUParams();
};





#ifdef __cplusplus
extern "C" {
#endif

// TODO: Everything below this line is being considered for migration into the
//         Platform class, or eliminated in favor of independent break-outs.

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
void sleep_millis(unsigned long millis);

int createThread(unsigned long*, void*, ThreadFxnPtr, void*);
int deleteThread(unsigned long*);
int wakeThread(unsigned long);

#if defined(__BUILD_HAS_PTHREADS)
  inline int  yieldThread() {   return pthread_yield();   };
  inline void suspendThread() {  sleep_millis(100); };   // TODO
#elif defined(__BUILD_HAS_FREERTOS)
  inline int  yieldThread() {   taskYIELD();  return 0;   };
  inline void suspendThread() {  sleep_millis(100); };   // TODO
#else
  inline int  yieldThread() {   return 0;   };
  inline void suspendThread() { };
#endif


/*
* Randomness
*/
uint32_t randomInt();                        // Fetches one of the stored randoms and blocks until one is available.
int8_t random_fill(uint8_t* buf, size_t len);


/*
* GPIO and change-notice.
*/
int8_t gpioDefine(uint8_t pin, GPIOMode mode);
void   unsetPinIRQ(uint8_t pin);
int8_t setPinEvent(uint8_t pin, uint8_t condition, ManuvrMsg* isr_event);
int8_t setPinFxn(uint8_t pin, uint8_t condition, FxnPointer fxn);
int8_t setPin(uint8_t pin, bool high);
int8_t readPin(uint8_t pin);
int8_t setPinAnalog(uint8_t pin, int);
int    readPinAnalog(uint8_t pin);


#ifdef __cplusplus
}
#endif



/* Tail inclusion... */
#include <DataStructures/Argument.h>
#include <Platform/Cryptographic.h>
#include <Platform/Identity.h>

#if defined(MANUVR_STORAGE)
  #include <Platform/Storage.h>
#endif

#if defined(CONFIG_MANUVR_PRNG)
  #include "Peripherals/RNG/pcg_basic/pcg_basic.h"
#endif


// TODO: Until the final-step of the build system rework, this is how we
//         selectively support specific platforms.
#if defined(__MK20DX256__) || defined(__MK20DX128__)
  #include <Platform/Targets/Teensy3/Teensy3.h>
  typedef Teensy3 Platform;
#elif defined(STM32F7XX) || defined(STM32F746xx)
  #include <Platform/Targets/STM32F7/STM32F7.h>
  typedef STM32F7Platform Platform;
#elif defined(STM32F4XX)
  // Not yet converted
#elif defined(__MANUVR_PHOTON)
  // Not yet converted
#elif defined(__MANUVR_ESP32)
  #include <Platform/Targets/ESP32/ESP32.h>
  typedef ESP32Platform Platform;
#elif defined(ARDUINO)
  #include <Platform/Targets/Arduino/Arduino.h>
  typedef ArduinoWrapper Platform;
#elif defined(RASPI)
  #include <Platform/Targets/Raspi/Raspi.h>
  typedef Raspi Platform;
#elif defined(__MANUVR_LINUX)
  #include <Platform/Targets/Linux/Linux.h>
  typedef LinuxPlatform Platform;
#elif defined(__MANUVR_APPLE)
  #include <Platform/Targets/AppleOSX/AppleOSX.h>
  typedef ApplePlatform Platform;
#else
  // Unsupportage.
  #error Unsupported platform.
#endif

extern Platform platform;

#endif  // __PLATFORM_SUPPORT_H__
