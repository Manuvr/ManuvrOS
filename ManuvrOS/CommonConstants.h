/*
File:   CommonConstants.h
Author: J. Ian Lindsay
Date:   2015.12.01

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


Apart from including the user-suppied configuration header, there
  ought to be no inclusion in this file. It is the bottom.
*/

#include <inttypes.h>
#include <stddef.h>  // TODO: Only needed for size_t

#ifndef __MANUVR_COMMON_CONSTANTS_H__
  #define __MANUVR_COMMON_CONSTANTS_H__

  #define EVENT_PRIORITY_HIGHEST            100
  #define EVENT_PRIORITY_DEFAULT              2
  #define EVENT_PRIORITY_LOWEST               0

  #define LOG_EMERG   0    /* system is unusable */
  #define LOG_ALERT   1    /* action must be taken immediately */
  #define LOG_CRIT    2    /* critical conditions */
  #define LOG_ERR     3    /* error conditions */
  #define LOG_WARNING 4    /* warning conditions */
  #define LOG_NOTICE  5    /* normal but significant condition */
  #define LOG_INFO    6    /* informational */
  #define LOG_DEBUG   7    /* debug-level messages */


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
  * These are defines for const char* that are commonly used.
  * We are collecting them here so that future additions to the
  *   platform can provide a macro to keep them flash-resident.
  */
  #define MANUVR_CONST_STR_WHOAMI           "_whoami"
  #define MANUVR_CONST_STR_PLATFORM_CONF    "_pconf"
  #define NUL_STR   "NULL"
  #define YES_STR   "Yes"
  #define NO_STR    "No"


  #if defined(MANUVR_CONF_FILE)
    #include MANUVR_CONF_FILE
  #else
    #include "ManuvrConf.h"
  #endif

  #include <Rationalizer.h>


// TODO: Function defs do not belong here. This area under re-org.
#if defined(__BUILD_HAS_BASE64)
int wrapped_base64_decode(uint8_t* dst, size_t dlen, size_t* olen, const uint8_t* src, size_t slen);
#endif
// Everytime you macro a function, baby Jesus cries.
inline float    strict_max(float    a, float    b) {  return (a > b) ? a : b; };
inline uint32_t strict_max(uint32_t a, uint32_t b) {  return (a > b) ? a : b; };
inline uint16_t strict_max(uint16_t a, uint16_t b) {  return (a > b) ? a : b; };
inline uint8_t  strict_max(uint8_t  a, uint8_t  b) {  return (a > b) ? a : b; };
inline int32_t  strict_max(int32_t  a, int32_t  b) {  return (a > b) ? a : b; };
inline int16_t  strict_max(int16_t  a, int16_t  b) {  return (a > b) ? a : b; };
inline int8_t   strict_max(int8_t   a, int8_t   b) {  return (a > b) ? a : b; };

inline float    strict_min(float    a, float    b) {  return (a < b) ? a : b; };
inline uint32_t strict_min(uint32_t a, uint32_t b) {  return (a < b) ? a : b; };
inline uint16_t strict_min(uint16_t a, uint16_t b) {  return (a < b) ? a : b; };
inline uint8_t  strict_min(uint8_t  a, uint8_t  b) {  return (a < b) ? a : b; };
inline int32_t  strict_min(int32_t  a, int32_t  b) {  return (a < b) ? a : b; };
inline int16_t  strict_min(int16_t  a, int16_t  b) {  return (a < b) ? a : b; };
inline int8_t   strict_min(int8_t   a, int8_t   b) {  return (a < b) ? a : b; };

inline uint32_t wrap_accounted_delta(uint32_t a, uint32_t b) {
  return (a > b) ? (a - b) : (b - a);
};

#endif
