/*
File:   LPS331.h
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

#ifndef __LPS331_DRIVER_H__
#define __LPS331_DRIVER_H__

#include "Drivers/SensorWrapper/SensorWrapper.h"
#include "Platform/Peripherals/I2C/I2CAdapter.h"

// TODO: This class was never fully ported back from Digitabulum. No GPIO change-over...
//#include <stm32f4xx_gpio.h>

#define LPS331_REG_REF_P_XL      0x08  //
#define LPS331_REG_REF_P_L       0x09  // Reference pressure (24-bit, 2's complement)
#define LPS331_REG_REF_P_H       0x0A  //
#define LPS331_REG_WHO_AM_I      0x0F  // This register should return 0xbb always.
#define LPS331_REG_RES_CONF      0x10
#define LPS331_REG_CTRL_1        0x20
#define LPS331_REG_CTRL_2        0x21
#define LPS331_REG_CTRL_3        0x22
#define LPS331_REG_INT_CONFIG    0x23
#define LPS331_REG_INT_SOURCE    0x24
#define LPS331_REG_THS_P_LO      0x25
#define LPS331_REG_THS_P_HI      0x26
#define LPS331_REG_STATUS        0x27
#define LPS331_REG_PRS_P_OUT_XL  0x28
#define LPS331_REG_PRS_OUT_LO    0x29
#define LPS331_REG_PRS_OUT_HI    0x2A
#define LPS331_REG_TEMP_OUT_LO   0x2B
#define LPS331_REG_TEMP_OUT_HI   0x2C
#define LPS331_REG_AMP_CTRL      0x30


#define LPS331_RES_AVGP_BITS     0x0F
#define LPS331_RES_AVGT_BITS     0x70
#define LPS331_RES_RFU           0x80

#define LPS331_CONF1_PD          0x80  // Power down. Defaults to 0 (powered off).
#define LPS331_CONF1_ODR_BITS    0x70  // Control over the output data rate. Default is one-shot.
#define LPS331_CONF1_DIFF_EN     0x08  // Enable interrupts.
#define LPS331_CONF1_DBDU        0x04  // Block data update. Defaults to 0.
#define LPS331_CONF1_DELTA_EN    0x02  // Enable delta pressure registers. Default is 0.
#define LPS331_CONF1_SPI_MODE    0x01  // SPI 3 or 4 wire? Digitabulum doesn't care. We use i2c.


#define LPS331_I2CADDR           0x5C


class LPS331 : public I2CDeviceWithRegisters, public SensorWrapper {
  public:
    LPS331(uint8_t addr = LPS331_I2CADDR);
    ~LPS331();

    void gpioSetup();

    /* Overrides from SensorWrapper */
    SensorError init();
    SensorError readSensor();
    SensorError setParameter(uint16_t reg, int len, uint8_t*);  // Used to set operational parameters for the sensor.
    SensorError getParameter(uint16_t reg, int len, uint8_t*);  // Used to read operational parameters from the sensor.

    /* Overrides from I2CDeviceWithRegisters... */
    void operationCompleteCallback(I2CBusOp*);
    void printDebug(StringBuilder*);


  private:
    uint8_t power_mode  = 0;
    const float p0_sea_level = 101325;

    bool calculate_temperature();
    bool calculate_pressure();
    SensorError set_power_mode(uint8_t);

};

#endif
