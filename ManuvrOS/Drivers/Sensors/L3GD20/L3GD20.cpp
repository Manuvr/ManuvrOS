/***************************************************
This is a library for the L3GD20 GYROSCOPE

Designed specifically to work with the Adafruit L3GD20 Breakout
----> https://www.adafruit.com/products/1032

These sensors use I2C or SPI to communicate, 2 pins (I2C)
or 4 pins (SPI) are required to interface.

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Kevin "KTOWN" Townsend for Adafruit Industries.
BSD license, all text above must be included in any redistribution
****************************************************/

#ifdef ARDUINO
  #include "Arduino.h"
#endif

#include <limits.h>
#include "../i2c-adapter/i2c-adapter.h"
extern I2CAdapter i2c;

#include "L3GD20.h"

/***************************************************************************
CONSTRUCTOR
***************************************************************************/
L3GD20::L3GD20() {
  _autoRangeEnabled = false;
  this->s_id = "6b4810506da5d590f5a5bf61ee7e2273";
  this->name = "L3GD20 Gyro";
  this->defineDatum("X", SensorWrapper::COMMON_UNITS_DEG_SEC, TCode::FLOAT);
  this->defineDatum("Y", SensorWrapper::COMMON_UNITS_DEG_SEC, TCode::FLOAT);
  this->defineDatum("Z", SensorWrapper::COMMON_UNITS_DEG_SEC, TCode::FLOAT);
}

/***************************************************************************
PUBLIC FUNCTIONS
***************************************************************************/


int8_t L3GD20::init() {
  /* Set the range the an appropriate value */
  _range = GYRO_RANGE_250DPS;

  /* Make sure we have the correct chip ID since this checks for correct address and that the IC is properly connected */
  uint8_t whoami = i2c.read8(_ADDRESS, L3GD20::reg_defs[L3GD20_REGISTER_WHO_AM_I].addr);
  if (whoami != L3GD20::reg_defs[L3GD20_REGISTER_WHO_AM_I].default_val) {
     return SensorWrapper::SENSOR_ERROR_NO_ERROR;
  }

  /* Set CTRL_REG1 (0x20)
====================================================================
BIT Symbol Description Default
--- ------ --------------------------------------------- -------
7-6 DR1/0 Output data rate 00
5-4 BW1/0 Bandwidth selection 00
3 PD 0 = Power-down mode, 1 = normal/sleep mode 0
2 ZEN Z-axis enable (0 = disabled, 1 = enabled) 1
1 YEN Y-axis enable (0 = disabled, 1 = enabled) 1
0 XEN X-axis enable (0 = disabled, 1 = enabled) 1 */

  /* Reset then switch to normal mode and enable all three channels */
  i2c.write8(_ADDRESS, L3GD20::reg_defs[L3GD20_REGISTER_CTRL_REG1].addr, 0x00);
  i2c.write8(_ADDRESS, L3GD20::reg_defs[L3GD20_REGISTER_CTRL_REG1].addr, 0x0F);
  /* ------------------------------------------------------------------ */

  /* Set CTRL_REG2 (0x21)
====================================================================
BIT Symbol Description Default
--- ------ --------------------------------------------- -------
5-4 HPM1/0 High-pass filter mode selection 00
3-0 HPCF3..0 High-pass filter cutoff frequency selection 0000 */

  /* Nothing to do ... keep default values */
  /* ------------------------------------------------------------------ */

  /* Set CTRL_REG3 (0x22)
====================================================================
BIT Symbol Description Default
--- ------ --------------------------------------------- -------
7 I1_Int1 Interrupt enable on INT1 (0=disable,1=enable) 0
6 I1_Boot Boot status on INT1 (0=disable,1=enable) 0
5 H-Lactive Interrupt active config on INT1 (0=high,1=low) 0
4 PP_OD Push-Pull/Open-Drain (0=PP, 1=OD) 0
3 I2_DRDY Data ready on DRDY/INT2 (0=disable,1=enable) 0
2 I2_WTM FIFO wtrmrk int on DRDY/INT2 (0=dsbl,1=enbl) 0
1 I2_ORun FIFO overrun int on DRDY/INT2 (0=dsbl,1=enbl) 0
0 I2_Empty FIFI empty int on DRDY/INT2 (0=dsbl,1=enbl) 0 */

  /* Nothing to do ... keep default values */
  /* ------------------------------------------------------------------ */

  /* Set CTRL_REG4 (0x23)
====================================================================
BIT Symbol Description Default
--- ------ --------------------------------------------- -------
7 BDU Block Data Update (0=continuous, 1=LSB/MSB) 0
6 BLE Big/Little-Endian (0=Data LSB, 1=Data MSB) 0
5-4 FS1/0 Full scale selection 00
00 = 250 dps
01 = 500 dps
10 = 2000 dps
11 = 2000 dps
0 SIM SPI Mode (0=4-wire, 1=3-wire) 0 */

  /* Adjust resolution if requested */
  switch(_range)
  {
    case GYRO_RANGE_250DPS:
      i2c.write8(_ADDRESS, L3GD20::reg_defs[L3GD20_REGISTER_CTRL_REG4].addr, 0x00);
      break;
    case GYRO_RANGE_500DPS:
      i2c.write8(_ADDRESS, L3GD20::reg_defs[L3GD20_REGISTER_CTRL_REG4].addr, 0x10);
      break;
    case GYRO_RANGE_2000DPS:
      i2c.write8(_ADDRESS, L3GD20::reg_defs[L3GD20_REGISTER_CTRL_REG4].addr, 0x20);
      break;
  }
  /* ------------------------------------------------------------------ */

  /* Set CTRL_REG5 (0x24)
====================================================================
BIT Symbol Description Default
--- ------ --------------------------------------------- -------
7 BOOT Reboot memory content (0=normal, 1=reboot) 0
6 FIFO_EN FIFO enable (0=FIFO disable, 1=enable) 0
4 HPen High-pass filter enable (0=disable,1=enable) 0
3-2 INT1_SEL INT1 Selection config 00
1-0 OUT_SEL Out selection config 00 */

  /* Nothing to do ... keep default values */
  /* ------------------------------------------------------------------ */
  this->sensor_active = true;
  return SensorWrapper::SENSOR_ERROR_NO_ERROR;
}


