/*
File:   LTC294x.h
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

#ifndef __LTC294X_DRIVER_H__
#define __LTC294X_DRIVER_H__

#include "Platform/Peripherals/I2C/I2CAdapter.h"
#include "Drivers/Sensors/SensorWrapper.h"


/* Sensor registers that exist in hardware */
#define LTC294X_REG_STATUS        0x00
#define LTC294X_REG_CONTROL       0x01
#define LTC294X_REG_AC_MSB        0x02
#define LTC294X_REG_AC_LSB        0x03
#define LTC294X_REG_CHRG_THRESH_0 0x04
#define LTC294X_REG_CHRG_THRESH_1 0x05
#define LTC294X_REG_CHRG_THRESH_2 0x06
#define LTC294X_REG_CHRG_THRESH_3 0x07
#define LTC294X_REG_VOLTAGE_MSB   0x08
#define LTC294X_REG_VOLTAGE_LSB   0x09
#define LTC294X_REG_V_THRESH_HIGH 0x0A
#define LTC294X_REG_V_THRESH_LOW  0x0B
#define LTC294X_REG_TEMP_MSB      0x0C
#define LTC294X_REG_TEMP_LSB      0x0D
#define LTC294X_REG_T_THRESH_HIGH 0x0E
#define LTC294X_REG_T_THRESH_LOW  0x0F

#define LTC294X_I2CADDR        0x64


/**
* Pin defs for this module.
* Set pin def to 255 to mark it as unused.
*/
class LTC294xOpts {
  public:
    // If valid, will use a gpio operation to pull the SDA pin low to wake device.
    const uint8_t alert_pin;

    LTC294xOpts(const LTC294xOpts* o) :
      alert_pin(o->alert_pin) {};

    LTC294xOpts(uint8_t _alert_pin) :
      alert_pin(_alert_pin) {};

    inline bool useAlertPin() const {
      return (255 != alert_pin);
    };


  private:
};


class LTC294x : public I2CDeviceWithRegisters {
  public:
    LTC294x(const LTC294xOpts*);
    ~LTC294x();


    /* Overrides from I2CDeviceWithRegisters... */
    int8_t io_op_callback(BusOp*);
    void printDebug(StringBuilder*);

    static LTC294x* INSTANCE;


  private:
    const LTC294xOpts _opts;
    bool _init_complete = false;

    int8_t init();
};


#endif  // __LTC294X_DRIVER_H__
