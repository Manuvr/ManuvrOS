/*
File:   BQ24155.h
Author: J. Ian Lindsay
Date:   2017.06.08

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

#ifndef __BQ24155_DRIVER_H__
#define __BQ24155_DRIVER_H__

#include "Platform/Peripherals/I2C/I2CAdapter.h"
#include "Drivers/Sensors/SensorWrapper.h"

/* Sensor registers that exist in hardware */
#define BQ24155_REG_STATUS     0x00
#define BQ24155_REG_LIMITS     0x01
#define BQ24155_REG_BATT_REGU  0x02
#define BQ24155_REG_PART_REV   0x03
#define BQ24155_REG_FAST_CHRG  0x04

#define BQ24155_I2CADDR        0x6B


/*
* Pin defs for this module.
* Set pin def to 255 to mark it as unused.
*/
class BQ24155Opts {
  public:
    // If valid, will use a gpio operation to pull the SDA pin low to wake device.
    const uint8_t stat_pin;
    const uint8_t isel_pin;

    BQ24155Opts(const BQ24155Opts* o) :
      stat_pin(o->stat_pin),
      isel_pin(o->isel_pin) {};

    BQ24155Opts(uint8_t _stat_pin, uint8_t _isel_pin) :
      stat_pin(_stat_pin),
      isel_pin(_isel_pin) {};

    inline bool useStatPin() const {
      return (255 != stat_pin);
    };

    inline bool useISELPin() const {
      return (255 != isel_pin);
    };


  private:
};


class BQ24155 : public I2CDeviceWithRegisters {
  public:
    BQ24155(const BQ24155Opts*);
    ~BQ24155();


    /* Overrides from I2CDeviceWithRegisters... */
    int8_t io_op_callback(BusOp*);
    void printDebug(StringBuilder*);


    static BQ24155* INSTANCE;


  private:
    const BQ24155Opts _opts;
    bool _init_complete = false;

    int8_t init();
};

#endif  // __LTC294X_DRIVER_H__
