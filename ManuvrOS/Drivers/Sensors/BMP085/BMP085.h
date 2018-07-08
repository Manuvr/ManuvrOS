/*
File:   BMP085.h
Author: J. Ian Lindsay
Date:   2013.08.08

I have adapted the code written by Jim Lindblom, and located at this URL:
https://www.sparkfun.com/tutorial/Barometric/BMP085_Example_Code.pde

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

#ifndef __BMP085_SENSOR_H__
#define __BMP085_SENSOR_H__

#include <Drivers/Sensors/SensorWrapper.h>
#include <Platform/Peripherals/I2C/I2CAdapter.h>

#define BMP085_I2CADDR              0x77


class BMP085 : public I2CDevice, public SensorWrapper {
  public:
    double temperature = 0;      // The present temperature reading, in degerees C.
    double pressure    = 0;      // The present pressure reading, in Pa.
    float  altitude    = 0;      // The present calculated altitude, in meters.

    BMP085(uint8_t addr, uint8_t pin);   // What pin is the EOC IRQ?
    BMP085(uint8_t addr = BMP085_I2CADDR);
    ~BMP085();

    /* Overrides from SensorWrapper */
    SensorError init();
    SensorError readSensor();
    SensorError setParameter(uint16_t reg, int len, uint8_t*);  // Used to set operational parameters for the sensor.
    SensorError getParameter(uint16_t reg, int len, uint8_t*);  // Used to read operational parameters from the sensor.

    /* Overrides from I2CDevice... */
    int8_t io_op_callback(I2CBusOp*);
    void printDebug(StringBuilder*);


  private:
    uint8_t irq_pin    = 255; // To which pin (if any) is the sensor's IRQ signal tied?
    const uint16_t OSS = 3;   // Oversampling Setting
    int8_t read_step   = -1;  // Used to keep track of which stage of the read we are in.

    int16_t  ac1;    // Calibration values
    int16_t  ac2;
    int16_t  ac3;
    uint16_t ac4;
    uint16_t ac5;
    uint16_t ac6;
    int16_t  b1;
    int16_t  b2;
    int16_t  mb;
    int16_t  mc;
    int16_t  md;

    // b5 is calculated in getTemperature(...), this variable is also used in getPressure(...)
    // so getTemperature(...) must be called before getPressure(...).
    long b5          = 0;
    long uncomp_temp = 0;  // Uncompensated temperature.
    long uncomp_pres = 0;  // Uncompensated pressure.

    void  calibrate();   // This must be called at least once. Also: idempotent.

    char  bmp085Read(unsigned char address);      // Read 1 byte from the BMP085 at 'address'
    int   bmp085ReadInt(unsigned char address);   // Read 2 bytes from the BMP085

    void  initiate_UT_read();
    long  getTemperature();   // Given uncompensated temperature, return degrees Celcius.

    void  initiate_UP_read();
    long  read_up();
    long  getPressure();      // Given uncompensated pressure, return Pascals.

    float calculateAltitude();
};

#endif  // __BMP085_SENSOR_H__
