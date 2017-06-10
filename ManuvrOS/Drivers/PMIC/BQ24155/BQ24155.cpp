/*
File:   BQ24155.cpp
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


Support for the Texas Instruments li-ion charger.
*/

#ifdef CONFIG_MANUVR_BQ24155

#include "BQ24155.h"

/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/
/*
* Constructor. Takes pin numbers as arguments.
*/
BQ24155::BQ24155(const BQ24155Opts* o) : I2CDeviceWithRegisters(BQ24155_I2CADDR), _opts(o) {
  if (nullptr == BQ24155::INSTANCE) {
    BQ24155::INSTANCE = this;
  }

  if (255 != _opts.stat_pin) {
    gpioDefine(_opts.stat_pin, GPIOMode::INPUT_PULLUP);
  }

  if (255 != _opts.isel_pin) {
    gpioDefine(_opts.isel_pin, GPIOMode::OUTPUT);
    setPin(_opts.isel_pin, false);  // Defaults to 100mA charge current.
  }

  defineRegister(BQ24155_REG_STATUS,    (uint8_t) 0x00, false, true,  true);
  defineRegister(BQ24155_REG_LIMITS,    (uint8_t) 0x30, false, false, true);
  defineRegister(BQ24155_REG_BATT_REGU, (uint8_t) 0x0A, false, false, true);
  defineRegister(BQ24155_REG_PART_REV,  (uint8_t) 0x00, false, true,  false);
  defineRegister(BQ24155_REG_FAST_CHRG, (uint8_t) 0x89, false, false, true);
}


/*
* Destructor.
*/
BQ24155::~BQ24155() {
}


int8_t BQ24155::init() {
  //writeIndirect(BQ24155_REG_STATUS,    0x00, true);
  //writeIndirect(BQ24155_REG_LIMITS,    0x00, true);
  //writeIndirect(BQ24155_REG_BATT_REGU, 0x00, true);
  //writeIndirect(BQ24155_REG_FAST_CHRG, 0x00);
  _init_complete = true;
  return 0;
}


/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

int8_t BQ24155::io_op_callback(BusOp* _op) {
  I2CBusOp* completed = (I2CBusOp*) _op;
  I2CDeviceWithRegisters::io_op_callback(_op);

  DeviceRegister *temp_reg = getRegisterByBaseAddress(completed->sub_addr);
  switch (completed->sub_addr) {
      case BQ24155_REG_PART_REV:
        if ((0x48 & 0xF8) == *(temp_reg->val)) {
          temp_reg->unread = false;
          // Must be 0b01001xxx. If so, we init...
          init();
        }
        break;
      case BQ24155_REG_STATUS:
        break;
      case BQ24155_REG_LIMITS:
        break;
      case BQ24155_REG_BATT_REGU:
        break;
      case BQ24155_REG_FAST_CHRG:
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
void BQ24155::printDebug(StringBuilder* temp) {
  temp->concatf("\tSTAT pin:          %d\n", _opts.stat_pin);
  temp->concatf("\tISEL pin:          %d\n", _opts.isel_pin);
}



/*******************************************************************************
* Functions specific to this class....                                         *
*******************************************************************************/


#endif  // CONFIG_MANUVR_BQ24155
