/*
File:   LightSensor.cpp
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


This is a quick-and-dirty class to support reading a CdS cell from an analog
  pin, and then relaying the uncontrolled, unitless value out to the other
  modules in the system.
*/


#include "LightSensor.h"
#include <Platform/Platform.h>

uint16_t last_lux_read = 0;
uint8_t  last_lux_bin  = 0;

// These are only here until they are migrated to each receiver that deals with them.
const MessageTypeDef message_defs_light_sensor[] = {
  {  MANUVR_MSG_AMBIENT_LIGHT_LEVEL,  MSG_FLAG_EXPORTABLE,  "LIGHT_LEVEL",  ManuvrMsg::MSG_ARGS_NONE  }, // Unitless light-level report.
};


const DatumDef datum_defs[] = {
  {
    .desc    = "LightLevel",
    .units   = COMMON_UNITS_PERCENT,
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


LightSensor::LightSensor(const LightSensorOpts* o) : SensorWrapper("LightSensor"), _opts(o) {
  define_datum(&datum_defs[0]);
  if (0 == pinMode(_opts.pin, GPIOMode::ANALOG_IN)) {
    isActive(true);
  }
}


/*
* Destructor.
*/
LightSensor::~LightSensor() {
}


void LightSensor::light_check() {
  _current_value = readPinAnalog(_opts.pin);
  uint8_t current_lux_bin = _current_value >> 2;

  uint8_t lux_delta = (last_lux_bin > current_lux_bin) ? (last_lux_bin - current_lux_bin) : (current_lux_bin - last_lux_bin);
  if (lux_delta > 3) {
  }

  SensorError err = updateDatum(0, last_lux_bin);
}


/*******************************************************************************
*  ,-.                      ,   .
* (   `                     | . |
*  `-.  ,-. ;-. ,-. ,-. ;-. | ) ) ;-. ,-: ;-. ;-. ,-. ;-.
* .   ) |-' | | `-. | | |   |/|/  |   | | | | | | |-' |
*  `-'  `-' ' ' `-' `-' '   ' '   '   `-` |-' |-' `-' '
******************************************|***|********************************/

/**
* Needs to be called once on reset. This will set our temperature thresholds for
*  setting off the fan, alerts, etc...
*/
SensorError LightSensor::init() {
  isCalibrated(true);
  return SensorError::NO_ERROR;
}

/**
*  Call to query the TMP102 and return the reading in degrees Celcius.
*  Will return -1 on failure.
*/
SensorError LightSensor::readSensor() {
  light_check();
  return SensorError::NO_ERROR;
}


SensorError LightSensor::setParameter(uint16_t reg, int len, uint8_t*) {
  return SensorError::INVALID_PARAM_ID;
}


SensorError LightSensor::getParameter(uint16_t reg, int len, uint8_t*) {
  return SensorError::INVALID_PARAM_ID;
}


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void LightSensor::printDebug(StringBuilder *output) {
  if (output == nullptr) return;
  output->concatf("LightSensor\t%snitialized%s", (isActive() ? "I": "Uni"), PRINT_DIVIDER_1_STR);
  output->concatf("-- Pin:    %u\n-- Value:  %u\n\n", _opts.pin, _current_value);
}
