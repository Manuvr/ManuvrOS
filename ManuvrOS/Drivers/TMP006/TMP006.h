/*
File:   TMP006.h
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
 

#ifndef TMP006_H
#define TMP006_H

#include "ManuvrOS/Drivers/SensorWrapper/SensorWrapper.h"
#include "math.h"
#include "StringBuilder/StringBuilder.h"
#include "ManuvrOS/Drivers/i2c-adapter/i2c-adapter.h"

 
 
#define TMP006_B0 -0.0000294
#define TMP006_B1 -0.00000057
#define TMP006_B2 0.00000000463
#define TMP006_C2 13.4
#define TMP006_TREF 298.15
#define TMP006_A2 -0.00001678
#define TMP006_A1 0.00175
#define TMP006_S0 6.4  // * 10^-14

#if (ARDUINO >= 100)
  #include "Arduino.h"
#endif


#define TMP006_CFG_RESET    0x8000
#define TMP006_CFG_MODEON   0x7000
#define TMP006_CFG_1SAMPLE  0x0000
#define TMP006_CFG_2SAMPLE  0x0200
#define TMP006_CFG_4SAMPLE  0x0400
#define TMP006_CFG_8SAMPLE  0x0600
#define TMP006_CFG_16SAMPLE 0x0800
#define TMP006_CFG_DRDYEN   0x0100
#define TMP006_CFG_DRDY     0x0080

#define TMP006_I2CADDR        0x41

#define TMP006_REG_VOBJ       0x00
#define TMP006_REG_TAMB       0x01
#define TMP006_REG_CONFIG     0x02
#define TMP006_REG_MANID      0xFE
#define TMP006_REG_DEVID      0xFF



class TMP006 : public I2CDeviceWithRegisters, public SensorWrapper {
  public:
    TMP006(uint8_t addr = TMP006_I2CADDR);

    /* Overrides from SensorWrapper */
    int8_t init(void);
    int8_t readSensor(void);
    int8_t setParameter(uint16_t reg, int len, uint8_t*);  // Used to set operational parameters for the sensor.
    int8_t getParameter(uint16_t reg, int len, uint8_t*);  // Used to read operational parameters from the sensor.

    /* Overrides from I2CDeviceWithRegisters... */
    void operationCompleteCallback(I2CQueuedOperation*);
    void printDebug(StringBuilder*);

    
  private:
    /* Class-specific */
    int8_t check_identity(void);
    int8_t check_data(void);        // If all the data required is fresh, updates derived data.
    
};

#endif
