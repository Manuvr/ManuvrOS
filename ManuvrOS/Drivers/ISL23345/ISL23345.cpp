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
int8_t ISL23345::init() {
	int8_t return_value = ISL23345_ERROR_NO_ERROR;
	if (syncRegisters() == I2C_ERR_SLAVE_NO_ERROR) {
		return_value = ISL23345_ERROR_ABSENT;
	}
	return return_value;
}


/*
* Enable the device. Reconnects Rh pins and restores the wiper settings.
*/
int8_t ISL23345::enable() {
	int8_t return_value = ISL23345_ERROR::NO_ERROR;
	if (I2C_ERR_SLAVE_NO_ERROR != writeIndirect(ISL23345_REG_ACR, 0x40)) {
		return_value = ISL23345_ERROR::ABSENT;
	}
	return return_value;
}


/*
* Disable the device. Disconnects all Rh pins and sets the wiper to 2k ohm WRT Rl.
* Retains wiper settings.
*/
int8_t ISL23345::disable() {
	int8_t return_value = ISL23345_ERROR::NO_ERROR;

	if (I2C_ERR_SLAVE_NO_ERROR != writeIndirect(ISL23345_REG_ACR, 0x00)) {
		return_value = ISL23345_ERROR::ABSENT;
	}
	return return_value;
}


/*
* Set the value of the given wiper to the given value.
*/
int8_t ISL23345::setValue(uint8_t pot, uint8_t val) {
	if (pot > 3)    return ISL23345_ERROR::INVALID_POT;
	if (!dev_init)  return ISL23345_ERROR::DEVICE_DISABLED;

	int8_t return_value = ISL23345_ERROR::NO_ERROR;
	if (I2C_ERR_SLAVE_NO_ERROR != writeIndirect(pot, val)) {
		return_value = ISL23345_ERROR::ABSENT;
	}
	return return_value;
}


int8_t ISL23345::reset(uint8_t val) {
	int8_t result = 0;
	for (int i = 0; i < 4; i++) {
		result = setValue(0, val);
		if (result != ISL23345_ERROR_NO_ERROR) {
			return result;
		}
	}
	return ISL23345_ERROR_NO_ERROR;
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
		output->concat("\t Not initialized\n");
		return;
	}

	for (int i = 0; i < 4; i++) {
		output->concatf("\t POT %d: 0x%02x\n", i, values[i]);
	}
	output->concatf("\n");
}
