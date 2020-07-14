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
#include <EnumeratedTypeCodes.h>


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



class BufferPipe;
class Argument;
class ManuvrMsg;

/*******************************************************************************
* Function-pointer definitions.                                                *
* These are typedefs to accomodate different types of callbacks.               *
*******************************************************************************/
/* A unifying type for different threading models. */
typedef void* (*ThreadFxnPtr)(void*);

/* Takes an Argument list and returns a code. */
typedef int (*ArgumentFxnPtr)(Argument*);

typedef int (*listenerFxnPtr)(ManuvrMsg*);

/*
* Used in Async BufferPipes to move data in idle-time.
* if direction is negative, buffer will flush toward counterparty.
* if direction is positive, buffer will flush toward application.
* if direction is zero, buffer will purge with no transfer.
*/
typedef void (*PipeIOCallback)(BufferPipe*, int direction);


/*******************************************************************************
* Type codes, flags, and other surrounding fixed values.                       *
*******************************************************************************/

/**
* These are the different flags that might be applied to a type. These should inform
*   parsing and packing for other systems, as well as inform client classes about nuances
*   of a specific type.
*/
#define TYPE_CODE_FLAG_EXPORTABLE          0b00000001   // This type is exportable to other systems, and we might receive it.
#define TYPE_CODE_FLAG_IS_POINTER          0b00000010   // Is this type a pointer type?
#define TYPE_CODE_FLAG_VARIABLE_LENGTH     0b00000100   // Some types do not have a fixed-length.
#define TYPE_CODE_FLAG_IS_NULL_DELIMITED   0b00001000   // Various string types are variable-length, yet self-delimiting.
#define TYPE_CODE_FLAG_RESERVED_3          0b00010000   // Reserved for future use.
#define TYPE_CODE_FLAG_RESERVED_2          0b00100000   // Reserved for future use.
#define TYPE_CODE_FLAG_RESERVED_1          0b01000000   // Reserved for future use.
#define TYPE_CODE_FLAG_RESERVED_0          0b10000000   // Reserved for future use.

/**
* It is important that these values remain stable for now.
*/
//enum class TCode : uint8_t {
//  /*
//  * These are aliases for buffers that contain data structured according to
//  *   some published standard. Although vague, each type will be structured in
//  *   a manner that describes the type-specific nuances of the data.
//  * Also worth noting is that presence of the type code doesn't necessarilly
//  *   imply that a binary has the machinary to handle that type. So these should
//  *   not be conditionally-defined.
//  */
//  //CHAIN     = 0x18,  // An event chain.
//
//  /*
//  * These are types for Manuvr classes and structures that are either not
//  *   exportable, or degenerate into a less-structured type when exported.
//  */
//  SYS_EVENTRECEIVER  = 0xE0,  // A pointer to an EventReceiver.
//  SYS_MANUVR_XPORT   = 0xE1,  // A pointer to a ManuvrXport.
//  SYS_MANUVRMSG      = 0xE2,  // A pointer to a ManuvrMsg.
//  SYS_SENSOR_WRAPPER = 0xE3,  // A pointer to a SensorWrapper.
//  ARGUMENT           = 0xE4,  // A pointer to an Argument.
//  BUFFERPIPE         = 0xE5,  // A pointer to a BufferPipe.
//  STR_BUILDER        = 0xE6,  // A pointer to a StringBuilder.
//  SYS_FXN_PTR        = 0xEC,  // FxnPointer
//  SYS_THREAD_FXN_PTR = 0xED,  // ThreadFxnPtr
//  SYS_ARG_FXN_PTR    = 0xEE,  // ArgumentFxnPtr
//  SYS_PIPE_FXN_PTR   = 0xEF,  // PipeIOCallback
//};

/**
* This is the structure with which we define types. These types are used by a variety of
*   systems in this program, and need to be consistent. Sometimes, we might be talking to
*   another system that lacks support for some of these types.
*
* This structrue conveys the type, its size, and any special attributes of the type.
*/
typedef struct typecode_def_t {
  const TCode    type_code;   // This field identifies the type.
  const uint8_t  type_flags;  // Flags that give us metadata about a type.
  const uint16_t fixed_len;   // If this type has a fixed length, it will be set here. 0 if no fixed length.
  const char* const t_name;   // The name of the type.
} TypeCodeDef;


/*******************************************************************************
* Support functions for dealing with type codes.                               *
*******************************************************************************/
// TODO: There are more gains possible here with const.
const TypeCodeDef* getManuvrTypeDef(const TCode);
const char* getTypeCodeString(TCode);
int sizeOfType(TCode);
bool typeIsFixedLength(TCode);

/**
* The host sends binary data little-endian. This will convert bytes to the indicated type.
* It is the responsibility of the caller to check that these bytes actually exist in the buffer.
* TODO: These are terrible and I should feel terrible for still having them.
*/
inline double   parseDoubleFromchars(unsigned char *input) {  return ((double)   *((double*)   input)); }
inline float    parseFloatFromchars(unsigned char *input) {   return ((float)    *((float*)    input)); }
inline uint32_t parseUint32Fromchars(unsigned char *input) {  return ((uint32_t) *((uint32_t*) input)); }
inline uint16_t parseUint16Fromchars(unsigned char *input) {  return ((uint16_t) *((uint16_t*) input)); }

int getTypemapSizeAndPointer(const unsigned char **pointer);
int getMinimumSizeByTypeString(char *str);
bool containsVariableLengthArgument(char* mode);


#endif    // __MANUVR_COMMON_CONSTANTS_H__
