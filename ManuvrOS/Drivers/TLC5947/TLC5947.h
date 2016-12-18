/*
File:   TLC5947.h
Author: J. Ian Lindsay
Date:   2016.12.14

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


Driver for 24-Channel 12-bit PWM chip.

Driver supports daisy-chaining by passing constructor parameter.
*/


#ifndef __TLC5947_H__
#define __TLC5947_H__

#include <Drivers/BusQueue/BusQueue.h>
#include <DataStructures/StringBuilder.h>
#include <Drivers/DeviceWithRegisters/DeviceRegister.h>
#include <Platform/Platform.h>
#include <Platform/Peripherals/SPI/SPIAdapter.h>

/*
* This class represents one or more TLC5947 chips daisy-chained on the given
*   SPI bus.
*/
class TLC5947 : public BusOpCallback {
  public:
    TLC5947(uint8_t count, SPIAdapter*, uint8_t cs_pin, uint8_t oe_pin);
    ~TLC5947();

    /* Overrides from the SPICallback interface */
    int8_t io_op_callback(BusOp*);
    int8_t queue_io_job(BusOp*);

    void printDebug(StringBuilder*);

    void refresh();
    void zeroAll();
    void blank(bool x) {  setPin(_oe_pin, x);  };
    int setChannel(uint8_t c, uint16_t val);
    uint16_t getChannel(uint8_t c);

    // 12-bits for each of 24-channels = 288-bits  = 36-bytes.
    inline size_t   bufLen() {    return (_chain_len*36);   };
    inline uint8_t* buffer() {    return _buffer;           };


  protected:
    uint8_t _cs_pin    = 0;
    uint8_t _oe_pin    = 0;
    uint8_t _chain_len = 0;
    bool    _buf_dirty = false;


  private:
    SPIAdapter* _bus = nullptr;

    /* Register memory */
    uint8_t* _buffer = nullptr;  // TODO: Should be stack allocated.

    static constexpr unsigned int CHAN_DEPTH = 12;
};

#endif  // __TLC5947_H__
