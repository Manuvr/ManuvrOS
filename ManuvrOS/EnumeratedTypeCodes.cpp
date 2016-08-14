/*
File:   EnumeratedTypeCodes.cpp
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


#include "EnumeratedTypeCodes.h"

/**
* Static Initializer for our type map.
*/
const TypeCodeDef type_codes[] = {
  // Reserved codes
  {RSRVD_FM         , 0, 0},
  {NOTYPE_FM        , 0, 0},

  // Numerics
  {INT8_FM          , (TYPE_CODE_FLAG_EXPORTABLE), 1},
  {UINT8_FM         , (TYPE_CODE_FLAG_EXPORTABLE), 1},
  {INT16_FM         , (TYPE_CODE_FLAG_EXPORTABLE), 2},
  {UINT16_FM        , (TYPE_CODE_FLAG_EXPORTABLE), 2},
  {INT32_FM         , (TYPE_CODE_FLAG_EXPORTABLE), 4},
  {UINT32_FM        , (TYPE_CODE_FLAG_EXPORTABLE), 4},
  {FLOAT_FM         , (TYPE_CODE_FLAG_EXPORTABLE), 4},
  {BOOLEAN_FM       , (TYPE_CODE_FLAG_EXPORTABLE), 1},
  {UINT128_FM       , (TYPE_CODE_FLAG_EXPORTABLE), 16},
  {INT128_FM        , (TYPE_CODE_FLAG_EXPORTABLE), 16},
  {UINT64_FM        , (TYPE_CODE_FLAG_EXPORTABLE), 8},
  {INT64_FM         , (TYPE_CODE_FLAG_EXPORTABLE), 8},
  {DOUBLE_FM        , (TYPE_CODE_FLAG_EXPORTABLE), 8},

  // High-level numerics
  {VECT_3_UINT16    , (TYPE_CODE_FLAG_EXPORTABLE), 6},
  {VECT_3_INT16     , (TYPE_CODE_FLAG_EXPORTABLE), 6},
  {VECT_3_FLOAT     , (TYPE_CODE_FLAG_EXPORTABLE), 12},
  {VECT_4_FLOAT     , (TYPE_CODE_FLAG_EXPORTABLE), 16},

  // String types. Note that if VARIABLE_LENGTH is set, the TypeCodeDef.fixed_len field
  //   will be interpreted as a minimum length.
  {URL_FM           , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_VARIABLE_LENGTH | TYPE_CODE_FLAG_IS_NULL_DELIMITED), 1},
  {STR_FM           , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_VARIABLE_LENGTH | TYPE_CODE_FLAG_IS_NULL_DELIMITED), 1},

  // Big types.
  {BINARY_FM        , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_VARIABLE_LENGTH), 1},
  {AUDIO_FM         , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_VARIABLE_LENGTH), 1},
  {IMAGE_FM         , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_VARIABLE_LENGTH), 1},
  {RELAYED_MSG_FM   , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_VARIABLE_LENGTH), 8},

  // Pointer types.
  {UINT32_PTR_FM    , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_IS_POINTER), 4},
  {UINT16_PTR_FM    , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_IS_POINTER), 2},
  {UINT8_PTR_FM     , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_IS_POINTER), 1},
  {INT32_PTR_FM     , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_IS_POINTER), 4},
  {INT16_PTR_FM     , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_IS_POINTER), 2},
  {INT8_PTR_FM      , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_IS_POINTER), 1},
  {FLOAT_PTR_FM     , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_IS_POINTER), 4},
  {JSON_FM          , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_VARIABLE_LENGTH), 1},

  {STR_BUILDER_FM       , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_IS_POINTER), 1},
  {BUFFERPIPE_PTR_FM    , TYPE_CODE_FLAG_IS_POINTER, 0},
  {SYS_EVENTRECEIVER_FM , TYPE_CODE_FLAG_IS_POINTER, 0},
  {SYS_MANUVR_XPORT_FM  , TYPE_CODE_FLAG_IS_POINTER, 0},
};


