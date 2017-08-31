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


// oxoo
#define BQ24155_OPT_FLAG_DISABLE_STAT    0x4000

enum class BQ24155Fault {
  NOMINAL      = 0x00,
  VBUS_OVP     = 0x01,
  SLEEP        = 0x02,
  VBUS_SAG     = 0x03,
  OUTPUT_OVP   = 0x04,
  THERMAL      = 0x05,
  TIMER        = 0x06,
  NO_BATTERY   = 0x07
};

enum class BQ24155State {
  READY    = 0x00,
  CHARGING = 0x01,
  CHARGED  = 0x02,
  FAULT    = 0x03
};

// oxo1
enum class BQ24155USBCurrent {
  LIMIT_100 = 0x00,
  LIMIT_500 = 0x01,
  LIMIT_800 = 0x02,
  NO_LIMIT  = 0x03
};

#define BQ24155_OPT_FLAG_UHC_UNLIM 0x00C0
#define BQ24155_OPT_FLAG_UHC_800MA 0x0080
#define BQ24155_OPT_FLAG_UHC_500MA 0x0040
#define BQ24155_OPT_FLAG_UHC_100MA 0x0000
#define BQ24155_OPT_FLAG_UHC_MASK  0x00C0  // Mask: USB current limits

#define BQ24155_OPT_FLAG_WB_3V7    0x0030
#define BQ24155_OPT_FLAG_WB_3V6    0x0020
#define BQ24155_OPT_FLAG_WB_3V5    0x0010
#define BQ24155_OPT_FLAG_WB_3V4    0x0000
#define BQ24155_OPT_FLAG_WB_MASK   0x0030  // Mask: weak battery threshold

/* Driver state flags. */
#define BQ24155_FLAG_INIT_COMPLETE 0x01    // Is the device initialized?


// TODO: Remove the rest of the floating point from this class.
/* Offsets for various register manipulations. */
#define BQ24155_VOREGU_OFFSET  3.5f
#define BQ24155_VLOW_OFFSET    3400
#define BQ24155_VITERM_OFFSET  0.0034f
#define BQ24155_VIREGU_OFFSET  0.0374f


/*
* Pin defs and options for this module.
* Set pin def to 255 to mark it as unused.
* Pass no flags to accept default hardware assumptions.
*/
class BQ24155Opts {
  public:
    // If valid, will use a gpio operation to pull the SDA pin low to wake device.
    const uint16_t sense_milliohms;
    const uint8_t  stat_pin;
    const uint8_t  isel_pin;

    BQ24155Opts(const BQ24155Opts* o) :
      sense_milliohms(o->sense_milliohms),
      stat_pin(o->stat_pin),
      isel_pin(o->isel_pin) {};

    /**
    * Constructor.
    *
    * @param sense_milliohms
    * @param stat_pin
    * @param isel_pin
    */
    BQ24155Opts(
      uint16_t _smo,
      uint8_t _stat_pin = 255,
      uint8_t _isel_pin = 255
    ) :
      sense_milliohms(_smo),
      stat_pin(_stat_pin),
      isel_pin(_isel_pin)
    {};

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
    int8_t register_write_cb(DeviceRegister*);
    int8_t register_read_cb(DeviceRegister*);
    void printDebug(StringBuilder*);
    inline void printRegisters(StringBuilder* output) {
      I2CDeviceWithRegisters::printDebug(output);
    };

    int8_t init();
    inline int8_t refresh() {  return syncRegisters();  };
    inline int8_t regRead(int a) {    return readRegister(a);  };

    int8_t   put_charger_in_reset_mode();
    int8_t   punch_safety_timer();

    bool     charger_enabled();
    int8_t   charger_enabled(bool en);
    bool     charge_current_termination_enabled();
    int8_t   charge_current_termination_enabled(bool en);

    int16_t  usb_current_limit();
    int8_t   usb_current_limit(int16_t milliamps);

    uint16_t batt_weak_voltage();
    int8_t   batt_weak_voltage(unsigned int mV);

    float    batt_reg_voltage();
    int8_t   batt_reg_voltage(float);

    BQ24155Fault getFault();
    BQ24155State getChargerState();

    static BQ24155* INSTANCE;


  private:
    const BQ24155Opts _opts;
    uint8_t _flgs = 0;

    // Flag manipulation inlines.
    inline bool _flag(uint8_t _flag) {        return (_flgs & _flag);  };
    inline void _flip_flag(uint8_t _flag) {   _flgs ^= _flag;          };
    inline void _clear_flag(uint8_t _flag) {  _flgs &= ~_flag;         };
    inline void _set_flag(uint8_t _flag) {    _flgs |= _flag;          };
    inline void _set_flag(uint8_t _flag, bool nu) {
      if (nu) _flgs |= _flag;
      else    _flgs &= ~_flag;
    };

    /**
    * Conversion fxn.
    * Given an optional step, returns the corresponding terminal-phase charging
    *   current (expressed in amps).
    * Compiler should optimize away the floating point div. TODO: verify this.
    *
    * @param integer for the step count.
    * @return The number of amps delivered in the termination phase.
    */
    inline float terminateChargeCurrent(uint8_t step = 0) {
      return (((step & 0x07) * (3.4f / _opts.sense_milliohms)) + BQ24155_VITERM_OFFSET);
    };

    /**
    * Conversion fxn.
    * Given an optional step, returns the corresponding bulk-phase charging
    *   current (expressed in amps).
    * Compiler should optimize away the floating point divs. TODO: verify this.
    *
    * @param integer for the step count.
    * @return The number of amps delivered in the bulk phase.
    */
    inline float bulkChargeCurrent(uint8_t step = 0) {
      return (((step & 0x07) * (6.8f / _opts.sense_milliohms)) + (BQ24155_VIREGU_OFFSET / _opts.sense_milliohms));
    };
};

#endif  // __LTC294X_DRIVER_H__
