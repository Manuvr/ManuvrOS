/*
File:   INA219.cpp
Author: J. Ian Lindsay
Date:   2014.05.27

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

#include "INA219.h"

const V_Cap_Point* batt_capacity_curves[6] = {chem_index_0, chem_index_1, chem_index_2, chem_index_3, chem_index_4, chem_index_5};  // Ten points on the curve ought to be enough for reliable interpolation.

const DatumDef datum_defs[] = {
  {
    .desc    = "Instantaneous Current",
    .units   = COMMON_UNITS_AMPS,
    .type_id = FLOAT_FM,
    .flgs    = SENSE_DATUM_FLAG_HARDWARE
  },
  {
    .desc    = "Instantaneous Voltage",
    .units   = COMMON_UNITS_VOLTS,
    .type_id = FLOAT_FM,
    .flgs    = SENSE_DATUM_FLAG_HARDWARE
  },
  {
    .desc    = "Instantaneous Power",
    .units   = COMMON_UNITS_WATTS,
    .type_id = FLOAT_FM,
    .flgs    = 0x00
  }
};


/*
* Constructor. Takes i2c address as argument.
*/
INA219::INA219(uint8_t addr) : I2CDeviceWithRegisters(), SensorWrapper() {
  _dev_addr = addr;
  defineDatum(&datum_defs[0], SensorReporting::OFF);
  defineDatum(&datum_defs[1], SensorReporting::OFF);
  defineDatum(&datum_defs[2], SensorReporting::OFF);
  s_id = "e1671797c52e15f763380b45e841ec32";
  name = "INA219";
  //batt_min_v         = 0;  // We will be unable to init() with these values.
  //batt_max_v         = 0;  // We will be unable to init() with these values.
  //batt_capacity      = 0;  // We will be unable to init() with these values.
  //shunt_value        = 0;  // We will be unable to init() with these values.
  //max_voltage_delta  = 0;  // We will be unable to init() with these values.

  batt_min_v         = 3.7f;  // We will be unable to init() with these values.
  batt_max_v         = 4.3f;  // We will be unable to init() with these values.
  batt_capacity      = 2000.0f;  // We will be unable to init() with these values.
  shunt_value        = 0.01f;  // We will be unable to init() with these values.
  max_voltage_delta  = batt_max_v;  // We will be unable to init() with these values.
  batt_chemistry     = INA219_BATTERY_CHEM_UNDEF;

  init_complete = false;

  defineRegister(INA219_REG_CONFIGURATION, (uint16_t) 0, false, false, true);
  defineRegister(INA219_REG_SHUNT_VOLTAGE, (uint16_t) 0, false, false, false);
  defineRegister(INA219_REG_BUS_VOLTAGE,   (uint16_t) 0, false, false, false);
  defineRegister(INA219_REG_POWER,         (uint16_t) 0, false, false, false);
  defineRegister(INA219_REG_CURRENT,       (uint16_t) 0, false, false, false);
  defineRegister(INA219_REG_CALIBRATION,   (uint16_t) 0, false, false, true);
}


/*
* Destructor.
*/
INA219::~INA219(void) {
}


/**************************************************************************
* Overrides...                                                            *
**************************************************************************/
SensorError INA219::init() {
  if (batt_max_v > 0) {
    max_voltage_delta = (batt_max_v >= 16) ? 32 : 16;
  }
  else {
    max_voltage_delta = 0;
  }

  if ((batt_min_v > 0) && (batt_max_v > 0) && (batt_capacity > 0) && (shunt_value > 0) && (max_voltage_delta > 0)) {
      // If we have all this, we should be safe to init(). If the user lied to us, we will not go to space today.
  }

  //batt_min_v;
  //batt_max_v;
  //batt_capacity;
  //shunt_value;
  uint16_t cal_value = 0;
  uint16_t cfg_value = INA219_CONFIG_BVOLTAGERANGE_16V |
                    INA219_CONFIG_GAIN_4_160MV |
                    INA219_CONFIG_BADCRES_12BIT |
                    INA219_CONFIG_SADCRES_12BIT_1S_532US |
                    INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;
  cal_value = 6;

  writeIndirect(INA219_REG_CALIBRATION, cal_value, true);
  writeIndirect(INA219_REG_CONFIGURATION, cfg_value);
  //if (syncRegisters() == I2C_ERR_CODE_NO_ERROR) {
    isActive(true);
    return SensorError::NO_ERROR;
  //}
  //else {
  //  return SensorError::BUS_ERROR;
  //}
}


SensorError INA219::setParameter(uint16_t reg, int len, uint8_t *data) {
  switch (reg) {
    case INA219_REG_CLASS_MODE:
      { uint8_t nu = *(data);
        if (nu & INA219_MODE_BATTERY) {
        }
        if (nu & INA219_MODE_INTEGRATION) {
          // Add datum for integrated current flow.
        }
      }
      break;
    case INA219_REG_BATTERY_SIZE:
      break;
    case INA219_REG_BATTERY_MIN_V:

      break;
    case INA219_REG_BATTERY_MAX_V:
      break;
    case INA219_REG_BATTERY_CHEM:
      {
          uint8_t nu = *(data);
          if (nu <= 5) {
              batt_chemistry = nu;
              return SensorError::NO_ERROR;
          }
          else {
              return SensorError::INVALID_PARAM;
          }
      }
      break;
    default:
      break;
  }
  return SensorError::INVALID_PARAM_ID;
}


SensorError INA219::getParameter(uint16_t reg, int len, uint8_t*) {
  return SensorError::INVALID_PARAM_ID;
}


