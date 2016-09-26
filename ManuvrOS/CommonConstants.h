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

  /* These are defines for const char* that are commonly used. */
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

  /*
  * These are defines for const char* that are commonly used.
  * We are collecting them here so that future additions to the
  *   platform can provide a macro to keep them flash-resident.
  */
  #define MANUVR_CONST_STR_WHOAMI           "_whoami"
  #define MANUVR_CONST_STR_PLATFORM_CONF    "_pconf"
#endif
