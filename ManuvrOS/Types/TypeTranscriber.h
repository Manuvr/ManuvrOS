/*
File:   TypeTranscriber.h
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


This is the base class for a type transcriber.
*/

#ifndef __MANUVR_TYPE_TRANSCRIBER_H__
#define __MANUVR_TYPE_TRANSCRIBER_H__

#include <CommonConstants.h>
#include <EnumeratedTypeCodes.h>


#if defined(MANUVR_CBOR)
  #include <Types/cbor-cpp/cbor.h>
  // Until per-type ideosyncracies are migrated to standard CBOR representations,
  //   we will be using a tag from the IANA 'unassigned' space to avoid confusion.
  //   The first byte after the tag is the native Manuvr type.
  #define MANUVR_CBOR_VENDOR_TYPE 0x00E97800

  /* If we have CBOR support, we define a helper class to assist decomposition. */
  class CBORArgListener : public cbor::listener {
    public:
      CBORArgListener(Argument**);
      ~CBORArgListener();

      void on_integer(int8_t);
      void on_integer(int16_t);
      void on_integer(int32_t);
      void on_integer(uint8_t);
      void on_integer(uint16_t);
      void on_integer(uint32_t);
      void on_float32(float value);
      void on_bytes(unsigned char* data, int size);
      void on_string(char* str);
      void on_array(int size);
      void on_map(int size);
      void on_tag(unsigned int tag);
      void on_special(unsigned int code);
      void on_error(const char* error);

      void on_extra_integer(unsigned long long value, int sign);
      void on_extra_integer(long long value, int sign);
      void on_extra_tag(unsigned long long tag);
      void on_extra_special(unsigned long long tag);

    private:
      Argument** built                = nullptr;
      char*      _wait                = nullptr;
      int        _wait_map            = 0;
      int        _wait_array          = 0;
      TCode      _pending_manuvr_tag  = TCode::NONE;

      /* Please forgive the stupid name. */
      void _caaa(Argument*);
  };
#endif  // MANUVR_CBOR


#endif  // __MANUVR_TYPE_TRANSCRIBER_H__
