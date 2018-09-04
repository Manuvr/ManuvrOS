/*
File:   TMP006.cpp
Author: J. Ian Lindsay
Date:   2014.01.20


This class is an addaption of the AdaFruit driver for the same chip.
I have adapted it for ManuvrOS.

*/

/***************************************************
  This is a library for the TMP006 Temp Sensor

  Designed specifically to work with the Adafruit TMP006 Breakout
  ----> https://www.adafruit.com/products/1296

  These displays use I2C to communicate, 2 pins are required to
  interface
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/

#include "TMP006.h"

const DatumDef datum_defs[] = {
  {
    .desc    = "Object Temperature",
    .units   = COMMON_UNITS_C,
    .type_id = TCode::FLOAT,
    .flgs    = SENSE_DATUM_FLAG_HARDWARE
  },
  {
    .desc    = "Die Temperature",
    .units   = COMMON_UNITS_C,
    .type_id = TCode::FLOAT,
    .flgs    = SENSE_DATUM_FLAG_HARDWARE
  }
};


TMP006::TMP006(uint8_t addr) : I2CDeviceWithRegisters(addr, 5, 10), SensorWrapper("TMP006") {
  define_datum(&datum_defs[0]);
  define_datum(&datum_defs[1]);

  defineRegister(TMP006_REG_VOBJ,   (uint16_t) 0x0000, false, false, false);
  defineRegister(TMP006_REG_TAMB,   (uint16_t) 0x0000, false, false, false);
  defineRegister(TMP006_REG_CONFIG, (uint16_t) 0x7500, true, false, true);
  defineRegister(TMP006_REG_MANID,  (uint16_t) 0x0000, false, false, false);
  defineRegister(TMP006_REG_DEVID,  (uint16_t) 0x0000, false, false, false);
}


TMP006::TMP006(uint8_t addr, uint8_t pin) : TMP006(addr) {
  irq_pin = pin;
}


TMP006::~TMP006() {
}


/**************************************************************************
* Overrides...                                                            *
**************************************************************************/

SensorError TMP006::init() {
  if (syncRegisters() == I2C_ERR_SLAVE_NO_ERROR) {
    return SensorError::NO_ERROR;
  }
  return SensorError::BUS_ERROR;
}


SensorError TMP006::readSensor() {
  if (I2C_ERR_SLAVE_NO_ERROR == readRegister((uint8_t) TMP006_REG_TAMB)) {
    if (I2C_ERR_SLAVE_NO_ERROR == readRegister((uint8_t) TMP006_REG_VOBJ)) {
      return SensorError::NO_ERROR;
    }
  }
  return SensorError::BUS_ERROR;
}


SensorError TMP006::setParameter(uint16_t reg, int len, uint8_t*) {
  return SensorError::INVALID_PARAM_ID;
}


SensorError TMP006::getParameter(uint16_t reg, int len, uint8_t*) {
  return SensorError::INVALID_PARAM_ID;
}



/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

int8_t TMP006::register_write_cb(DeviceRegister* reg) {
  switch (reg->addr) {
    case TMP006_REG_CONFIG:
      break;

    case TMP006_REG_MANID:
    case TMP006_REG_DEVID:
    case TMP006_REG_VOBJ:
    case TMP006_REG_TAMB:
    default:
      // Illegal write target.
      break;
  }
  return 0;
}


int8_t TMP006::register_read_cb(DeviceRegister* reg) {
  switch (reg->addr) {
    case TMP006_REG_MANID:
    case TMP006_REG_DEVID:
	    if (!isActive()) {
	      check_identity();
	      if (isActive()) {
	        writeDirtyRegisters();
	      }
	    }
	    else {
	      check_identity();
	    }
      break;

    case TMP006_REG_VOBJ:
    case TMP006_REG_TAMB:
      if (SensorError::NO_ERROR == check_data()) {
        Kernel::raiseEvent(MANUVR_MSG_SENSOR_TMP006, nullptr);
      }
      break;

    case TMP006_REG_CONFIG:
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
void TMP006::printDebug(StringBuilder* temp) {
  temp->concatf("Thermopile sensor (TMP006)\t%snitialized%s", (isActive() ? "I": "Uni"), PRINT_DIVIDER_1_STR);
  I2CDeviceWithRegisters::printDebug(temp);
  //SensorWrapper::issue_json_map(temp, this);
  temp->concatf("\n");
}



/****************************************************************************************************
* Class-specific functions...                                                                       *
****************************************************************************************************/

SensorError TMP006::check_identity() {
  if ((regValue(TMP006_REG_DEVID) == 0x67) && (regValue(TMP006_REG_MANID) == 0x5449)) {
    isActive(true);
    markRegRead(TMP006_REG_DEVID);
    markRegRead(TMP006_REG_MANID);

    return SensorError::NO_ERROR;
  }
  return SensorError::WRONG_IDENTITY;
}


SensorError TMP006::check_data() {
  if (isActive()) {
    if (regUpdated(TMP006_REG_TAMB) && regUpdated(TMP006_REG_VOBJ)) {
      float raw_die_temp = (float) (regValue(TMP006_REG_TAMB) >> 2);
      raw_die_temp *= 0.03125; // Convert to celsius
      raw_die_temp += 273.15;  // Rebase to kelvin

      float raw_voltage = (float) (regValue(TMP006_REG_VOBJ) * 156.25) / 1000;  // 156.25 nV per LSB
      raw_voltage /= 1000; // uV -> mV
      raw_voltage /= 1000; // mV -> V

      float tdie_tref = raw_die_temp - TMP006_TREF;

      float S = (1 + (TMP006_A1 * tdie_tref) + (TMP006_A2 * tdie_tref * tdie_tref));
      S *= TMP006_S0;
      S /= 10000000;
      S /= 10000000;

      float Vos = TMP006_B0 + (TMP006_B1 * tdie_tref) + (TMP006_B2 * tdie_tref * tdie_tref);
      float fVobj = (raw_voltage - Vos) + TMP006_C2*(raw_voltage-Vos)*(raw_voltage-Vos);
      float temperature = sqrt(sqrt(raw_die_temp * raw_die_temp * raw_die_temp * raw_die_temp + fVobj/S));

      temperature -= 273.15; // Kelvin -> *C

      updateDatum(0, temperature);
      updateDatum(1, raw_die_temp);

      markRegRead(TMP006_REG_TAMB);
      markRegRead(TMP006_REG_VOBJ);
      return SensorError::NO_ERROR;
    }
    return SensorError::WRONG_IDENTITY;  // TODO: Not correct code.
  }
  return SensorError::WRONG_IDENTITY;  // TODO: Not correct code.
}