int8_t L3GD20::setParameter(uint16_t reg, int len, uint8_t*) {
  return SensorWrapper::SENSOR_ERROR_INVALID_PARAM_ID;
}


int8_t L3GD20::getParameter(uint16_t reg, int len, uint8_t*) {
  return SensorWrapper::SENSOR_ERROR_INVALID_PARAM_ID;
}


// Report ourselves as a triple-axis Gryroscope.
uint8_t L3GD20::reportCapabilities(void) { return 0x31; }


int8_t L3GD20::readOutstandingData(int8_t slot, Measurement &msrmnt) {
	return SensorWrapper::SENSOR_ERROR_NO_ERROR;
}


int8_t L3GD20::setErrorRates() {
	return SensorWrapper::SENSOR_ERROR_NO_ERROR;
}




void L3GD20::enableAutoRange(bool enabled) {
  _autoRangeEnabled = enabled;
}



int8_t L3GD20::readSensor() {
  bool readingValid = false;

  while (!readingValid) {
    /* Read 6 bytes from the sensor */
    uint8_t *buf = (uint8_t *) alloca(10);
    i2c.readX(_ADDRESS, reg_defs[L3GD20_REGISTER_OUT_X_L].addr | 0x80, 6, buf);

    uint8_t xlo = *(buf + 0);
    uint8_t xhi = *(buf + 1);
    uint8_t ylo = *(buf + 2);
    uint8_t yhi = *(buf + 3);
    uint8_t zlo = *(buf + 4);
    uint8_t zhi = *(buf + 5);

    /* Shift values to create properly formed integer (low byte first) */
    gyro_x = (int16_t)(xlo | (xhi << 8));
    gyro_y = (int16_t)(ylo | (yhi << 8));
    gyro_z = (int16_t)(zlo | (zhi << 8));

    /* Make sure the sensor isn't saturating if auto-ranging is enabled */
    if (!_autoRangeEnabled) {
      readingValid = true;
    }
    else {
      /* Check if the sensor is saturating or not */
      if ( (gyro_x >= 32760) | (gyro_x <= -32760) |
           (gyro_y >= 32760) | (gyro_y <= -32760) |
           (gyro_z >= 32760) | (gyro_z <= -32760) )
      {
        /* Saturating .... increase the range if we can */
        switch(_range) {
          case GYRO_RANGE_500DPS:
            /* Push the range up to 2000dps */
            _range = GYRO_RANGE_2000DPS;
            i2c.write8(_ADDRESS, L3GD20::reg_defs[L3GD20_REGISTER_CTRL_REG1].addr, 0x00);
            i2c.write8(_ADDRESS, L3GD20::reg_defs[L3GD20_REGISTER_CTRL_REG1].addr, 0x0F);
            i2c.write8(_ADDRESS, L3GD20::reg_defs[L3GD20_REGISTER_CTRL_REG4].addr, 0x20);
            i2c.write8(_ADDRESS, L3GD20::reg_defs[L3GD20_REGISTER_CTRL_REG5].addr, 0x80);
            readingValid = false;
            break;
          case GYRO_RANGE_250DPS:
            /* Push the range up to 500dps */
            _range = GYRO_RANGE_500DPS;
            i2c.write8(_ADDRESS, L3GD20::reg_defs[L3GD20_REGISTER_CTRL_REG1].addr, 0x00);
            i2c.write8(_ADDRESS, L3GD20::reg_defs[L3GD20_REGISTER_CTRL_REG1].addr, 0x0F);
            i2c.write8(_ADDRESS, L3GD20::reg_defs[L3GD20_REGISTER_CTRL_REG4].addr, 0x10);
            i2c.write8(_ADDRESS, L3GD20::reg_defs[L3GD20_REGISTER_CTRL_REG5].addr, 0x80);
            readingValid = false;
            break;
          default:
            readingValid = true;
            break;
        }
      }
      else {
        /* All values are withing range */
        readingValid = true;
      }
    }
  }

  /* Compensate values depending on the resolution */
  switch(_range) {
    case GYRO_RANGE_250DPS:
      gyro_x *= GYRO_SENSITIVITY_250DPS;
      gyro_y *= GYRO_SENSITIVITY_250DPS;
      gyro_z *= GYRO_SENSITIVITY_250DPS;
      break;
    case GYRO_RANGE_500DPS:
      gyro_x *= GYRO_SENSITIVITY_500DPS;
      gyro_y *= GYRO_SENSITIVITY_500DPS;
      gyro_z *= GYRO_SENSITIVITY_500DPS;
      break;
    case GYRO_RANGE_2000DPS:
      gyro_x *= GYRO_SENSITIVITY_2000DPS;
      gyro_y *= GYRO_SENSITIVITY_2000DPS;
      gyro_z *= GYRO_SENSITIVITY_2000DPS;
      break;
  }

  /* Convert values to rad/s */
  //gyro_x *= SENSORS_DPS_TO_RADS;
  //gyro_y *= SENSORS_DPS_TO_RADS;
  //gyro_z *= SENSORS_DPS_TO_RADS;
  updateDatum(0, gyro_x);
  updateDatum(1, gyro_y);
  updateDatum(2, gyro_z);
  return SensorWrapper::SENSOR_ERROR_NO_ERROR;
}


/**************************************************************************/
/*!
@brief Gets the sensor_t data
*/
/**************************************************************************/
/*void L3GD20::getSensor(sensor_t* sensor)
{
  // Clear the sensor_t object
  memset(sensor, 0, sizeof(sensor_t));

  // Insert the sensor name in the fixed length char array
  strncpy (sensor->name, "L3GD20", sizeof(sensor->name) - 1);
  sensor->name[sizeof(sensor->name)- 1] = 0;
  sensor->version = 1;
  sensor->type = SENSOR_TYPE_GYROSCOPE;
  sensor->min_delay = 0;
  sensor->max_value = (float)this->_range * SENSORS_DPS_TO_RADS;
  sensor->min_value = (this->_range * -1.0) * SENSORS_DPS_TO_RADS;
  sensor->resolution = 0.0F; // TBD
}
*/
