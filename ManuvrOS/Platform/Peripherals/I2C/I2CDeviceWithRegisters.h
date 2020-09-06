/*
File:   I2CAdapter.h
Author: J. Ian Lindsay
Date:   2014.03.10

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


This class is supposed to be an i2c abstraction layer. The goal is to have
an object of this class that can be instantiated and used to communicate
with i2c devices (as a bus master) regardless of the platform.

This file is the tortured result of growing pains since the beginning of
  ManuvrOS. It has been refactored fifty-eleven times, suffered the brunt
  of all porting efforts, and has reached the point where it must be split
  apart into a more-portable platform-abstraction strategy.
*/



#include <I2CAdapter.h>
#include <RingBuffer.h>
#include <Drivers/DeviceWithRegisters/DeviceRegister.h>

#ifndef __PLATFORM_I2C_EXTENSION_H__
#define __PLATFORM_I2C_EXTENSION_H__

/*
* This class is an extension of I2CDevice for the special (most common) case where the
*   device being represented has an internal register set. This extended class just takes
*   some of the burden of routine register definition and manipulation off the extending class.
*/
class I2CDeviceWithRegisters : public I2CDevice {
  public:
    I2CDeviceWithRegisters(uint8_t addr, uint8_t reg_count) : I2CDeviceWithRegisters(addr, reg_count, reg_count) {};
    I2CDeviceWithRegisters(uint8_t addr, uint8_t reg_count, uint16_t mem_size);
    ~I2CDeviceWithRegisters();

    bool sync();


  protected:
    RingBuffer<DeviceRegister*> reg_defs;      // Here is where registers will be enumerated.
    bool      multi_access_support = false;    // TODO: Make this happen! Depends on _pooled_reg_mem.


    // Callback for requested operation completion.
    virtual int8_t io_op_callahead(BusOp*);
    int8_t io_op_callback(BusOp*);

    virtual int8_t register_write_cb(DeviceRegister*) = 0;  // Mandatory overrides.
    virtual int8_t register_read_cb(DeviceRegister*)  = 0;  // Mandatory overrides.

    bool defineRegister(uint16_t _addr, uint8_t  val, bool dirty, bool unread, bool writable);
    bool defineRegister(uint16_t _addr, uint16_t val, bool dirty, bool unread, bool writable);
    bool defineRegister(uint16_t _addr, uint32_t val, bool dirty, bool unread, bool writable);

    DeviceRegister* getRegisterByBaseAddress(int b_addr);

    unsigned int regValue(uint8_t base_addr);

    bool regUpdated(uint8_t base_addr);
    bool regDirty(uint8_t base_addr);
    void markRegRead(uint8_t base_addr);

    int8_t writeIndirect(uint8_t base_addr, unsigned int val);
    int8_t writeIndirect(uint8_t base_addr, unsigned int val, bool defer);

    int8_t writeDirtyRegisters();   // Automatically writes all registers marked as dirty.

    // Sync all registers.
    int8_t syncRegisters();
    int8_t zeroRegisters();

    /* Low-level stuff. These are the ONLY fxns in this ENTIRE class that should care */
    /*   about ACTUAL register addresses. Everything else ought to be using the index */
    /*   for the desired register in reg_defs[].                                      */
    int8_t writeRegister(uint8_t base_addr);
    int8_t readRegister(uint8_t base_addr);
    //int8_t readRegisters(uint8_t count, ...);
    /* This is the end of the low-level functions.                                    */

    /* Debug stuff... */
    virtual void printDebug(StringBuilder* temp);


  private:
    uint8_t* _pooled_reg_mem  = nullptr;

    int8_t writeRegister(DeviceRegister* reg);
    int8_t readRegister(DeviceRegister* reg);
};

#endif  // __PLATFORM_I2C_EXTENSION_H__
