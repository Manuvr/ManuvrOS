/*
File:   SX1503.h
Author: J. Ian Lindsay
Date:   2019.11.30

Copyright 2019 Manuvr, Inc

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

#ifndef __SX1503_DRIVER_H__
#define __SX1503_DRIVER_H__

#include <Platform/Peripherals/I2C/I2CAdapter.h>


#define SX1503_I2C_ADDR           0x20  // Not configurable.
#define SX1503_SERIALIZE_VERSION  0x01  // Version code for serialized states.
#define SX1503_SERIALIZE_SIZE       36


/* Class flags. */
#define SX1503_FLAG_PRESERVE_STATE   0x0001
#define SX1503_FLAG_PINS_CONFD       0x2000
#define SX1503_FLAG_INITIALIZED      0x4000
#define SX1503_FLAG_FROM_BLOB        0x8000

#define SX1503_FLAG_SERIAL_MASK      0x000F  // Only these bits are serialized.


/* These are the i2c register addresses. */
#define SX1503_REG_DATA_B         0x00  //
#define SX1503_REG_DATA_A         0x01  //
#define SX1503_REG_DIR_B          0x02  //
#define SX1503_REG_DIR_A          0x03  //
#define SX1503_REG_PULLUP_B       0x04  //
#define SX1503_REG_PULLUP_A       0x05  //
#define SX1503_REG_PULLDOWN_B     0x06  //
#define SX1503_REG_PULLDOWN_A     0x07  //
#define SX1503_REG_IRQ_MASK_B     0x08  //
#define SX1503_REG_IRQ_MASK_A     0x09  //
#define SX1503_REG_SENSE_H_B      0x0A  //
#define SX1503_REG_SENSE_H_A      0x0B  //
#define SX1503_REG_SENSE_L_B      0x0C  //
#define SX1503_REG_SENSE_L_A      0x0D  //
#define SX1503_REG_IRQ_SRC_B      0x0E  //
#define SX1503_REG_IRQ_SRC_A      0x0F  //
#define SX1503_REG_EVENT_STAT_B   0x10  //
#define SX1503_REG_EVENT_STAT_A   0x11  //
#define SX1503_REG_PLD_MODE_B     0x20  //
#define SX1503_REG_PLD_MODE_A     0x21  //
#define SX1503_REG_PLD_TABLE_0B   0x22  //
#define SX1503_REG_PLD_TABLE_0A   0x23  //
#define SX1503_REG_PLD_TABLE_1B   0x24  //
#define SX1503_REG_PLD_TABLE_1A   0x25  //
#define SX1503_REG_PLD_TABLE_2B   0x26  //
#define SX1503_REG_PLD_TABLE_2A   0x27  //
#define SX1503_REG_PLD_TABLE_3B   0x28  //
#define SX1503_REG_PLD_TABLE_3A   0x29  //
#define SX1503_REG_PLD_TABLE_4B   0x2A  //
#define SX1503_REG_PLD_TABLE_4A   0x2B  //
#define SX1503_REG_ADVANCED       0xAD  //

/* An optional IRQ handler function that the application can pass in. */
typedef void (*SX1503Callback)(uint8_t pin, uint8_t level);


/*
* Driver class.
*/
class SX1503 : public I2CDeviceWithRegisters {
  public:
    SX1503(const uint8_t irq_pin, const uint8_t reset_pin);
    SX1503(const uint8_t* buf, const unsigned int len);  // Takes serialized state as args.
    ~SX1503();

    /* Overrides from I2CDeviceWithRegisters... */
    int8_t register_write_cb(DeviceRegister*);
    int8_t register_read_cb(DeviceRegister*);
    void printDebug(StringBuilder*);
    int8_t init();
    int8_t reset();




    int8_t poll();
    bool isrFired();
    int8_t refresh();

    // Basic usage as pins...
    int8_t  gpioMode(uint8_t pin, GPIOMode);
    int8_t  digitalWrite(uint8_t pin, uint8_t value);
    uint8_t digitalRead(uint8_t pin);

    // Interrupt and callback management...
    int8_t  attachInterrupt(uint8_t pin, SX1503Callback, uint8_t condition);
    int8_t  detachInterrupt(uint8_t pin);
    int8_t  detachInterrupt(SX1503Callback);

    // Advanced usage...
    int8_t  setPLD();  // TODO: Define API for this feature.
    int8_t  useBoost(bool enable);

    // No NVM on this part, so these fxns help do init in a single step.
    uint8_t serialize(uint8_t* buf, unsigned int len);
    int8_t  unserialize(const uint8_t* buf, const unsigned int len);

    inline bool initialized() {  return _sx_flag(SX1503_FLAG_INITIALIZED);  };
    inline bool preserveOnDestroy() {
      return _sx_flag(SX1503_FLAG_PRESERVE_STATE);
    };
    inline void preserveOnDestroy(bool x) {
      _sx_set_flag(SX1503_FLAG_PRESERVE_STATE, x);
    };


  private:
    const uint8_t  _IRQ_PIN;
    const uint8_t  _RESET_PIN;
    uint16_t       _flags = 0;
    uint8_t        priorities[16];
    SX1503Callback callbacks[16];
    uint8_t        _a_dat = 0;
    uint8_t        _b_dat = 0;

    int8_t _invoke_pin_callback(uint8_t pin, bool value);

    int8_t  _ll_pin_init();
    int8_t  _clobber_all_registers(const uint8_t* buf);

    inline bool _from_blob() {   return _sx_flag(SX1503_FLAG_FROM_BLOB); };

    /* Flag manipulation inlines */
    inline uint16_t _sx_flags() {                return _flags;           };
    inline bool _sx_flag(uint16_t _flag) {       return (_flags & _flag); };
    inline void _sx_clear_flag(uint16_t _flag) { _flags &= ~_flag;        };
    inline void _sx_set_flag(uint16_t _flag) {   _flags |= _flag;         };
    inline void _sx_set_flag(uint16_t _flag, bool nu) {
      if (nu) _flags |= _flag;
      else    _flags &= ~_flag;
    };
};

#endif   // __SX1503_DRIVER_H__
