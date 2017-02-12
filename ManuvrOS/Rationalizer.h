/**
* This is a header file for error-checking combinations of options
*   at compile-time and providing defines that isolate downstream
*   code from potential mistakes or oversights in the options. As
*   a consequence, this file should be included immediately after
*   the user configuration file. Which naturally, would mean it
*   occupies a similar place in the inclusion hierarchy.
* The only inclusion in this file should be headers with similar tasks.
*
*
* TODO: It would be nice to be able to wrap malloc here...
*
* TODO: Until the build system migration to kconfig is complete,
*   this is where we will (try to) itemize all the build options to
*   make the future migration easier.
*
* __BUILD_HAS_THREADS
*   __BUILD_HAS_PTHREADS
*   __BUILD_HAS_FREERTOS
*   __BUILD_HAS_ZEPHYR
*   __BUILD_HAS_RIOTOS
*   __BUILD_HAS_CONTIKI
*
*
* __BUILD_HAS_BASE64
*
* __HAS_CRYPT_WRAPPER
*   __BUILD_HAS_ASYMMETRIC
*   __BUILD_HAS_SYMMETRIC
*   __BUILD_HAS_DIGEST
*
* __HAS_IDENT_CERT
*
*/

/* Major/Minor/Patch */
#define MANUVR_SEMVER_MAJOR 1
#define MANUVR_SEMVER_MINOR 3
#define MANUVR_SEMVER_PATCH 0

/* To override the defaults, supply this at build time. */
#if defined(MANUVR_CONF_FILE)
  #include MANUVR_CONF_FILE
#else
  #include "ManuvrConf.h"
#endif

/* Cryptographic stuff... */
#include <Platform/Cryptographic/CryptOptUnifier.h>

#ifndef __MANUVR_OPTION_RATIONALIZER_H__
#define __MANUVR_OPTION_RATIONALIZER_H__

// This is the string that identifies this Manuvrable to other Manuvrables.
//   In MHB's case, this value will select the mEngine.
#ifndef FIRMWARE_NAME
  #error You need to name the firmware by providing FIRMWARE_NAME.
#endif

// How many random numbers should be cached? Must be > 0.
#ifndef PLATFORM_RNG_CARRY_CAPACITY
  #define PLATFORM_RNG_CARRY_CAPACITY 32
#endif

// How large a preallocation buffer should we keep?
#ifndef EVENT_MANAGER_PREALLOC_COUNT
  #define EVENT_MANAGER_PREALLOC_COUNT 8
#endif

#ifndef MAXIMUM_SEQUENTIAL_SKIPS
  #define MAXIMUM_SEQUENTIAL_SKIPS 20
#endif

// Debug support requires Console.
// NOTE: If your Makefile passes the __MANUVR_DEBUG option, this will be enabled regardless.
#if defined(__MANUVR_DEBUG) && !defined(MANUVR_CONSOLE_SUPPORT)
  #define MANUVR_CONSOLE_SUPPORT
#endif

// Use the build system to set default logging levels for modules.
#if defined(MANUVR_CONSOLE_SUPPORT)
  #if defined(__MANUVR_DEBUG)
    #define DEFAULT_CLASS_VERBOSITY    6
  #else
    #define DEFAULT_CLASS_VERBOSITY    4
  #endif
#else
  #define DEFAULT_CLASS_VERBOSITY      0
#endif


/*
* Threading models...
* Threading choice exists independently of platform.
*/
#if defined(__MANUVR_LINUX) | defined(__MANUVR_APPLE)
  //#pragma message "Building with pthread support."
  #define __BUILD_HAS_THREADS
  #define __BUILD_HAS_PTHREADS
#elif defined(__MANUVR_FREERTOS)
  //#pragma message "Building with freeRTOS support."
  #define __BUILD_HAS_THREADS
  #define __BUILD_HAS_FREERTOS
#elif defined(__MANUVR_ZEPHYR)
  //#pragma message "Building with Zephyr support."
  #define __BUILD_HAS_THREADS
#elif defined(__MANUVR_RIOTOS)
  //#pragma message "Building with RIOT support."
  #define __BUILD_HAS_THREADS
#elif defined(__MANUVR_CONTIKI)
  //#pragma message "Building with no threading support."
  #define MANUVR_PLATFORM_TIMER_PERIOD_MS 1
#else
  //#pragma message "Building with no threading support."
  #define MANUVR_PLATFORM_TIMER_PERIOD_MS 1
#endif

// What is the granularity of our scheduler?
#ifndef MANUVR_PLATFORM_TIMER_PERIOD_MS
  #define MANUVR_PLATFORM_TIMER_PERIOD_MS 10
#endif

/* Encodings... */
// CBOR

// JSON
#if defined(MANUVR_JSON)
  #define __BUILD_HAS_JSON
  // We do this to prevent the Makefile having to do it....
  #define HAVE_CONFIG_H
#endif

// Base64. The wrapper can only route a single implementation.
//   so we case-off exclusively, in order of preference. If we
//   are already sitting on cryptographic support, check those
//   libraries first.
#if defined(WITH_MBEDTLS) && defined(MBEDTLS_BASE64_C)
  #define __BUILD_HAS_BASE64
#elif defined (WITH_OPENSSL)

#endif



/* BufferPipe support... */

/* IP support... */
// LWIP
// Arduino
// *nix

/*
  Now that we've done all that work, we can provide some flags to the build
    system to give high-value assurances at compile-time such as....
    * Presence of cryptographic hardware.
    * Signature for firmware integrity.
    * Reliable feature maps at runtime.
    * Allowing modules written against Manuvr to easilly leverage supported
        capabilities, and providing an easy means to write fall-back code if
        a given option is not present at compile-time.
*/

/*
 Notions of Identity:
 If we have cryptographic wrappers, we can base these choices from those flags.
*/
#if defined(__BUILD_HAS_ASYMMETRIC)
  #define __HAS_IDENT_CERT        // We support X509 identity.
#endif   // __HAS_CRYPT_WRAPPER


#endif // __MANUVR_OPTION_RATIONALIZER_H__
