/*
File:   VEML6070.cpp
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

#include "VEML6070.h"

const DatumDef datum_defs[] = {
  {
    .desc    = "UVA",
    .units   = COMMON_UNITS_MW_PER_SQCM,
    .type_id = TCode::FLOAT,
    .flgs    = SENSE_DATUM_FLAG_HARDWARE
  }
};


VEML6070::VEML6070() : I2CDevice(0x38), SensorWrapper("VEML6070") {
  define_datum(&datum_defs[0]);
}


VEML6070::VEML6070(uint8_t pin) : VEML6070() {
  irq_pin = pin;
}


VEML6070::~VEML6070() {
}


/******************************************************************************
* Overrides...                                                                *
******************************************************************************/

SensorError VEML6070::init() {
  return SensorError::NO_ERROR;  // TODO: Wrong code.
}


SensorError VEML6070::readSensor() {
  return SensorError::NO_ERROR;  // TODO: Wrong code.
}


SensorError VEML6070::setParameter(uint16_t reg, int len, uint8_t*) {
  return SensorError::INVALID_PARAM_ID;
}


SensorError VEML6070::getParameter(uint16_t reg, int len, uint8_t*) {
  return SensorError::INVALID_PARAM_ID;
}



/******************************************************************************
* These are overrides from I2CDevice.                                         *
******************************************************************************/

int8_t VEML6070::io_op_callback(I2CBusOp* completed) {
  I2CDevice::io_op_callback(completed);
  return 0;
}


/*
* Dump this item to the dev log.
*/
void VEML6070::printDebug(StringBuilder* temp) {
  temp->concatf("UVA sensor (VEML6070)\t%snitialized\n---------------------------------------------------\n", (isActive() ? "I": "Uni"));
  I2CDevice::printDebug(temp);
  temp->concatf("\n");
}



/*******************************************************************************
* Class-specific functions...                                                  *
*******************************************************************************/

SensorError VEML6070::check_identity() {
  return SensorError::WRONG_IDENTITY;
}
