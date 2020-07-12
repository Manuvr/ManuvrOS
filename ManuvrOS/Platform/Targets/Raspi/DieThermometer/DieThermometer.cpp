/*
File:   RaspiTempSensor.h
Author: J. Ian Lindsay
Date:   2014.03.17

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

#include "DieThermometer.h"
#include <StringBuilder.h>

#if defined(RASPI)

#include <stdio.h>

const DatumDef datum_defs[] = {
  {
    .desc    = "Temperature",
    .units   = COMMON_UNITS_C,
    .type_id = TCode::FLOAT,
    .flgs    = SENSE_DATUM_FLAG_HARDWARE
  }
};


RaspiTempSensor::RaspiTempSensor() : SensorWrapper("CPU") {
  define_datum(&datum_defs[0]);
}


RaspiTempSensor::~RaspiTempSensor() {
}



/**************************************************************************
* Overrides...                                                            *
**************************************************************************/
/*
* In this case, the only way to know if the sensor is present or not is to
*   attempt to read it.
*/
SensorError RaspiTempSensor::init() {
  if (readSensor() == SensorError::NO_ERROR) {
    isActive(true);
    return SensorError::NO_ERROR;
  }
  else {
    return SensorError::ABSENT;
  }
}


/*
* The RasPi exports its GPU temperature to a file in /sys/class. The file
*   contains nothing but a number (as a string). Dividing that number by
*   1000 yields degrees C.
* Reads that file, does the appropriate rounding and division, and updates
*   its only datum.
*/
SensorError RaspiTempSensor::readSensor() {
    FILE* temperature_file = fopen(RaspiTempSensor::RASPI_TEMPERATURE_FILE, "r");
    if (temperature_file) {
        char *buf = (char*) alloca(1000);
        memset(buf, 0x00, 1000);
        char *res = fgets(buf, 1000, temperature_file);
        if (res) {
            long temp_temp = strtol(buf, (char **) nullptr, 10);
            if (temp_temp != 0) {
                // We got a good read from the file. Now while it is still an integer, we should
                // round it...
                temp_temp = temp_temp - (temp_temp % 100);
                float temperature = (float) temp_temp / 1000;
                updateDatum(0, temperature);
            }
            else {
                Kernel::log("Failed to parse the data from the temperature file.\n");
                return SensorError::ABSENT;
            }
        }
        fclose(temperature_file);
    }
    else {
        Kernel::log("Failed to open the temperature file for reading.\n");
        return SensorError::ABSENT;
    }
    return SensorError::NO_ERROR;
}


SensorError RaspiTempSensor::setParameter(uint16_t reg, int len, uint8_t*) {
  return SensorError::INVALID_PARAM_ID;
}


SensorError RaspiTempSensor::getParameter(uint16_t reg, int len, uint8_t*) {
  return SensorError::INVALID_PARAM_ID;
}

#endif // defined(RASPI)
