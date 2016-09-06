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
#include "Drivers/i2c-adapter/i2c-adapter.h"


/*
* This class represents an Analog Devices ADG2128 8x12 analog cross-point switch. This switch is controlled via i2c.
* The 8-pin group are the columns, and the 12-pin group are rows.
*/

class ADG2128 : public I2CDevice {
  public:
    ADG2128(uint8_t i2c_addr);
    virtual ~ADG2128();

    int8_t init(void);                            // Perform bus-related init tasks.
    void preserveOnDestroy(bool);

    int8_t setRoute(uint8_t col, uint8_t row);    // Sets a route between two pins. Returns error code.
    int8_t unsetRoute(uint8_t col, uint8_t row);  // Unsets a route between two pins. Returns error code.
    int8_t reset(void);                           // Resets the entire device.

    uint8_t getValue(uint8_t row);

    /* Overrides from I2CDevice... */
    void operationCompleteCallback(I2CBusOp*);
    void printDebug(StringBuilder*);


    static const int8_t ADG2128_ERROR_NO_ERROR;    // There was no error.
    static const int8_t ADG2128_ERROR_ABSENT;      // The ADG2128 appears to not be connected to the bus.
    static const int8_t ADG2128_ERROR_BUS;         // The ADG2128 appears to not be connected to the bus.
    static const int8_t ADG2128_ERROR_BAD_COLUMN;  // Column was out-of-bounds.
    static const int8_t ADG2128_ERROR_BAD_ROW;     // Row was out-of-bounds.


  private:
    bool    dev_init;
    bool preserve_state_on_destroy;
    uint8_t values[12];

    int8_t readback(uint8_t row);
};
#endif
