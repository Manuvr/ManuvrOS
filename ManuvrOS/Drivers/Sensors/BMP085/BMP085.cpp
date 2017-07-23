/*
File:   BMP085.cpp
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

#include "BMP085.h"


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
  }
};


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
BMP085::BMP085(uint8_t addr) : I2CDeviceWithRegisters(addr), SensorWrapper("BMP085") {
  define_datum(&datum_defs[0]);
  define_datum(&datum_defs[1]);
  define_datum(&datum_defs[2]);
}

/*
* Constructor. Takes i2c address and irq pin as arguments.
*/
BMP085::BMP085(uint8_t addr, uint8_t pin) : BMP085(addr) {
  irq_pin = pin;
}


/*
* Destructor.
*/
BMP085::~BMP085() {
}



/*******************************************************************************
*  ,-.                      ,   .
* (   `                     | . |
*  `-.  ,-. ;-. ,-. ,-. ;-. | ) ) ;-. ,-: ;-. ;-. ,-. ;-.
* .   ) |-' | | `-. | | |   |/|/  |   | | | | | | |-' |
*  `-'  `-' ' ' `-' `-' '   ' '   '   `-` |-' |-' `-' '
******************************************|***|********************************/

SensorError BMP085::init() {
  return SensorError::BUS_ERROR;
}


SensorError BMP085::readSensor() {
  // Depending on which stage of the read we are on, jump to the next stage.
  if (!isCalibrated()) {
    calibrate();
  }

  switch(read_step) {
    case -1:
      initiate_UT_read();
      read_step++;
      break;
    case 0:
      // Read two bytes from registers 0xF6 and 0xF7
      uncomp_temp = bmp085ReadInt(0xF6);
      temperature = (((double) getTemperature()) / 10.0);
      updateDatum(1, temperature);
      initiate_UP_read();
      read_step++;
      break;
    case 1:
      read_up();
      pressure = (((double) getPressure()) / 1000);
      read_step++;
      updateDatum(0, pressure);
      break;
    case 2:
      read_step = -1;
      calculateAltitude();
      updateDatum(2, altitude);
      break;
    default:
      // We should never see this block.
      read_step = -1;
      break;
  }
  return SensorError::NO_ERROR;
}


SensorError BMP085::setParameter(uint16_t reg, int len, uint8_t*) {
  return SensorError::INVALID_PARAM_ID;
}


SensorError BMP085::getParameter(uint16_t reg, int len, uint8_t*) {
  return SensorError::INVALID_PARAM_ID;
}


/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/


int8_t BMP085::io_op_callback(I2CBusOp* completed) {
  I2CDeviceWithRegisters::io_op_callback(completed);
  int i = 0;
  DeviceRegister *temp_reg = reg_defs.get(i++);
  while (temp_reg) {
    switch (temp_reg->addr) {
      default:
        temp_reg->unread = false;
        break;
    }
    temp_reg = reg_defs.get(i++);
  }
  return 0;
}


/*
* Dump this item to the dev log.
*/
void BMP085::printDebug(StringBuilder* temp) {
  temp->concatf("Altitude sensor (BMP085)\t%snitialized\n---------------------------------------------------\n", (isActive() ? "I": "Uni"));
  I2CDeviceWithRegisters::printDebug(temp);
  //SensorWrapper::issue_json_map(temp, this);
  temp->concatf("\n");
}



/*******************************************************************************
* Class-specific functions...                                                  *
*******************************************************************************/

/*
* Stores all of the bmp085's calibration values into global variables
* Calibration values are required to calculate temp and pressure
* This function should be called at the beginning of the program
*/
void BMP085::calibrate() {
  if (!isCalibrated()) {
    ac1 = bmp085ReadInt(0xAA);
    ac2 = bmp085ReadInt(0xAC);
    ac3 = bmp085ReadInt(0xAE);
    ac4 = bmp085ReadInt(0xB0);
    ac5 = bmp085ReadInt(0xB2);
    ac6 = bmp085ReadInt(0xB4);
    b1  = bmp085ReadInt(0xB6);
    b2  = bmp085ReadInt(0xB8);
    mb  = bmp085ReadInt(0xBA);
    mc  = bmp085ReadInt(0xBC);
    md  = bmp085ReadInt(0xBE);
    isCalibrated(true);
  }
}

/*
* Calculate temperature given ut.
* Value returned will be in units of 0.1 deg C
*/
long BMP085::getTemperature() {
  long x1, x2;

  x1 = (((long)uncomp_temp - (long)ac6)*(long)ac5) >> 15;
  x2 = ((long)mc << 11)/(x1 + md);
  b5 = x1 + x2;

  return ((b5 + 8)>>4);
}


// Calculate pressure given up
// calibration values must be known
// b5 is also required so getTemperature(...) must be called first.
// Value returned will be pressure in units of Pa.
long BMP085::getPressure() {
  long x1, x2, x3, b3, b6, p;
  unsigned long b4, b7;

  b6 = b5 - 4000;
  // Calculate B3
  x1 = (b2 * (b6 * b6)>>12)>>11;
  x2 = (ac2 * b6)>>11;
  x3 = x1 + x2;
  b3 = (((((long)ac1)*4 + x3)<<OSS) + 2)>>2;

  // Calculate B4
  x1 = (ac3 * b6)>>13;
  x2 = (b1 * ((b6 * b6)>>12))>>16;
  x3 = ((x1 + x2) + 2)>>2;
  b4 = (ac4 * (unsigned long)(x3 + 32768))>>15;

  b7 = ((unsigned long)(uncomp_pres - b3) * (50000>>OSS));
  if (b7 < 0x80000000)
    p = (b7<<1)/b4;
  else
    p = (b7/b4)<<1;

  x1 = (p>>8) * (p>>8);
  x1 = (x1 * 3038)>>16;
  x2 = (-7357 * p)>>16;
  p += (x1 + x2 + 3791)>>4;

  return p;
}


float BMP085::calculateAltitude() {
  altitude = (float) 443.3 * (1 - pow(((float) pressure/PRESSURE_AT_SEA_LEVEL), 0.190295));
  return altitude;
}


// Read 1 byte from the BMP085 at 'address'
char BMP085::bmp085Read(uint8_t address) {
  uint8_t temp_int = read8(address);
  return temp_int;
}


// Read 2 bytes from the BMP085
// First byte will be from 'address'
// Second byte will be from 'address'+1
int BMP085::bmp085ReadInt(uint8_t address) {
  uint16_t return_value = read16(address);
  return (int) return_value;
}


// Read the uncompensated temperature value
void BMP085::initiate_UT_read() {
  // This requests a temperature reading
  write8(0xF4, 0x2E);
}


void BMP085::initiate_UP_read() {
  // Request a pressure reading w/ oversampling setting
  write8(0xF4, (0x34 + (OSS<<6)));
}


// Read the uncompensated pressure value
long BMP085::read_up() {
  // Wait for conversion, delay time dependent on OSS
  //delay(2 + (3<<OSS));

  // Read register 0xF6 (MSB), 0xF7 (LSB), and 0xF8 (XLSB)
  uint8_t *buf = (uint8_t *) alloca(5);
  readX(0xF6, 3, buf);

  //uncomp_pres = (((unsigned long) msb << 16) | ((unsigned long) lsb << 8) | (unsigned long) xlsb) >> (8-OSS);
  uncomp_pres = (((unsigned long) buf[0] << 16) | ((unsigned long) buf[1] << 8) | (unsigned long) buf[2]) >> (8-OSS);

  return uncomp_pres;
}
