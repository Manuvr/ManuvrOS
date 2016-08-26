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


There ought to be no inclusion in this file. It is the bottom.
*/

#ifndef __MANUVR_COMMON_CONSTANTS_H__
  #define __MANUVR_COMMON_CONSTANTS_H__

  #define LOG_EMERG   0    /* system is unusable */
  #define LOG_ALERT   1    /* action must be taken immediately */
  #define LOG_CRIT    2    /* critical conditions */
  #define LOG_ERR     3    /* error conditions */
  #define LOG_WARNING 4    /* warning conditions */
  #define LOG_NOTICE  5    /* normal but significant condition */
  #define LOG_INFO    6    /* informational */
  #define LOG_DEBUG   7    /* debug-level messages */

  // Function-pointer definitions
  typedef void  (*FunctionPointer) ();
  typedef void* (*ThreadFxnPtr)    (void*);



  /*
  * These are just lables. We don't really ever care about the *actual* integers being defined here. Only
  *   their consistency.
  */
  #define MANUVR_RTC_STARTUP_UNINITED       0x00000000
  #define MANUVR_RTC_STARTUP_UNKNOWN        0x23196400
  #define MANUVR_RTC_OSC_FAILURE            0x23196401
  #define MANUVR_RTC_STARTUP_GOOD_UNSET     0x23196402
  #define MANUVR_RTC_STARTUP_GOOD_SET       0x23196403



  /*
  * These are flag definitions for the device self-description message.
  */
  #define DEVICE_FLAG_AUTHORITATIVE_TIME          0x00000001  // Devices that can provide accurate time.
  #define DEVICE_FLAG_INTERNET_ACCESS             0x00000002  // Can this firmware potentially provide net access?
  #define DEVICE_FLAG_MESSAGE_RELAY               0x00000004  // Can we act as a message relay?
  #define DEVICE_FLAG_MESSAGE_SEMANTICS           0x00000008  // Will this firmware supply message semantics?

  /*
  * String constants that don't need to be replicated all over the program...
  */
  #define STR_KEY_CP_IDENTITY          "CP-Identity"
#endif
