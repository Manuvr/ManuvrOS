/*
File:   DS1807.cpp
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

*/

#include "DS1807.h"


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

DS1807::DS1807() : I2CDevice(DS1807_I2CADDR) {
}

DS1807::~DS1807() {
}


/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

int8_t DS1807::io_op_callback(I2CBusOp* completed) {
  I2CDevice::io_op_callback(completed);
  return 0;
}


/*
* Dump this item to the dev log.
*/
void DS1807::printDebug(StringBuilder* temp) {
  temp->concatf("Potentiometer (DS1807)\t%snitialized%s", (isActive() ? "I": "Uni"), PRINT_DIVIDER_1_STR);
  I2CDevice::printDebug(temp);
  temp->concatf("\n");
}



/*******************************************************************************
* Class-specific functions...                                                  *
*******************************************************************************/

int8_t DS1807::check_identity() {
  return 0;
}
