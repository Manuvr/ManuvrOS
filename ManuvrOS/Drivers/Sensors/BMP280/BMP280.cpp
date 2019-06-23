/*
File:   BMP280.cpp
Author: J. Ian Lindsay
Date:   2018.07.08

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

#include "BMP280.h"

#define BMP280_REG_CAL00      0x88   // 26 bytes
#define BMP280_REG_ID         0xD0
#define BMP280_REG_VER        0xD1
#define BMP280_REG_RESET      0xE0
#define BMP280_REG_CAL26      0xE1   // 9 bytes
#define BMP280_REG_HUMID_CTRL 0xF2
#define BMP280_REG_STATUS     0xF3
#define BMP280_REG_CONTROL    0xF4
#define BMP280_REG_CONFIG     0xF5
#define BMP280_REG_DATA       0xF7   // 8 bytes


const DatumDef datum_defs[] = {
  {
    .desc    = "Barometric Pressure",
    .units   = COMMON_UNITS_PRESSURE,
    .type_id = TCode::DOUBLE,
    .flgs    = SENSE_DATUM_FLAG_HARDWARE
  },
  {
    .desc    = "Temperature",
    .units   = COMMON_UNITS_C,
    .type_id = TCode::DOUBLE,
    .flgs    = SENSE_DATUM_FLAG_HARDWARE
  },
  {
    .desc    = "Altitude",
    .units   = COMMON_UNITS_METERS,
    .type_id = TCode::FLOAT,
    .flgs    = 0   // We use our two "real" senses to derive a third.
  },
  {
    .desc    = "Humidity",
    .units   = COMMON_UNITS_PERCENT,
    .type_id = TCode::FLOAT,
    .flgs    = SENSE_DATUM_FLAG_HARDWARE
  }
};

static uint8_t RESET_BYTE = 0xB6;


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
BMP280::BMP280(uint8_t addr) : I2CDevice(addr), SensorWrapper("BMP280") {
  define_datum(&datum_defs[0]);
  define_datum(&datum_defs[1]);
  define_datum(&datum_defs[2]);
  define_datum(&datum_defs[3]);
  _class_init();
  #if defined(CONFIG_MANUVR_SENSOR_MGR)
    platform.sensorManager()->addSensor(this);
  #endif
}


/*
* Destructor.
*/
BMP280::~BMP280() {
}


void BMP280::_class_init() {
  _bme280      = false;
  _dev_present = false;
  _class_cal0  = false;
  _class_cal1  = false;
  _proc_temp   = false;
  _proc_pres   = false;
  _proc_alt    = false;
  _proc_hum    = false;
  _reg_id        = 0;
  _reg_config    = 0;
  _reg_status    = 0;
  _reg_ctrl_pres = 0;
  _reg_ctrl_hum  = 0;
  memset(_raw_data, 0, 8);
  memset(_cal_data, 0, 24);
  memset(_cal_h_data, 0, 9);
}



/*******************************************************************************
*  ,-.                      ,   .
* (   `                     | . |
*  `-.  ,-. ;-. ,-. ,-. ;-. | ) ) ;-. ,-: ;-. ;-. ,-. ;-.
* .   ) |-' | | `-. | | |   |/|/  |   | | | | | | |-' |
*  `-'  `-' ' ' `-' `-' '   ' '   '   `-` |-' |-' `-' '
******************************************|***|********************************/

SensorError BMP280::init() {
  // Read the device ID register, and if it comes back as expected, dispatch the
  //   rest of the init routine from the callbacks.
  return readX(BMP280_REG_ID, 1, &_reg_id) ? SensorError::NO_ERROR : SensorError::BUS_ERROR;
}

SensorError BMP280::readSensor() {
  // Depending on which stage of the read we are on, jump to the next stage.
  if (!isCalibrated()) {
    return SensorError::NOT_CALIBRATED;
  }
  return readX(BMP280_REG_DATA, 8, _raw_data) ? SensorError::NO_ERROR : SensorError::BUS_ERROR;
}


SensorError BMP280::setParameter(uint16_t reg, int len, uint8_t*) {
  return SensorError::INVALID_PARAM_ID;
}


