/*
File:   PowerManager.h
Author: J. Ian Lindsay
Date:   2017.06.21

Copyright 2017 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


This file represents the platform-specific interface to a power managment unit.
*/

#include <StringBuilder.h>

#ifndef __MANUVR_PMU_ABSTRACTION_H__
#define __MANUVR_PMU_ABSTRACTION_H__

//#define MANUVR_PMU_    0x0001  //
//#define MANUVR_PMU_    0x0002  //
//#define MANUVR_PMU_    0x0004  //
//#define MANUVR_PMU_    0x0008  //
//#define MANUVR_PMU_    0x0010  //
//#define MANUVR_PMU_    0x0020  //
//#define MANUVR_PMU_    0x0040  //
//#define MANUVR_PMU_    0x0080  //
//#define MANUVR_PMU_    0x4000  //
//#define MANUVR_PMU_    0x8000  //

//enum class PwrMgrClass : uint8_t {
//  READY    = 0x00,
//  CHARGING = 0x01
//};

/**
* This class is a gateway to block-oriented I/O. It will almost certainly need to have
*   some of it's operations run asynchronously or threaded. We leave those
*   concerns for any implementing class.
*/
class PowerManager {
  public:
    /*
    * Sense and State functions.
    */

    /*
    * Action functions.
    */
    //virtual int8_t setPowerLevel(int)  =0;


  protected:
    PowerManager();  // Protected constructor.

    virtual void printDebug(StringBuilder*);

    inline bool _pm_flag(uint16_t f) {   return ((_pm_flags & f) == f);  };
    inline void _pm_set_flag(bool nu, uint16_t _flag) {
      _pm_flags = (nu) ? (_pm_flags | _flag) : (_pm_flags & ~_flag);
    };


  private:
    uint16_t      _pm_flags    = 0;
};

#endif // __MANUVR_PMU_ABSTRACTION_H__
