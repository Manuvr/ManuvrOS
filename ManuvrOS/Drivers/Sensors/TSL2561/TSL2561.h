/*
File:   TSL2561.h
Author: J. Ian Lindsay
Date:   2016.12.17

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

#ifndef __TSL2561_DRIVER_H__
#define __TSL2561_DRIVER_H__

#include <Drivers/SensorWrapper/SensorWrapper.h>
#include <Platform/Peripherals/I2C/I2CAdapter.h>

/*
Quirk of this part: Internal register addresses are the lower 4-bits
  of the first byte the device. These are the bit definitions for that byte.
*/
#define TSL2561_BYTE0_BIT7_CMD    0x80  // Must be set for a command write.
#define TSL2561_BYTE0_BIT6_CLR    0x40  // Write 1 at this bit to clear IRQ.
#define TSL2561_BYTE0_BIT5_WRD    0x20  // SMB word-wide protocol selection.
#define TSL2561_BYTE0_BIT4_BLK    0x10  // Multibyte transaction.
#define TSL2561_BYTE0_ADDR_MASK   0x0F  // The actual internal register addr.

// 16-bit registers are little-endian.
#define TSL2561_REG_CONTROL       0x00  //
#define TSL2561_REG_TIMING        0x01  //
#define TSL2561_REG_THRESH_LO     0x02  // 16-bit interrupt threshhold.
#define TSL2561_REG_THRESH_HI     0x04  // 16-bit interrupt threshhold.
#define TSL2561_REG_INTERRUPT     0x06  //
#define TSL2561_REG_ID            0x0A  //
#define TSL2561_REG_DATA0         0x0C  // 16-bit data register.
#define TSL2561_REG_DATA1         0x0E  // 16-bit data register.


#define TSL2561_I2CADDR           0x39


class TSL2561 : public I2CDeviceWithRegisters, public SensorWrapper {
  public:
    TSL2561(uint8_t addr = TSL2561_I2CADDR);
    TSL2561(uint8_t addr, uint8_t irq_pin);
    ~TSL2561();

    /* Overrides from SensorWrapper */
    SensorError init();
    SensorError readSensor();
    SensorError setParameter(uint16_t reg, int len, uint8_t*);  // Used to set operational parameters for the sensor.
    SensorError getParameter(uint16_t reg, int len, uint8_t*);  // Used to read operational parameters from the sensor.

    /* Overrides from I2CDeviceWithRegisters... */
    void operationCompleteCallback(I2CBusOp*);
    void printDebug(StringBuilder*);


  private:
    uint8_t _pwr_mode  = 0;
    uint8_t _irq_pin   = 0;

    bool calculate_lux();

    SensorError set_power_mode(uint8_t);
};

#endif
