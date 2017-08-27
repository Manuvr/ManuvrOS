/*
File:   LTC294x.cpp
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


Support for the Linear Technology battery gas guage.

TODO: It should be noted that this class assumes an LTC2942-1. This should be
        expanded in the future.
*/

#include "LTC294x.h"

#ifdef CONFIG_MANUVR_LTC294X


/*******************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here.
*******************************************************************************/

LTC294x* LTC294x::INSTANCE = nullptr;


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

/**
* Constructor. Takes pin numbers as arguments.
*/
LTC294x::LTC294x(const LTC294xOpts* o) : I2CDeviceWithRegisters(LTC294X_I2CADDR, 9, 16), _opts(o) {
  if (nullptr == LTC294x::INSTANCE) {
    LTC294x::INSTANCE = this;
  }

  if (_opts.useAlertPin()) {
    // This is the chip's default configuration.
    gpioDefine(_opts.pin, GPIOMode::INPUT_PULLUP);
  }
  else if (_opts.useCCPin()) {
    // TODO: This requires testing before it is safe to enable.
    //gpioDefine(_opts.pin, GPIOMode::OUTPUT_OD);
  }

  defineRegister(LTC294X_REG_STATUS,        (uint8_t)  0x00,   false, true,  false);
  defineRegister(LTC294X_REG_CONTROL,       (uint8_t)  0x3C,   false, false, true);
  defineRegister(LTC294X_REG_ACC_CHARGE,    (uint16_t) 0x7FFF, false, false, true);
  defineRegister(LTC294X_REG_CHRG_THRESH_H, (uint16_t) 0xFFFF, false, false, true);
  defineRegister(LTC294X_REG_CHRG_THRESH_L, (uint16_t) 0x0000, false, false, true);
  defineRegister(LTC294X_REG_VOLTAGE,       (uint16_t) 0x0000, false, false, false);
  defineRegister(LTC294X_REG_V_THRESH,      (uint16_t) 0xFF00, false, false, true);
  defineRegister(LTC294X_REG_TEMP,          (uint16_t) 0x0000, false, false, false);
  defineRegister(LTC294X_REG_TEMP_THRESH,   (uint16_t) 0xFF00, false, false, true);
}


/*
* Destructor.
*/
LTC294x::~LTC294x() {
}


/*
* Here, we will compare options and set them as defaults.
*/
int8_t LTC294x::init() {
  uint8_t val = regValue(LTC294X_REG_CONTROL);
  uint8_t rewrite = val;
  uint8_t dps = _derive_prescaler();

  // Set the prescalar if it isn't already...
  rewrite = (rewrite & 0xC7) | (dps << 3);

  // Normalize hardware against pin selection...
  if (_opts.useAlertPin()) {    rewrite = (rewrite & 0xF9) | 0x04;  }
  else if (_opts.useCCPin()) {  rewrite = (rewrite & 0xF9) | 0x02;  }
  else {                        rewrite = (rewrite & 0xF9);         }

  if (_opts.autostartReading()) {
    // Set the ADC to sample automatically.
    rewrite = (rewrite & ~LTC294X_OPT_MASK_ADC_MODE) | (uint8_t) LTC294xADCModes::AUTO;
  }

  if (val != rewrite) {
    writeIndirect(LTC294X_REG_CONTROL, rewrite, true);
  }
  else {
    _flags |= LTC294X_FLAG_INIT_CTRL;
  }

  _flags |= LTC294X_FLAG_INIT_AC;         // TODO: Convenient lie until feature done.
  _flags |= LTC294X_FLAG_INIT_THRESH_CL;  // TODO: Convenient lie until feature done.
  _flags |= LTC294X_FLAG_INIT_THRESH_CH;  // TODO: Convenient lie until feature done.

  // This is a conservative range for the 'C' variant. The 'I' variant range is
  //   -65C to 85C.
  // NOTE: That's 'f' for float. These values are Celcius.
  setTemperatureThreshold(0.0f, 70.0f);
  setVoltageThreshold(3.3f, 4.4f);

  return writeDirtyRegisters();
}


//_init_complete(true);

/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

