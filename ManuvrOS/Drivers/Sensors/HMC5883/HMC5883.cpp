/*
File:   HMC5883.cpp
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

#include "HMC5883.h"
#include <math.h>


const DatumDef datum_defs[] = {
  {
    .desc    = "Field Strength",
    .units   = COMMON_UNITS_GAUSS,
    .type_id = TCode::FLOAT,
    .flgs    = SENSE_DATUM_FLAG_HARDWARE
  },
  {
    .desc    = "Heading",
    .units   = COMMON_UNITS_DEGREES,
    .type_id = TCode::FLOAT,
    .flgs    = SENSE_DATUM_FLAG_HARDWARE
  }
};


// How many milligauss does each LSB represent? Eight possible gain settings.
static const float _mG_per_lsb[8]  = {0.73, 0.92, 1.22, 1.52, 2.27, 2.56, 3.03, 4.35};

// Dynamic ranges
static const float _gauss_range[8] = {0.88, 1.3, 1.9, 2.5, 4.0, 4.7, 5.6, 8.1};




/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/
/*
* Constructor. Takes i2c address as argument.
*/
HMC5883::HMC5883(uint8_t addr) : I2CDevice(addr), SensorWrapper("HMC5883") {
  define_datum(&datum_defs[0]);
  define_datum(&datum_defs[1]);
  _class_init();
}


/*
* Destructor.
*/
HMC5883::~HMC5883() {
}


void HMC5883::_class_init() {
  memset(_reg_shadows, 0, sizeof(_reg_shadows));
  _bearing(0.0, 0.0, 0.0);
  _scaled_mag(0.0, 0.0, 0.0);
  _tilt(0.0, 0.0, 0.0);
  _unit_front(0, 0, 0);
  _declination = 0.0;
}



/*******************************************************************************
*  ,-.                      ,   .
* (   `                     | . |
*  `-.  ,-. ;-. ,-. ,-. ;-. | ) ) ;-. ,-: ;-. ;-. ,-. ;-.
* .   ) |-' | | `-. | | |   |/|/  |   | | | | | | |-' |
*  `-'  `-' ' ' `-' `-' '   ' '   '   `-` |-' |-' `-' '
******************************************|***|********************************/

SensorError HMC5883::init() {
  // Read the device ID register, and if it comes back as expected, dispatch the
  //   rest of the init routine from the callbacks.
  return readX(HMC5883_REG_STATUS, 4, &_reg_shadows[HMC5883_REG_STATUS]) ? SensorError::NO_ERROR : SensorError::BUS_ERROR;
}

SensorError HMC5883::readSensor() {
  // Depending on which stage of the read we are on, jump to the next stage.
  if (!isCalibrated()) {
    return SensorError::NOT_CALIBRATED;
  }
  return readX(HMC5883_REG_DATA, 6, (uint8_t*) &_reg_shadows[HMC5883_REG_DATA]) ? SensorError::NO_ERROR : SensorError::BUS_ERROR;
}


SensorError HMC5883::setParameter(uint16_t reg, int len, uint8_t*) {
  return SensorError::INVALID_PARAM_ID;
}


SensorError HMC5883::getParameter(uint16_t reg, int len, uint8_t*) {
  return SensorError::INVALID_PARAM_ID;
}


/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

int8_t HMC5883::io_op_callback(BusOp* op) {
  I2CBusOp* completed = (I2CBusOp*) op;
  StringBuilder output;
  if (completed->hasFault()) {
    output.concat("An i2c operation requested by the HMC5883 came back failed.\n");
    completed->printDebug(&output);
    Kernel::log(&output);
    return -1;
  }
  uint8_t* buf     = completed->buffer();
  uint16_t buf_len = completed->bufferLen();

  if (BusOpcode::RX == completed->get_opcode()) {
    // We read.
    switch (completed->sub_addr) {
      case HMC5883_REG_CONFA:
        break;
      case HMC5883_REG_CONFB:
        break;
      case HMC5883_REG_MODE:
        break;

      case HMC5883_REG_DATA:
        _proc_data();
        break;
      case HMC5883_REG_STATUS:
        if (4 != buf_len) break;   // NOTE: Conditional break.
      case HMC5883_REG_ID:
        if (0x48 == *(buf + 1)) {
          if (0x34 == *(buf + 2)) {
            if (0x33 == *(buf + 3)) {
              if (0 == _set_range(1.3)) {
                if (0 == _set_mode(HMC5883Mode::CONTINUOUS)) {
                  isActive(true);
                  isCalibrated(true);
                }
              }
            }
          }
        }
        break;
      default:   // Illegal read target.
        break;
    }
  }
  else if (BusOpcode::TX == completed->get_opcode()) {
    // We wrote.
    switch (completed->sub_addr) {
      case HMC5883_REG_CONFA:
        break;
      case HMC5883_REG_CONFB:
        break;
      case HMC5883_REG_MODE:
        break;
      default:   // Illegal write target.
        break;
    }
  }
  if (output.length() > 0) {  Kernel::log(&output);  }
  return 0;
}


/**
* Dump this item to the dev log.
*
* @param The buffer to receive the output.
*/
void HMC5883::printDebug(StringBuilder* temp) {
  temp->concatf("Magnetic sensor (HMC5883)\t%snitialized%s", (isActive() ? "I": "Uni"), PRINT_DIVIDER_1_STR);
  I2CDevice::printDebug(temp);
  SensorWrapper::printSensorData(temp);
  temp->concat("\n");
}


