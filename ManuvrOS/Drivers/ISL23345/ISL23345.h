/*
File:   ISL23345.h
Author: J. Ian Lindsay
Date:   2014.03.10


This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


This is the class that represents an ISL23345 digital potentiometer.
*/


#ifndef ISL23345_DIGIPOT_H
#define ISL23345_DIGIPOT_H 1

#include <inttypes.h>
#include "StringBuilder/StringBuilder.h"
#include "ManuvrOS/Drivers/i2c-adapter/i2c-adapter.h"


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
    ~ISL23345(void);
    
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
    void operationCompleteCallback(I2CQueuedOperation*);
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
