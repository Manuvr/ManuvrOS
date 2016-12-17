/*
File:   TSL2561.cpp
Author: J. Ian Lindsay
Date:   2014.12.17

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

#include "TSL2561.h"


TSL2561::TSL2561(uint8_t addr, uint8_t irq) : TSL2561(addr) {
  _irq_pin  = irq;
}

/*
* Constructor. Takes i2c address as argument.
*/
TSL2561::TSL2561(uint8_t addr) : I2CDeviceWithRegisters(), SensorWrapper() {
  _dev_addr = addr;
  this->isHardware = true;
  this->defineDatum("Vis-IR Intensity", SensorWrapper::COMMON_UNITS_PRESSURE, FLOAT_FM);
  this->defineDatum("IR Intensity", SensorWrapper::COMMON_UNITS_C, FLOAT_FM);
  this->s_id = "32f9b0436dc76d77de116814263409fe";
  this->name = "TSL2561";

  // Now we should give them initial definitions. This is our chance to set default configs.
  defineRegister(TSL2561_REG_CONTROL,    (uint8_t)  0b00000000, false, false, true);
  defineRegister(TSL2561_REG_TIMING,     (uint8_t)  0b00000000, false, false, true);
  defineRegister(TSL2561_REG_THRESH_LO,  (uint16_t) 0b00000000, false, false, true);
  defineRegister(TSL2561_REG_THRESH_HI,  (uint16_t) 0b00000000, false, false, true);    //
  defineRegister(TSL2561_REG_INTERRUPT,  (uint8_t)  0b00000000, false, false, true);    //
  defineRegister(TSL2561_REG_ID,         (uint8_t)  0b00000000, false, false, false);   // Identity register. Should be 0xbb.
  defineRegister(TSL2561_REG_DATA0,      (uint16_t) 0b00000000, false, false, false);   //
  defineRegister(TSL2561_REG_DATA1,      (uint16_t) 0b00000000, false, false, false);   //
}

/*
* Destructor.
*/
TSL2561::~TSL2561() {
}



/**************************************************************************
* Overrides...                                                            *
**************************************************************************/

int8_t TSL2561::init() {
  if (readRegister(TSL2561_REG_ID) == I2C_ERR_CODE_NO_ERROR) {
    return SensorWrapper::SENSOR_ERROR_NO_ERROR;
  }
  else {
    return SensorWrapper::SENSOR_ERROR_BUS_ERROR;
  }
}


int8_t TSL2561::setParameter(uint16_t reg, int len, uint8_t *data) {
  return SensorWrapper::SENSOR_ERROR_INVALID_PARAM_ID;
}


int8_t TSL2561::getParameter(uint16_t reg, int len, uint8_t*) {
  return SensorWrapper::SENSOR_ERROR_INVALID_PARAM_ID;
}


int8_t TSL2561::readSensor() {
  if (readRegister(TSL2561_REG_DATA0) == I2C_ERR_CODE_NO_ERROR) {
    if (readRegister(TSL2561_REG_DATA1) == I2C_ERR_CODE_NO_ERROR) {
      return SensorWrapper::SENSOR_ERROR_NO_ERROR;
    }
  }
  return SensorWrapper::SENSOR_ERROR_BUS_ERROR;
}



/****************************************************************************************************
* These are overrides from I2CDevice.                                                               *
****************************************************************************************************/

void TSL2561::operationCompleteCallback(I2CBusOp* completed) {
  I2CDeviceWithRegisters::operationCompleteCallback(completed);
	int i = 0;
	DeviceRegister *temp_reg = reg_defs.get(i++);
	while (temp_reg != NULL) {
		switch (temp_reg->addr) {
		  case TSL2561_REG_ID:
		    temp_reg->unread = false;
		    if (!sensor_active) {
		      sensor_active = (0xBB == *(temp_reg->val));
		      if (sensor_active) {
		        writeDirtyRegisters();
		      }
		    }
		    else {
		      sensor_active = (0xBB == *(temp_reg->val));
		    }
		    break;

		  case TSL2561_REG_CONTROL:
		  case TSL2561_REG_TIMING:
		  case TSL2561_REG_INTERRUPT:
		  case TSL2561_REG_THRESH_LO:
		  case TSL2561_REG_THRESH_HI:
		    break;

		  case TSL2561_REG_DATA0:
		  case TSL2561_REG_DATA1:
		    if (BusOpcode::RX == completed->get_opcode()) {
		      if (calculate_lux()) {
		      }
		    }
		    break;

		  default:
		    temp_reg->unread = false;
		    break;
		}
		temp_reg = reg_defs.get(i++);
	}
}


/*
* Dump this item to the dev log.
*/
void TSL2561::printDebug(StringBuilder* temp) {
  if (NULL == temp) return;
  //SensorWrapper::issue_json_map(temp, this);
  temp->concatf("Lux sensor (TSL2561)\t%snitialized\n---------------------------------------------------\n", (sensor_active ? "I": "Uni"));
  I2CDeviceWithRegisters::printDebug(temp);
  temp->concatf("\n");
}




/**************************************************************************
* Class-specific functions...                                             *
**************************************************************************/

bool TSL2561::calculate_lux() {
  return false;
}




void TSL2561::set_power_mode(uint8_t nu__pwr_mode) {
  _pwr_mode = nu__pwr_mode;
  switch (_pwr_mode) {
    case 0:
      break;
    case 1:
      break;
    case 2:   // Enter standby.
      break;
    default:
      break;
  }
  #if defined(__MANUVR_DEBUG)
    Kernel::log("TSL2561 Power mode set. \n");
  #endif
}
