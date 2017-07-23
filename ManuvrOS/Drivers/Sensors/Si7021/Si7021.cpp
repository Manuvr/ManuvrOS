/*
File:   Si7021.cpp
Author: J. Ian Lindsay
Date:   2016.12.26

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

#include "Si7021.h"

const DatumDef datum_defs[] = {
  {
    .desc    = "Rel Humidity",
    .units   = COMMON_UNITS_PERCENT,
    .type_id = TCode::FLOAT,
    .flgs    = SENSE_DATUM_FLAG_HARDWARE
  },
  {
    .desc    = "Temperature",
    .units   = COMMON_UNITS_C,
    .type_id = TCode::FLOAT,
    .flgs    = SENSE_DATUM_FLAG_HARDWARE
  }
};


Si7021::Si7021() : I2CDevice(SI7021_I2CADDR), SensorWrapper("Si7021") {
  define_datum(&datum_defs[0]);
  define_datum(&datum_defs[1]);
}

Si7021::~Si7021() {
}


/******************************************************************************
* Overrides...                                                                *
******************************************************************************/

SensorError Si7021::init() {
  return SensorError::NO_ERROR;  // TODO: Wrong code.
}


SensorError Si7021::readSensor() {
  return SensorError::NO_ERROR;  // TODO: Wrong code.
}


SensorError Si7021::setParameter(uint16_t reg, int len, uint8_t*) {
  return SensorError::INVALID_PARAM_ID;
}


SensorError Si7021::getParameter(uint16_t reg, int len, uint8_t*) {
  return SensorError::INVALID_PARAM_ID;
}



/******************************************************************************
* These are overrides from I2CDevice.                                         *
******************************************************************************/

int8_t Si7021::io_op_callback(I2CBusOp* completed) {
  I2CDevice::io_op_callback(completed);
  return 0;
}


/*
* Dump this item to the dev log.
*/
void Si7021::printDebug(StringBuilder* temp) {
  temp->concatf("RH sensor (Si7021)\t%snitialized\n---------------------------------------------------\n", (isActive() ? "I": "Uni"));
  I2CDevice::printDebug(temp);
  temp->concatf("\n");
}



/******************************************************************************
* Class-specific functions...                                                 *
******************************************************************************/

SensorError Si7021::check_identity() {
  return SensorError::WRONG_IDENTITY;
}
