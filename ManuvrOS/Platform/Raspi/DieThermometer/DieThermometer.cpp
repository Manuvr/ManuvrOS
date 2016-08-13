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
#include <StringBuilder/StringBuilder.h>

#if defined(RASPI) | defined(RASPI2)

#include <stdio.h>


RaspiTempSensor::RaspiTempSensor() : SensorWrapper() {
  this->defineDatum("RasPi Temperature", SensorWrapper::COMMON_UNITS_C, FLOAT_FM);
  this->s_id = "009c4b32a55ee286f335c20bfe94975d";
  this->name = this->datum_list->description;
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
int8_t RaspiTempSensor::init(void) {
  if (readSensor() == SensorWrapper::SENSOR_ERROR_NO_ERROR) {
    this->sensor_active = true;
    return SensorWrapper::SENSOR_ERROR_NO_ERROR;
  }
  else {
    return SensorWrapper::SENSOR_ERROR_ABSENT;
  }
}


/*
* The RasPi exports its GPU temperature to a file in /sys/class. The file
*   contains nothing but a number (as a string). Dividing that number by
*   1000 yields degrees C.
* Reads that file, does the appropriate rounding and division, and updates
*   its only datum.
*/
int8_t RaspiTempSensor::readSensor(void) {
    FILE* temperature_file = fopen(RaspiTempSensor::RASPI_TEMPERATURE_FILE, "r");
    if (temperature_file != NULL) {
        char *buf = (char*) alloca(1000);
        memset(buf, 0x00, 1000);
        char *res = fgets(buf, 1000, temperature_file);
        if (res != NULL) {
            long temp_temp = strtol(buf, (char **) NULL, 10);
            if (temp_temp != 0) {
                // We got a good read from the file. Now while it is still an integer, we should
                // round it...
                temp_temp = temp_temp - (temp_temp % 100);
                float temperature = (float) temp_temp / 1000;
                updateDatum(0, temperature);
            }
            else {
                Kernel::log("Failed to parse the data from the file: %s\n", buf);
                return SensorWrapper::SENSOR_ERROR_ABSENT;
            }
        }
        fclose(temperature_file);
    }
    else {
        Kernel::log("Failed to open file for reading: %s\n", RaspiTempSensor::RASPI_TEMPERATURE_FILE);
        return SensorWrapper::SENSOR_ERROR_ABSENT;
    }
    return SensorWrapper::SENSOR_ERROR_NO_ERROR;
}


int8_t RaspiTempSensor::setParameter(uint16_t reg, int len, uint8_t*) {
  return SensorWrapper::SENSOR_ERROR_INVALID_PARAM_ID;
}


int8_t RaspiTempSensor::getParameter(uint16_t reg, int len, uint8_t*) {
  return SensorWrapper::SENSOR_ERROR_INVALID_PARAM_ID;
}

#endif // defined(RASPI) | defined(RASPI2)
