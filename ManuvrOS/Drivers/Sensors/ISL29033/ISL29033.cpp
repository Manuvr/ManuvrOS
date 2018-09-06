/*
File:   ISL29033.cpp
Author: J. Ian Lindsay
Date:   2014.05.27

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

#include "ISL29033.h"

// First index is res, second index is range. This table saves us the burden of 2 FPU operations
// when we need to calculate lux.
const float ISL29033::res_range_precalc[4][4] = {{7.8125,       31.25,        125,          500},
                                                 {0.48828125,   1.953125,     7.8125,       31.25},
                                                 {0.0305175781, 0.1220703125, 0.48828125,   1.953125},
                                                 {0.0019073486, 0.0076293945, 0.0305175781, 0.1220703125}};

const DatumDef datum_defs[] = {
  {
    .desc    = "Light level",
    .units   = COMMON_UNITS_LUX,
    .type_id = TCode::FLOAT,
    .flgs    = SENSE_DATUM_FLAG_HARDWARE
  }
};


/*
* Constructor. Takes i2c address as argument.
*/
ISL29033::ISL29033(uint8_t addr) : I2CDeviceWithRegisters(addr, 8, 8), SensorWrapper("ISL29033") {
  define_datum(&datum_defs[0]);

  // Default state: Maximum range and maximum resolution.
  autorange    = false;
  res          = ISL29033_RESOLUTION_16;
  range        = ISL29033_RANGE_8000_LUX;
  threshold_lo = 0x6000;
  threshold_hi = 0x8000;
  mode         = ISL29033_POWER_MODE_ALS;
  irq_persist  = 0x03;   // IRQ persists for 8 integration cycles.

  defineRegister(ISL29033_REG_COMMAND_1,  (uint8_t) 0b00000000, false, false, true);
  defineRegister(ISL29033_REG_COMMAND_2,  (uint8_t) 0b00000000, false, false, true);
  defineRegister(ISL29033_REG_DATA_LSB,   (uint8_t) 0b00000000, false, false, false);
  defineRegister(ISL29033_REG_DATA_MSB,   (uint8_t) 0b00000000, false, false, false);
  defineRegister(ISL29033_REG_INT_LT_LSB, (uint8_t) 0b00000000, false, false, true);
  defineRegister(ISL29033_REG_INT_LT_MSB, (uint8_t) 0b00000000, false, false, true);
  defineRegister(ISL29033_REG_INT_HT_LSB, (uint8_t) 0b00000000, false, false, true);
  defineRegister(ISL29033_REG_INT_HT_MSB, (uint8_t) 0b00000000, false, false, true);
}

/*
* Destructor.
*/
ISL29033::~ISL29033() {
}


/**************************************************************************
* Overrides...                                                            *
**************************************************************************/
SensorError ISL29033::init() {
  if (syncRegisters() == I2C_ERR_SLAVE_NO_ERROR) {
    isActive(true);
    setCommandReg();
    setResRange();
    setThresholds();
    return SensorError::NO_ERROR;
  }
  else {
    return SensorError::BUS_ERROR;
  }
}


SensorError ISL29033::setParameter(uint16_t reg, int len, uint8_t *data) {
  SensorError return_value = SensorError::INVALID_PARAM_ID;
  switch (reg) {
    case ISL29033_REG_AUTORANGE:
      autorange    = ((bool) *(data)) ? true : false;
      return_value = SensorError::NO_ERROR;
      break;
    case ISL29033_REG_RANGE:
      if ((uint8_t) *(data) < 4) { // Only four bits allowed here.
        range = *(data);
        return_value = setResRange();
      }
      else {
        return_value = SensorError::INVALID_PARAM;
      }
      break;
    case ISL29033_REG_RESOLUTION:
      if ((uint8_t) *(data) < 4) { // Only four bits allowed here.
        res = *(data);
        return_value = setResRange();
      }
      else {
        return_value = SensorError::INVALID_PARAM;
      }
      break;
    case ISL29033_REG_THRESHOLD_LO:
      return_value = SensorError::INVALID_PARAM;
      if (len == 2) {
        threshold_lo = (uint16_t) (*data);
        return_value = setThresholds();
      }
      break;
    case ISL29033_REG_THRESHOLD_HI:
      return_value = SensorError::INVALID_PARAM;
      if (len == 2) {
        threshold_hi = (uint16_t) (*data);
        return_value = setThresholds();
      }
      break;
    case ISL29033_REG_POWER_STATE:
      return_value = SensorError::INVALID_PARAM;
      if (len == 1) {
        uint8_t temp_mode = (uint8_t) (*data);
        if ((temp_mode == ISL29033_POWER_MODE_OFF) || (temp_mode == ISL29033_POWER_MODE_ALS) || (temp_mode == ISL29033_POWER_MODE_IR)) {
          mode = temp_mode;
          return_value = setCommandReg();
        }
      }
      break;
    default:
      break;
  }
  return return_value;
}



