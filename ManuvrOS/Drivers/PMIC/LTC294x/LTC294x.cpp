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

//#ifdef CONFIG_MANUVR_LTC294X
#include "LTC294x.h"


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
LTC294x::LTC294x(const LTC294xOpts* o) : I2CDeviceWithRegisters(LTC294X_I2CADDR), _opts(o) {
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


  defineRegister(LTC294X_REG_STATUS,        (uint8_t) 0x00, false, true,  true);
  defineRegister(LTC294X_REG_CONTROL,       (uint8_t) 0x3C, false, false, true);
  defineRegister(LTC294X_REG_ACC_CHARGE,    (uint16_t) 0x7FFF, false, false, true);
  defineRegister(LTC294X_REG_CHRG_THRESH_H, (uint16_t) 0xFFFF, false, false, true);
  defineRegister(LTC294X_REG_CHRG_THRESH_L, (uint16_t) 0x0000, false, false, true);
  defineRegister(LTC294X_REG_VOLTAGE,       (uint16_t) 0x0000, false, true,  false);
  defineRegister(LTC294X_REG_V_THRESH,      (uint16_t) 0xFF00, false, false, true);
  defineRegister(LTC294X_REG_TEMP,          (uint16_t) 0x0000, false, true,  false);
  defineRegister(LTC294X_REG_TEMP_THRESH,   (uint16_t) 0xFF00, false, false, true);
}


/*
* Destructor.
*/
LTC294x::~LTC294x() {
}


int8_t LTC294x::init() {
  //writeIndirect(LTC294x_REG_STATUS,    0x00, true);
  //writeIndirect(LTC294x_REG_LIMITS,    0x00, true);
  //writeIndirect(LTC294x_REG_BATT_REGU, 0x00, true);
  //writeIndirect(LTC294x_REG_FAST_CHRG, 0x00);
  _init_complete = true;
  return 0;
}


/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

int8_t LTC294x::io_op_callback(BusOp* _op) {
  I2CBusOp* completed = (I2CBusOp*) _op;
  I2CDeviceWithRegisters::io_op_callback(_op);

  DeviceRegister *temp_reg = getRegisterByBaseAddress(completed->sub_addr);
  switch (completed->sub_addr) {
    case LTC294X_REG_STATUS:
      break;
    case LTC294X_REG_CONTROL:
      break;
    case LTC294X_REG_ACC_CHARGE:    // The accumulated charge register.
      break;
    case LTC294X_REG_CHRG_THRESH_H:
      break;
    case LTC294X_REG_CHRG_THRESH_L:
      break;
    case LTC294X_REG_VOLTAGE:
      break;
    case LTC294X_REG_V_THRESH:
      break;
    case LTC294X_REG_TEMP:
      break;
    case LTC294X_REG_TEMP_THRESH:
      break;

    default:
      temp_reg->unread = false;
      break;
  }

  /* Null the buffer so the bus adapter isn't tempted to free it.
    TODO: This is silly. Fix this in the API. */
  _op->buf     = nullptr;
  _op->buf_len = 0;
  return 0;
}


/*
* Dump this item to the dev log.
*/
void LTC294x::printDebug(StringBuilder* temp) {
  temp->concatf("LTC294x %sinitialized\n", (_init_complete ? "un" : ""));
  temp->concatf("\tAnalog shutdown:   %c\n", _analog_shutdown ? 'y' : 'n');
  if (_opts.useAlertPin()) {
    temp->concatf("\tALERT pin:         %d\n", _opts.pin);
  }
  else if (_opts.useCCPin()) {
    temp->concatf("\tCC pin:            %d\n", _opts.pin);
  }
}



/*******************************************************************************
* Functions specific to this class....                                         *
*******************************************************************************/

/**
* Will return the minimum servicable prescaler value for the given battery capacity.
* TODO: Re-phrase as integer arithmetic and bitshift, rather than branch-soup.
*/
uint8_t LTC294x::_derive_prescaler() {
  float res = _opts.batt_capacity * 0.023f;
  uint8_t ret = 7;   // The max prescaler.
  if (res < 64.0) { ret--; }
  if (res < 32.0) { ret--; }
  if (res < 16.0) { ret--; }
  if (res < 8.0)  { ret--; }
  if (res < 4.0)  { ret--; }
  if (res < 2.0)  { ret--; }
  if (res < 1.0)  { ret--; }
  return ret;
};

int8_t LTC294x::_analog_enabled(bool en) {
  return -1;
}

float LTC294x::batteryCharge() {
  return (0.009155f * regValue(LTC294X_REG_ACC_CHARGE));
}

float LTC294x::batteryVoltage() {
  return (0.009155f * regValue(LTC294X_REG_VOLTAGE));
}

float LTC294x::temperature() {
  return (0.009155f * regValue(LTC294X_REG_TEMP)) - 272.15f;
}

int8_t LTC294x::setChargeThresholds(uint16_t low, uint16_t high) {
  return -1;
}

int8_t LTC294x::setVoltageThreshold(uint16_t low) {
  return -1;
}

int8_t LTC294x::setTemperatureThreshold(uint16_t high) {
  return -1;
}



//#endif  // CONFIG_MANUVR_LTC294X
