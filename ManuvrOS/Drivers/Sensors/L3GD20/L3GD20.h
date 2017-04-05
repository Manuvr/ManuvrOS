/***************************************************
This is a library for the L3GD20 GYROSCOPE

Designed specifically to work with the Adafruit L3GD20 Breakout
----> https://www.adafruit.com/products/1032

These sensors use I2C or SPI to communicate, 2 pins (I2C)
or 4 pins (SPI) are required to interface.

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Kevin "KTOWN" Townsend for Adafruit Industries.
BSD license, all text above must be included in any redistribution
****************************************************/
#ifndef __L3GD20_H__
#define __L3GD20_H__

#if (ARDUINO >= 100)
  #include "Arduino.h"
#endif

#include "../IMU/IMU.h"
#include "../SensorWrapper/SensorWrapper.h"



/*=========================================================================
I2C ADDRESS/BITS AND SETTINGS
-----------------------------------------------------------------------*/
#define L3GD20_POLL_TIMEOUT (100) // Maximum number of read attempts
//#define L3GD20_ID (0b11010100)
#define GYRO_SENSITIVITY_250DPS (0.00875F) // Roughly 22/256 for fixed point match
#define GYRO_SENSITIVITY_500DPS (0.0175F) // Roughly 45/256
#define GYRO_SENSITIVITY_2000DPS (0.070F) // Roughly 18/256
/*=========================================================================*/


/*=========================================================================
OPTIONAL SPEED SETTINGS
-----------------------------------------------------------------------*/
typedef enum {
  GYRO_RANGE_250DPS  = 250,
  GYRO_RANGE_500DPS  = 500,
  GYRO_RANGE_2000DPS = 2000
} gyroRange_t;




class L3GD20 : public IMU {
  public:
    L3GD20(void);

    void enableAutoRange( bool enabled );

    /* Overrides from SensorWrapper */
    int8_t init(void);
    int8_t readSensor(void);
    int8_t setParameter(uint16_t reg, int len, uint8_t*);  // Used to set operational parameters for the sensor.
    int8_t getParameter(uint16_t reg, int len, uint8_t*);  // Used to read operational parameters from the sensor.

    /* Overrides from IMU */
    uint8_t reportCapabilities(void);
    int8_t readOutstandingData(int8_t slot, Measurement &msrmnt);
    

  protected:
    int8_t setErrorRates();


  private:
    gyroRange_t _range;
    int32_t _sensorID;
    bool _autoRangeEnabled;
    
    float gyro_x;
    float gyro_y;
    float gyro_z;
    
    static const RegisterDefinition_8 reg_defs[26];
    
    static constexpr const int8_t _ADDRESS  = 0x6B;
};

#define L3GD20_REGISTER_WHO_AM_I        0x00
#define L3GD20_REGISTER_CTRL_REG1       0x01
#define L3GD20_REGISTER_CTRL_REG2       0x02
#define L3GD20_REGISTER_CTRL_REG3       0x03
#define L3GD20_REGISTER_CTRL_REG4       0x04
#define L3GD20_REGISTER_CTRL_REG5       0x05
#define L3GD20_REGISTER_REFERENCE       0x06
#define L3GD20_REGISTER_OUT_TEMP        0x07
#define L3GD20_REGISTER_STATUS_REG      0x08
#define L3GD20_REGISTER_OUT_X_L         0x09
#define L3GD20_REGISTER_OUT_X_H         0x0A
#define L3GD20_REGISTER_OUT_Y_L         0x0B
#define L3GD20_REGISTER_OUT_Y_H         0x0C
#define L3GD20_REGISTER_OUT_Z_L         0x0D
#define L3GD20_REGISTER_OUT_Z_H         0x0E
#define L3GD20_REGISTER_FIFO_CTRL_REG   0x0F
#define L3GD20_REGISTER_FIFO_SRC_REG    0x10
#define L3GD20_REGISTER_INT1_CFG        0x11
#define L3GD20_REGISTER_INT1_SRC        0x12
#define L3GD20_REGISTER_TSH_XH          0x13
#define L3GD20_REGISTER_TSH_XL          0x14
#define L3GD20_REGISTER_TSH_YH          0x15
#define L3GD20_REGISTER_TSH_YL          0x16
#define L3GD20_REGISTER_TSH_ZH          0x17
#define L3GD20_REGISTER_TSH_ZL          0x18
#define L3GD20_REGISTER_INT1_DURATION   0x19
    

const RegisterDefinition_8 L3GD20::reg_defs[26] = {
  {0x0F, false, false, 0b11010100, 0b11010100},  // GYRO_REGISTER_WHO_AM_I
  {0x20, true,  false, 0b00000111, 0b00000111},  // GYRO_REGISTER_CTRL_REG1    
  {0x21, true,  false, 0b00000000, 0b00000000},  // GYRO_REGISTER_CTRL_REG2    
  {0x22, true,  false, 0b00000000, 0b00000000},  // GYRO_REGISTER_CTRL_REG3    
  {0x23, true,  false, 0b00000000, 0b00000000},  // GYRO_REGISTER_CTRL_REG4    
  {0x24, true,  false, 0b00000000, 0b00000000},  // GYRO_REGISTER_CTRL_REG5    
  {0x25, true,  false, 0b00000000, 0b00000000},  // GYRO_REGISTER_REFERENCE    
  {0x26, false, false, 0b00000000, 0b00000000},  // GYRO_REGISTER_OUT_TEMP     
  {0x27, false, false, 0b00000000, 0b00000000},  // GYRO_REGISTER_STATUS_REG   
  {0x28, false, false, 0b00000000, 0b00000000},  // GYRO_REGISTER_OUT_X_L      
  {0x29, false, false, 0b00000000, 0b00000000},  // GYRO_REGISTER_OUT_X_H      
  {0x2A, false, false, 0b00000000, 0b00000000},  // GYRO_REGISTER_OUT_Y_L      
  {0x2B, false, false, 0b00000000, 0b00000000},  // GYRO_REGISTER_OUT_Y_H      
  {0x2C, false, false, 0b00000000, 0b00000000},  // GYRO_REGISTER_OUT_Z_L      
  {0x2D, false, false, 0b00000000, 0b00000000},  // GYRO_REGISTER_OUT_Z_H      
  {0x2E, true,  false, 0b00000000, 0b00000000},  // GYRO_REGISTER_FIFO_CTRL_REG
  {0x2F, false, false, 0b00000000, 0b00000000},  // GYRO_REGISTER_FIFO_SRC_REG 
  {0x30, true,  false, 0b00000000, 0b00000000},  // GYRO_REGISTER_INT1_CFG     
  {0x31, false, false, 0b00000000, 0b00000000},  // GYRO_REGISTER_INT1_SRC     
  {0x32, true,  false, 0b00000000, 0b00000000},  // GYRO_REGISTER_TSH_XH       
  {0x33, true,  false, 0b00000000, 0b00000000},  // GYRO_REGISTER_TSH_XL       
  {0x34, true,  false, 0b00000000, 0b00000000},  // GYRO_REGISTER_TSH_YH       
  {0x35, true,  false, 0b00000000, 0b00000000},  // GYRO_REGISTER_TSH_YL       
  {0x36, true,  false, 0b00000000, 0b00000000},  // GYRO_REGISTER_TSH_ZH       
  {0x37, true,  false, 0b00000000, 0b00000000},  // GYRO_REGISTER_TSH_ZL       
  {0x38, true,  false, 0b00000000, 0b00000000}   // GYRO_REGISTER_INT1_DURATION
};


#endif
