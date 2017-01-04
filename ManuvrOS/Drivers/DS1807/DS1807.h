/*
File:   DS1807.h
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

#ifndef __DS1807_DRIVER_H__
#define __DS1807_DRIVER_H__

#include <Platform/Peripherals/I2C/I2CAdapter.h>

/* Hardware-defined registers */
#define DS1807_REG_CONFIG           0x01

#define DS1807_I2CADDR              0x40


class DS1807 : public I2CDevice {
  public:
    DS1807();
    ~DS1807();

    /* Overrides from I2CDevice... */
    int8_t io_op_callback(I2CBusOp*);
    void printDebug(StringBuilder*);


  private:
    SensorError check_identity();
};

#endif   // __DS1807_DRIVER_H__
