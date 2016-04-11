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


DeviceRegister::DeviceRegister(uint16_t nu_addr, uint8_t nu_val, uint8_t* buf, bool d, bool u, bool w) {
  len      = 1;
  addr     = nu_addr;
  val      = buf;
  *(val)   = nu_val;
  dirty    = d;
  unread   = u;
  writable = w;
}

DeviceRegister::DeviceRegister(uint16_t nu_addr, uint16_t nu_val, uint8_t* buf, bool d, bool u, bool w) {
  len      = 2;
  addr     = nu_addr;
  val      = buf;
  *((uint16_t*)val)   = nu_val;
  dirty    = d;
  unread   = u;
  writable = w;
}


DeviceRegister::DeviceRegister(uint16_t nu_addr, uint32_t nu_val, uint8_t* buf, bool d, bool u, bool w) {
  len      = 4;
  addr     = nu_addr;
  val      = buf;
  *((uint32_t*)val)   = nu_val;
  dirty    = d;
  unread   = u;
  writable = w;
}


DeviceRegister::DeviceRegister() {
  len      = 0;
  addr     = 0;
  val      = NULL;
  dirty    = false;
  unread   = false;
  writable = false;
}




void DeviceRegister::set(unsigned int nu_val) {
  switch (len) {
    case 1:
      *((uint8_t*) val) = (uint8_t) nu_val & 0xFF;
      break;
    case 2:
      *((uint16_t*) val) = (uint16_t) nu_val & 0xFFFF;
      break;
    case 4:
      *((uint32_t*) val) = (uint32_t) nu_val & 0xFFFFFFFF;
      break;
    default:
      return;
  }
  dirty    = true;
}
