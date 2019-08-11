/*
File:   HMC5883.h
Author: J. Ian Lindsay
Date:   2019.07.15

Copyright 2019 Manuvr, Inc

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

#ifndef __HMC5883_SENSOR_H__
#define __HMC5883_SENSOR_H__

#include <DataStructures/Vector3.h>
#include <Drivers/Sensors/SensorWrapper.h>
#include <Platform/Peripherals/I2C/I2CAdapter.h>

#define HMC5883_REG_CONFA      0x00  // R/W
#define HMC5883_REG_CONFB      0x01  // R/W
#define HMC5883_REG_MODE       0x02  // R/W
#define HMC5883_REG_DATA       0x03  // Read-only, 6 bytes long.
#define HMC5883_REG_STATUS     0x09  // Read-only
#define HMC5883_REG_ID         0x0A  // Read-only, 3-bytes long

#define HMC5883_I2CADDR        0x1E


// We wait until this many consecutive samples are under
//   threshold to change scale downward.
#define HMC5883_HYSTERESIS_COUNT   50

// This is how far under the next-lowest scaler we need
//   to be to justify switching to it.
#define HMC5883_HYSTERESIS_FRAC  0.8f


enum class HMC5883Mode : uint8_t {
  CONTINUOUS = 0x00,
  SINGLE     = 0x01,
  IDLE       = 0x03
};


enum class HMC5883OutputRate : uint8_t {
  OR_075_HZ  = 0x00,
  OR_1_5_HZ  = 0x01,
  OR_3_HZ    = 0x02,
  OR_7_5_HZ  = 0x03,
  OR_15_HZ   = 0x04,
  OR_30_HZ   = 0x05,
  OR_75_HZ   = 0x06,
  OR_RESRVD  = 0x07
};


class HMC5883 : public I2CDevice, public SensorWrapper {
  public:
    HMC5883(uint8_t addr = HMC5883_I2CADDR);
    ~HMC5883();

    /* Overrides from SensorWrapper */
    SensorError init();
    SensorError readSensor();
    SensorError setParameter(uint16_t reg, int len, uint8_t*);  // Used to set operational parameters for the sensor.
    SensorError getParameter(uint16_t reg, int len, uint8_t*);  // Used to read operational parameters from the sensor.

    int8_t setDeclinationDegrees(float);

    /* Overrides from I2CDevice... */
    int8_t io_op_callback(BusOp*);
    void printDebug(StringBuilder*);


  private:
    Vector3<float>   _bearing;      // A unit vector representing "Mag-North".
    Vector3<float>   _scaled_mag;   // Scaled to gauss.
    Vector3<float>   _tilt;         // A unit vector representing "Down".
    float            _declination;  // Magnetic declination in radians.
    Vector3<int16_t> _unit_front;   // A unit vector representing "dead-ahead".
    uint8_t _reg_shadows[12];
    uint8_t _scaler_drop_hyst;      // A hysteresis counter for scale changes.

    void _class_init();
    int8_t _proc_data();
    bool _is_cal_complete();
    int8_t _autoscale_check_raise(Vector3<int16_t>*);
    int8_t _autoscale_check_lower(Vector3<float>*);
    int8_t _set_range(float);
    int8_t _set_mode(HMC5883Mode);
    int8_t _set_oversampling(uint8_t);
    int8_t _set_rate(HMC5883OutputRate);
    int8_t _write_register(uint8_t addr, uint8_t val);
    bool reset();

    inline uint8_t _reg_value(uint8_t idx) {
      return ((13 > idx) ? _reg_shadows[idx] : 0);
    };

    inline void _reg_set_internal(uint8_t idx, uint8_t val) {
      if (13 > idx) { _reg_shadows[idx] = val;  }
    };
};

#endif  // __HMC5883_SENSOR_H__