SensorError BMP280::getParameter(uint16_t reg, int len, uint8_t*) {
  return SensorError::INVALID_PARAM_ID;
}


/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

int8_t BMP280::io_op_callback(BusOp* op) {
  I2CBusOp* completed = (I2CBusOp*) op;
  StringBuilder output;
  if (completed->hasFault()) {
    output.concat("An i2c operation requested by the BMP280 came back failed.\n");
    completed->printDebug(&output);
    Kernel::log(&output);
    return -1;
  }

  if (BusOpcode::RX == completed->get_opcode()) {
    // We read.
    switch (completed->sub_addr) {
      case BMP280_REG_CAL00:
        if (platform.bigEndian()) {
          // This device is little-endian, so if we are a big-endian machine,
          //   we need to swap all the words so that arithmetic makes sense.
          *((uint16_t*) (_cal_data + 0))  = endianSwap16(_get_cal_t1());
          *((int16_t*)  (_cal_data + 2))  = endianSwap16(_get_cal_t2());
          *((int16_t*)  (_cal_data + 4))  = endianSwap16(_get_cal_t3());
          *((uint16_t*) (_cal_data + 6))  = endianSwap16(_get_cal_p1());
          *((int16_t*)  (_cal_data + 8))  = endianSwap16(_get_cal_p2());
          *((int16_t*)  (_cal_data + 10)) = endianSwap16(_get_cal_p3());
          *((int16_t*)  (_cal_data + 12)) = endianSwap16(_get_cal_p4());
          *((int16_t*)  (_cal_data + 14)) = endianSwap16(_get_cal_p5());
          *((int16_t*)  (_cal_data + 16)) = endianSwap16(_get_cal_p6());
          *((int16_t*)  (_cal_data + 18)) = endianSwap16(_get_cal_p7());
          *((int16_t*)  (_cal_data + 20)) = endianSwap16(_get_cal_p8());
          *((int16_t*)  (_cal_data + 22)) = endianSwap16(_get_cal_p9());
        }
        _class_cal0 = true;
        output.concat("BMP280_REG_CAL00\n");
        output.concatf("\t%d\t%d\n", *((uint16_t*) (_cal_data + 0)) , _get_cal_t1());
        output.concatf("\t%d\t%d\n", *((int16_t*)  (_cal_data + 2)) , _get_cal_t2());
        output.concatf("\t%d\t%d\n", *((int16_t*)  (_cal_data + 4)) , _get_cal_t3());
        output.concatf("\t%d\t%d\n", *((uint16_t*) (_cal_data + 6)) , _get_cal_p1());
        output.concatf("\t%d\t%d\n", *((int16_t*)  (_cal_data + 8)) , _get_cal_p2());
        output.concatf("\t%d\t%d\n", *((int16_t*)  (_cal_data + 10)), _get_cal_p3());
        output.concatf("\t%d\t%d\n", *((int16_t*)  (_cal_data + 12)), _get_cal_p4());
        output.concatf("\t%d\t%d\n", *((int16_t*)  (_cal_data + 14)), _get_cal_p5());
        output.concatf("\t%d\t%d\n", *((int16_t*)  (_cal_data + 16)), _get_cal_p6());
        output.concatf("\t%d\t%d\n", *((int16_t*)  (_cal_data + 18)), _get_cal_p7());
        output.concatf("\t%d\t%d\n", *((int16_t*)  (_cal_data + 20)), _get_cal_p8());
        output.concatf("\t%d\t%d\n", *((int16_t*)  (_cal_data + 22)), _get_cal_p9());
        if (_is_cal_complete()) {
          _set_config(BMP280StandbyDuration::MS_1000, BMP280Filter::X4);
          _set_sampling(BMP280Sampling::X4, BMP280Mode::NORM);
        }
        break;

      case BMP280_REG_ID:
        // If the value is acceptable, we make the identity a bit more granular,
        //   and read the calibration data.
        switch (_reg_id) {
          case 0x60:
            _bme280 = true;
            _dev_present = true;
            readX(BMP280_REG_CAL00, 24, _cal_data);
            readX(BMP280_REG_CAL26, 9, _cal_h_data);
            readX(BMP280_REG_CONTROL, 1, &_reg_config);
            readX(BMP280_REG_STATUS, 1, &_reg_status);
            readX(BMP280_REG_HUMID_CTRL, 1, &_reg_ctrl_hum);
            readX(BMP280_REG_CONTROL, 1, &_reg_ctrl_pres);
            break;
          case 0x58:
            Kernel::log("This driver does not support the BMP280 yet.\n");
            break;
          default:
            break;
        }
        break;

      case BMP280_REG_CAL26:
        _cal_h_data[4] <<= 4;
        _cal_h_data[5] &= 0x0F;
        if (platform.bigEndian()) {
          // This device is little-endian, so if we are a big-endian machine,
          //   we need to swap all the words so that arithmetic makes sense.
          uint8_t x      = _cal_h_data[1];
          _cal_h_data[1] = _cal_h_data[2];
          _cal_h_data[2] = x;
          *((int16_t*) (_cal_h_data + 4)) = endianSwap16(_get_cal_h4());
          *((int16_t*) (_cal_h_data + 6)) = endianSwap16(_get_cal_h5());
        }
        _class_cal1 = true;
        if (_is_cal_complete()) {
          _set_config(BMP280StandbyDuration::MS_1000, BMP280Filter::X4);
          _set_sampling(BMP280Sampling::X4, BMP280Mode::NORM);
        }
        break;

      case BMP280_REG_DATA:
        if (isCalibrated()) {
          // Data values are big-endian.
          uncomp_pres = (completed->buf[0] * 65536) + (completed->buf[1] * 256) + completed->buf[2];
          uncomp_temp = (completed->buf[3] * 65536) + (completed->buf[4] * 256) + completed->buf[5];
          uncomp_hum  = (int32_t) (completed->buf[6] * 256) + completed->buf[7];
          _proc_temp = (0x800000 != uncomp_temp);
          _proc_pres = (0x800000 != uncomp_pres);
          _proc_alt = _proc_temp;
          _proc_hum = (0x8000 != uncomp_hum) & _proc_temp;
          _proc_data();
        }
        break;

      case BMP280_REG_CONTROL:
        break;
      case BMP280_REG_CONFIG:
        break;
      case BMP280_REG_RESET:
      default:
        // Illegal read target.
        break;
    }
  }
  else if (BusOpcode::TX == completed->get_opcode()) {
    // We wrote.
    switch (completed->sub_addr) {
      case BMP280_REG_CONTROL:
        break;
      case BMP280_REG_CONFIG:
        break;
      case BMP280_REG_RESET:
        break;
      default:
        // Illegal write target.
        break;
    }
  }
  Kernel::log(&output);
  return 0;
}


