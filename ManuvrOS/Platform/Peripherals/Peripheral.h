/*
File:   Peripheral.h
Author: J. Ian Lindsay
Date:   2017.05.20

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


This is the basal interface for a peripheral driver. Instances of these drivers
  will be loaded into Platform to provide the abstract interface to the
  represented hardware.
*/

#include <CommonConstants.h>


#define PDRVR_FLAG_RESERVED_00000001 0x00000001  //
#define PDRVR_FLAG_RESERVED_00000002 0x00000002  //
#define PDRVR_FLAG_RESERVED_00000004 0x00000004  //
#define PDRVR_FLAG_RESERVED_00000008 0x00000008  //
#define PDRVR_FLAG_RESERVED_00000010 0x00000010  //
#define PDRVR_FLAG_RESERVED_00000020 0x00000020  //
#define PDRVR_FLAG_RESERVED_00000040 0x00000040  //
#define PDRVR_FLAG_RESERVED_00000080 0x00000080  //
#define PDRVR_FLAG_RESERVED_00000100 0x00000100  //
#define PDRVR_FLAG_RESERVED_00000200 0x00000200  //
#define PDRVR_FLAG_RESERVED_00000400 0x00000400  //
#define PDRVR_FLAG_RESERVED_00000800 0x00000800  //
#define PDRVR_FLAG_RESERVED_00001000 0x00001000  //
#define PDRVR_FLAG_RESERVED_00002000 0x00002000  //
#define PDRVR_FLAG_RESERVED_00004000 0x00004000  //
#define PDRVR_FLAG_RESERVED_00008000 0x00008000  //
#define PDRVR_FLAG_RESERVED_00010000 0x00010000  //
#define PDRVR_FLAG_RESERVED_00020000 0x00020000  //
#define PDRVR_FLAG_RESERVED_00040000 0x00040000  //
#define PDRVR_FLAG_RESERVED_00080000 0x00080000  //
#define PDRVR_FLAG_RESERVED_00100000 0x00100000  //
#define PDRVR_FLAG_RESERVED_00200000 0x00200000  //
#define PDRVR_FLAG_RESERVED_00400000 0x00400000  //
#define PDRVR_FLAG_RESERVED_00800000 0x00800000  //
#define PDRVR_FLAG_RESERVED_01000000 0x01000000  //
#define PDRVR_FLAG_RESERVED_02000000 0x02000000  //
#define PDRVR_FLAG_RESERVED_04000000 0x04000000  //
#define PDRVR_FLAG_RESERVED_08000000 0x08000000  //
#define PDRVR_FLAG_RESERVED_10000000 0x10000000  //
#define PDRVR_FLAG_RESERVED_20000000 0x20000000  //
#define PDRVR_FLAG_RESERVED_40000000 0x40000000  //
#define PDRVR_FLAG_RESERVED_80000000 0x80000000  //

#define PDRVR_FLAG_CLASS_MASK      0x00000000  //
#define PDRVR_FLAG_TEMPLATE_MASK   0x00000000  // Mask for downstream template.


/*
* Pin defs for this module.
* Set pin def to 255 to mark it as unused.
*/
class PeripheralOpts {
  public:
    PeripheralOpts(const PeripheralOpts* o) {};

    PeripheralOpts() {};


  private:
};



class PeriphDrvr {
  public:
    inline const char*    getDriverName() {   return _driver_name;   }
    inline const uint32_t getDriverClass() {  return _driver_flags & PDRVR_FLAG_CLASS_MASK;  }

  protected:
    PeriphDrvr(const char* nom, uint32_t flags);

  private:
    char const* const _driver_name;
    const uint32_t    _driver_flags;
};
