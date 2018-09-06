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
#include "DataStructures/StringBuilder.h"
#include "Platform/Peripherals/I2C/I2CAdapter.h"


enum class ISL23345_ERROR : int8_t {
  DEVICE_DISABLED = 3,   // A caller tried to set a wiper while the device is disabled. This may work...
  PEGGED_MAX      = 2,   // There was no error, but a call to change a wiper setting pegged the wiper at its highest position.
  PEGGED_MIN      = 1,   // There was no error, but a call to change a wiper setting pegged the wiper at its lowest position.
  NO_ERROR        = 0,   // There was no error.
  ABSENT          = -1,  // The ISL23345 appears to not be connected to the bus.
  BUS             = -2,  // Something went wrong with the i2c bus.
  ALREADY_AT_MAX  = -3,  // A caller tried to increase the value of the wiper beyond its maximum.
  ALREADY_AT_MIN  = -4,  // A caller tried to decrease the value of the wiper below its minimum.
  INVALID_POT     = -5   // The ISL23345 only has 4 potentiometers.
};


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

    ISL23345_ERROR init();                              // Perform bus-related init tasks.
    ISL23345_ERROR setValue(uint8_t pot, uint8_t val);  // Sets the value of the given pot.
    ISL23345_ERROR reset(uint8_t);                      // Sets all volumes levels to given.

    inline void preserveOnDestroy(bool x) {
      preserve_state_on_destroy = x;
    };

    /*
    * Will take the device out of shutdown mode and set all the wipers
    *   to their minimum values.
    */
    inline ISL23345_ERROR reset() {	  return reset(0x00);  };

    inline uint8_t getValue(uint8_t pot) {
      return (pot > 3) ? 0 : values[pot];
    };

    inline bool enabled() {   return dev_enabled; };  // Trivial accessor.
    inline ISL23345_ERROR disable() {  return _enable(false);  };
    inline ISL23345_ERROR enable() {   return _enable(true);   };

    /* Returns the maximum value of any single potentiometer. */
    inline uint16_t getRange() {  return 0x00FF;      };


  private:
    bool    dev_init;
    bool    dev_enabled;
    bool    preserve_state_on_destroy;

    uint8_t values[4];

    ISL23345_ERROR _enable(bool);
};
#endif
