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
#define LTC294X_REG_ACC_CHARGE    0x02  // 16-bit
#define LTC294X_REG_CHRG_THRESH_H 0x04  // 16-bit
#define LTC294X_REG_CHRG_THRESH_L 0x06  // 16-bit
#define LTC294X_REG_VOLTAGE       0x08  // 16-bit
#define LTC294X_REG_V_THRESH      0x0A  // 16-bit
#define LTC294X_REG_TEMP          0x0C  // 16-bit
#define LTC294X_REG_TEMP_THRESH   0x0E  // 16-bit

#define LTC294X_I2CADDR        0x64


#define LTC294X_OPT_PIN_IS_CC   0x01  // Is the I/O pin to be treated as an output?

/**
* Pin defs for this module.
* Set pin def to 255 to mark it as unused.
*
* Datasheet imposes the following constraints:
*   Maximum battery capacity is 5500 mAh.
*/
class LTC294xOpts {
  public:
    const uint16_t batt_capacity;  // The capacity of the battery in mAh.
    const uint8_t  pin;            // Which pin is bound to ~ALERT/CC?

    LTC294xOpts(const LTC294xOpts* o) :
      batt_capacity(o->batt_capacity),
      pin(o->pin),
      _flags(o->_flags) {};

    LTC294xOpts(uint16_t _bc, uint8_t _pin) :
      batt_capacity(_bc),
      pin(_pin),
      _flags(0) {};

    LTC294xOpts(uint16_t _bc, uint8_t _pin, uint8_t _f) :
      batt_capacity(_bc),
      pin(_pin),
      _flags(_f) {};

    inline bool useAlertPin() const {
      return (255 != pin) & !(_flags & LTC294X_OPT_PIN_IS_CC);
    };

    inline bool useCCPin() const {
      return (255 != pin) & (_flags & LTC294X_OPT_PIN_IS_CC);
    };


  private:
    const uint8_t _flags;
};



class LTC294x : public I2CDeviceWithRegisters {
  public:
    LTC294x(const LTC294xOpts*);
    ~LTC294x();

    /* Overrides from I2CDeviceWithRegisters... */
    int8_t io_op_callback(BusOp*);
    void printDebug(StringBuilder*);

    float temperature();
    float batteryVoltage();
    float batteryCharge();

    int8_t setChargeThresholds(uint16_t low, uint16_t high);
    int8_t setVoltageThreshold(uint16_t low);
    int8_t setTemperatureThreshold(uint16_t high);


    static LTC294x* INSTANCE;


  private:
    const LTC294xOpts _opts;
    bool _init_complete   = false;
    bool _analog_shutdown = false;

    uint8_t _derive_prescaler();
    int8_t  _analog_enabled(bool);

    inline bool _analog_enabled() {   return regValue(LTC294X_REG_CONTROL) & 0x01; };

    int8_t init();
};


#endif  // __LTC294X_DRIVER_H__