/**
* Dump this item to the dev log.
*
* @param The buffer to receive the output.
*/
void BMP280::printDebug(StringBuilder* temp) {
  temp->concatf("Altitude sensor (BMP280)\t%snitialized%s", (isActive() ? "I": "Uni"), PRINT_DIVIDER_1_STR);
  I2CDevice::printDebug(temp);
  temp->concat("\n");
}



/*******************************************************************************
* Class-specific functions...                                                  *
*******************************************************************************/

/**
* Executes a software reset.
*
* @return true if the reset was sent to the device.
*/
bool BMP280::reset() {
  _class_init();
  return writeX(BMP280_REG_RESET, 1, &RESET_BYTE);
}


/**
* Internal fxn to check if all of the calibration requirements are met.
* Updates class local variable in SensorWrapper.
*
* @return true if calibration is complete.
*/
bool BMP280::_is_cal_complete() {
  bool a = isCalibrated();
  bool b = _class_cal0 & _class_cal1;
  if (b != a) {
    isCalibrated(b & !a);
  }
  return b;
}


/**
* Set the sensor's config register with the chosen values for standby duration,
*   and filtering.
*
* @return true if the setting was dispatched to the device.
*/
bool BMP280::_set_config(BMP280StandbyDuration x, BMP280Filter y) {
  _reg_config = (_reg_config & 0x02) | ((uint8_t) x << 5) | ((uint8_t) y << 2);
  return writeX(BMP280_REG_CONFIG, 1, &_reg_config);
}


