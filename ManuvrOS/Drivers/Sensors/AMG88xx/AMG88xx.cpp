/*
File:   AMG88xx.cpp
Author: J. Ian Lindsay
Date:   2017.12.09

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

#include "AMG88xx.h"


const DatumDef datum_defs[] = {
  {
    .desc    = "Temperature",
    .units   = COMMON_UNITS_C,
    .type_id = TCode::FLOAT,
    .flgs    = SENSE_DATUM_FLAG_HARDWARE
  }
};


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

/*
* Constructor. Takes i2c address as argument.
*/
AMG88xx::AMG88xx(uint8_t addr) : I2CDevice(addr, 8, 8), SensorWrapper("AMG88xx") {
  define_datum(&datum_defs[0]);

  // Default state: Maximum range and maximum resolution.
  autorange    = false;
}

/*
* Destructor.
*/
AMG88xx::~AMG88xx() {
}


/*******************************************************************************
*  ,-.                      ,   .
* (   `                     | . |
*  `-.  ,-. ;-. ,-. ,-. ;-. | ) ) ;-. ,-: ;-. ;-. ,-. ;-.
* .   ) |-' | | `-. | | |   |/|/  |   | | | | | | |-' |
*  `-'  `-' ' ' `-' `-' '   ' '   '   `-` |-' |-' `-' '
******************************************|***|********************************/
SensorError AMG88xx::init() {
  if (syncRegisters() == I2C_ERR_SLAVE_NO_ERROR) {
    isActive(true);
    return SensorError::NO_ERROR;
  }
  else {
    return SensorError::BUS_ERROR;
  }
}


SensorError AMG88xx::setParameter(uint16_t reg, int len, uint8_t *data) {
  SensorError return_value = SensorError::INVALID_PARAM_ID;
  switch (reg) {
    case AMG88xx_REG_POWER_STATE:
      break;
    default:
      break;
  }
  return return_value;
}



SensorError AMG88xx::getParameter(uint16_t reg, int len, uint8_t*) {
  return SensorError::INVALID_PARAM_ID;
}


SensorError AMG88xx::readSensor() {
  if (isActive()) {
        return SensorError::NO_ERROR;
  }
  return SensorError::BUS_ERROR;
}



/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

int8_t AMG88xx::io_op_callback(I2CBusOp* completed) {
  if (completed->hasFault()) {
    StringBuilder output;
    output.concat("An i2c operation requested by the AMG88xx came back failed.\n");
    completed->printDebug(&output);
    Kernel::log(&output);
    return -1;
  }
  switch (completed->get_opcode()) {
    case BusOpcode::RX:
      switch (completed->sub_addr) {
        case AMG88XX_REG_PWR_CTRL:
          break;
        case AMG88XX_REG_FRAME_RATE:
          break;
        case AMG88XX_REG_IRQ_CTRL:
          break;
        case AMG88XX_REG_STATUS:
          break;
        case AMG88XX_REG_AVERAGING:
          break;
        default:
          break;
      }
      break;

    case BusOpcode::TX:
      switch (completed->sub_addr) {
        case AMG88XX_REG_PWR_CTRL:
          break;
        case AMG88XX_REG_RESET:
          break;
        case AMG88XX_REG_FRAME_RATE:
          break;
        case AMG88XX_REG_IRQ_CTRL:
          break;
        case AMG88XX_REG_STATUS_CLEAR:
          break;
        case AMG88XX_REG_AVERAGING:
          break;
        default:
          break;
      }
      break;

    default:
      break;
  }
  return 0;
}



/*
* Dump this item to the dev log.
*/
void AMG88xx::printDebug(StringBuilder* temp) {
  if (temp) {
    temp->concatf("Lux sensor (AMG88xx)\t%snitialized\n---------------------------------------------------\n", (isActive() ? "I": "Uni"));
    I2CDeviceWithRegisters::printDebug(temp);
    //SensorWrapper::issue_json_map(temp, this);
    temp->concatf("\n");
  }
}




/*******************************************************************************
* Class-specific functions...                                                  *
*******************************************************************************/

float AMG88xx::pixelTemperature(uint8_t x, uint8_t y) {
  return (_field_values[((x & 0x07) << 3) | (y & 0x07)] * 0.25);
};
