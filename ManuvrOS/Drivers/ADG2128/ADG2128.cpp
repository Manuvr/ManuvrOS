/*
File:   ADG2128.cpp
Author: J. Ian Lindsay
Date:   2014.03.10

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

#include "ADG2128.h"

const int8_t ADG2128::ADG2128_ERROR_NO_ERROR         = 0;
const int8_t ADG2128::ADG2128_ERROR_ABSENT           = -1;
const int8_t ADG2128::ADG2128_ERROR_BUS              = -2;
const int8_t ADG2128::ADG2128_ERROR_BAD_COLUMN       = -3;   // Column was out-of-bounds.
const int8_t ADG2128::ADG2128_ERROR_BAD_ROW          = -4;   // Row was out-of-bounds.




/*
* Constructor. Takes the i2c address of this device as sole argument.
*/
ADG2128::ADG2128(uint8_t addr) : I2CDevice(addr) {
  preserve_state_on_destroy = false;
  dev_init = false;
}

ADG2128::~ADG2128() {
  if (!preserve_state_on_destroy) {
    reset();
  }
}


/*
*
*/
int8_t ADG2128::init() {
  for (int i = 0; i < 12; i++) {
    if (readback(i) != ADG2128_ERROR_NO_ERROR) {
      dev_init = false;
      #ifdef MANUVR_DEBUG
        Kernel::log("Failed to init switch.\n");
      #endif
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
    #ifdef MANUVR_DEBUG
      Kernel::log("Failed to write new value.\n");
    #endif
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
    #ifdef MANUVR_DEBUG
      Kernel::log("Failed to write new value.\n");
    #endif
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
* being the state of the switches to the corresponding column.
* The readback address table is hard-coded in the readback_addr array.
*
*
*/
int8_t ADG2128::readback(uint8_t row) {
  if (row > 11) return ADG2128_ERROR_BAD_ROW;

  uint16_t readback_addr[12] = {0x3400, 0x3b00, 0x7400, 0x7b00, 0x3500, 0x3D00, 0x7500, 0x7D00, 0x3600, 0x3E00, 0x7600, 0x7E00};
  if (!read16(readback_addr[row])) {
    #ifdef MANUVR_DEBUG
      StringBuilder _log;
      _log.concatf("Bus error while reading readback address %d.\n", row);
      Kernel::log(&_log);
    #endif
    return ADG2128_ERROR_ABSENT;
  }
  return ADG2128_ERROR_NO_ERROR;
}


uint8_t ADG2128::getValue(uint8_t row) {
  if (row > 11) return ADG2128_ERROR_BAD_ROW;
  readback(row);
  return values[row];
}



/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

int8_t ADG2128::io_op_callback(BusOp* completed) {
  I2CBusOp* completed = (I2CBusOp*) op;
  if (completed->hasFault()) {
    StringBuilder output;
    output.concat("An i2c operation requested by the ADG2128 came back failed.\n");
    completed->printDebug(&output);
    Kernel::log(&output);
    return -1;
  }
  switch (completed->get_opcode()) {
    case BusOpcode::RX:
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
    case BusOpcode::TX:
      // We just confirmed a write to the switch.
      break;
    default:
      break;
  }
  return 0;
}


/*
* Dump this item to the dev log.
*/
void ADG2128::printDebug(StringBuilder* output) {
  output->concat("ADG2128 8x12 cross-point switch");
  output->concat(PRINT_DIVIDER_1_STR);
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