/**
* Set the sensor's sampling setting.
*
* @return true if the setting was dispatched to the device.
*/
bool BMP280::_set_sampling(BMP280Sampling x, BMP280Mode y) {
  _reg_ctrl_pres = ((uint8_t) x << 5) | ((uint8_t) x << 2) | ((uint8_t) y);
  _reg_ctrl_hum = (_reg_ctrl_hum & 0xF8) | ((uint8_t) x);
  if (writeX(BMP280_REG_HUMID_CTRL, 1, &_reg_ctrl_hum)) {
    return writeX(BMP280_REG_CONTROL, 1, &_reg_ctrl_pres);
  }
  return false;
}


/**
* Calculation code taken from Adafruit driver:
* https://github.com/adafruit/Adafruit_BMP280_Library/blob/master/Adafruit_BMP280.cpp
*
* @return 0 on success. Non-zero on error.
*/
int8_t BMP280::_proc_data() {
  StringBuilder output;
  //printDebug(&output);
  if (_proc_temp) {
    // Calculate temperature.
    uncomp_temp >>= 4;
    int32_t t_var1 = ((((uncomp_temp >> 3) - ((int32_t)_get_cal_t1() << 1))) *
    ((int32_t) _get_cal_t2())) >> 11;
    int32_t t_var2 = (((((uncomp_temp >> 4) - ((int32_t)_get_cal_t1())) *
    ((uncomp_temp >> 4) - ((int32_t)_get_cal_t1()))) >> 12) *
    ((int32_t)_get_cal_t3())) >> 14;
    int32_t t_fine = t_var1 + t_var2;
    float T  = ((t_fine * 5 + 128) >> 8) / 100;
    updateDatum(1, T);
    output.concatf("Temperature: %f\n", T);

    if (_proc_pres) {
      // Calculate pressure.
      uncomp_pres >>= 4;
      int64_t var1 = ((int64_t) t_fine) - 128000;
      int64_t var2 = var1 * var1 * (int64_t) _get_cal_p6();
      var2 = var2 + ((var1*(int64_t)_get_cal_p5()) << 17);
      var2 = var2 + (((int64_t)_get_cal_p4()) << 35);
      var1 = ((var1 * var1 * (int64_t)_get_cal_p3()) >> 8) + ((var1 * (int64_t)_get_cal_p2()) << 12);
      var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)_get_cal_p1()) >> 33;
      if (var1 != 0) {
        int64_t p = 1048576 - uncomp_pres;
        p = (((p << 31) - var2) * 3125) / var1;
        var1 = (((int64_t)_get_cal_p9()) * (p >> 13) * (p >> 13)) >> 25;
        var2 = (((int64_t)_get_cal_p8()) * p) >> 19;
        p = ((p + var1 + var2) >> 8) + (((int64_t)_get_cal_p7()) << 4);
        double result = (double) p/256;
        updateDatum(0, result);
        output.concatf("Pressure: %f\n", result);

        if (_proc_alt) {
          // Calculate altitude.
          altitude = (float) 443.3 * (1.0 - pow((result/PRESSURE_AT_SEA_LEVEL), 0.190295));
          updateDatum(2, altitude);
          output.concatf("Altitude: %f\n", altitude);
        }
      }
    }

    if (_proc_hum) {
      // Calculate humidity.
      int32_t v_x1_u32r = (t_fine - ((int32_t) 76800));
      v_x1_u32r = (((((uncomp_hum << 14) - (((int32_t)_get_cal_h4()) << 20) -
      (((int32_t)_get_cal_h5()) * v_x1_u32r)) + ((int32_t) 16384)) >> 15) *
      (((((((v_x1_u32r * ((int32_t)_get_cal_h6())) >> 10) *
      (((v_x1_u32r * ((int32_t)_get_cal_h3())) >> 11) + ((int32_t) 32768))) >> 10) +
      ((int32_t) 2097152)) * ((int32_t)_get_cal_h2()) + 8192) >> 14));
      v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)_get_cal_h1())) >> 4));
      v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
      v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
      float h = (v_x1_u32r >> 12) / 1024.0;
      updateDatum(3, h);
      output.concatf("Humidity: %f\n", h);
    }
  }

  Kernel::log(&output);
  return 0;
}
