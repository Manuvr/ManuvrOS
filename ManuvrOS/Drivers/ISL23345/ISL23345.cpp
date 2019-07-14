/*
File:   ISL23345.cpp
Author: J. Ian Lindsay
Date:   2014.03.10

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

#include "ISL23345.h"

#define ISL23345_REG_ACR   0x10
#define ISL23345_REG_WR0   0x00
#define ISL23345_REG_WR1   0x01
#define ISL23345_REG_WR2   0x02
#define ISL23345_REG_WR3   0x03


/*
* Constructor. Takes the i2c address of this device as sole argument.
*/
ISL23345::ISL23345(uint8_t addr) : I2CDeviceWithRegisters(addr, 5) {
  dev_enabled = false;
  dev_init    = false;
  preserve_state_on_destroy = false;

  defineRegister(ISL23345_REG_ACR, (uint8_t) 0x40, false, false, true);
  defineRegister(ISL23345_REG_WR0, (uint8_t) 0x80, false, false, true);
  defineRegister(ISL23345_REG_WR1, (uint8_t) 0x80, false, false, true);
  defineRegister(ISL23345_REG_WR2, (uint8_t) 0x80, false, false, true);
  defineRegister(ISL23345_REG_WR3, (uint8_t) 0x80, false, false, true);
}


/*
* When we destroy the class instance, the hardware will be disabled.
*/
ISL23345::~ISL23345() {
  if (!preserve_state_on_destroy) {
    disable();
  }
}


/*
* Call to read the device and cause this class's state to reflect that of the device.
*/
DIGITALPOT_ERROR ISL23345::init() {
  DIGITALPOT_ERROR return_value = DIGITALPOT_ERROR::NO_ERROR;
  if (syncRegisters() == I2C_ERR_SLAVE_NO_ERROR) {
    return_value = DIGITALPOT_ERROR::ABSENT;
  }
  return return_value;
}


/*
* Enabling the device reconnects Rh pins and restores the wiper settings.
* Disabling the device disconnects all Rh pins and sets the wiper to 2k ohm WRT Rl.
* Retains wiper settings.
*/
DIGITALPOT_ERROR ISL23345::_enable(bool x) {
  DIGITALPOT_ERROR return_value = DIGITALPOT_ERROR::NO_ERROR;
  if (I2C_ERR_SLAVE_NO_ERROR != writeIndirect(ISL23345_REG_ACR, x ? 0x40 : 0x00)) {
    return_value = DIGITALPOT_ERROR::ABSENT;
  }
  return return_value;
}


/*
* Set the value of the given wiper to the given value.
*/
DIGITALPOT_ERROR ISL23345::setValue(uint8_t pot, uint8_t val) {
  if (pot > 3)    return DIGITALPOT_ERROR::INVALID_POT;
  if (!dev_init)  return DIGITALPOT_ERROR::DEVICE_DISABLED;

  DIGITALPOT_ERROR return_value = DIGITALPOT_ERROR::NO_ERROR;
  if (I2C_ERR_SLAVE_NO_ERROR != writeIndirect(pot, val)) {
    return_value = DIGITALPOT_ERROR::ABSENT;
  }
  return return_value;
}


DIGITALPOT_ERROR ISL23345::reset(uint8_t val) {
  DIGITALPOT_ERROR result = DIGITALPOT_ERROR::NO_ERROR;
  for (int i = 0; i < 4; i++) {
    result = setValue(0, val);
    if (result != DIGITALPOT_ERROR::NO_ERROR) {
      return result;
    }
  }
  return DIGITALPOT_ERROR::NO_ERROR;
}



/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

int8_t ISL23345::register_write_cb(DeviceRegister* reg) {
  dev_init = true;
  switch (reg->addr) {
    case ISL23345_REG_ACR:
      dev_enabled = (reg->getVal() != 0x00);
      break;
    case ISL23345_REG_WR0:
    case ISL23345_REG_WR1:
    case ISL23345_REG_WR2:
    case ISL23345_REG_WR3:
      values[reg->addr] = reg->getVal();
      break;
    default:
      break;
  }
  return 0;
}


int8_t ISL23345::register_read_cb(DeviceRegister* reg) {
  dev_init = true;
  switch (reg->addr) {
    case ISL23345_REG_ACR:
      dev_enabled = (reg->getVal() != 0x00);
      break;
    case ISL23345_REG_WR0:
    case ISL23345_REG_WR1:
    case ISL23345_REG_WR2:
    case ISL23345_REG_WR3:
      values[reg->addr] = reg->getVal();
      break;
    default:
      break;
  }
  reg->unread = false;
  return 0;
}



/*
* Dump this item to the dev log.
*/
void ISL23345::printDebug(StringBuilder* output) {
  output->concat("ISL23345 digital potentiometer");
  output->concat(PRINT_DIVIDER_1_STR);
  I2CDeviceWithRegisters::printDebug(output);

  if (!dev_init) {
    output->concat("\tNot initialized\n");
    return;
  }

  for (int i = 0; i < 4; i++) {
    output->concatf("\tPOT %d: 0x%02x\n", i, values[i]);
  }
  output->concatf("\n");
}