int8_t LTC294x::register_write_cb(DeviceRegister* reg) {
  uint16_t val = (uint16_t) reg->getVal();
  StringBuilder log;
  switch (reg->addr) {
    case LTC294X_REG_CONTROL:
      _flags |= LTC294X_FLAG_INIT_CTRL;
      log.concatf("LTC294X_REG_CONTROL (W): %u\n", val);
      break;
    case LTC294X_REG_ACC_CHARGE:    // The accumulated charge register.
      _flags |= LTC294X_FLAG_INIT_AC;
      log.concatf("LTC294X_REG_ACC_CHARGE (W): %u\n", val);
      break;
    case LTC294X_REG_CHRG_THRESH_H:
      _flags |= LTC294X_FLAG_INIT_THRESH_CH;
      log.concatf("LTC294X_REG_CHRG_THRESH_H (W): %u\n", val);
      break;
    case LTC294X_REG_CHRG_THRESH_L:
      _flags |= LTC294X_FLAG_INIT_THRESH_CL;
      log.concatf("LTC294X_REG_CHRG_THRESH_L (W): %u\n", val);
      break;
    case LTC294X_REG_V_THRESH:
      _flags |= LTC294X_FLAG_INIT_THRESH_V;
      log.concatf("LTC294X_REG_V_THRESH (W): %u\n", val);
      break;
    case LTC294X_REG_TEMP_THRESH:
      _flags |= LTC294X_FLAG_INIT_THRESH_T;
      log.concatf("LTC294X_REG_TEMP_THRESH (W): %u\n", val);
      break;
    default:
      break;
  }
  reg->dirty = false;
  Kernel::log(&log);
  return 0;
}


int8_t LTC294x::register_read_cb(DeviceRegister* reg) {
  uint16_t val = (uint16_t) reg->getVal();
  StringBuilder log;
  switch (reg->addr) {
    case LTC294X_REG_STATUS:
      log.concatf("LTC294X_REG_STATUS (R): %u\n", val);
      break;
    case LTC294X_REG_CONTROL:
      log.concatf("LTC294X_REG_CONTROL (R): %u\n", val);
      break;
    case LTC294X_REG_ACC_CHARGE:    // The accumulated charge register.
    case LTC294X_REG_VOLTAGE:
    case LTC294X_REG_TEMP:
      break;
    case LTC294X_REG_CHRG_THRESH_H:
      // TODO: This will need auditing after the endianness filter in the superclass.
      _thrsh_h_chrg = val;
      log.concatf("LTC294X_REG_CHRG_THRESH_H (R): %u\n", val);
      break;
    case LTC294X_REG_CHRG_THRESH_L:
      // TODO: This will need auditing after the endianness filter in the superclass.
      _thrsh_l_chrg = val;
      log.concatf("LTC294X_REG_CHRG_THRESH_L (R): %u\n", val);
      break;
    case LTC294X_REG_V_THRESH:
      { // TODO: This will need auditing after the endianness filter in the superclass.
        _thrsh_l_volt = val & 0xFF;
        _thrsh_h_volt = (val >> 8) & 0xFF;
        log.concatf("LTC294X_REG_V_THRESH (R): %u\n", val);
      }
      break;
    case LTC294X_REG_TEMP_THRESH:
      { // TODO: This will need auditing after the endianness filter in the superclass.
        _thrsh_l_temp = val & 0xFF;
        _thrsh_h_temp = (val >> 8) & 0xFF;
        log.concatf("LTC294X_REG_TEMP_THRESH (R): %u\n", val);
      }
      break;
    default:
      break;
  }
  reg->unread = false;
  Kernel::log(&log);
  return 0;
}


