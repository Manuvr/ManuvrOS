/*
File:   TMP102.h
Author: J. Ian Lindsay
Date:   2013.07.12

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

#ifndef __TMP102_SENSOR_H__
#define __TMP102_SENSOR_H__

#include <Drivers/Sensors/SensorWrapper.h>
#include <Platform/Peripherals/I2C/I2CAdapter.h>

#define TMP102_REG_RESULT           0x00
#define TMP102_REG_CONFIG           0x01
#define TMP102_REG_ALRT_LO          0x02
#define TMP102_REG_ALRT_HI          0x03

#define TMP102_ADDRESS              0x48


class TMP102 : public I2CDeviceWithRegisters, public SensorWrapper {
  public:
    TMP102(uint8_t addr, uint8_t pin);   // What pin is the EOC IRQ?
    TMP102(uint8_t addr = TMP102_ADDRESS);
    ~TMP102();

    /* Overrides from SensorWrapper */
    SensorError init();
    SensorError readSensor();
    SensorError setParameter(uint16_t reg, int len, uint8_t*);  // Used to set operational parameters for the sensor.
    SensorError getParameter(uint16_t reg, int len, uint8_t*);  // Used to read operational parameters from the sensor.

    /* Overrides from I2CDeviceWithRegisters... */
    int8_t register_write_cb(DeviceRegister*);
    int8_t register_read_cb(DeviceRegister*);
    void printDebug(StringBuilder*);

    SensorError setTempRange(float lo, float hi);  // Set the temperature alert range and re-initialize the TMP102.
    SensorError setHighTemperatureAlert(float nu); // Set the high-end of the alert range in degrees C.
    SensorError setLowTemperatureAlert(float nu);  // Set the low-end of the alert range in degrees C.

    /*
    * Return the high-end of the alert range in degrees C.
    * Number is <deg-C> / 0.0625
    */
    inline float getHighTemperatureAlert() {
      return (convertShortToTemperature(regValue(TMP102_REG_ALRT_HI)));
    };

    /*
    * Return the low-end of the alert range in degrees C.
    * Number is <deg-C> / 0.0625
    */
    inline float getLowTemperatureAlert() {
      return (convertShortToTemperature(regValue(TMP102_REG_ALRT_LO)));
    };


  private:
    uint8_t irq_pin    = 255;

    /**
    *  Give this function a temperature in Celcius, and it will
    *  convert the value into the corresponding sensor output.
    */
    inline short convertTemperatureToShort(float temp_c) {
      return ((short)(temp_c / 0.0625));
    };

    /**
    *  Give this function a temperature by the sensor's reckoning,
    *   and it will convert the value into degrees Celcius.
    */
    inline float convertShortToTemperature(short temp_s) {
      return ((float)(temp_s * 0.0625));
    };
};

#endif   // __TMP102_SENSOR_H__
