/*
File:   DeviceWithRegisters.h
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


#ifndef __OFFBOARD_REGISTER_DEV_H__
  #define __OFFBOARD_REGISTER_DEV_H__ 1

  #include <StringBuilder.h>
  #include "DeviceRegister.h"

  /*
  * This class is a representation of a generic offboard device that contains
  *   its own registers.
  */
  class DeviceWithRegisters {
    public:
      DeviceWithRegisters(uint8_t r_count);
      ~DeviceWithRegisters();

    protected:
      DeviceRegister *reg_defs;  // Here is where they will be enumerated.
      uint8_t*  register_pool        = NULL;
      uint8_t   reg_count;       // This is how many registers this device has.
      bool      multi_access_support = false;   // TODO: Make this happen! Depends on pooled_registers.

      unsigned int regValue(uint8_t idx);
      bool reg_valid(uint8_t idx);

      void mark_it_zero(uint8_t);   // Zeroes the register without marking it dirty or unread.
      void mark_it_zero();          // Zeroes all registers.

      /* An implementation must override these with functions appropriate for its bus/transport. */
      virtual int8_t writeRegister(DeviceRegister *reg) = 0;
      virtual int8_t readRegister(DeviceRegister *reg)  = 0;

      /* Debug stuff... */
      void dumpDevRegs(StringBuilder*);
  };

#endif
