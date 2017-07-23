/***************************************************************************
  This is a library for the LSM303 Accelerometer and magnentometer/compass

  Designed specifically to work with the Adafruit LSM303DLHC Breakout

  These displays use I2C to communicate, 2 pins are required to interface.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!

  Written by Kevin Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ***************************************************************************/
//#include "Arduino.h"
//#include <limits.h>

#include "../i2c-adapter/i2c-adapter.h"
extern I2CAdapter i2c;

#include "LSM303.h"



/****************************************************************************************************
* Accelerometer                                                                                     *
****************************************************************************************************/
LSM303_Accel::LSM303_Accel() : IMU() {
  this->defineDatum("X", SensorWrapper::COMMON_UNITS_ACCEL, TCode::FLOAT);
  this->defineDatum("Y", SensorWrapper::COMMON_UNITS_ACCEL, TCode::FLOAT);
  this->defineDatum("Z", SensorWrapper::COMMON_UNITS_ACCEL, TCode::FLOAT);
  this->s_id = "53c0926ddbdad6822e925c5d45bef03c";
  this->name = "LSM303 Accelerometer";

  this->_lsm303Accel_MG_LSB     = 0.001F;
}



int8_t LSM303_Accel::init() {
  // Enable the accelerometer
  i2c.write8(_ADDRESS, LSM303_REGISTER_ACCEL_CTRL_REG1_A, 0x27);
  this->sensor_active = true;
  return SensorWrapper::SENSOR_ERROR_NO_ERROR;
}


int8_t LSM303_Accel::readSensor() {
  // Read the accelerometer
  uint8_t *buf = (uint8_t *) alloca(10);
  i2c.readX(_ADDRESS, LSM303_REGISTER_ACCEL_OUT_X_L_A | 0x80, 6, buf);

  uint8_t xlo = *(buf + 0);
  uint8_t xhi = *(buf + 1);
  uint8_t ylo = *(buf + 2);
  uint8_t yhi = *(buf + 3);
  uint8_t zlo = *(buf + 4);
  uint8_t zhi = *(buf + 5);

  // Shift values to create properly formed integer (low byte first)
  _accelData.x = ((int16_t)(xlo | (xhi << 8)) >> 4) * _lsm303Accel_MG_LSB * 9.80665F;
  _accelData.y = ((int16_t)(ylo | (yhi << 8)) >> 4) * _lsm303Accel_MG_LSB * 9.80665F;
  _accelData.z = ((int16_t)(zlo | (zhi << 8)) >> 4) * _lsm303Accel_MG_LSB * 9.80665F;

  updateDatum(0, _accelData.x);
  updateDatum(1, _accelData.y);
  updateDatum(2, _accelData.z);

  return SensorWrapper::SENSOR_ERROR_NO_ERROR;
}


int8_t LSM303_Accel::setParameter(uint16_t reg, int len, uint8_t*) {
  return SensorWrapper::SENSOR_ERROR_INVALID_PARAM_ID;
}


int8_t LSM303_Accel::getParameter(uint16_t reg, int len, uint8_t*) {
  return SensorWrapper::SENSOR_ERROR_INVALID_PARAM_ID;
}

// Report ourselves as a triple-axis Accelerometer
uint8_t LSM303_Accel::reportCapabilities(void) { return 0x32; }


int8_t LSM303_Accel::setErrorRates() {
	return SensorWrapper::SENSOR_ERROR_NO_ERROR;
}

int8_t LSM303_Accel::readOutstandingData(int8_t slot, Measurement &msrmnt) {
	return SensorWrapper::SENSOR_ERROR_NO_ERROR;
}




/****************************************************************************************************
* Magnetometer                                                                                      *
****************************************************************************************************/
LSM303_Mag::LSM303_Mag() : IMU() {
  this->defineDatum("X", SensorWrapper::COMMON_UNITS_U_TESLA, TCode::FLOAT);
  this->defineDatum("Y", SensorWrapper::COMMON_UNITS_U_TESLA, TCode::FLOAT);
  this->defineDatum("Z", SensorWrapper::COMMON_UNITS_U_TESLA, TCode::FLOAT);
  this->s_id = "9345a88becc973cde84f3454f09d9547";
  this->name = "LSM303 Magnetometer";

  this->_lsm303Mag_Gauss_LSB_XY = 1100.0F;
  this->_lsm303Mag_Gauss_LSB_Z  = 980.0F;
}


int8_t LSM303_Mag::init() {
  i2c.write8(_ADDRESS, LSM303_REGISTER_MAG_MR_REG_M, 0x00);  // Enable the magnetometer
  setMagGain(LSM303_MAGGAIN_1_3);                                  // Set the gain to a known level
  this->sensor_active = true;
  return SensorWrapper::SENSOR_ERROR_NO_ERROR;
}



