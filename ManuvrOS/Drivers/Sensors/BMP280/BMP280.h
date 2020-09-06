/*
File:   BMP280.h
Author: J. Ian Lindsay
Date:   2013.08.08

I have adapted the code written by Jim Lindblom, and located at this URL:
https://www.sparkfun.com/tutorial/Barometric/BMP280_Example_Code.pde

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

#ifndef __BMP280_SENSOR_H__
#define __BMP280_SENSOR_H__

#include <Drivers/Sensors/SensorWrapper.h>
#include <I2CAdapter.h>

#define BMP280_I2CADDR              0x77


enum class BMP280StandbyDuration : uint8_t {
  MS_5      = 0x00,
  MS_63     = 0x01,
  MS_125    = 0x02,
  MS_250    = 0x03,
  MS_500    = 0x04,
  MS_1000   = 0x05,
  MS_10     = 0x06,
  MS_20     = 0x07
};

enum class BMP280Filter : uint8_t {
  OFF = 0x00,
  X2  = 0x01,
  X4  = 0x02,
  X8  = 0x03,
  X16 = 0x04
};

enum class BMP280Mode : uint8_t {
  SLEEP = 0x00,
  FORCE = 0x01,
  NORM  = 0x02
};

enum class BMP280Sampling : uint8_t {
  OFF = 0x00,
  X1  = 0x01,
  X2  = 0x02,
  X4  = 0x03,
  X8  = 0x04,
  X16 = 0x05
};


class BMP280 : public I2CDevice, public SensorWrapper {
  public:
    double temperature = 0;      // The present temperature reading, in degerees C.
    double pressure    = 0;      // The present pressure reading, in Pa.
    float  altitude    = 0;      // The present calculated altitude, in meters.

    BMP280(uint8_t addr = BMP280_I2CADDR);
    ~BMP280();

    /* Overrides from SensorWrapper */
    SensorError init();
    SensorError readSensor();
    SensorError setParameter(uint16_t reg, int len, uint8_t*);  // Used to set operational parameters for the sensor.
    SensorError getParameter(uint16_t reg, int len, uint8_t*);  // Used to read operational parameters from the sensor.

    /* Overrides from I2CDevice... */
    int8_t io_op_callback(BusOp*);
    void printDebug(StringBuilder*);


  private:
    // TODO: Condense into integer flags.
    bool _bme280      = false;
    bool _dev_present = false;
    bool _class_cal0  = false;
    bool _class_cal1  = false;
    bool _proc_temp   = false;
    bool _proc_pres   = false;
    bool _proc_alt    = false;
    bool _proc_hum    = false;

    uint8_t _cal_h_data[9];
    uint8_t _cal_data[24];
    uint8_t _raw_data[8];
    uint8_t _reg_id        = 0;
    uint8_t _reg_config    = 0;
    uint8_t _reg_status    = 0;
    uint8_t _reg_ctrl_pres = 0;
    uint8_t _reg_ctrl_hum  = 0;

    int32_t uncomp_temp = 0;  // Uncompensated temperature.
    int32_t uncomp_pres = 0;  // Uncompensated pressure.
    int32_t uncomp_hum  = 0;  // Uncompensated humidity.

    /* Inline accessors for the calibration data. */
    inline uint16_t _get_cal_t1() {  return *((uint16_t*) (_cal_data + 0));  };
    inline int16_t  _get_cal_t2() {  return *((int16_t*)  (_cal_data + 2));  };
    inline int16_t  _get_cal_t3() {  return *((int16_t*)  (_cal_data + 4));  };
    inline uint16_t _get_cal_p1() {  return *((uint16_t*) (_cal_data + 6));  };
    inline int16_t  _get_cal_p2() {  return *((int16_t*)  (_cal_data + 8));  };
    inline int16_t  _get_cal_p3() {  return *((int16_t*)  (_cal_data + 10)); };
    inline int16_t  _get_cal_p4() {  return *((int16_t*)  (_cal_data + 12)); };
    inline int16_t  _get_cal_p5() {  return *((int16_t*)  (_cal_data + 14)); };
    inline int16_t  _get_cal_p6() {  return *((int16_t*)  (_cal_data + 16)); };
    inline int16_t  _get_cal_p7() {  return *((int16_t*)  (_cal_data + 18)); };
    inline int16_t  _get_cal_p8() {  return *((int16_t*)  (_cal_data + 20)); };
    inline int16_t  _get_cal_p9() {  return *((int16_t*)  (_cal_data + 22)); };
    inline uint8_t  _get_cal_h1() {  return *((uint8_t*)  (_cal_h_data + 0)); };
    inline int16_t  _get_cal_h2() {  return _cal_h_data[1] + (_cal_h_data[2] << 8); };
    inline uint8_t  _get_cal_h3() {  return *((uint8_t*)  (_cal_h_data + 3)); };
    inline int16_t  _get_cal_h4() {  return *((int16_t*)  (_cal_h_data + 4)); };
    inline int16_t  _get_cal_h5() {  return *((int16_t*)  (_cal_h_data + 6)); };
    inline int8_t   _get_cal_h6() {  return *((int8_t*)   (_cal_h_data + 8)); };

    void _class_init();
    int8_t _proc_data();
    bool _is_cal_complete();
    bool _set_config(BMP280StandbyDuration, BMP280Filter);
    bool _set_sampling(BMP280Sampling, BMP280Mode);
    bool reset();
};

#endif  // __BMP280_SENSOR_H__
