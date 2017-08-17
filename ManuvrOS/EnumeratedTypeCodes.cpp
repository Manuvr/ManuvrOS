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
* Static Initializer for our type map. The enum is only good for the compiler.
* This table gives us our runtime type information. If the type isn't here, we
*   won't be able to handle it.
*/
const TypeCodeDef type_codes[] = {
  // Reserved codes
  {TCode::RESERVED      , 0, 0, "RESERVED"},
  {TCode::NONE          , 0, 0, "NONE"},

  // Numerics
  {TCode::INT8          , (TYPE_CODE_FLAG_EXPORTABLE), 1,  "INT8"},
  {TCode::UINT8         , (TYPE_CODE_FLAG_EXPORTABLE), 1,  "UINT8"},
  {TCode::INT16         , (TYPE_CODE_FLAG_EXPORTABLE), 2,  "INT16"},
  {TCode::UINT16        , (TYPE_CODE_FLAG_EXPORTABLE), 2,  "UINT16"},
  {TCode::INT32         , (TYPE_CODE_FLAG_EXPORTABLE), 4,  "INT32"},
  {TCode::UINT32        , (TYPE_CODE_FLAG_EXPORTABLE), 4,  "UINT32"},
  {TCode::FLOAT         , (TYPE_CODE_FLAG_EXPORTABLE), 4,  "FLOAT"},
  {TCode::BOOLEAN       , (TYPE_CODE_FLAG_EXPORTABLE), 1,  "BOOL"},
  {TCode::UINT128       , (TYPE_CODE_FLAG_EXPORTABLE), 16, "UINT128"},
  {TCode::INT128        , (TYPE_CODE_FLAG_EXPORTABLE), 16, "INT128"},
  {TCode::UINT64        , (TYPE_CODE_FLAG_EXPORTABLE), 8,  "UINT64"},
  {TCode::INT64         , (TYPE_CODE_FLAG_EXPORTABLE), 8,  "INT64"},
  {TCode::DOUBLE        , (TYPE_CODE_FLAG_EXPORTABLE), 8,  "DOUBLE"},

  // High-level numerics
  {TCode::VECT_3_UINT16    , (TYPE_CODE_FLAG_EXPORTABLE), 6,  "VECT_3_UINT16"},
  {TCode::VECT_3_INT16     , (TYPE_CODE_FLAG_EXPORTABLE), 6,  "VECT_3_INT16"},
  {TCode::VECT_3_FLOAT     , (TYPE_CODE_FLAG_EXPORTABLE), 12, "VECT_3_FLOAT"},
  {TCode::VECT_4_FLOAT     , (TYPE_CODE_FLAG_EXPORTABLE), 16, "VECT_4_FLOAT"},

  // String types. Note that if VARIABLE_LENGTH is set, the TypeCodeDef.fixed_len field
  //   will be interpreted as a minimum length.
  {TCode::URL           , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_VARIABLE_LENGTH | TYPE_CODE_FLAG_IS_NULL_DELIMITED), 1, "URL"},
  {TCode::STR           , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_VARIABLE_LENGTH | TYPE_CODE_FLAG_IS_NULL_DELIMITED), 1, "STR"},

  // Big types.
  {TCode::BINARY        , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_VARIABLE_LENGTH), 1, "BINARY"},
  {TCode::AUDIO         , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_VARIABLE_LENGTH), 1, "AUDIO"},
  {TCode::IMAGE         , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_VARIABLE_LENGTH), 1, "IMAGE"},

  // Pointer types.
  {TCode::UINT32_PTR    , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_IS_POINTER), 4, "UINT32_PTR"},
  {TCode::UINT16_PTR    , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_IS_POINTER), 2, "UINT16_PTR"},
  {TCode::UINT8_PTR     , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_IS_POINTER), 1, "UINT8_PTR"},
  {TCode::INT32_PTR     , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_IS_POINTER), 4, "INT32_PTR"},
  {TCode::INT16_PTR     , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_IS_POINTER), 2, "INT16_PTR"},
  {TCode::INT8_PTR      , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_IS_POINTER), 1, "INT8_PTR"},
  {TCode::FLOAT_PTR     , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_IS_POINTER), 4, "FLOAT_PTR"},
  {TCode::JSON          , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_VARIABLE_LENGTH), 1, "JSON"},

  {TCode::STR_BUILDER       , (TYPE_CODE_FLAG_EXPORTABLE | TYPE_CODE_FLAG_IS_POINTER), 1, "STR_BLDR"},
  {TCode::BUFFERPIPE        , TYPE_CODE_FLAG_IS_POINTER, 0, "BPIPE"},
  {TCode::SYS_EVENTRECEIVER , TYPE_CODE_FLAG_IS_POINTER, 0, "EVENTRECEIVER"},
  {TCode::SYS_MANUVR_XPORT  , TYPE_CODE_FLAG_IS_POINTER, 0, "XPORT"},
};


/**
* Given type code, find size in bytes. Returns 1 for variable-length arguments,
*   since this is their minimum size.
*
* @param  TCode the type code being asked about.
* @return The size of the type, or -1 on failure.
*/
int sizeOfType(TCode typecode) {
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
* Given type code, find and return the entire TypeCodeDef.
*
* @param  TCode the type code being asked about.
* @return The desired TypeCodeDef, or nullptr on "not supported".
*/
const TypeCodeDef* const getManuvrTypeDef(TCode typecode) {
  uint8_t idx = 0;
  while (idx < (sizeof(type_codes) / sizeof(TypeCodeDef))) {  // Search for the code...
    if (type_codes[idx].type_code == typecode) {
      return &type_codes[idx];
    }
    idx++;
  }
  return nullptr;
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
      return_value += sizeOfType((TCode) *(((uint8_t*)str++)));  // TODO: Horrid...
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
      if (((uint8_t)type_codes[n].type_code) == mode[i]) {
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
* Given a type code, return the string representation.
*
* @param  uint8_t the type code being asked about.
* @return The string representation. Never NULL.
*/
const char* getTypeCodeString(TCode typecode) {
  const uint16_t max = sizeof(type_codes) / sizeof(TypeCodeDef);
  for (int i = 0; i < max; i++) {
    if (typecode == type_codes[i].type_code) {
      return type_codes[i].t_name;
    }
  }
  return "<UNSUPPORTED>";
}


bool typeIsFixedLength(TCode typecode) {
  const uint16_t max = sizeof(type_codes) / sizeof(TypeCodeDef);
  for (int i = 0; i < max; i++) {
    if (typecode == type_codes[i].type_code) {
      return !(type_codes[i].type_flags & TYPE_CODE_FLAG_VARIABLE_LENGTH);
    }
  }
  return false;
}
