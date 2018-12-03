/*
File:   TMP102.cpp
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

#include "TMP102.h"


const DatumDef datum_defs[] = {
  {
    .desc    = "Temperature",
    .units   = COMMON_UNITS_C,
    .type_id = TCode::FLOAT,
    .flgs    = SENSE_DATUM_FLAG_HARDWARE
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

TMP102::TMP102() : TMP102(TMP102_ADDRESS) {
}


TMP102::TMP102(uint8_t addr) : I2CDeviceWithRegisters(addr, 4, 8), SensorWrapper("TMP102") {
  define_datum(&datum_defs[0]);

  // Set the config register.
  defineRegister(TMP102_REG_RESULT,    (uint16_t) 0,      false, false, false);
  defineRegister(TMP102_REG_CONFIG,    (uint16_t) 0x60B0, true,  false, true);
  defineRegister(TMP102_REG_ALRT_LO,   (uint16_t) 720,    true,  false, true);
  defineRegister(TMP102_REG_ALRT_HI,   (uint16_t) 880,    true,  false, true);
}


TMP102::TMP102(uint8_t addr, uint8_t pin) : TMP102(addr) {
  irq_pin = pin;
}


/*
* Destructor.
*/
TMP102::~TMP102() {
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
SensorError TMP102::init() {
  if (syncRegisters() == I2C_ERR_SLAVE_NO_ERROR) {
    return SensorError::NO_ERROR;
  }
  return SensorError::BUS_ERROR;
}

/**
*  Call to query the TMP102 and return the reading in degrees Celcius.
*  Will return -1 on failure.
*/
SensorError TMP102::readSensor() {
  if (I2C_ERR_SLAVE_NO_ERROR == readRegister((uint8_t) TMP102_REG_RESULT)) {
  }
  return SensorError::BUS_ERROR;
}


SensorError TMP102::setParameter(uint16_t reg, int len, uint8_t*) {
  return SensorError::INVALID_PARAM_ID;
}


SensorError TMP102::getParameter(uint16_t reg, int len, uint8_t*) {
  return SensorError::INVALID_PARAM_ID;
}



/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

int8_t TMP102::register_write_cb(DeviceRegister* reg) {
  switch (reg->addr) {
    case TMP102_REG_CONFIG:
      break;

    case TMP102_REG_ALRT_LO:
      break;

    case TMP102_REG_ALRT_HI:
      break;

    case TMP102_REG_RESULT:
    default:
      // Illegal write target.
      break;
  }
  return 0;
}


int8_t TMP102::register_read_cb(DeviceRegister* reg) {
  switch (reg->addr) {
    case TMP102_REG_RESULT:
      {
        uint16_t temperature_read  = reg->getVal();
        uint8_t msb = temperature_read >> 8;
        uint8_t lsb = temperature_read & 0xFF;
        temperature_read = ((msb * (0x01 << (4+(lsb & 0x01)))) + (lsb >> (4-(lsb & 0x01))));  // Handles EXTended mode.
        float celcius  = ((int16_t) temperature_read) * 0.0625;  // The scale of the TMP102.
        SensorError err = updateDatum(0, celcius);
        if (SensorError::NO_ERROR != err) {
          Kernel::log("Failed to update datum.");
        }
      }
      break;

    case TMP102_REG_CONFIG:
      break;

    case TMP102_REG_ALRT_LO:
      break;

    case TMP102_REG_ALRT_HI:
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
void TMP102::printDebug(StringBuilder* temp) {
  temp->concatf("Temperature sensor (TMP102)\t%snitialized%s", (isActive() ? "I": "Uni"), PRINT_DIVIDER_1_STR);
  I2CDeviceWithRegisters::printDebug(temp);
  temp->concatf("\n");
}


/*******************************************************************************
* Class-specific functions...                                                  *
*******************************************************************************/
/*
* Set the high-end of the alert range in degrees C.
*/
SensorError TMP102::setHighTemperatureAlert(float hi) {
  if (0 == writeIndirect(TMP102_REG_ALRT_HI, convertTemperatureToShort(hi))) {
    return SensorError::NO_ERROR;
  }
  return SensorError::BUS_ERROR;
};

/*
* Set the low-end of the alert range in degrees C.
*/
SensorError TMP102::setLowTemperatureAlert(float lo) {
  if (0 == writeIndirect(TMP102_REG_ALRT_LO, convertTemperatureToShort(lo))) {
    return SensorError::NO_ERROR;
  }
  return SensorError::BUS_ERROR;
};

/**
*  Call this function to set the temperature alert range and
*  re-initialize the TMP102.
*/
SensorError TMP102::setTempRange(float lo, float hi) {
  if (0 == writeIndirect(TMP102_REG_ALRT_LO, convertTemperatureToShort(lo), true)) {
    if (0 == writeIndirect(TMP102_REG_ALRT_HI, convertTemperatureToShort(hi))) {
      return SensorError::NO_ERROR;
    }
  }
  return SensorError::BUS_ERROR;
}
