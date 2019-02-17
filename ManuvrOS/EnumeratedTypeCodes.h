/*
File:   EnumeratedTypeCodes.h
Author: J. Ian Lindsay
Date:   2014.03.10

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


*/

// TODO: Might-should adopt some IANA standard code-spaces here? Is there a
//   painless way to get better inter-op? Dig...

// TODO: Also... this entire file is ugly-as-hell.

#ifndef __ENUMERATED_TYPE_CODES_H__
#define __ENUMERATED_TYPE_CODES_H__

#include <inttypes.h>
#include <stddef.h>

class BufferPipe;
class Argument;
class ManuvrMsg;

/*******************************************************************************
* Function-pointer definitions.                                                *
* These are typedefs to accomodate different types of callbacks.               *
*******************************************************************************/
/* Vanilla. */
typedef void  (*FxnPointer)();

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
enum class TCode : uint8_t {
  NONE     = 0x00,    // Reserved: Data is invalid without a type.

  /*
  * These are small enough to be cast into a pointer's space. They are therefore
  *   "pass-by-value" for classes that interchange them.
  * Cheap because: no malloc()/free() cycle.
  */
  INT8     = 0x01,    // 8-bit integer
  INT16    = 0x02,    // 16-bit integer
  INT32    = 0x03,    // 32-bit integer
  UINT8    = 0x06,    // Unsigned 8-bit integer
  UINT16   = 0x07,    // Unsigned 16-bit integer
  UINT32   = 0x08,    // Unsigned 32-bit integer
  BOOLEAN  = 0x0B,    // A boolean
  FLOAT    = 0x0C,    // A float

  /*
  * These are types that might be treatable as those above, but this would be
  *   contingent on the target and the compiler flags.
  */
  DOUBLE        = 0x0D,  // A double
  STR           = 0x0E,  // A null-terminated string
  BINARY        = 0x0F,  // A collection of bytes
  INT64         = 0x04,  // 64-bit integer
  INT128        = 0x05,  // 128-bit integer
  UINT64        = 0x09,  // Unsigned 64-bit integer
  UINT128       = 0x0A,  // Unsigned 128-bit integer
  VECT_4_FLOAT  = 0x16,  // A float vector in 4-space.
  VECT_3_FLOAT  = 0x12,  // A vector of floats in 3-space
  VECT_3_INT16  = 0x13,  // A vector of 16-bit integers in 3-space
  VECT_3_UINT16 = 0x14,  // A vector of unsigned 16-bit integers in 3-space

  // TODO: This whole block is slated for removal because it is obsoleted by
  //   flag usage.
    UINT32_PTR   =  0xA0,  // A pointer to an unsigned 32-bit integer
    UINT16_PTR   =  0xA1,  // A pointer to an unsigned 16-bit integer
    UINT8_PTR    =  0xA2,  // A pointer to an unsigned 8-bit integer
    INT32_PTR    =  0xA3,  // A pointer to a signed 32-bit integer
    INT16_PTR    =  0xA4,  // A pointer to a signed 16-bit integer
    INT8_PTR     =  0xA5,  // A pointer to a signed 8-bit integer
    FLOAT_PTR    =  0xA6,  // A pointer to a float
  // TODO: End removal marker.

  /*
  * These are aliases for buffers that contain data structured according to
  *   some published standard. Although vague, each type will be structured in
  *   a manner that describes the type-specific nuances of the data.
  * Also worth noting is that presence of the type code doesn't necessarilly
  *   imply that a binary has the machinary to handle that type. So these should
  *   not be conditionally-defined.
  */
  JSON      = 0x15,  // A JSON object. Export to other systems implies string conversion.
  CBOR      = 0x20,  // A CBOR object.
  IDENTITY  = 0x21,  // Identity.
  URL       = 0x17,  // An alias of string that carries the semantic 'URL'.
  CHAIN     = 0x18,  // An event chain.
  AUDIO     = 0x10,  // Audio stream
  IMAGE     = 0x11,  // Image data
  COLOR8    = 0x19,  // 8-bit  color data. Color is represented as RGB.
  COLOR24   = 0x1A,  // 24-bit color data. Color is represented as RGB.
  COLOR32   = 0x1B,  // 32-bit color data. Color is represented as RGB.

  /*
  * These are types for Manuvr classes and structures that are either not
  *   exportable, or degenerate into a less-structured type when exported.
  */
  ARGUMENT           = 0xA7,  // A pointer to an Argument.
  BUFFERPIPE         = 0xAE,  // A pointer to a BufferPipe.
  STR_BUILDER        = 0xAF,  // A pointer to a StringBuilder.
  SYS_EVENTRECEIVER  = 0xE0,  // A pointer to an EventReceiver.
  SYS_MANUVR_XPORT   = 0xE1,  // A pointer to a ManuvrXport.
  SYS_MANUVRMSG      = 0xE2,  // A pointer to a ManuvrMsg.
  SYS_SENSOR_WRAPPER = 0xE3,  // A pointer to a SensorWrapper.
  SYS_FXN_PTR        = 0xEC,  // FxnPointer
  SYS_THREAD_FXN_PTR = 0xED,  // ThreadFxnPtr
  SYS_ARG_FXN_PTR    = 0xEE,  // ArgumentFxnPtr
  SYS_PIPE_FXN_PTR   = 0xEF,  // PipeIOCallback

  RESERVED           = 0xFF   // Reserved for custom extension.
};

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

/* Quick inlines to facilitate moving into and out of serialization. */
inline uint8_t TcodeToInt(const TCode code) {   return (const uint8_t) code; };
inline TCode IntToTcode(const uint8_t code) {   return (const TCode) code;   };

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

/**
* Endian conversion fxns
*/
inline uint16_t endianSwap16(uint16_t x) {   return __builtin_bswap16(x);    };
inline uint32_t endianSwap32(uint32_t x) {   return __builtin_bswap32(x);    };
inline uint64_t endianSwap64(uint64_t x) {   return __builtin_bswap64(x);    };

/**
* Inlines to support erasure of native types...
*/
//inline uint8_t pointerTypeCode(float) {       return FLOAT_FM;     };
//inline uint8_t pointerTypeCode(int8_t) {      return INT8_FM;      };
//inline uint8_t pointerTypeCode(int16_t) {     return INT16_FM;     };
//inline uint8_t pointerTypeCode(int32_t) {     return INT32_FM;     };
//inline uint8_t pointerTypeCode(uint8_t) {     return UINT8_FM;     };
//inline uint8_t pointerTypeCode(uint16_t) {    return UINT16_FM;    };
//inline uint8_t pointerTypeCode(uint32_t) {    return UINT32_FM;    };


int getTypemapSizeAndPointer(const unsigned char **pointer);
int getMinimumSizeByTypeString(char *str);
bool containsVariableLengthArgument(char* mode);


#endif
