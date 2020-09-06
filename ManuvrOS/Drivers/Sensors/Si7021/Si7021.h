/*
File:   Si7021.h
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

#ifndef __Si7021_DRIVER_H__
#define __Si7021_DRIVER_H__

#include <Drivers/Sensors/SensorWrapper.h>
#include <I2CAdapter.h>

/* Hardware-defined registers */
#define SI7021_REG_CONFIG           0x01
#define SI7021_REG_DATA_LSB         0x02
#define SI7021_REG_DATA_MSB         0x03

#define SI7021_I2CADDR              0x40


class Si7021 : public I2CDevice, public SensorWrapper {
  public:
    Si7021();
    ~Si7021();

    /* Overrides from SensorWrapper */
    SensorError init();
    SensorError readSensor();
    SensorError setParameter(uint16_t reg, int len, uint8_t*);  // Used to set operational parameters for the sensor.
    SensorError getParameter(uint16_t reg, int len, uint8_t*);  // Used to read operational parameters from the sensor.

    /* Overrides from I2CDevice... */
    int8_t io_op_callback(I2CBusOp*);
    void printDebug(StringBuilder*);


  private:
    SensorError check_identity();
};

#endif   // __Si7021_DRIVER_H__
