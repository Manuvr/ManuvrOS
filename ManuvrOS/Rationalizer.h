/**
* This is a header file for error-checking combinations of options
*   at compile-time and providing defines that isolate downstream
*   code from potential mistakes or oversights in the options. As
*   a consequence, this file should be included immediately after
*   the user configuration file. Which naturally, would mean it
*   occupies a similar place in the inclusion hierarchy.
* The only inclusion in this file should be headers with similar tasks.
*/


#ifndef __MANUVR_OPTION_RATIONALIZER_H__
#define __MANUVR_OPTION_RATIONALIZER_H__


/* Threading models... */
  // None
  // pthreads   // Each of these exist independently of hardware.
  // freeRTOS   // Each of these exist independently of hardware.


/* Encodings... */
  // CBOR
  // JSON
  // Base64 (URL-Safe?)


/* Cryptographic stuff... */


/* BufferPipe support... */


/* IP support... */


#endif // __MANUVR_OPTION_RATIONALIZER_H__
