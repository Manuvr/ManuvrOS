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

#ifdef CONFIG_MANUVR_LTC294X
#include "LTC294x.h"

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

  if (255 != _opts.alert_pin) {
    gpioDefine(_opts.alert_pin, GPIOMode::INPUT_PULLUP);
  }


  defineRegister(LTC294X_REG_STATUS,        (uint8_t) 0x00, false, true,  true);
  defineRegister(LTC294X_REG_CONTROL,       (uint8_t) 0x3C, false, false, true);
  defineRegister(LTC294X_REG_AC_MSB,        (uint8_t) 0x7F, false, false, true);
  defineRegister(LTC294X_REG_AC_LSB,        (uint8_t) 0xFF, false, false, false);
  defineRegister(LTC294X_REG_CHRG_THRESH_0, (uint8_t) 0xFF, false, false, true);
  defineRegister(LTC294X_REG_CHRG_THRESH_1, (uint8_t) 0xFF, false, false, true);
  defineRegister(LTC294X_REG_CHRG_THRESH_2, (uint8_t) 0x00, false, false, true);
  defineRegister(LTC294X_REG_CHRG_THRESH_3, (uint8_t) 0x00, false, false, false);
  defineRegister(LTC294X_REG_VOLTAGE_MSB,   (uint8_t) 0x00, false, true,  true);
  defineRegister(LTC294X_REG_VOLTAGE_LSB,   (uint8_t) 0x00, false, true,  true);
  defineRegister(LTC294X_REG_V_THRESH_HIGH, (uint8_t) 0xFF, false, false, true);
  defineRegister(LTC294X_REG_V_THRESH_LOW,  (uint8_t) 0x00, false, false, false);
  defineRegister(LTC294X_REG_TEMP_MSB,      (uint8_t) 0x00, false, true,  true);
  defineRegister(LTC294X_REG_TEMP_LSB,      (uint8_t) 0x00, false, true,  true);
  defineRegister(LTC294X_REG_T_THRESH_HIGH, (uint8_t) 0xFF, false, false, true);
  defineRegister(LTC294X_REG_T_THRESH_LOW,  (uint8_t) 0x00, false, false, false);
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
    case LTC294X_REG_AC_MSB:
      break;
    case LTC294X_REG_AC_LSB:
      break;
    case LTC294X_REG_CHRG_THRESH_0:
      break;
    case LTC294X_REG_CHRG_THRESH_1:
      break;
    case LTC294X_REG_CHRG_THRESH_2:
      break;
    case LTC294X_REG_CHRG_THRESH_3:
      break;
    case LTC294X_REG_VOLTAGE_MSB:
      break;
    case LTC294X_REG_VOLTAGE_LSB:
      break;
    case LTC294X_REG_V_THRESH_HIGH:
      break;
    case LTC294X_REG_V_THRESH_LOW:
      break;
    case LTC294X_REG_TEMP_MSB:
      break;
    case LTC294X_REG_TEMP_LSB:
      break;
    case LTC294X_REG_T_THRESH_HIGH:
      break;
    case LTC294X_REG_T_THRESH_LOW:
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
  temp->concatf("\tALERT pin:         %d\n", _opts.alert_pin);
}



/*******************************************************************************
* Functions specific to this class....                                         *
*******************************************************************************/



#endif  // CONFIG_MANUVR_LTC294X
