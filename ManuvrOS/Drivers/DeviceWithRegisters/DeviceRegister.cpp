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
  op_pending = false;
}

DeviceRegister::DeviceRegister(uint16_t nu_addr, uint8_t nu_val, uint8_t* buf, bool d, bool u, bool w) :
  DeviceRegister(buf, nu_addr, 1, d, u, w) {
  *((uint8_t*) val) = nu_val;
}

DeviceRegister::DeviceRegister(uint16_t nu_addr, uint16_t nu_val, uint8_t* buf, bool d, bool u, bool w) :
  DeviceRegister(buf, nu_addr, 2, d, u, w) {
  *((uint8_t*) val + 0) = (uint8_t) (nu_val >> 8);
  *((uint8_t*) val + 1) = (uint8_t) (nu_val & 0xFF);
}


DeviceRegister::DeviceRegister(uint16_t nu_addr, uint32_t nu_val, uint8_t* buf, bool d, bool u, bool w) :
  DeviceRegister(buf, nu_addr, 4, d, u, w) {
  *((uint8_t*) val + 0) = (uint8_t) (nu_val >> 24) & 0xFF;
  *((uint8_t*) val + 1) = (uint8_t) (nu_val >> 16) & 0xFF;
  *((uint8_t*) val + 2) = (uint8_t) (nu_val >> 8) & 0xFF;
  *((uint8_t*) val + 3) = (uint8_t) (nu_val & 0xFF);
}


/**
* All multibyte values are stored big-endian.
*
* @param nu_val The value to be stored in the register.
*/
void DeviceRegister::set(unsigned int nu_val) {
  switch (len) {
    case 1:
      *((uint8_t*)  val) = (uint8_t) (nu_val & 0xFF);
      break;
    case 2:
      *((uint8_t*) val + 0) = (uint8_t) (nu_val >> 8);
      *((uint8_t*) val + 1) = (uint8_t) (nu_val & 0xFF);
      break;
    case 4:
      *((uint8_t*) val + 0) = (uint8_t) (nu_val >> 24) & 0xFF;
      *((uint8_t*) val + 1) = (uint8_t) (nu_val >> 16) & 0xFF;
      *((uint8_t*) val + 2) = (uint8_t) (nu_val >> 8) & 0xFF;
      *((uint8_t*) val + 3) = (uint8_t) (nu_val & 0xFF);
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
      return_value += *((uint8_t*) val + 0) << 8;
      return_value += *((uint8_t*) val + 1);
      break;
    case 4:
      return_value += *((uint8_t*) val + 0) << 24;
      return_value += *((uint8_t*) val + 1) << 16;
      return_value += *((uint8_t*) val + 2) << 8;
      return_value += *((uint8_t*) val + 3);
      break;
    default:
      break;
  }
  return return_value;
}
