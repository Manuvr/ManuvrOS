/*
File:   TypeTranscriber.cpp
Author: J. Ian Lindsay
Date:   2016.08.13

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


This is where we collect adapters and converters between type-representations.
*/

#include "TypeTranscriber.h"
#include <DataStructures/Argument.h>
#include <CommonConstants.h>
#include <stdlib.h>  // TODO: Cut free() and malloc().

#if defined(MANUVR_CBOR)
CBORArgListener::CBORArgListener(Argument** target) {    built = target;    }

CBORArgListener::~CBORArgListener() {
  // JIC...
  if (nullptr != _wait) {
    free(_wait);
    _wait = nullptr;
  }
}

void CBORArgListener::_caaa(Argument* nu) {
  if (nullptr != _wait) {
    nu->setKey(_wait);
    _wait = nullptr;
    nu->reapKey(true);
  }

  if ((nullptr != built) && (nullptr != *built)) {
    (*built)->link(nu);
  }
  else {
    *built = nu;
  }

  if (0 < _wait_map)   _wait_map--;
  if (0 < _wait_array) _wait_array--;
}


void CBORArgListener::on_string(char* val) {
  if (nullptr == _wait) {
    // We need to copy the string. It will be the key for the Argument
    //   who's value is forthcoming.
    int len = strlen(val);
    _wait = (char*) malloc(len+1);
    if (nullptr != _wait) {
      memcpy(_wait, val, len+1);
    }
  }
  else {
    // There is a key assignment waiting. This must be the value.
    _caaa(new Argument(val));
  }
};

void CBORArgListener::on_bytes(uint8_t* data, int size) {
  if (TCode::NONE != _pending_manuvr_tag) {
    // If we've seen our vendor code in a tag, we interpret the first byte as a Manuvr
    //   Typecode, and build an Argument the hard way.
    const TypeCodeDef* const m_type_def = getManuvrTypeDef(_pending_manuvr_tag) ;
    if (m_type_def) {
      if (m_type_def->fixed_len) {
        if (size == (m_type_def->fixed_len)) {
          _caaa(new Argument(data, (m_type_def->fixed_len), m_type_def->type_code));
        }
      }
      else {
        // We will have to pass validation to the Argument class.
        _caaa(new Argument((data+1), (size-1), m_type_def->type_code));
      }
    }
    _pending_manuvr_tag = TCode::NONE;
  }
  else {
    _caaa(new Argument(data, size));
  }
};

void CBORArgListener::on_integer(int8_t v) {           _caaa(new Argument(v));               };
void CBORArgListener::on_integer(int16_t v) {          _caaa(new Argument(v));               };
void CBORArgListener::on_integer(int32_t v) {          _caaa(new Argument(v));               };
void CBORArgListener::on_integer(uint8_t v) {          _caaa(new Argument(v));               };
void CBORArgListener::on_integer(uint16_t v) {         _caaa(new Argument(v));               };
void CBORArgListener::on_integer(uint32_t v) {         _caaa(new Argument(v));               };
void CBORArgListener::on_float32(float f) {            _caaa(new Argument(f));               };
void CBORArgListener::on_double(double f) {            _caaa(new Argument(f));               };
void CBORArgListener::on_special(unsigned int code) {  _caaa(new Argument((uint32_t) code)); };
void CBORArgListener::on_error(const char* error) {    _caaa(new Argument(error));           };

void CBORArgListener::on_undefined() {   _caaa(new Argument("<UNDEF>"));   };
void CBORArgListener::on_null() {        _caaa(new Argument("<NULL>"));    };
void CBORArgListener::on_bool(bool x) {  _caaa(new Argument(x));           };

// NOTE: IANA gives of _some_ guidance....
// https://www.iana.org/assignments/cbor-tags/cbor-tags.xhtml
void CBORArgListener::on_tag(unsigned int tag) {
  switch (tag & 0xFFFFFF00) {
    case MANUVR_CBOR_VENDOR_TYPE:
      _pending_manuvr_tag = IntToTcode(tag & 0x000000FF);
      break;
    default:
      break;
  }
};

void CBORArgListener::on_array(int size) {
  _wait_array = (int32_t) size;
};

void CBORArgListener::on_map(int size) {
  _wait_map   = (int32_t) size;

  if (nullptr != _wait) {
    // Flush so we can discover problems.
    free(_wait);
    _wait = nullptr;
  }
};

void CBORArgListener::on_extra_integer(unsigned long long value, int sign) {}
void CBORArgListener::on_extra_integer(long long value, int sign) {}
void CBORArgListener::on_extra_tag(unsigned long long tag) {}
void CBORArgListener::on_extra_special(unsigned long long tag) {}

#endif  // MANUVR_CBOR
