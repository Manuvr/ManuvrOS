/*
File:   ISL23345.h
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


This is the class that represents an ISL23345 digital potentiometer.
*/


#ifndef ISL23345_DIGIPOT_H
#define ISL23345_DIGIPOT_H 1

#include <inttypes.h>
#include <StringBuilder.h>
#include "I2CAdapter.h"
#include "Drivers/DigitalPots/DigitalPots.h"


/*
*
*
*/
class ISL23345 : public I2CDeviceWithRegisters {
  public:
    ISL23345(uint8_t i2c_addr);
    virtual ~ISL23345();

    /* Overrides from I2CDeviceWithRegisters... */
    int8_t register_write_cb(DeviceRegister*);
    int8_t register_read_cb(DeviceRegister*);
    void printDebug(StringBuilder*);

    DIGITALPOT_ERROR init();                              // Perform bus-related init tasks.
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
      return (pot > 3) ? 0 : values[pot];
    };

    inline bool enabled() {   return dev_enabled; };  // Trivial accessor.
    inline DIGITALPOT_ERROR disable() {  return _enable(false);  };
    inline DIGITALPOT_ERROR enable() {   return _enable(true);   };

    /* Returns the maximum value of any single potentiometer. */
    inline uint16_t getRange() {  return 0x00FF;      };


  private:
    bool    dev_init;
    bool    dev_enabled;
    bool    preserve_state_on_destroy;

    uint8_t values[4];

    DIGITALPOT_ERROR _enable(bool);
};
#endif
