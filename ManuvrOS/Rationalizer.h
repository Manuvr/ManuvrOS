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
* __HAS_IDENT_ONEID
*
*/


/* Cryptographic stuff... */
#include <Platform/Cryptographic/CryptOptUnifier.h>

#ifndef __MANUVR_OPTION_RATIONALIZER_H__
#define __MANUVR_OPTION_RATIONALIZER_H__

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
#else
  //#pragma message "Building with no threading support."
#endif


/* Encodings... */
// CBOR

// JSON

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
  #if defined(WRAPPED_PK_OPT_SECP256R1) && defined(WRAPPED_ASYM_ECDSA)
    #define __HAS_IDENT_ONEID     // We support OneID's asymmetric identities.
  #endif
#endif   // __HAS_CRYPT_WRAPPER


#endif // __MANUVR_OPTION_RATIONALIZER_H__
