/*
File:   ADG2128.h
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

*/                                     

#ifndef ADG2128_CROSSPOINT_H
#define ADG2128_CROSSPOINT_H

#include <inttypes.h>
#include "StringBuilder/StringBuilder.h"
#include "ManuvrOS/Drivers/i2c-adapter/i2c-adapter.h"


/*
* This class represents an Analog Devices ADG2128 8x12 analog cross-point switch. This switch is controlled via i2c. 
* The 8-pin group are the columns, and the 12-pin group are rows. 
*/

class ADG2128 : public I2CDevice {
  public:
    ADG2128(uint8_t i2c_addr);
    ~ADG2128(void);

    int8_t init(void);                            // Perform bus-related init tasks.
    void preserveOnDestroy(bool);
                                 
    int8_t setRoute(uint8_t col, uint8_t row);    // Sets a route between two pins. Returns error code.
    int8_t unsetRoute(uint8_t col, uint8_t row);  // Unsets a route between two pins. Returns error code.
    int8_t reset(void);                           // Resets the entire device.
                           
    uint8_t getValue(uint8_t row);

    /* Overrides from I2CDevice... */
    void operationCompleteCallback(I2CQueuedOperation*);
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
