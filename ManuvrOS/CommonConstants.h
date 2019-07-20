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


Apart from including the rationalizing configuration header, there
  ought to be no Manuvr inclusion in this file. It is the bottom.
*/

#include <inttypes.h>
#include <stddef.h>  // TODO: Only needed for size_t

#include "Rationalizer.h"


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

  #define PRINT_DIVIDER_1_STR "\n---------------------------------------------------\n"

  // Some constants for units...
  #define COMMON_UNITS_DEGREES     "degrees"
  #define COMMON_UNITS_DEG_SEC     "deg/s"
  #define COMMON_UNITS_C           "C"
  #define COMMON_UNITS_ONOFF       "On/Off"
  #define COMMON_UNITS_LUX         "lux"
  #define COMMON_UNITS_METERS      "meters"
  #define COMMON_UNITS_GAUSS       "gauss"
  #define COMMON_UNITS_MET_SEC     "m/s"
  #define COMMON_UNITS_KM_PER_HOUR "km/hr"
  #define COMMON_UNITS_ACCEL       "m/s^2"
  #define COMMON_UNITS_EPOCH       "timestamp"
  #define COMMON_UNITS_PRESSURE    "kPa"
  #define COMMON_UNITS_VOLTS       "V"
  #define COMMON_UNITS_U_VOLTS     "uV"
  #define COMMON_UNITS_AMPS        "A"
  #define COMMON_UNITS_AMP_HOURS   "Ah"
  #define COMMON_UNITS_WATTS       "W"
  #define COMMON_UNITS_PERCENT     "%"
  #define COMMON_UNITS_U_TESLA     "uT"
  #define COMMON_UNITS_MW_PER_SQCM "mW/cm^2"

  #define SENSOR_DATUM_NOT_FOUND   "Sensor datum not found"

  // Some common properties of the environment.
  #define PRESSURE_AT_SEA_LEVEL  101325.0f  // Atmosphereic pressure at sea-level.

  // Some common mathematical constants.
  #if !defined(PI)
    #define PI 3.14159265358979323846264338327950288419716939937510
  #endif

#endif    // __MANUVR_COMMON_CONSTANTS_H__