SensorError INA219::readSensor() {
  if (isActive() && init_complete) {
    if (I2C_ERR_CODE_NO_ERROR == readRegister((uint8_t) INA219_REG_SHUNT_VOLTAGE)) {
      if (I2C_ERR_CODE_NO_ERROR == readRegister((uint8_t) INA219_REG_BUS_VOLTAGE)) {
        if (I2C_ERR_CODE_NO_ERROR == readRegister((uint8_t) INA219_REG_POWER)) {
          if (I2C_ERR_CODE_NO_ERROR == readRegister((uint8_t) INA219_REG_CURRENT)) {
            return SensorError::NO_ERROR;
          }
        }
      }
    }
  }
  return SensorError::BUS_ERROR;
}


/****************************************************************************************************
* These are overrides from I2CDeviceWithRegisters.                                                  *
****************************************************************************************************/

void INA219::operationCompleteCallback(I2CBusOp* completed) {
  I2CDeviceWithRegisters::operationCompleteCallback(completed);
  int i = 0;
  DeviceRegister *temp_reg = reg_defs.get(i++);
  while (temp_reg != NULL) {
    switch (temp_reg->addr) {
      case INA219_REG_SHUNT_VOLTAGE:
      case INA219_REG_BUS_VOLTAGE:
      case INA219_REG_CURRENT:
      case INA219_REG_POWER:
        if (process_read_data()) {
          //Kernel::raiseEvent(MANUVR_MSG_SENSOR_INA219, NULL);   // Raise an event
        }
        break;
      case INA219_REG_CONFIGURATION:
        temp_reg->unread = false;
        break;
      case INA219_REG_CALIBRATION:
        temp_reg->unread = false;
        if (!init_complete) {
          syncRegisters();
          init_complete = true;
        }
        break;
      default:
        temp_reg->unread = false;
        break;
    }
    temp_reg = reg_defs.get(i++);
  }
}


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void INA219::printDebug(StringBuilder* temp) {
  temp->concatf("Current sensor (INA219)\t%snitialized\n---------------------------------------------------\n", (isActive() ? "I": "Uni"));
  I2CDeviceWithRegisters::printDebug(temp);
  //SensorWrapper::issue_json_map(temp, this);
  //temp->concatf("\n");
}



/**************************************************************************
* Class-specific functions...                                             *
**************************************************************************/

/*
* Returns true if it did its job. Returns false if we are still missing some data.
*/
bool INA219::process_read_data() {
  if (regUpdated(INA219_REG_POWER) && regUpdated(INA219_REG_CURRENT) && regUpdated(INA219_REG_BUS_VOLTAGE) && regUpdated(INA219_REG_SHUNT_VOLTAGE)) {
    float local_shunt   = (float) ((((int16_t) regValue(INA219_REG_SHUNT_VOLTAGE)) >> 3) * 0.004f);   // So many mV.
    float local_bus     = (float) ((((int16_t) regValue(INA219_REG_BUS_VOLTAGE))   >> 3) * 0.004f);   // So many mV.
    float local_current = (float) (((int16_t) regValue(INA219_REG_CURRENT)) / 10.0f);     // Much electrons.
    float local_power   = (float) (((int16_t) regValue(INA219_REG_POWER)) / 2.0f);       // Such joules.


    updateDatum(0, local_current);
    updateDatum(1, local_bus);
    updateDatum(2, local_power);

    markRegRead(INA219_REG_POWER);
    markRegRead(INA219_REG_CURRENT);
    markRegRead(INA219_REG_BUS_VOLTAGE);
    markRegRead(INA219_REG_SHUNT_VOLTAGE);

    #ifdef __MANUVR_DEBUG
      StringBuilder output;
      output.concatf("%.3f\t %.3f\t %.3f\t %.3f\n", (double) local_shunt, (double) local_bus, (double) local_current, (double) local_power);
      Kernel::log(&output);
    #endif
    return true;
  }
  return false;
}


/*
* Adapted from K.Townsend's Adafruit driver...
* https://github.com/adafruit/Adafruit_INA219/blob/master/
*/
//float INA219::readCurrent(void) {
//  //write16(INA219_REG_CALIBRATION, ina219_calValue);
//  float valueDec = (float) read16(INA219_REG_CURRENT);
//  //valueDec /= ina219_currentDivider_mA;
//  updateDatum(0, valueDec);
//  return valueDec;
//}


/*
* Adapted from K.Townsend's Adafruit driver...
* https://github.com/adafruit/Adafruit_INA219/blob/master/
*/
//float INA219::readBusVoltage(void) {
//  if (all_registers_read()) {
//  }
//  int16_t value = (int16_t) reg_bus_voltage;
//  value = (int16_t)((value >> 3) * 4);  // Shift to the right 3 to drop CNVR and OVF and multiply by LSB
//  updateDatum(1, value);
//  return value * 0.001;
//}


///*
//* Adapted from K.Townsend's Adafruit driver...
//* https://github.com/adafruit/Adafruit_INA219/blob/master/
//*/
//float INA219::readShuntVoltage(void) {
//  int16_t valueDec = (int16_t) reg_shunt_voltage;
//  return valueDec * 0.01;
//}


/*
* Pass true to set the battery mode on. False to turn it off.
* The only benefit to leaving battery mode off is CPU.
*/
SensorError INA219::setBatteryMode(bool nu_mode) {
  if (batt_min_v != 0) {
    if (batt_max_v != 0) {
      if (batt_capacity != 0) {
        if (battery_monitor) {
        }
        else {
          battery_monitor = true;
        }
        return SensorError::NO_ERROR;
      }
    }
  }
  return SensorError::MISSING_CONF;
}
