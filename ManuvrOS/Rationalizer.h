/**
* This is a header file for error-checking combinations of options
*   at compile-time and providing defines that isolate downstream
*   code from potential mistakes or oversights in the options. As
*   a consequence, this file should be included immediately after
*   the user configuration file. Which naturally, would mean it
*   occupies a similar place in the inclusion hierarchy.
* The only inclusion in this file should be headers with similar tasks.
*/


/* Cryptographic stuff... */
#include <Platform/Cryptographic/CryptOptUnifier.h>

#ifndef __MANUVR_OPTION_RATIONALIZER_H__
#define __MANUVR_OPTION_RATIONALIZER_H__


/* Threading models... */
#if defined(__MANUVR_LINUX) | defined(__MANUVR_FREERTOS)
  // None
  // pthreads   // Each of these exist independently of platform.
  // freeRTOS   // Each of these exist independently of platform.
  #define __BUILD_HAS_THREADS
  //#pragma message "Building with thread support."
#endif


/* Encodings... */
  // CBOR
  // JSON
  // Base64 (URL-Safe?)

#if defined(WITH_MBEDTLS) && defined(MBEDTLS_BASE64_C)
  #define __BUILD_HAS_BASE64
#endif



/* BufferPipe support... */


/* IP support... */


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
#if defined(__HAS_CRYPT_WRAPPER)
  #define __HAS_IDENT_CERT        // We support X509 identity.
  #if defined(WRAPPED_PK_OPT_SECP256R1) && defined(WRAPPED_ASYM_ECDSA)
    #define __HAS_IDENT_ONEID     // We support OneID's asymmetric identities.
  #endif
#endif   // __HAS_CRYPT_WRAPPER


#endif // __MANUVR_OPTION_RATIONALIZER_H__
