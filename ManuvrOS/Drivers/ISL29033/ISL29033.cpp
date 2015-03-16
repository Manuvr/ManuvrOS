/*
File:   ISL29033.cpp
Author: J. Ian Lindsay
Date:   2014.05.27


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

#include "ISL29033.h"

// First index is res, second index is range. This table saves us the burden of 2 FPU operations
// when we need to calculate lux.
const float ISL29033::res_range_precalc[4][4] = {{7.8125,       31.25,        125,          500},
                                                 {0.48828125,   1.953125,     7.8125,       31.25},
                                                 {0.0305175781, 0.1220703125, 0.48828125,   1.953125},
                                                 {0.0019073486, 0.0076293945, 0.0305175781, 0.1220703125}};

/*
* Constructor. Takes i2c address as argument.
*/
ISL29033::ISL29033(uint8_t addr) : I2CDeviceWithRegisters(), SensorWrapper() {
  _dev_addr = addr;
  this->isHardware = true;
  this->defineDatum("Light level", SensorWrapper::COMMON_UNITS_LUX, FLOAT_FM);
  this->s_id = "10899cd83f9ab1218c7cec2bbd589d91";
  this->name = "ISL29033";
  gpioSetup();

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
ISL29033::~ISL29033(void) {
}


/*
* Setup GPIO pins and their bindings to on-chip peripherals, if required.
*/
void ISL29033::gpioSetup(void) {
#if defined(STM32F4XX)
  GPIO_InitTypeDef GPIO_InitStruct;

    /* These Port E pins are inputs:
    *
    * #   Purpose
    * --------------------------------------
    * 11  LUX_IRQ (Interrupt)
    */
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_11;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IN;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_UP;   // Need a pull-up on the IRQ pin.
    GPIO_Init(GPIOE, &GPIO_InitStruct);
#endif
}


/**************************************************************************
* Overrides...                                                            *
**************************************************************************/
int8_t ISL29033::init() {
  if (syncRegisters() == I2C_ERR_CODE_NO_ERROR) {
    sensor_active = true;
    setCommandReg();
    setResRange();
    setThresholds();
    return SensorWrapper::SENSOR_ERROR_NO_ERROR;
  }
  else {
    return SensorWrapper::SENSOR_ERROR_BUS_ERROR;
  }
}


int8_t ISL29033::setParameter(uint16_t reg, int len, uint8_t *data) {
  int8_t return_value = SensorWrapper::SENSOR_ERROR_INVALID_PARAM_ID;
  switch (reg) {
    case ISL29033_REG_AUTORANGE:
      autorange    = ((bool) *(data)) ? true : false;
      return_value = SensorWrapper::SENSOR_ERROR_NO_ERROR;
      break;
    case ISL29033_REG_RANGE:
      if ((uint8_t) *(data) < 4) { // Only four bits allowed here.
        range = *(data);
        return_value = setResRange();
      }
      else {
        return_value = SensorWrapper::SENSOR_ERROR_INVALID_PARAM;
      }
      break;
    case ISL29033_REG_RESOLUTION:
      if ((uint8_t) *(data) < 4) { // Only four bits allowed here.
        res = *(data);
        return_value = setResRange();
      }
      else {
        return_value = SensorWrapper::SENSOR_ERROR_INVALID_PARAM;
      }
      break;
    case ISL29033_REG_THRESHOLD_LO:
      return_value = SensorWrapper::SENSOR_ERROR_INVALID_PARAM;
      if (len == 2) {
        threshold_lo = (uint16_t) (*data);
        return_value = setThresholds();
      }
      break;
    case ISL29033_REG_THRESHOLD_HI:
      return_value = SensorWrapper::SENSOR_ERROR_INVALID_PARAM;
      if (len == 2) {
        threshold_hi = (uint16_t) (*data);
        return_value = setThresholds();
      }
      break;
    case ISL29033_REG_POWER_STATE:
      return_value = SensorWrapper::SENSOR_ERROR_INVALID_PARAM;
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



int8_t ISL29033::getParameter(uint16_t reg, int len, uint8_t*) {
  return SensorWrapper::SENSOR_ERROR_INVALID_PARAM_ID;
}

                               
int8_t ISL29033::readSensor(void) {
  if (sensor_active) {
    if (I2C_ERR_CODE_NO_ERROR == readRegister((uint8_t) ISL29033_REG_DATA_LSB)) {
      if (I2C_ERR_CODE_NO_ERROR == readRegister((uint8_t) ISL29033_REG_DATA_MSB)) {
        return SensorWrapper::SENSOR_ERROR_NO_ERROR;
      }
    }
  }
  return SensorWrapper::SENSOR_ERROR_BUS_ERROR;
}



/****************************************************************************************************
* These are overrides from I2CDevice.                                                               *
****************************************************************************************************/

void ISL29033::operationCompleteCallback(I2CQueuedOperation* completed) {
  I2CDeviceWithRegisters::operationCompleteCallback(completed);
  if (completed != NULL) {
    switch (completed->opcode) {
      case I2C_OPERATION_READ:
        switch (completed->sub_addr) {
          case ISL29033_REG_COMMAND_1:
            break;
          case ISL29033_REG_COMMAND_2:
            break;
          case ISL29033_REG_DATA_LSB:
          case ISL29033_REG_DATA_MSB:
            if (calculateLux()) {
              EventManager::raiseEvent(MANUVR_MSG_SENSOR_ISL29033, NULL);   // Raise an event
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
        break;
      case I2C_OPERATION_WRITE:
        switch (completed->sub_addr) {
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
        break;
      case I2C_OPERATION_PING:
        StaticHub::log("Ping received.");
        break;
      default:
        break;
    }
  }
  else {
    StaticHub::log("SENSOR_ERROR_WRONG_IDENTITY");
  }
}


/*
* Dump this item to the dev log.
*/
void ISL29033::printDebug(StringBuilder* temp) {
  if (temp != NULL) {
    temp->concatf("Lux sensor (ISL29033)\t%snitialized\n---------------------------------------------------\n", (sensor_active ? "I": "Uni"));
    I2CDeviceWithRegisters::printDebug(temp);
    //SensorWrapper::issue_json_map(temp, this);
    temp->concatf("\n");
  }
}




/**************************************************************************
* Class-specific functions...                                             *
**************************************************************************/

/*
* Actually makes the call to the i2c-adapter to set light level thresholds.
*/
int8_t ISL29033::setThresholds(void) {
  int8_t return_value = SensorWrapper::SENSOR_ERROR_BUS_ERROR;
  if (writeIndirect(ISL29033_REG_INT_LT_LSB, (threshold_lo & 0x00FF), true)) {
    if (writeIndirect(ISL29033_REG_INT_LT_MSB, (threshold_lo >> 8), true)) {
      if (writeIndirect(ISL29033_REG_INT_HT_LSB, (threshold_hi & 0x00FF), true)) {
        if (writeIndirect(ISL29033_REG_INT_HT_MSB, (threshold_hi >> 8)))  {
          return_value = SensorWrapper::SENSOR_ERROR_NO_ERROR;
        }
      }
    }
  }
  return return_value;
}


/*
* Actually makes the call to the i2c-adapter to set res and range.
*/
int8_t ISL29033::setResRange(void) {
  int8_t return_value = SensorWrapper::SENSOR_ERROR_BUS_ERROR;
  uint8_t temp_val = (0x0F & ((res << 2) + range));
  if (writeIndirect(ISL29033_REG_COMMAND_2, temp_val)) {
    return_value = SensorWrapper::SENSOR_ERROR_NO_ERROR;
  }
  return return_value;
}


int8_t ISL29033::setCommandReg(void) {
  int8_t return_value = SensorWrapper::SENSOR_ERROR_BUS_ERROR;
  uint8_t temp_val = (0xE3 & ((mode << 5) + irq_persist));
  if (writeIndirect(ISL29033_REG_COMMAND_1, temp_val)) {
    return_value = SensorWrapper::SENSOR_ERROR_NO_ERROR;
  }
  return return_value;
}


uint8_t ISL29033::getCommandReg(void) {
  uint8_t return_value = read8(ISL29033_REG_COMMAND_1);
  return return_value;
}


/*
* Given a raw sensor read, converts result into a lux value and updates the datum.
*/
bool ISL29033::calculateLux(void) {
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

