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



TMP006::TMP006(uint8_t i2caddr) : I2CDeviceWithRegisters(), SensorWrapper() {
  _dev_addr = i2caddr;
  this->isHardware = true;
  this->defineDatum("TMP006 Object Temperature", SensorWrapper::COMMON_UNITS_C, FLOAT_FM);
  this->defineDatum("TMP006 Die Temperature", SensorWrapper::COMMON_UNITS_C, FLOAT_FM);
  this->s_id = "857fd6d1a5eda4ec0f2eb32aea518f6c";
  this->name = "TMP006 Thermopile";
  this->sensor_active = false;
  
  defineRegister(TMP006_REG_VOBJ,   (uint16_t) 0x0000, false, false, false);
  defineRegister(TMP006_REG_TAMB,   (uint16_t) 0x0000, false, false, false);
  defineRegister(TMP006_REG_CONFIG, (uint16_t) 0x7500, true, false, true);
  defineRegister(TMP006_REG_MANID,  (uint16_t) 0x0000, false, false, false);
  defineRegister(TMP006_REG_DEVID,  (uint16_t) 0x0000, false, false, false);
}


/**************************************************************************
* Overrides...                                                            *
**************************************************************************/

int8_t TMP006::init() {
  if (syncRegisters() == I2C_ERR_CODE_NO_ERROR) {
    return SensorWrapper::SENSOR_ERROR_NO_ERROR;
  }
  else {
    return SensorWrapper::SENSOR_ERROR_BUS_ERROR;
  }
//  if (check_identity() == SENSOR_ERROR_NO_ERROR) {
//    if (write16(TMP006_CONFIG, TMP006_CFG_MODEON | TMP006_CFG_DRDYEN | TMP006_CFG_16SAMPLE)) {
//      return SensorWrapper::SENSOR_ERROR_NO_ERROR;
//    }
//  }
//  else if (read16((uint8_t) TMP006_MANID)) {
//    if (read16((uint8_t) TMP006_DEVID)) {
//      return SensorWrapper::SENSOR_ERROR_NO_ERROR;
//    }
//  }
//  return SensorWrapper::SENSOR_ERROR_BUS_ERROR;
}


int8_t TMP006::readSensor(void) {
  if (I2C_ERR_CODE_NO_ERROR == readRegister((uint8_t) TMP006_REG_TAMB)) {
    if (I2C_ERR_CODE_NO_ERROR == readRegister((uint8_t) TMP006_REG_VOBJ)) {
      return SensorWrapper::SENSOR_ERROR_NO_ERROR;
    }
  }
  return SensorWrapper::SENSOR_ERROR_NO_ERROR;  // TODO: Wrong code.
}


int8_t TMP006::setParameter(uint16_t reg, int len, uint8_t*) {
  return SensorWrapper::SENSOR_ERROR_INVALID_PARAM_ID;
}


int8_t TMP006::getParameter(uint16_t reg, int len, uint8_t*) {
  return SensorWrapper::SENSOR_ERROR_INVALID_PARAM_ID;
}



/****************************************************************************************************
* These are overrides from I2CDeviceWithRegisters.                                                  *
****************************************************************************************************/

void TMP006::operationCompleteCallback(I2CQueuedOperation* completed) {
  I2CDeviceWithRegisters::operationCompleteCallback(completed);
  int i = 0;
  DeviceRegister *temp_reg = reg_defs.get(i++);
  while (temp_reg != NULL) {
    switch (temp_reg->addr) {
      case TMP006_REG_MANID:
      case TMP006_REG_DEVID:
		    if (!sensor_active) {
		      check_identity();
		      if (sensor_active) {
		        writeDirtyRegisters();
		      }
		    }
		    else {
		      check_identity();
		    }
        break;

      case TMP006_REG_VOBJ:
      case TMP006_REG_TAMB:
        if (SENSOR_ERROR_NO_ERROR == check_data()) {
          Kernel::raiseEvent(MANUVR_MSG_SENSOR_TMP006, NULL);
        }
        break;
        
      case TMP006_REG_CONFIG:
        temp_reg->unread = false;
        break;
      default:
        temp_reg->unread = false;
        break;
    }
    temp_reg = reg_defs.get(i++);
  }
}


/*
* Dump this item to the dev log.
*/
void TMP006::printDebug(StringBuilder* temp) {
  temp->concatf("Thermopile sensor (TMP006)\t%snitialized\n---------------------------------------------------\n", (sensor_active ? "I": "Uni"));
  I2CDeviceWithRegisters::printDebug(temp);
  //SensorWrapper::issue_json_map(temp, this);
  temp->concatf("\n");
}



/****************************************************************************************************
* Class-specific functions...                                                                       *
****************************************************************************************************/

int8_t TMP006::check_identity(void) {
  if ((regValue(TMP006_REG_DEVID) == 0x67) && (regValue(TMP006_REG_MANID) == 0x5449)) {
    sensor_active = true;
    markRegRead(TMP006_REG_DEVID);
    markRegRead(TMP006_REG_MANID);
    
    return SENSOR_ERROR_NO_ERROR;
  }
  return SENSOR_ERROR_WRONG_IDENTITY;
}


int8_t TMP006::check_data(void) {
  if (sensor_active) {
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
      return SensorWrapper::SENSOR_ERROR_NO_ERROR;
    }
    return SensorWrapper::SENSOR_ERROR_WRONG_IDENTITY;  // TODO: Not correct code. 
  }
  return SensorWrapper::SENSOR_ERROR_WRONG_IDENTITY;  // TODO: Not correct code.
}

