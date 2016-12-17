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


#define ISL23345_REG_ACR        0x10
#define ISL23345_REG_WR0        0x00
#define ISL23345_REG_WR1        0x01
#define ISL23345_REG_WR2        0x02
#define ISL23345_REG_WR3        0x03


/*
*
*
*/
class ISL23345 : public I2CDeviceWithRegisters {
  public:
    ISL23345(uint8_t i2c_addr);
    virtual ~ISL23345();

    int8_t init(void);                            // Perform bus-related init tasks.
    void preserveOnDestroy(bool);

    int8_t setValue(uint8_t pot, uint8_t val);    // Sets the value of the given pot.
    uint8_t getValue(uint8_t pot);
    int8_t reset(void);                           // Sets all volumes levels to zero.
    int8_t reset(uint8_t);                        // Sets all volumes levels to given.

    int8_t disable(void);
    int8_t enable(void);
    bool enabled(void);

    uint16_t getRange(void);                      // Discover the range of this pot.

    /* Overrides from I2CDeviceWithRegisters... */
    void operationCompleteCallback(I2CBusOp*);
    void printDebug(StringBuilder*);


    static const int8_t ISL23345_ERROR_DEVICE_DISABLED;   // A caller tried to set a wiper while the device is disabled. This may work...
    static const int8_t ISL23345_ERROR_PEGGED_MAX;        // There was no error, but a call to change a wiper setting pegged the wiper at its highest position.
    static const int8_t ISL23345_ERROR_PEGGED_MIN;        // There was no error, but a call to change a wiper setting pegged the wiper at its lowest position.
    static const int8_t ISL23345_ERROR_NO_ERROR;          // There was no error.
    static const int8_t ISL23345_ERROR_ABSENT;            // The ISL23345 appears to not be connected to the bus.
    static const int8_t ISL23345_ERROR_BUS;               // Something went wrong with the i2c bus.
    static const int8_t ISL23345_ERROR_ALREADY_AT_MAX;    // A caller tried to increase the value of the wiper beyond its maximum.
    static const int8_t ISL23345_ERROR_ALREADY_AT_MIN;    // A caller tried to decrease the value of the wiper below its minimum.
    static const int8_t ISL23345_ERROR_INVALID_POT;       // The ISL23345 only has 4 potentiometers.


  private:
    bool    dev_init;
    bool    dev_enabled;
    bool    preserve_state_on_destroy;

    uint8_t values[4];
};
#endif