/*
* Dump this item to the dev log.
*/
void LTC294x::printDebug(StringBuilder* output) {
  uint8_t dsp = _derive_prescaler();
  output->concatf("-- LTC294%c-1 %sinitialized\n", (_is_2942() ? '2' : '1'), (_init_complete() ? "" : "un"));
  if (_opts.useAlertPin()) {
    output->concatf("\tALERT pin:         %d\n", _opts.pin);
  }
  else if (_opts.useCCPin()) {
    output->concatf("\tCC pin:            %d\n", _opts.pin);
  }
  output->concatf("\tPrescaler:         %u\n", dsp);
  output->concatf("\tDev/ADC state      A%s / ", asleep() ? "sleep":"wake");
  switch (_adc_mode()) {
    case LTC294xADCModes::SLEEP:
      output->concat("SLEEP");
      break;
    case LTC294xADCModes::MANUAL_T:
      output->concat("MANUAL_T");
      break;
    case LTC294xADCModes::MANUAL_V:
      output->concat("MANUAL_V");
      break;
    case LTC294xADCModes::AUTO:
      output->concat("AUTO");
      break;
  }
  output->concat("\n\tCurrent:     \t Thresholds:\n");
  output->concatf("\t  C:   %.2f\%%\t %.2f%% / %.2f%%\n", batteryPercent(),  (dsp * 0.06640625f * _thrsh_l_chrg), (dsp * 0.06640625f * _thrsh_h_chrg));
  output->concatf("\t  V:   %.2f \t %.2f / %.2f\n", batteryVoltage(), (0.0234368f * _thrsh_l_volt), (0.0234368f * _thrsh_h_volt));
  output->concatf("\t  T:   %.2f \t %.2f / %.2f\n", temperature(),    convertT(_thrsh_l_temp << 8), convertT(_thrsh_h_temp << 8));
}



/*******************************************************************************
* Functions specific to this class....                                         *
*******************************************************************************/

int8_t LTC294x::_adc_mode(LTC294xADCModes m) {
  uint8_t val = regValue(LTC294X_REG_CONTROL);
  val = (val & ~LTC294X_OPT_MASK_ADC_MODE) | (uint8_t) m;
  return _write_control_reg(val);
}

int8_t LTC294x::sleep(bool x) {
  uint8_t val = regValue(LTC294X_REG_CONTROL);
  val = (val & 0xFE) | (x ? 1:0);
  return _write_control_reg(val);
}

/**
* @return Our best estimate of the battery's charge state, as a percentage.
*/
float LTC294x::batteryPercent() {
  return (_derive_prescaler() * 0.06640625f * regValue(LTC294X_REG_ACC_CHARGE));
}

/**
* @return The battery voltage.
*/
float LTC294x::batteryVoltage() {
  return (regValue(LTC294X_REG_VOLTAGE) * 0.00009155f);
}

/**
* @return The chip's temperature in degrees Celcius.
*/
float LTC294x::temperature() {
  return ((regValue(LTC294X_REG_TEMP) * 0.009155f) - DEG_K_C_OFFSET);
}

int8_t LTC294x::setChargeThresholds(uint16_t low, uint16_t high) {
  // TODO: Fill
  _thrsh_l_chrg = low;
  _thrsh_h_chrg = high;
  return _set_thresh_reg_charge(low, high);
}

int8_t LTC294x::setVoltageThreshold(float low, float high) {
  // NOTE: 0.0234368 == (0.00009155f >> 8)  // single byte register
  _thrsh_l_volt = (uint8_t) (low  / 0.0234368f);
  _thrsh_h_volt = (uint8_t) (high / 0.0234368f);
  return _set_thresh_reg_voltage(_thrsh_l_volt, _thrsh_h_volt);
}

int8_t LTC294x::setTemperatureThreshold(float low, float high) {
  // NOTE: 2.34368 == (0.009155f >> 8)  // single byte register
  _thrsh_l_temp = (uint8_t) (low  / 2.34368f);
  _thrsh_h_temp = (uint8_t) (high / 2.34368f);
  return _set_thresh_reg_temperature(_thrsh_l_temp, _thrsh_h_temp);
}

int8_t LTC294x::_set_thresh_reg_charge(uint16_t l, uint16_t h) {
  int8_t r0 = writeIndirect(LTC294X_REG_CHRG_THRESH_H, h, true);
  if (I2C_ERR_SLAVE_NO_ERROR == r0) {
    return writeIndirect(LTC294X_REG_CHRG_THRESH_L, l);
  }
  return r0;
};


/**
* @return The minimum servicable prescaler value for the given battery capacity.
*/
uint8_t LTC294x::_derive_prescaler() {
  uint8_t res = _opts.batt_capacity * 0.022978f;
  uint8_t ret = 7;   // The max prescaler.
  if (res < 64) { ret--; }
  if (res < 32) { ret--; }
  if (res < 16) { ret--; }
  if (res < 8)  { ret--; }
  if (res < 4)  { ret--; }
  if (res < 2)  { ret--; }
  if (res < 1)  { ret--; }
  return ret;
};


#endif  // CONFIG_MANUVR_LTC294X