SensorError ISL29033::getParameter(uint16_t reg, int len, uint8_t*) {
  return SensorError::INVALID_PARAM_ID;
}


SensorError ISL29033::readSensor() {
  if (isActive()) {
    if (I2C_ERR_SLAVE_NO_ERROR == readRegister((uint8_t) ISL29033_REG_DATA_LSB)) {
      if (I2C_ERR_SLAVE_NO_ERROR == readRegister((uint8_t) ISL29033_REG_DATA_MSB)) {
        return SensorError::NO_ERROR;
      }
    }
  }
  return SensorError::BUS_ERROR;
}



/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

int8_t ISL29033::register_write_cb(DeviceRegister* reg) {
  switch (reg->addr) {
    case ISL29033_REG_COMMAND_1:
      break;
    case ISL29033_REG_COMMAND_2:
      break;
    case ISL29033_REG_INT_LT_LSB:
      break;
    case ISL29033_REG_INT_LT_MSB:
      break;
    case ISL29033_REG_INT_HT_LSB:
      break;
    case ISL29033_REG_INT_HT_MSB:
      break;
    default:
      break;
  }
  return 0;
}


int8_t ISL29033::register_read_cb(DeviceRegister* reg) {
  switch (reg->addr) {
    case ISL29033_REG_COMMAND_1:
      break;
    case ISL29033_REG_COMMAND_2:
      break;
    case ISL29033_REG_DATA_LSB:
    case ISL29033_REG_DATA_MSB:
      if (calculateLux()) {
        Kernel::raiseEvent(MANUVR_MSG_SENSOR_ISL29033, nullptr);   // Raise an event
      }
      break;
    case ISL29033_REG_INT_LT_LSB:
      break;
    case ISL29033_REG_INT_LT_MSB:
      break;
    case ISL29033_REG_INT_HT_LSB:
      break;
    case ISL29033_REG_INT_HT_MSB:
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
void ISL29033::printDebug(StringBuilder* temp) {
  if (temp) {
    temp->concatf("Lux sensor (ISL29033)\t%snitialized%s", (isActive() ? "I": "Uni"), PRINT_DIVIDER_1_STR);
    I2CDeviceWithRegisters::printDebug(temp);
    //SensorWrapper::issue_json_map(temp, this);
    temp->concatf("\n");
  }
}




/**************************************************************************
* Class-specific functions...                                             *
**************************************************************************/

/*
* Actually makes the call to the I2CAdapter to set light level thresholds.
*/
SensorError ISL29033::setThresholds() {
  if (writeIndirect(ISL29033_REG_INT_LT_LSB, (threshold_lo & 0x00FF), true)) {
    if (writeIndirect(ISL29033_REG_INT_LT_MSB, (threshold_lo >> 8), true)) {
      if (writeIndirect(ISL29033_REG_INT_HT_LSB, (threshold_hi & 0x00FF), true)) {
        if (writeIndirect(ISL29033_REG_INT_HT_MSB, (threshold_hi >> 8)))  {
          return SensorError::NO_ERROR;
        }
      }
    }
  }
  return SensorError::BUS_ERROR;
}


/*
* Actually makes the call to the I2CAdapter to set res and range.
*/
SensorError ISL29033::setResRange() {
  uint8_t temp_val = (0x0F & ((res << 2) + range));
  if (writeIndirect(ISL29033_REG_COMMAND_2, temp_val)) {
    return SensorError::NO_ERROR;
  }
  return SensorError::BUS_ERROR;
}


SensorError ISL29033::setCommandReg() {
  uint8_t temp_val = (0xE3 & ((mode << 5) + irq_persist));
  if (writeIndirect(ISL29033_REG_COMMAND_1, temp_val)) {
    return SensorError::NO_ERROR;
  }
  return SensorError::BUS_ERROR;
}


uint8_t ISL29033::getCommandReg() {
  // TODO: Probably has API consequences.
  uint8_t return_value = readRegister((uint8_t) ISL29033_REG_COMMAND_1);
  return return_value;
}


/*
* Given a raw sensor read, converts result into a lux value and updates the datum.
*/
bool ISL29033::calculateLux() {
  if (regUpdated(ISL29033_REG_DATA_LSB) && regUpdated(ISL29033_REG_DATA_MSB)) {
    int16_t raw_read = (int16_t) (regValue(ISL29033_REG_DATA_LSB) + (regValue(ISL29033_REG_DATA_MSB) << 8));
    float result = raw_read * ISL29033::res_range_precalc[res][range];
    updateDatum(0, result);
    markRegRead(ISL29033_REG_DATA_LSB);
    markRegRead(ISL29033_REG_DATA_MSB);
    return true;
  }
  return false;
}
