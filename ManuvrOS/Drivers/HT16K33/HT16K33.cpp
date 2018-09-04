/*
File:   HT16K33.cpp
Author: J. Ian Lindsay
Date:   2016.12.26

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


NOTE: This driver is not presently useful for the keyscan feature of this part.

*/

#include "HT16K33.h"


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

HT16K33::HT16K33(uint8_t addr) : I2CDevice(addr) {
  memset(_matrix, 0, 16);
}

HT16K33::~HT16K33() {
}


/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

int8_t HT16K33::io_op_callback(BusOp* op) {
  I2CBusOp* completed = (I2CBusOp*) op;
  if (completed->get_opcode() == BusOpcode::RX) {
    // We read.
    switch (completed->sub_addr) {
      case HT16K33_REG_CAL00:
        break;

      case HT16K33_REG_ID:
        break;

      default:
        // Illegal read target.
        break;
    }
  }
  else if (completed->get_opcode() == BusOpcode::TX) {
    // We wrote.
    switch (completed->sub_addr) {
      default:
        // Illegal write target.
        break;
    }
  }
  return 0;
}


/*
* Dump this item to the dev log.
*/
void HT16K33::printDebug(StringBuilder* temp) {
  temp->concatf("LED Driver (HT16K33)\t%snitialized%s", (isActive() ? "I": "Uni"), PRINT_DIVIDER_1_STR);
  I2CDevice::printDebug(temp);
  temp->concatf("\n");
}



/*******************************************************************************
* Class-specific functions...                                                  *
*******************************************************************************/
