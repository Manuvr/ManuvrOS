/*
File:   ADG2128.h
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

#ifndef ADG2128_CROSSPOINT_H
#define ADG2128_CROSSPOINT_H

#include <inttypes.h>
#include "DataStructures/StringBuilder.h"
#include "Platform/Peripherals/I2C/I2CAdapter.h"


enum class ADG2128_ERROR : int8_t {
  NO_ERROR    = 0,   // There was no error.
  ABSENT      = -1,  // The ADG2128 appears to not be connected to the bus.
  BUS         = -2,  // Something went wrong with the i2c bus.
  BAD_COLUMN  = -3,  // Column was out-of-bounds.
  BAD_ROW     = -4   // Row was out-of-bounds.
};


/*
* This class represents an Analog Devices ADG2128 8x12 analog cross-point switch. This switch is controlled via i2c.
* The 8-pin group are the columns, and the 12-pin group are rows.
*/
class ADG2128 : public I2CDevice {
  public:
    ADG2128(uint8_t i2c_addr);
    virtual ~ADG2128();

    /* Overrides from I2CDevice... */
    int8_t io_op_callback(BusOp*);
    void printDebug(StringBuilder*);

    ADG2128_ERROR init();                            // Perform bus-related init tasks.
    ADG2128_ERROR setRoute(uint8_t col, uint8_t row);    // Sets a route between two pins. Returns error code.
    ADG2128_ERROR unsetRoute(uint8_t col, uint8_t row);  // Unsets a route between two pins. Returns error code.
    ADG2128_ERROR reset();                               // Resets the entire device.

    uint8_t getValue(uint8_t row);

    inline void preserveOnDestroy(bool x) {
      preserve_state_on_destroy = x;
    };


  private:
    bool dev_init;
    bool preserve_state_on_destroy;
    uint8_t values[12];

    ADG2128_ERROR readback(uint8_t row);
};
#endif
