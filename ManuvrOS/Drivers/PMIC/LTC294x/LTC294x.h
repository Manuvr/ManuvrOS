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


#define LTC294X_FLAG_INIT_COMPLETE   0x0001  // Is the I/O pin to be treated as an output?


#define LTC294X_OPT_PIN_IS_CC   0x01  // Is the I/O pin to be treated as an output?
#define LTC294X_OPT_INTEG_SENSE 0x02  // This is a "-1" varient, and has an integrated sense resistor.
#define LTC294X_OPT_ACD_AUTO    0x04  // The converter should run automatically.


enum class LTC294xADCModes {
  SLEEP     = 0x00,
  MANUAL_T  = 0x40,
  MANUAL_V  = 0x80,
  AUTO      = 0xC0
};
#define LTC294X_OPT_MASK_ADC_MODE   0xC0


#define DEG_K_C_OFFSET   272.15f


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

    inline bool autostartReading() const {
      return (_flags & LTC294X_OPT_ACD_AUTO);
    };

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
    inline void printRegisters(StringBuilder* output) {
      I2CDeviceWithRegisters::printDebug(output);
    };

    int8_t init();
    inline int8_t refresh() {  return syncRegisters();  };

    float temperature();
    float batteryVoltage();
    float batteryCharge();

    int8_t setChargeThresholds(uint16_t low, uint16_t high);
    int8_t setVoltageThreshold(float low, float high);
    int8_t setTemperatureThreshold(float low, float high);


    static LTC294x* INSTANCE;


  private:
    const LTC294xOpts _opts;
    uint16_t _flags        = 0;
    uint16_t _thrsh_l_chrg = 0;
    uint16_t _thrsh_h_chrg = 0;
    uint8_t  _thrsh_l_temp = 0;
    uint8_t  _thrsh_h_temp = 0;
    uint8_t  _thrsh_l_volt = 0;
    uint8_t  _thrsh_h_volt = 0;

    inline int8_t  _write_control_reg(uint8_t v) {
      return writeIndirect(LTC294X_REG_CONTROL, v);
    };
    inline int8_t _set_thresh_reg_voltage(uint8_t l, uint8_t h) {
      return writeIndirect(LTC294X_REG_V_THRESH, ((uint16_t) l) + ((uint16_t) h << 8));
    };
    inline int8_t _set_thresh_reg_temperature(uint8_t l, uint8_t h) {
      return writeIndirect(LTC294X_REG_TEMP_THRESH, ((uint16_t) l) + ((uint16_t) h << 8));
    };
    inline int8_t _set_charge_register(uint16_t x) {
      return writeIndirect(LTC294X_REG_ACC_CHARGE, x);
    };
    int8_t _set_thresh_reg_charge(uint16_t l, uint16_t h);

    inline bool _analog_shutdown() {
      return regValue(LTC294X_REG_CONTROL) & 0x01;
    };

    int8_t  _adc_mode(LTC294xADCModes);
    inline LTC294xADCModes _adc_mode() {
      return (LTC294xADCModes) (regValue(LTC294X_REG_CONTROL) & LTC294X_OPT_MASK_ADC_MODE);
    };

    inline bool _is_2942() {   return (0 == (regValue(LTC294X_REG_STATUS) & 0x80));   };
    inline bool _asleep() {    return (1 == (regValue(LTC294X_REG_CONTROL) & 0x01));  };

    /**
    * Is the schedule pending execution ahread of schedule (next tick)?
    *
    * @return true if the schedule will execute ahread of schedule.
    */
    inline bool _init_complete() { return (_flags & LTC294X_FLAG_INIT_COMPLETE); };
    inline void _init_complete(bool x) {
      _flags = (x) ? (_flags | LTC294X_FLAG_INIT_COMPLETE) : (_flags & ~(LTC294X_FLAG_INIT_COMPLETE));
    };

    int8_t  _sleep(bool);
    uint8_t _derive_prescaler();

    float convertT(uint16_t v) {
      return ((0.009155f * v) - DEG_K_C_OFFSET);
    };
};


#endif  // __LTC294X_DRIVER_H__