/**
* Given type code, find size in bytes. Returns 1 for variable-length arguments,
*   since this is their minimum size.
*
* @param  uint8_t the type code being asked about.
* @return The size of the type, or -1 on failure.
*/
int sizeOfArg(uint8_t typecode) {
  int return_value = -1;
  uint8_t idx = 0;
  while (idx < (sizeof(type_codes) / sizeof(TypeCodeDef))) {  // Search for the code...
    if (type_codes[idx].type_code == typecode) {
      return type_codes[idx].fixed_len;
    }
    idx++;
  }
  return return_value;
}


/**
* We cast the type-map into an unsigned char buffer because there is no need to
*   parse it, but it DOES need to be sent to another machine occasionally.
*
* @param  const unsigned char** A receptical for the type legend.
* @return The size of the type legend, in bytes.
*/
int getTypemapSizeAndPointer(const unsigned char **pointer) {
  *(pointer) = (const unsigned char*) type_codes;
  return sizeof(type_codes);
}


/**
* Given a string of type codes, return the size required to carry it.
* Note: Variable length types will be assumed to have a length of 1.
*
* @param  char* The form of the message. NULL-terminated.
* @return The minimum size required to carry the expressed form.
*/
int getMinimumSizeByTypeString(char *str) {
  int return_value = 0;
  if (str != nullptr) {
    while (*(str)) {
      return_value += sizeOfArg(*(str++));
    }
  }
  return return_value;
}


/**
* @return true if this message contains variable-length arguments.
*/
bool containsVariableLengthArgument(char* mode) {
  int i = 0;
  while (mode[i] != 0) {
    for (unsigned int n = 0; n < sizeof(type_codes); n++) {
      if (type_codes[n].type_code == mode[i]) {
        // Does the type code match?
        if (type_codes[n].type_flags & TYPE_CODE_FLAG_VARIABLE_LENGTH) {
          // Is it variable length?
          return true;
        }
      }
    }
  }
  return false;
}


/**
* Given a type code, return the string representation for debug.
*
* @param  uint8_t the type code being asked about.
* @return The string representation. Never NULL.
*/
const char* getTypeCodeString(uint8_t typecode) {
  switch (typecode) {
    // TODO: Fill this out.
    case NOTYPE_FM:             return "NOTYPE";
    case RSRVD_FM:              return "RSRVD";
    case VECT_4_FLOAT:          return "VECT_4_FLOAT";
    case VECT_3_FLOAT:          return "VECT_3_FLOAT";
    case VECT_3_INT16:          return "VECT_3_INT16";
    case VECT_3_UINT16:         return "VECT_3_UINT16";
    case BUFFERPIPE_PTR_FM:     return "BUFFER_PIPE";
    case SYS_MANUVR_XPORT_FM:   return "MANUVR_XPORT";
    case SYS_EVENTRECEIVER_FM:  return "EVENTRECEIVER";
    case INT8_FM:               return "INT8";
    case UINT8_FM:              return "UINT8";
    case INT16_FM:              return "INT16";
    case UINT16_FM:             return "UINT16";
    case INT32_FM:              return "INT32";
    case UINT32_FM:             return "UINT32";
    case UINT64_FM:             return "UINT64";
    case INT64_FM:              return "INT64";
    case UINT128_FM:            return "UINT128";
    case INT128_FM:             return "INT128";
    case FLOAT_FM:              return "FLOAT";
    case DOUBLE_FM:             return "DOUBLE";
    case BOOLEAN_FM:            return "BOOLEAN";
    case STR_FM:                return "STR";
    case URL_FM:                return "URL";
    case BINARY_FM:             return "BINARY";
    case STR_BUILDER_FM:        return "STR_BLDR";
    case JSON_FM:               return "JSON";
    default:                    return "<UNSUPPORTED>";
  }
}
