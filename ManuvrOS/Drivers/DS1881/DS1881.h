/*
File:   DS1881.h
Author: J. Ian Lindsay
Date:   2019.07.13

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

#ifndef __DS1881_DRIVER_H__
#define __DS1881_DRIVER_H__

#include "I2CAdapter.h"
#include "Drivers/DigitalPots/DigitalPots.h"

/* Hardware-defined registers */
#define DS1881_BASE_I2C_ADDR   0x28


class DS1881 : public I2CDevice {
  public:
    DS1881(uint8_t address);
    ~DS1881();

    /* Overrides from I2CDevice... */
    int8_t io_op_callback(BusOp*);
    void printDebug(StringBuilder*);

    DIGITALPOT_ERROR init();                              // Perform bus-related init tasks.
    DIGITALPOT_ERROR setValue(uint8_t val);               // Sets the value of both pots.
    DIGITALPOT_ERROR setValue(uint8_t pot, uint8_t val);  // Sets the value of the given pot.
    DIGITALPOT_ERROR reset(uint8_t);                      // Sets all volumes levels to given.

    inline void preserveOnDestroy(bool x) {
      preserve_state_on_destroy = x;
    };

    /*
    * Will take the device out of shutdown mode and set all the wipers
    *   to their minimum values.
    */
    inline DIGITALPOT_ERROR reset() {    return reset(0x00);  };

    inline uint8_t getValue(uint8_t pot) {
      return (pot > 1) ? 0 : 0x3F & registers[pot];
    };

    inline bool enabled() {   return true; };  // This device can't be disabled.
    inline DIGITALPOT_ERROR disable() {  return _enable(false);  };
    inline DIGITALPOT_ERROR enable() {   return _enable(true);   };

    /* Returns the maximum value of any single potentiometer. */
    inline uint16_t getRange() {  return 0x00FF;      };



  private:
    bool    dev_init;
    bool    preserve_state_on_destroy;

    uint8_t registers[3];
    uint8_t alt_values[2];

    DIGITALPOT_ERROR _enable(bool);
};

#endif   // __DS1881_DRIVER_H__
