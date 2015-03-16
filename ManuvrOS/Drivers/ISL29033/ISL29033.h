/*
File:   ISL29033.h
Author: J. Ian Lindsay
Date:   2014.05.27


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

#ifndef __ISL29033_DRIVER_H__
#define __ISL29033_DRIVER_H__

#include "ManuvrOS/Drivers/SensorWrapper/SensorWrapper.h"
#include "ManuvrOS/Drivers/i2c-adapter/i2c-adapter.h"

/* Hardware-defined registers */
#define ISL29033_REG_COMMAND_1        0x00
#define ISL29033_REG_COMMAND_2        0x01
#define ISL29033_REG_DATA_LSB         0x02
#define ISL29033_REG_DATA_MSB         0x03
#define ISL29033_REG_INT_LT_LSB       0x04
#define ISL29033_REG_INT_LT_MSB       0x05
#define ISL29033_REG_INT_HT_LSB       0x06
#define ISL29033_REG_INT_HT_MSB       0x07

/* Software-defined registers */
#define ISL29033_REG_AUTORANGE        0x1000
#define ISL29033_REG_THRESHOLD_HI     0x1001
#define ISL29033_REG_THRESHOLD_LO     0x1002
#define ISL29033_REG_RESOLUTION       0x1003
#define ISL29033_REG_RANGE            0x1004
#define ISL29033_REG_POWER_STATE      0x1005

#define ISL29033_I2CADDR              0x44

#define ISL29033_RANGE_125_LUX        0x00
#define ISL29033_RANGE_500_LUX        0x01
#define ISL29033_RANGE_2000_LUX       0x02
#define ISL29033_RANGE_8000_LUX       0x03

#define ISL29033_RESOLUTION_16        0x00
#define ISL29033_RESOLUTION_12        0x01
#define ISL29033_RESOLUTION_8         0x02
#define ISL29033_RESOLUTION_4         0x03

#define ISL29033_POWER_MODE_OFF       0x00
#define ISL29033_POWER_MODE_ALS       0x05
#define ISL29033_POWER_MODE_IR        0x06


class ISL29033 : public I2CDeviceWithRegisters, public SensorWrapper {
  public:
    ISL29033(uint8_t addr = ISL29033_I2CADDR);
    ~ISL29033();
    
    void gpioSetup();
    
    /* Overrides from SensorWrapper */
    int8_t init(void);
    int8_t readSensor(void);

    int8_t setParameter(uint16_t reg, int len, uint8_t*);  // Used to set operational parameters for the sensor.
    int8_t getParameter(uint16_t reg, int len, uint8_t*);  // Used to read operational parameters from the sensor.

    /* Overrides from I2CDevice... */
    void operationCompleteCallback(I2CQueuedOperation*);
    void printDebug(StringBuilder*);

    static const float res_range_precalc[4][4];   // First index is res, second index is range.

    

  private:
    bool     autorange;          // Set this flag to allow this class to automatically adjust the dynamic range of the sensor.
    uint8_t  res;
    uint8_t  range;
    uint8_t  mode;
    uint8_t  irq_persist;
    uint16_t threshold_lo;
    uint16_t threshold_hi;
    
    int8_t setResRange(void);
    int8_t setThresholds(void);
    int8_t setCommandReg(void);
    uint8_t getCommandReg(void);
    
    bool calculateLux(void);
};

#endif