int8_t LSM303_Mag::readSensor() {
  bool readingValid = true;
  // Read the magnetometer
  uint8_t *buf = (uint8_t *) alloca(10);
  memset(buf, 0x00, 10);
  i2c.readX(_ADDRESS, LSM303_REGISTER_MAG_OUT_X_H_M | 0x80, 6, buf);

  // Note high before low (different than accel)
  uint8_t xhi = *(buf + 0);
  uint8_t xlo = *(buf + 1);
  uint8_t zhi = *(buf + 2);
  uint8_t zlo = *(buf + 3);
  uint8_t yhi = *(buf + 4);
  uint8_t ylo = *(buf + 5);

  // Shift values to create properly formed integer (low byte first)
  _magData.x = (int16_t)(xlo | ((int16_t)xhi << 8));
  _magData.y = (int16_t)(ylo | ((int16_t)yhi << 8));
  _magData.z = (int16_t)(zlo | ((int16_t)zhi << 8));

  // ToDo: Calculate orientation
  _magData.orientation = 0.0;


  if ( (_magData.x >= 4090) | (_magData.x <= -4090) |
       (_magData.y >= 4090) | (_magData.y <= -4090) |
       (_magData.z >= 4090) | (_magData.z <= -4090) ) {
    /* Saturating .... increase the range if we can */
    switch(_magGain) {
      case LSM303_MAGGAIN_5_6:
        setMagGain(LSM303_MAGGAIN_8_1);
        readingValid = false;
        break;
      case LSM303_MAGGAIN_4_7:
        setMagGain(LSM303_MAGGAIN_5_6);
        readingValid = false;
        break;
      case LSM303_MAGGAIN_4_0:
        setMagGain(LSM303_MAGGAIN_4_7);
        readingValid = false;
        break;
      case LSM303_MAGGAIN_2_5:
        setMagGain(LSM303_MAGGAIN_4_0);
        readingValid = false;
        break;
      case LSM303_MAGGAIN_1_9:
        setMagGain(LSM303_MAGGAIN_2_5);
        readingValid = false;
        break;
      case LSM303_MAGGAIN_1_3:
        setMagGain(LSM303_MAGGAIN_1_9);
        readingValid = false;
        break;
      default:
        readingValid = true;
        break;
    }
  }

  if (readingValid) {
    updateDatum(0, _magData.x);
    updateDatum(1, _magData.y);
    updateDatum(2, _magData.z);
  }

  return SensorWrapper::SENSOR_ERROR_NO_ERROR;
}


/*
* Sets the magnetometer's gain
*/
void LSM303_Mag::setMagGain(lsm303MagGain gain) {
  i2c.write8(_ADDRESS, (uint8_t) LSM303_REGISTER_MAG_CRB_REG_M, gain);

  _magGain = gain;

  switch(gain) {
    case LSM303_MAGGAIN_1_3:
      _lsm303Mag_Gauss_LSB_XY = 1100;
      _lsm303Mag_Gauss_LSB_Z  = 980;
      break;
    case LSM303_MAGGAIN_1_9:
      _lsm303Mag_Gauss_LSB_XY = 855;
      _lsm303Mag_Gauss_LSB_Z  = 760;
      break;
    case LSM303_MAGGAIN_2_5:
      _lsm303Mag_Gauss_LSB_XY = 670;
      _lsm303Mag_Gauss_LSB_Z  = 600;
      break;
    case LSM303_MAGGAIN_4_0:
      _lsm303Mag_Gauss_LSB_XY = 450;
      _lsm303Mag_Gauss_LSB_Z  = 400;
      break;
    case LSM303_MAGGAIN_4_7:
      _lsm303Mag_Gauss_LSB_XY = 400;
      _lsm303Mag_Gauss_LSB_Z  = 255;
      break;
    case LSM303_MAGGAIN_5_6:
      _lsm303Mag_Gauss_LSB_XY = 330;
      _lsm303Mag_Gauss_LSB_Z  = 295;
      break;
    case LSM303_MAGGAIN_8_1:
      _lsm303Mag_Gauss_LSB_XY = 230;
      _lsm303Mag_Gauss_LSB_Z  = 205;
      break;
  }
}


int8_t LSM303_Mag::setParameter(uint16_t reg, int len, uint8_t*) {
  return SensorWrapper::SENSOR_ERROR_INVALID_PARAM_ID;
}


int8_t LSM303_Mag::getParameter(uint16_t reg, int len, uint8_t*) {
  return SensorWrapper::SENSOR_ERROR_INVALID_PARAM_ID;
}


// Report ourselves as a triple-axis Magnetometer.
uint8_t LSM303_Mag::reportCapabilities(void) { return 0x34; }


int8_t LSM303_Mag::setErrorRates() {
	return SensorWrapper::SENSOR_ERROR_NO_ERROR;
}

int8_t LSM303_Mag::readOutstandingData(int8_t slot, Measurement &msrmnt) {
	return SensorWrapper::SENSOR_ERROR_NO_ERROR;
}
