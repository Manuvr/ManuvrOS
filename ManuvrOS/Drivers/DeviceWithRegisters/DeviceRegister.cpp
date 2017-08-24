/*
File:   DeviceWithRegisters.cpp
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


#include "DeviceRegister.h"
#include <Platform/Platform.h>

/**
* Private constructor to which we delegate.
*/
DeviceRegister::DeviceRegister(uint8_t* b, uint16_t a, uint8_t l, bool d, bool u, bool w) : val(b), addr(a), len(l) {
  dirty    = d;
  unread   = u;
  writable = w;
}

DeviceRegister::DeviceRegister(uint16_t nu_addr, uint8_t nu_val, uint8_t* buf, bool d, bool u, bool w) :
  DeviceRegister(buf, nu_addr, 1, d, u, w) {
  *((uint8_t*) val) = nu_val;
}

DeviceRegister::DeviceRegister(uint16_t nu_addr, uint16_t nu_val, uint8_t* buf, bool d, bool u, bool w) :
  DeviceRegister(buf, nu_addr, 2, d, u, w) {
  *((uint16_t*) val) = nu_val;
}


DeviceRegister::DeviceRegister(uint16_t nu_addr, uint32_t nu_val, uint8_t* buf, bool d, bool u, bool w) :
  DeviceRegister(buf, nu_addr, 4, d, u, w) {
  *((uint32_t*) val) = nu_val;
}


void DeviceRegister::set(unsigned int nu_val) {
  switch (len) {
    case 1:
      *((uint8_t*)  val) = nu_val & 0xFF;
      break;
    case 2:
      *((uint16_t*) val) = (platform.bigEndian() ? ((uint16_t) nu_val) : endianSwap16((uint16_t) nu_val)) & 0xFFFF;
      break;
    case 4:
      *((uint32_t*) val) = (platform.bigEndian() ? ((uint32_t) nu_val) : endianSwap32((uint32_t) nu_val)) & 0xFFFFFFFF;
      break;
  }
  dirty = true;
}


unsigned int DeviceRegister::getVal() {
  unsigned int return_value = 0;
  switch (len) {
    case 1:
      return_value = *((uint8_t*) val) & 0xFF;
      break;
    case 2:
      return_value = (platform.bigEndian() ? *((uint16_t*) val) : endianSwap16(*((uint16_t*) val))) & 0xFFFF;
      break;
    case 4:
      return_value = (platform.bigEndian() ? *((uint32_t*) val) : endianSwap32(*((uint32_t*) val))) & 0xFFFFFFFF;
      break;
    default:
      break;
  }
  return return_value;
}
