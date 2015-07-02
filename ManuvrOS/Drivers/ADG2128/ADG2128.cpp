/*
File:   ADG2128.cpp
Author: J. Ian Lindsay
Date:   2014.03.10


This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include "ADG2128.h"

const int8_t ADG2128::ADG2128_ERROR_NO_ERROR         = 0; 
const int8_t ADG2128::ADG2128_ERROR_ABSENT           = -1;
const int8_t ADG2128::ADG2128_ERROR_BUS              = -2;
const int8_t ADG2128::ADG2128_ERROR_BAD_COLUMN       = -3;   // Column was out-of-bounds.
const int8_t ADG2128::ADG2128_ERROR_BAD_ROW          = -4;   // Row was out-of-bounds.




/*
* Constructor. Takes the i2c address of this device as sole argument.
*/
ADG2128::ADG2128(uint8_t i2caddr) : I2CDevice() {
  _dev_addr = i2caddr;

  preserve_state_on_destroy = false;
  dev_init = false;
}

ADG2128::~ADG2128(void) {
  if (!preserve_state_on_destroy) {
    reset();
  }
}


/* 
* 
*/
int8_t ADG2128::init(void) {
  for (int i = 0; i < 12; i++) {
    if (readback(i) != ADG2128_ERROR_NO_ERROR) {
      dev_init = false;
      StaticHub::log(__PRETTY_FUNCTION__, LOG_ERR, "Failed to init switch.");
      return ADG2128_ERROR_BUS;
    }
  }
  dev_init = true;
  return ADG2128_ERROR_NO_ERROR;
}


    
int8_t ADG2128::setRoute(uint8_t col, uint8_t row) {
  if (col > 7)  return ADG2128_ERROR_BAD_COLUMN;
  if (row > 11) return ADG2128_ERROR_BAD_ROW;

  uint8_t safe_row = row;
  if (safe_row >= 6) safe_row = safe_row + 2;
  uint16_t val = 0x01 + ((0x80 + (safe_row << 3) + col) << 8);
  if (!write16(-1, val)) {
    StaticHub::log(__PRETTY_FUNCTION__, LOG_ERR, "Failed to write new value.");
    return ADG2128_ERROR_BUS;
  }
  values[row] = values[row] | (0x01 << col);  // TODO: Should be in the callback.
  return ADG2128_ERROR_NO_ERROR;
}


int8_t ADG2128::unsetRoute(uint8_t col, uint8_t row) {
  if (col > 7)  return ADG2128_ERROR_BAD_COLUMN;
  if (row > 11) return ADG2128_ERROR_BAD_ROW;

  uint8_t safe_row = row;
  if (safe_row >= 6) safe_row = safe_row + 2;
  uint16_t val = 0x01 + ((0x00 + (safe_row << 3) + col) << 8);
  if (!write16(-1, val)) {
    StaticHub::log(__PRETTY_FUNCTION__, LOG_ERR, "Failed to write new value.");
    return ADG2128_ERROR_BUS;
  }
  values[row] = values[row] & ~(0x01 << col);  // TODO: Should be in the callback.
  return ADG2128_ERROR_NO_ERROR;
}


void ADG2128::preserveOnDestroy(bool x) {
  preserve_state_on_destroy = x;
}


/*
* Opens all switches.
*/
int8_t ADG2128::reset(void) {
  for (int i = 0; i < 12; i++) {
    for (int j = 0; j < 8; j++) {
      if (unsetRoute(j, i) != ADG2128_ERROR_NO_ERROR) {
        return ADG2128_ERROR_BUS;
      }
    }
  }
  return init();
}


/*
* Readback on this part is organized by rows, with the return bits
* being the state of the switches to the ocrresponding column.
* The readback address table is hard-coded in the readback_addr array.
*
*
*/
int8_t ADG2128::readback(uint8_t row) {
  if (row > 11) return ADG2128_ERROR_BAD_ROW;

  uint16_t readback_addr[12] = {0x3400, 0x3b00, 0x7400, 0x7b00, 0x3500, 0x3D00, 0x7500, 0x7D00, 0x3600, 0x3E00, 0x7600, 0x7E00};
  if (!read16(readback_addr[row])) {
    StaticHub::log(__PRETTY_FUNCTION__, LOG_ERR, "Bus error while reading readback address %d.\n", row);
    return ADG2128_ERROR_ABSENT;
  }
  return ADG2128_ERROR_NO_ERROR;
}


uint8_t ADG2128::getValue(uint8_t row) {
  if (row > 11) return ADG2128_ERROR_BAD_ROW;
  readback(row);
  return values[row];
}




/****************************************************************************************************
* These are overrides from I2CDevice                                                                *
****************************************************************************************************/

void ADG2128::operationCompleteCallback(I2CQueuedOperation* completed) {
  if (completed->err_code != I2C_ERR_CODE_NO_ERROR) {
    StringBuilder output;
    output.concat("An i2c operation requested by the ADG2128 came back failed.\n");
    completed->printDebug(&output);
    StaticHub::log(&output);
    return;
  }
  switch (completed->opcode) {
    case I2C_OPERATION_READ:
      switch (completed->sub_addr) {
        case 0x3400:  values[0] = (uint8_t) *(completed->buf);
          break;
        case 0x3b00:  values[1] = (uint8_t) *(completed->buf);
          break;
        case 0x7400:  values[2] = (uint8_t) *(completed->buf);
          break;
        case 0x7b00:  values[3] = (uint8_t) *(completed->buf);
          break;
        case 0x3500:  values[4] = (uint8_t) *(completed->buf);
          break;
        case 0x3D00:  values[5] = (uint8_t) *(completed->buf);
          break;
        case 0x7500:  values[6] = (uint8_t) *(completed->buf);
          break;
        case 0x7D00:  values[7] = (uint8_t) *(completed->buf);
          break;
        case 0x3600:  values[8] = (uint8_t) *(completed->buf);
          break;
        case 0x3E00:  values[9] = (uint8_t) *(completed->buf);
          break;
        case 0x7600:  values[10] = (uint8_t) *(completed->buf);
          break;
        case 0x7E00:  values[11] = (uint8_t) *(completed->buf);
          break;
        default:
          break;
      }
      // We just read back data from the switch.
      break;
    case I2C_OPERATION_WRITE:
      // We just confirmed a write to the switch.
      break;
    default:
      break;
  }
}


/*
* Dump this item to the dev log.
*/
void ADG2128::printDebug(StringBuilder* output) {
  output->concat("ADG2128 8x12 cross-point switch\n---------------------------------------------------\n");
  I2CDevice::printDebug(output);
  if (dev_init) {
    for (int i = 0; i < 12; i++) {
      output->concatf("\t Row %d: %u\n", i, values[i]);
    }
  }
  else {
    output->concat("\t Not initialized.\n");
  }
  output->concat("\n");
}



