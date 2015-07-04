/*
File:   ISL23345.cpp
Author: J. Ian Lindsay
Date:   2014.03.10


This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include "ISL23345.h"
const int8_t ISL23345::ISL23345_ERROR_DEVICE_DISABLED  = 3;    // A caller tried to set a wiper while the device is disabled. This may work...
const int8_t ISL23345::ISL23345_ERROR_PEGGED_MAX       = 2;    // There was no error, but a call to change a wiper setting pegged the wiper at its highest position.
const int8_t ISL23345::ISL23345_ERROR_PEGGED_MIN       = 1;    // There was no error, but a call to change a wiper setting pegged the wiper at its lowest position.
const int8_t ISL23345::ISL23345_ERROR_NO_ERROR         = 0;    // There was no error.
const int8_t ISL23345::ISL23345_ERROR_ABSENT           = -1;   // The ISL23345 appears to not be connected to the bus.
const int8_t ISL23345::ISL23345_ERROR_BUS              = -2;   // Something went wrong with the i2c bus.
const int8_t ISL23345::ISL23345_ERROR_ALREADY_AT_MAX   = -3;   // A caller tried to increase the value of the wiper beyond its maximum.  
const int8_t ISL23345::ISL23345_ERROR_ALREADY_AT_MIN   = -4;   // A caller tried to decrease the value of the wiper below its minimum.
const int8_t ISL23345::ISL23345_ERROR_INVALID_POT      = -5;   // The ISL23345 only has 4 potentiometers. 



/*
* Constructor. Takes the i2c address of this device as sole argument.
*/
ISL23345::ISL23345(uint8_t i2caddr) : I2CDeviceWithRegisters() {
	_dev_addr = i2caddr;
	
	dev_enabled = false;
	dev_init    = false;
	preserve_state_on_destroy = false;

  defineRegister(ISL23345_REG_ACR,  (uint8_t) 0x40, false, false, true);
  defineRegister(ISL23345_REG_WR0,  (uint8_t) 0x80, false, false, true);
  defineRegister(ISL23345_REG_WR1,  (uint8_t) 0x80, false, false, true);
  defineRegister(ISL23345_REG_WR2,  (uint8_t) 0x80, false, false, true);
  defineRegister(ISL23345_REG_WR3,  (uint8_t) 0x80, false, false, true);
}

/*
* When we destroy the class instance, the hardware will be disabled.
*/
ISL23345::~ISL23345(void) {
	if (!preserve_state_on_destroy) {
		disable();
	}
}


/*
* Call to read the device and cause this class's state to reflect that of the device. 
*/
int8_t ISL23345::init(void) {
	int8_t return_value = ISL23345_ERROR_NO_ERROR;

	if (syncRegisters() == I2C_ERR_CODE_NO_ERROR) {
		//return_value = ISL23345_ERROR_ABSENT;
	}
	return return_value;
}



void ISL23345::preserveOnDestroy(bool x) {
	preserve_state_on_destroy = x;
}



/*
* Enable the device. Reconnects Rh pins and restores the wiper settings.
*/
int8_t ISL23345::enable() {
	int8_t return_value = ISL23345::ISL23345_ERROR_NO_ERROR;
	if (I2C_ERR_CODE_NO_ERROR != writeIndirect(ISL23345_REG_ACR, 0x40)) {
		return_value = ISL23345::ISL23345_ERROR_ABSENT;
	}
	return return_value;
}


/*
* Disable the device. Disconnects all Rh pins and sets the wiper to 2k ohm WRT Rl.
* Retains wiper settings.
*/
int8_t ISL23345::disable() {
	int8_t return_value = ISL23345::ISL23345_ERROR_NO_ERROR;

	if (I2C_ERR_CODE_NO_ERROR != writeIndirect(ISL23345_REG_ACR, 0x00)) {
		return_value = ISL23345::ISL23345_ERROR_ABSENT;
	}
	return return_value;
}


/*
* Set the value of the given wiper to the given value.
*/
int8_t ISL23345::setValue(uint8_t pot, uint8_t val) {
	if (pot > 3)    return ISL23345::ISL23345_ERROR_INVALID_POT;
	if (!dev_init)  return ISL23345::ISL23345_ERROR_DEVICE_DISABLED;

	int8_t return_value = ISL23345::ISL23345_ERROR_NO_ERROR;
	if (I2C_ERR_CODE_NO_ERROR != writeIndirect(pot, val)) {
		return_value = ISL23345::ISL23345_ERROR_ABSENT;
	}
	return return_value;
}



uint8_t ISL23345::getValue(uint8_t pot) {
	if (pot > 3) return ISL23345_ERROR_INVALID_POT;
	return values[pot];
}


/*
* Calling this function will take the device out of shutdown mode and set all the wipers
*   to their minimum values.
*/
int8_t ISL23345::reset(void) {
	return reset(0x00);
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


bool     ISL23345::enabled(void) {     return dev_enabled;  }  // Trivial accessor.
uint16_t ISL23345::getRange(void) {    return 0x00FF;       }  // Trivial. Returns the maximum vaule of any single potentiometer.




/****************************************************************************************************
* These are overrides from I2CDeviceWithRegisters.                                                  *
****************************************************************************************************/

void ISL23345::operationCompleteCallback(I2CQueuedOperation* completed) {
  I2CDeviceWithRegisters::operationCompleteCallback(completed);
  
  if (completed->err_code != I2C_ERR_CODE_NO_ERROR) {
    return;
  }
  
  dev_init = true;
  
  switch (completed->sub_addr) {
    case ISL23345_REG_ACR:
      if (regValue(ISL23345_REG_ACR) == 0x00) dev_enabled = false;
      else {
        dev_enabled = true;
      }
      break;
    case ISL23345_REG_WR0:
    case ISL23345_REG_WR1:
    case ISL23345_REG_WR2:
    case ISL23345_REG_WR3:
      values[completed->sub_addr] = regValue(completed->sub_addr);
      break;
    default:
      break;
  }
}




/*
* Dump this item to the dev log.
*/
void ISL23345::printDebug(StringBuilder* output) {
  output->concat("ISL23345 digital potentiometer\n---------------------------------------------------\n");
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



