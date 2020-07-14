/*
File:   ADXL345.cpp
Author: J. Ian Lindsay
Date:   2019.06.01

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

#include "ADXL345.h"


const DatumDef datum_defs[] = {
  {
    .desc    = "X",
    .units   = COMMON_UNITS_ACCEL,
    .type_id = TCode::FLOAT,
    .flgs    = SENSE_DATUM_FLAG_HARDWARE
  },
  {
    .desc    = "Y",
    .units   = COMMON_UNITS_ACCEL,
    .type_id = TCode::FLOAT,
    .flgs    = SENSE_DATUM_FLAG_HARDWARE
  },
  {
    .desc    = "Z",
    .units   = COMMON_UNITS_ACCEL,
    .type_id = TCode::FLOAT,
    .flgs    = SENSE_DATUM_FLAG_HARDWARE
  }
};


static const float scale_multipliers[4] = {
	4.0 / 1024.0,
	8.0 / 1024.0,
	16.0/ 1024.0,
	32.0/ 1024.0
};


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/


ADXL345::ADXL345(const ADXL345Opts* o) : I2CDeviceWithRegisters(o->addr, 27, 30), SensorWrapper("ADXL345"), _opts(o) {
	if (255 != _opts.irq1) {
    pinMode(_opts.irq1, _opts.usePullup1() ? GPIOMode::INPUT_PULLUP : GPIOMode::INPUT);
	}
	if (255 != _opts.irq2) {
    pinMode(_opts.irq2, _opts.usePullup2() ? GPIOMode::INPUT_PULLUP : GPIOMode::INPUT);
	}

  define_datum(&datum_defs[0]);
  define_datum(&datum_defs[1]);
  define_datum(&datum_defs[2]);

  defineRegister(ADXL345_REGISTER_DEVID,          (uint8_t)  0, true, false, true);
  defineRegister(ADXL345_REGISTER_THRESH_TAP,     (uint8_t)  0, true, false, true);
  defineRegister(ADXL345_REGISTER_OFSX,           (uint8_t)  0, true, false, true);
  defineRegister(ADXL345_REGISTER_OFSY,           (uint8_t)  0, true, false, true);
  defineRegister(ADXL345_REGISTER_OFSZ,           (uint8_t)  0, true, false, true);
  defineRegister(ADXL345_REGISTER_DUR,            (uint8_t)  0, true, false, true);
  defineRegister(ADXL345_REGISTER_LATENT,         (uint8_t)  0, true, false, true);
  defineRegister(ADXL345_REGISTER_WINDOW,         (uint8_t)  0, true, false, true);
  defineRegister(ADXL345_REGISTER_THRESH_ACT,     (uint8_t)  0, true, false, true);
  defineRegister(ADXL345_REGISTER_THRESH_INACT,   (uint8_t)  0, true, false, true);
  defineRegister(ADXL345_REGISTER_TIME_INACT,     (uint8_t)  0, true, false, true);
  defineRegister(ADXL345_REGISTER_ACT_INACT_CTL,  (uint8_t)  0, true, false, true);
  defineRegister(ADXL345_REGISTER_THRESH_FF,      (uint8_t)  0, true, false, true);
  defineRegister(ADXL345_REGISTER_TIME_FF,        (uint8_t)  0, true, false, true);
  defineRegister(ADXL345_REGISTER_TAP_AXES,       (uint8_t)  0, true, false, true);
  defineRegister(ADXL345_REGISTER_ACT_TAP_STATUS, (uint8_t)  0, true, false, true);
  defineRegister(ADXL345_REGISTER_BW_RATE,        (uint8_t)  0, true, false, true);
  defineRegister(ADXL345_REGISTER_POWER_CTL,      (uint8_t)  0, true, false, true);
  defineRegister(ADXL345_REGISTER_INT_ENABLE,     (uint8_t)  0, true, false, true);
  defineRegister(ADXL345_REGISTER_INT_MAP,        (uint8_t)  0, true, false, true);
  defineRegister(ADXL345_REGISTER_INT_SOURCE,     (uint8_t)  0, true, false, true);
  defineRegister(ADXL345_REGISTER_DATA_FORMAT,    (uint8_t)  0, true, false, true);
  defineRegister(ADXL345_REGISTER_DATAX,          (uint16_t) 0, true, false, true);
  defineRegister(ADXL345_REGISTER_DATAY,          (uint16_t) 0, true, false, true);
  defineRegister(ADXL345_REGISTER_DATAZ,          (uint16_t) 0, true, false, true);
  defineRegister(ADXL345_REGISTER_FIFO_CTRL,      (uint8_t)  0, true, false, true);
  defineRegister(ADXL345_REGISTER_FIFO_STATUS,    (uint8_t)  0, true, false, true);
}


/*
* Destructor.
*/
ADXL345::~ADXL345() {
}



/*******************************************************************************
*  ,-.                      ,   .
* (   `                     | . |
*  `-.  ,-. ;-. ,-. ,-. ;-. | ) ) ;-. ,-: ;-. ;-. ,-. ;-.
* .   ) |-' | | `-. | | |   |/|/  |   | | | | | | |-' |
*  `-'  `-' ' ' `-' `-' '   ' '   '   `-` |-' |-' `-' '
******************************************|***|********************************/

/**
* Needs to be called once on reset. This will set our temperature thresholds for
*  setting off the fan, alerts, etc...
*/
SensorError ADXL345::init() {
	writeIndirect(ADXL345_REGISTER_POWER_CTL,   0x29, true);
	writeIndirect(ADXL345_REGISTER_DATA_FORMAT, 0x28, true);
	writeIndirect(ADXL345_REGISTER_FIFO_CTRL,   0x00);   // No FIFO use

  if (syncRegisters() == I2C_ERR_SLAVE_NO_ERROR) {
    return SensorError::NO_ERROR;
  }
  return SensorError::BUS_ERROR;
}

