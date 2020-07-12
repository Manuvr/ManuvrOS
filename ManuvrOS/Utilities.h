/*
File:   Utilities.h
Author: J. Ian Lindsay
Date:   2018.10.28

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

#include <inttypes.h>
#include <stddef.h>  // TODO: Only needed for size_t
#include <CppPotpourri.h>

#include "Rationalizer.h"


#ifndef __MANUVR_ISOLATED_UTILITIES_H__
#define __MANUVR_ISOLATED_UTILITIES_H__

class StringBuilder;

#if defined(__BUILD_HAS_BASE64)
  int wrapped_base64_decode(uint8_t* dst, size_t dlen, size_t* olen, const uint8_t* src, size_t slen);
  int wrapped_base64_encode(uint8_t* dst, size_t dlen, size_t* olen, const uint8_t* src, size_t slen);
#endif


void epochTimeToString(uint64_t, StringBuilder*);  //

#endif    // __MANUVR_ISOLATED_UTILITIES_H__
