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


#ifndef __OFFBOARD_REGISTER_DEFS_H__
  #define __OFFBOARD_REGISTER_DEFS_H__ 1

  #include <inttypes.h>
  #include <stdlib.h>

  /*
  * Some classes (espescially those that drive hardware) may want to access registers in the device.
  *   These structures provide a uniform means of defining availible registers.
  * Software sensors can use these as well to control various aspects of their operation.
  */
  class DeviceRegister {
    public:
      DeviceRegister();
      DeviceRegister(uint16_t, uint8_t val,  uint8_t* buf, bool dirty, bool unread, bool writable);
      DeviceRegister(uint16_t, uint16_t val, uint8_t* buf, bool dirty, bool unread, bool writable);
      DeviceRegister(uint16_t, uint32_t val, uint8_t* buf, bool dirty, bool unread, bool writable);

      uint16_t addr;       // The internal address of the register.
      uint8_t  len;        // How large is the register?
      uint8_t  *val;       // A buffer that mirrors the register.
      bool     dirty;      // Marked true by the class when the value needs to be written to device.
      bool     unread;     // Marked true by automated read functions if a value changed.
      bool     writable;   // Is this register writable.

      void set(unsigned int nu_val);   // Causes the given value to be written to the register.
  };

#endif