/**
*  Call to query the TMP102 and return the reading in degrees Celcius.
*  Will return -1 on failure.
*/
SensorError ADXL345::readSensor() {
  if (I2C_ERR_SLAVE_NO_ERROR == readRegister((uint8_t) ADXL345_REGISTER_DATAX)) {
    if (I2C_ERR_SLAVE_NO_ERROR == readRegister((uint8_t) ADXL345_REGISTER_DATAY)) {
      if (I2C_ERR_SLAVE_NO_ERROR == readRegister((uint8_t) ADXL345_REGISTER_DATAZ)) {
        return SensorError::NO_ERROR;
      }
    }
  }
  return SensorError::BUS_ERROR;
}


SensorError ADXL345::setParameter(uint16_t reg, int len, uint8_t*) {
  return SensorError::INVALID_PARAM_ID;
}


SensorError ADXL345::getParameter(uint16_t reg, int len, uint8_t*) {
  return SensorError::INVALID_PARAM_ID;
}



/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

int8_t ADXL345::register_write_cb(DeviceRegister* reg) {
  switch (reg->addr) {
    case ADXL345_REGISTER_THRESH_TAP:
      break;
    case ADXL345_REGISTER_OFSX:
      break;
    case ADXL345_REGISTER_OFSY:
      break;
    case ADXL345_REGISTER_OFSZ:
      break;
    case ADXL345_REGISTER_DUR:
      break;
    case ADXL345_REGISTER_LATENT:
      break;
    case ADXL345_REGISTER_WINDOW:
      break;
    case ADXL345_REGISTER_THRESH_ACT:
      break;
    case ADXL345_REGISTER_THRESH_INACT:
      break;
    case ADXL345_REGISTER_TIME_INACT:
      break;
    case ADXL345_REGISTER_ACT_INACT_CTL:
      break;
    case ADXL345_REGISTER_THRESH_FF:
      break;
    case ADXL345_REGISTER_TIME_FF:
      break;
    case ADXL345_REGISTER_TAP_AXES:
      break;
    case ADXL345_REGISTER_ACT_TAP_STATUS:
      break;
    case ADXL345_REGISTER_BW_RATE:
      break;
    case ADXL345_REGISTER_POWER_CTL:
      break;
    case ADXL345_REGISTER_INT_ENABLE:
      break;
    case ADXL345_REGISTER_INT_MAP:
      break;
    case ADXL345_REGISTER_INT_SOURCE:
      break;
    case ADXL345_REGISTER_DATA_FORMAT:
      break;
    case ADXL345_REGISTER_FIFO_CTRL:
      break;
    case ADXL345_REGISTER_FIFO_STATUS:
      break;

    case ADXL345_REGISTER_DEVID:
    case ADXL345_REGISTER_DATAX:
    case ADXL345_REGISTER_DATAY:
    case ADXL345_REGISTER_DATAZ:
    default:
      // Illegal write target.
      break;
  }
  return 0;
}


int8_t ADXL345::register_read_cb(DeviceRegister* reg) {
  switch (reg->addr) {
    case ADXL345_REGISTER_THRESH_TAP:
      break;
    case ADXL345_REGISTER_OFSX:
      break;
    case ADXL345_REGISTER_OFSY:
      break;
    case ADXL345_REGISTER_OFSZ:
      break;
    case ADXL345_REGISTER_DUR:
      break;
    case ADXL345_REGISTER_LATENT:
      break;
    case ADXL345_REGISTER_WINDOW:
      break;
    case ADXL345_REGISTER_THRESH_ACT:
      break;
    case ADXL345_REGISTER_THRESH_INACT:
      break;
    case ADXL345_REGISTER_TIME_INACT:
      break;
    case ADXL345_REGISTER_ACT_INACT_CTL:
      break;
    case ADXL345_REGISTER_THRESH_FF:
      break;
    case ADXL345_REGISTER_TIME_FF:
      break;
    case ADXL345_REGISTER_TAP_AXES:
      break;
    case ADXL345_REGISTER_ACT_TAP_STATUS:
      break;
    case ADXL345_REGISTER_BW_RATE:
      break;
    case ADXL345_REGISTER_POWER_CTL:
      break;
    case ADXL345_REGISTER_INT_ENABLE:
      break;
    case ADXL345_REGISTER_INT_MAP:
      break;
    case ADXL345_REGISTER_INT_SOURCE:
      break;
    case ADXL345_REGISTER_DATA_FORMAT:
      break;
    case ADXL345_REGISTER_FIFO_CTRL:
      break;
    case ADXL345_REGISTER_FIFO_STATUS:
      break;

    case ADXL345_REGISTER_DEVID:
      if (0xE5 == reg->getVal()) {
      }
      else {
        // Chip not present.
      }
      break;
    case ADXL345_REGISTER_DATAX:
    case ADXL345_REGISTER_DATAY:
    case ADXL345_REGISTER_DATAZ:
      {
        float mult = scale_multipliers[_get_scale_bits()];
        SensorError err = updateDatum((ADXL345_REGISTER_DATAZ - reg->addr) >> 1, mult * reg->getVal());
        if (SensorError::NO_ERROR != err) {
          Kernel::log("Failed to update datum.");
        }
      }
      break;
    default:
      break;
  }
  reg->unread = false;
  return 0;
}


/*
* Dump this item to the dev log.
*/
void ADXL345::printDebug(StringBuilder* temp) {
  SensorWrapper::printSensorSummary(temp);
  I2CDeviceWithRegisters::printDebug(temp);
  temp->concatf("\n");
}


/*******************************************************************************
* Class-specific functions...                                                  *
*******************************************************************************/
