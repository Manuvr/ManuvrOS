/*
File:   LPS331.cpp
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

#include "LPS331.h"


const DatumDef datum_defs[] = {
  {
    .desc    = "Barometric pressure",
    .units   = COMMON_UNITS_PRESSURE,
    .type_id = FLOAT_FM,
    .flgs    = SENSE_DATUM_FLAG_HARDWARE
  },
  {
    .desc    = "Air temperature",
    .units   = COMMON_UNITS_C,
    .type_id = FLOAT_FM,
    .flgs    = SENSE_DATUM_FLAG_HARDWARE
  },
  {
    .desc    = "Inferred altitude",
    .units   = COMMON_UNITS_METERS,
    .type_id = FLOAT_FM,
    .flgs    = 0x00
  }
};


/*
* Constructor. Takes i2c address as argument.
*/
LPS331::LPS331(uint8_t addr) : I2CDeviceWithRegisters(addr), SensorWrapper("LPS331") {
  define_datum(&datum_defs[0]);
  define_datum(&datum_defs[1]);
  define_datum(&datum_defs[2]);
  gpioSetup();

  // Now we should give them initial definitions. This is our chance to set default configs.
  defineRegister(LPS331_REG_REF_P_XL,     (uint8_t) 0b00000000, false, false, true);
  defineRegister(LPS331_REG_REF_P_L,      (uint8_t) 0b00000000, false, false, true);
  defineRegister(LPS331_REG_REF_P_H,      (uint8_t) 0b00000000, false, false, true);
  defineRegister(LPS331_REG_WHO_AM_I,     (uint8_t) 0b00000000, false, false, false);   // Identity register. Should be 0xbb.
  defineRegister(LPS331_REG_RES_CONF,     (uint8_t) 0x79,       true,  false, true);
  defineRegister(LPS331_REG_CTRL_1,       (uint8_t) 0b10011100, true,  false, true);
  defineRegister(LPS331_REG_CTRL_2,       (uint8_t) 0b00000000, false, false, true);
  defineRegister(LPS331_REG_CTRL_3,       (uint8_t) 0b00000100, true,  false, true);
  defineRegister(LPS331_REG_INT_CONFIG,   (uint8_t) 0b00000000, false, false, true);
  defineRegister(LPS331_REG_INT_SOURCE,   (uint8_t) 0b00000000, false, false, false);
  defineRegister(LPS331_REG_THS_P_LO,     (uint8_t) 0b00000000, false, false, true);
  defineRegister(LPS331_REG_THS_P_HI,     (uint8_t) 0b00000000, false, false, true);
  defineRegister(LPS331_REG_STATUS,       (uint8_t) 0b00000000, false, false, false);
  defineRegister(LPS331_REG_PRS_P_OUT_XL, (uint8_t) 0b00000000, false, false, false);
  defineRegister(LPS331_REG_PRS_OUT_LO,   (uint8_t) 0b00000000, false, false, false);
  defineRegister(LPS331_REG_PRS_OUT_HI,   (uint8_t) 0b00000000, false, false, false);
  defineRegister(LPS331_REG_TEMP_OUT_LO,  (uint8_t) 0b00000000, false, false, false);
  defineRegister(LPS331_REG_TEMP_OUT_HI,  (uint8_t) 0b00000000, false, false, false);
  defineRegister(LPS331_REG_AMP_CTRL,     (uint8_t) 0b00000000, false, false, true);
}

/*
* Destructor.
*/
LPS331::~LPS331() {
}


/*
* Setup GPIO pins and their bindings to on-chip peripherals, if required.
*/
void LPS331::gpioSetup() {
// TODO: This class was never fully ported back from Digitabulum. No GPIO change-over...
//	GPIO_InitTypeDef GPIO_InitStruct;
//
//    /* These Port E pins are inputs:
//    *
//    * #   Purpose
//    * --------------------------------------
//    * 12  Baro1_IRQ (Interrupt)
//    * 13  Baro2_IRQ (Interrupt)
//    */
//    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_12 | GPIO_Pin_13;
//    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IN;
//    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
//    GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_NOPULL;
//    GPIO_Init(GPIOE, &GPIO_InitStruct);
}


/**************************************************************************
* Overrides...                                                            *
**************************************************************************/

SensorError LPS331::init() {
  if (readRegister(LPS331_REG_WHO_AM_I) == I2C_ERR_CODE_NO_ERROR) {
    return SensorError::NO_ERROR;
  }
  else {
    return SensorError::BUS_ERROR;
  }
}


SensorError LPS331::setParameter(uint16_t reg, int len, uint8_t *data) {
  return SensorError::INVALID_PARAM_ID;
}


SensorError LPS331::getParameter(uint16_t reg, int len, uint8_t*) {
  return SensorError::INVALID_PARAM_ID;
}