/*******************************************************************************
* Class-specific functions...                                                  *
*******************************************************************************/

int8_t HMC5883::setDeclinationDegrees(float dd) {
  // TODO: Should put bounds checking on this?
  _declination = dd * 180.0 / PI;
  return 0;
}


int8_t HMC5883::_autoscale_check_raise(Vector3<int16_t>* raw) {
  if (2047 < abs(raw->x)) {
    if (2047 < abs(raw->y)) {
      if (2047 < abs(raw->z)) {
        return 0;
      }
    }
  }
  return -1;
}


int8_t HMC5883::_autoscale_check_lower(Vector3<float>* raw) {
  uint8_t scale_idx = (_reg_shadows[HMC5883_REG_CONFB] >> 5) & 0x07;
  if (0 < scale_idx) {   // Do we have any more scale below us?
    float scalar = _gauss_range[scale_idx - 1] * HMC5883_HYSTERESIS_FRAC;
    if (scalar > abs(raw->x)) {
      if (scalar > abs(raw->y)) {
        if (scalar > abs(raw->z)) {
          return 0;
        }
      }
    }
  }
  return -1;
}


/*
* Some of the data handling code was adapted from FrankieChu's driver for the same part.
* TODO: build in magnetic declination tables or GPS correction.
*/
int8_t HMC5883::_proc_data() {
  Vector3<int16_t> raw;
  uint8_t scale_idx = (_reg_shadows[HMC5883_REG_CONFB] >> 5) & 0x07;
  // NOTE: Sensor has axes in a strange order. This is where we stop caring.
  raw.x = (_reg_shadows[HMC5883_REG_DATA + 0] << 8) | _reg_shadows[HMC5883_REG_DATA + 1];
  raw.y = (_reg_shadows[HMC5883_REG_DATA + 2] << 8) | _reg_shadows[HMC5883_REG_DATA + 3];
  raw.z = (_reg_shadows[HMC5883_REG_DATA + 4] << 8) | _reg_shadows[HMC5883_REG_DATA + 5];

  if (0 == _autoscale_check_raise(&raw)) {  // Check for scale overflow.
    float scalar = _gauss_range[scale_idx];
    _scaled_mag.x = raw.x * scalar;
    _scaled_mag.y = raw.y * scalar;
    _scaled_mag.z = raw.z * scalar;
    float field_strength = _scaled_mag.length();
    if (0 == _autoscale_check_lower(&_scaled_mag)) {  // Check for scale underflow.
      if (HMC5883_HYSTERESIS_COUNT < _scaler_drop_hyst++) {
        _set_range(_gauss_range[scale_idx-1]);
      }
    }
    else {
      _scaler_drop_hyst = 0;
    }

    // Calculate heading when the magnetometer is level, then correct for signs of axis.
    // TODO: Correct for unit/sensor orientation.
    // TODO: Add integration for tilt compensation from external sensors.
    float heading = atan2(_scaled_mag.y, _scaled_mag.x);

    heading += _declination;                    // Declination correction.
    if (heading < 0) {     heading += 2*PI;  }  // We want [0-2PI), rather than straddling zero.
    if (heading > 2*PI) {  heading -= 2*PI;  }  // Wrap correction.
    updateDatum(0, field_strength);             // Magnitude of scaled vector is field strength.
    updateDatum(1, heading * 180/PI);           // Store heading as degrees.
    return 0;
  }
  else if (scale_idx < (sizeof(_gauss_range) - 1)) {
    // If we are over-running our scale on an axis, and we still have headroom
    //   in our dynamic range, we automatically use it.
    _set_range(_gauss_range[scale_idx+1]);
    return -2;
  }
  return -1;
}


int8_t HMC5883::_set_range(float gauss) {
  for (uint8_t i = 0; i < sizeof(_gauss_range); i++) {
    if (gauss <= _gauss_range[i]) {
      _scaler_drop_hyst = 0;   // Reset the hysteresis counter.
      return _write_register(HMC5883_REG_CONFB, (i << 5) & 0xE0);
    }
  }
  return -1;  // Out of our range.
}


int8_t HMC5883::_set_mode(HMC5883Mode mode) {
  return _write_register(HMC5883_REG_MODE, ((uint8_t) mode) & 0x03);
}


int8_t HMC5883::_set_oversampling(uint8_t os) {
  uint8_t value = _reg_shadows[HMC5883_REG_CONFA] & 0x1F;
  value |= (os & 0x03) << 5;
  return _write_register(HMC5883_REG_CONFA, value);
}


int8_t HMC5883::_set_rate(HMC5883OutputRate r) {
  if (HMC5883OutputRate::OR_RESRVD != r) {
    uint8_t value = _reg_shadows[HMC5883_REG_CONFA] & 0x63;
    value |= (((uint8_t) r) & 0x03) << 2;
    return _write_register(HMC5883_REG_CONFA, value);
  }
  return -1;
}


int8_t HMC5883::_write_register(uint8_t addr, uint8_t val) {
  if (13 > addr) {
    _reg_shadows[addr] = val;
    return writeX(addr, 1, &_reg_shadows[addr]);
  }
  return -1;
}
