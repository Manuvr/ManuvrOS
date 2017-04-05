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

#include <Drivers/Sensors/SensorWrapper.h>


#ifndef RPI_TEMP_SENSE_H
#define RPI_TEMP_SENSE_H

class RaspiTempSensor : public SensorWrapper {
    public:
        float temperature;            // The present temperature reading.

        RaspiTempSensor();
        ~RaspiTempSensor();

        /* Overrides from SensorWrapper */
        SensorError init();                                         // Needs to be called once on reset.
        SensorError readSensor();                                   // Read the sensor.
        SensorError setParameter(uint16_t reg, int len, uint8_t*);  // Used to set operational parameters for the sensor.
        SensorError getParameter(uint16_t reg, int len, uint8_t*);  // Used to read operational parameters from the sensor.

    private:
        static constexpr const char* RASPI_TEMPERATURE_FILE = "/sys/class/thermal/thermal_zone0/temp";
};

#endif