SensorError LPS331::readSensor() {
  if (readRegister(LPS331_REG_PRS_OUT_HI) == I2C_ERR_CODE_NO_ERROR) {
    if (readRegister(LPS331_REG_PRS_OUT_LO) == I2C_ERR_CODE_NO_ERROR) {
      if (readRegister(LPS331_REG_PRS_P_OUT_XL) == I2C_ERR_CODE_NO_ERROR) {
        if (readRegister(LPS331_REG_TEMP_OUT_LO) == I2C_ERR_CODE_NO_ERROR) {
          if (readRegister(LPS331_REG_TEMP_OUT_HI) == I2C_ERR_CODE_NO_ERROR) {
            return SensorError::NO_ERROR;
          }
        }
      }
    }
  }
  return SensorError::BUS_ERROR;
}



/****************************************************************************************************
* These are overrides from I2CDevice.                                                               *
****************************************************************************************************/

int8_t LPS331::io_op_callback(I2CBusOp* completed) {
  I2CDeviceWithRegisters::io_op_callback(completed);
	int i = 0;
	DeviceRegister *temp_reg = reg_defs.get(i++);
	while (temp_reg) {
		switch (temp_reg->addr) {
		  case LPS331_REG_WHO_AM_I:
		    temp_reg->unread = false;
		    if (!isActive()) {
		      isActive(0xBB == *(temp_reg->val));
		      if (isActive()) {
		        writeDirtyRegisters();
		      }
		    }
		    else {
		      isActive(0xBB == *(temp_reg->val));
		    }
		    break;

		  case LPS331_REG_PRS_P_OUT_XL:
		  case LPS331_REG_PRS_OUT_LO:
		  case LPS331_REG_PRS_OUT_HI:
		    if (BusOpcode::RX == completed->get_opcode()) {
		      if (calculate_pressure()) {
		      }
		    }
		    break;

		  case LPS331_REG_TEMP_OUT_HI:
		  case LPS331_REG_TEMP_OUT_LO:
		    if (BusOpcode::RX == completed->get_opcode()) {
		      if (calculate_temperature()) {
		      }
		    }
		    break;

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
void LPS331::printDebug(StringBuilder* temp) {
  if (nullptr == temp) return;
  //SensorWrapper::issue_json_map(temp, this);
  temp->concatf("Baro sensor (LPS331)\t%snitialized\n---------------------------------------------------\n", (isActive() ? "I": "Uni"));
  I2CDeviceWithRegisters::printDebug(temp);
  temp->concatf("\n");
}




/**************************************************************************
* Class-specific functions...                                             *
**************************************************************************/

bool LPS331::calculate_temperature() {
  if (regUpdated(LPS331_REG_TEMP_OUT_HI) && regUpdated(LPS331_REG_TEMP_OUT_LO)) {
    float result = (float) ((regValue(LPS331_REG_TEMP_OUT_LO) + (regValue(LPS331_REG_TEMP_OUT_HI) << 8)) / 480) + 42.5;
    updateDatum(1, result);
    markRegRead(LPS331_REG_TEMP_OUT_LO);
    markRegRead(LPS331_REG_TEMP_OUT_HI);
    return true;
  }
  return false;
}


bool LPS331::calculate_pressure() {
  if (regUpdated(LPS331_REG_PRS_P_OUT_XL) && regUpdated(LPS331_REG_PRS_OUT_LO) && regUpdated(LPS331_REG_PRS_OUT_HI)) {
    uint32_t result = (uint32_t) (regValue(LPS331_REG_PRS_P_OUT_XL) + (regValue(LPS331_REG_PRS_OUT_LO) << 8) + (regValue(LPS331_REG_PRS_OUT_HI) << 16));
    float kpasc = result / 4096000;
    float altitude = (float) (44330 * (1 - pow((kpasc/p0_sea_level), 0.190295)));
    updateDatum(0, kpasc);  // 1 mBar == 1 Pascal
    updateDatum(2, altitude);  // 1 mBar == 1 Pascal
    markRegRead(LPS331_REG_PRS_P_OUT_XL);
    markRegRead(LPS331_REG_PRS_OUT_LO);
    markRegRead(LPS331_REG_PRS_OUT_HI);
    return true;
  }
  return false;
}




SensorError LPS331::set_power_mode(uint8_t nu_power_mode) {
  power_mode = nu_power_mode;
  switch (power_mode) {
    case 0:
      //writeIndirect(LDS8160_BANK_A_CURRENT, 0x8E, true);
      //writeIndirect(LDS8160_BANK_B_CURRENT, 0x8E, true);
      //writeIndirect(LDS8160_BANK_C_CURRENT, 0x8E, true);
      //writeIndirect(LDS8160_SOFTWARE_RESET, 0x00);
      break;
    case 1:
      //writeIndirect(LDS8160_BANK_A_CURRENT, 0x50, true);
      //writeIndirect(LDS8160_BANK_B_CURRENT, 0x50, true);
      //writeIndirect(LDS8160_BANK_C_CURRENT, 0x50, true);
      //writeIndirect(LDS8160_SOFTWARE_RESET, 0x00);
      break;
    case 2:   // Enter standby.
      //writeIndirect(LDS8160_SOFTWARE_RESET, 0x40);
      break;
    default:
      break;
  }
  #if defined(__MANUVR_DEBUG)
    Kernel::log("LPS331 Power mode set. \n");
  #endif
  return SensorError::NO_ERROR;
}
