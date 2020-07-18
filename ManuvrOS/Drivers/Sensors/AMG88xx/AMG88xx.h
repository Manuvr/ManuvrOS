/*
File:   AMG88xx.h
Author: J. Ian Lindsay
Date:   2017.12.09

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

#ifndef __AMG88XX_DRIVER_H__
#define __AMG88XX_DRIVER_H__

#include <Drivers/Sensors/SensorWrapper.h>
#include <I2CAdapter.h>

// Assumes AD_SELECT is tied low. If tied high, address is 0x69.
#define AMG88XX_I2CADDR               0x68


/*******************************************************************************
* Hardware-defined registers
*******************************************************************************/
/*
* A 128-byte read from base address 0x80 will return an array[64] of 12-bit,
*   little-endian signed integers representing the temperature of each pixel.
* These will need to be sign-extended and endian-normalized before being
*   interpreted as 0.25C per bit.
*/
#define AMG88XX_REG_PIX_VALUE_BASE 0x80

/*
* Control over power state machine.
* Values are mut-ex. 2-bits of real information.
*/
#define AMG88XX_REG_PWR_CTRL           0x00  // RW
  #define AMG88XX_PWR_MODE_NORMAL      0x00
  #define AMG88XX_PWR_MODE_SLEEP       0x10
  #define AMG88XX_PWR_MODE_STNDBY_60   0x20
  #define AMG88XX_PWR_MODE_STNDBY_10   0x21

/*
* Write-only register to do two levels of reset.
*/
#define AMG88XX_REG_RESET              0x01  // WO
  #define AMG88XX_RESET_SOFT           0x30
  #define AMG88XX_RESET_HARD           0x3F

/*
* Frame-rate control.
* 1-bit
*/
#define AMG88XX_REG_FRAME_RATE         0x02  // RW
  #define AMG88XX_FPS_1                0x01
  #define AMG88XX_FPS_10               0x00

/*
* Control of the interrupt pin.
* 2-bit
*/
#define AMG88XX_REG_IRQ_CTRL           0x03  // RW
  #define AMG88XX_IRQ_PIN_PUSH_PULL    0x01
  #define AMG88XX_IRQ_ABSOLUTE_MODE    0x02

/*
* Split-access status register, Bits all line up, and masks
*   apply to both AMG88XX_REG_STATUS and AMG88XX_REG_STATUS_CLEAR
*/
#define AMG88XX_REG_STATUS             0x04  // RO
#define AMG88XX_REG_STATUS_CLEAR       0x05  // WO
  #define AMG88XX_STATUS_TABLE         0x02
  #define AMG88XX_STATUS_OVF_TEMP_OUT  0x04
  #define AMG88XX_STATUS_OVF_THERMSTR  0x08

// NOTE: Register 0x06 is skipped.

/*
* Enables @X moving average on the frames.
* There is a ceremony for changing this to preserve data integrity:
*     0x50   --> ADDR 0x1F
*     0x45   --> ADDR 0x1F
*     0x57   --> ADDR 0x1F
*     <val>  --> ADDR 0x07 (AMG88XX_REG_AVERAGING)
*     0x00   --> ADDR 0x1F
*/
#define AMG88XX_REG_AVERAGING        0x07  // RW
  #define AMG88XX_MOV_AVG_MODE_2X    0x20


#define AMG88XX_REG_IRQ_LEVEL        0x08



/*******************************************************************************
* Options object
*******************************************************************************/

/**
* Set pin def to 255 to mark it as unused.
*/
class AMG88xxOpts {
  public:
    const uint8_t pin;            // Which pin is bound to ~ALERT/CC?

    /** Copy constructor. */
    AMG88xxOpts(const AMG88xxOpts* o) :
      pin(o->pin),
      _flags(o->_flags) {};

    /**
    * Constructor.
    *
    * @param pin
    * @param Initial flags
    */
    AMG88xxOpts(
      uint16_t _p = 255,
      uint8_t _fi = 0
    ) :
      pin(_p),
      _flags(_fi) {};

    inline bool useIRQPin() const {
      return (255 != pin);
    };


  private:
    const uint8_t _flags;
};



// NOTE: Although we could extend I2CDeviceWithRegisters, we choose I2CDevice
//   because of the nuanced treatment of a small set of registers.
class AMG88xx : public I2CDevice, public SensorWrapper {
  public:
    AMG88xx(const AMG88xxOpts*, const uint8_t addr = AMG88XX_I2CADDR);
    ~AMG88xx();

    /* Overrides from SensorWrapper */
    SensorError init();
    SensorError readSensor();
    SensorError setParameter(uint16_t reg, int len, uint8_t*);  // Used to set operational parameters for the sensor.
    SensorError getParameter(uint16_t reg, int len, uint8_t*);  // Used to read operational parameters from the sensor.

    /* Overrides from I2CDevice... */
    int8_t io_op_callback(I2CBusOp*);
    void printDebug(StringBuilder*);

    /**
    *  Given the pixel coordinates, return the temperature.
    *  convert it to a 12-bit signed int, and return it.
    */
    float pixelTemperature(uint8_t x, uint8_t y);


  private:
    const AMG88xxOpts _opts;
    ManuvrMsg isr_event;
    int16_t  _field_values[64];         // Sign-extended and endian-normalized.
    uint8_t  _field_thresholds[64][6];  // 6 bytes of threshold per-pixel.

    /**
    * Give this function a 12-bit signed int (expressed as unsigned) to extend
    *   extend the sign to 16-bit.
    */
    inline int16_t _sign_extend_int12(uint16_t temp_c) {
      temp_c &= 0x0FFF;  // Reduce to 12-bit.
      if (temp_c & 0x0800) {
        // TODO: There is a smart way to eliminate this conditional.
        // Represents a negative number.
        temp_c |= 0xF000;  // Fill in the high four.
      }
      return ((int16_t) temp_c);
    };

    /**
    * Give this function a temperature in Celcius, and it will
    *   convert it to a 12-bit signed int, and return it.
    */
    inline int16_t _celcius_to_int12(float temp_c) {
      return ((short)(temp_c / 0.25));
    };
};

#endif  // __AMG88XX_DRIVER_H__
