/*
File:   I2CDeviceWithRegisters.cpp
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

*/


#include <Platform/Peripherals/I2C/I2CAdapter.h>

#if defined(MANUVR_SUPPORT_I2C)

I2CDeviceWithRegisters::I2CDeviceWithRegisters(uint8_t addr, uint8_t reg_count, uint16_t mem_size) : I2CDevice(addr), reg_defs(reg_count) {
  _pooled_reg_mem = (uint8_t*) malloc(mem_size);
}


I2CDeviceWithRegisters::~I2CDeviceWithRegisters() {
  DeviceRegister *temp = nullptr;
  while (reg_defs.count()) {
    temp = reg_defs.get();
    //if (nullptr == pooled_registers) {   // If we don't have pooled registers...
    //  if (temp->val != nullptr) {        // ...and our buffer slot is populated...
    //    free(temp->val);              // Free the buffer space.
    //  }
    //}
    delete temp;      // Delete the register definition.
  }
  if (nullptr != _pooled_reg_mem) {  // Free the pooled register memory.
    free(_pooled_reg_mem);
    _pooled_reg_mem = nullptr;
  }
}


bool I2CDeviceWithRegisters::sync() {
  bool return_value = (syncRegisters() == I2C_ERR_SLAVE_NO_ERROR);
  return return_value;
}


/*
* Called to define a new register.
* Definition is presently accounting for length based on the size of the default value passed in.
*/
bool I2CDeviceWithRegisters::defineRegister(uint16_t _addr, uint8_t val, bool dirty, bool unread, bool writable) {
  uint8_t* buffer = _pooled_reg_mem;
  if (reg_defs.count() > 0) {
    DeviceRegister* last = reg_defs.get(reg_defs.count()-1);
    buffer = (uint8_t*) last->val + last->len;
  }
  DeviceRegister* nu = new DeviceRegister(_addr, val, buffer, dirty, unread, writable);
  reg_defs.insert(nu);
  return true;
}

bool I2CDeviceWithRegisters::defineRegister(uint16_t _addr, uint16_t val, bool dirty, bool unread, bool writable) {
  uint8_t* buffer = _pooled_reg_mem;
  if (reg_defs.count() > 0) {
    DeviceRegister* last = reg_defs.get(reg_defs.count()-1);
    buffer = (uint8_t*) last->val + last->len;
  }
  DeviceRegister *nu = new DeviceRegister(_addr, val, buffer, dirty, unread, writable);
  reg_defs.insert(nu);
  return true;
}

bool I2CDeviceWithRegisters::defineRegister(uint16_t _addr, uint32_t val, bool dirty, bool unread, bool writable) {
  uint8_t* buffer = _pooled_reg_mem;
  if (reg_defs.count() > 0) {
    DeviceRegister* last = reg_defs.get(reg_defs.count()-1);
    buffer = (uint8_t*) last->val + last->len;
  }
  DeviceRegister *nu = new DeviceRegister(_addr, val, buffer, dirty, unread, writable);
  reg_defs.insert(nu);
  return true;
}


DeviceRegister* I2CDeviceWithRegisters::getRegisterByBaseAddress(int b_addr) {
  int i = 0;
  DeviceRegister *nu = reg_defs.get(0);
  while (nu) {
    if (nu->addr == b_addr) {
      return nu;
    }
    i++;
    nu = reg_defs.get(i);
  }
  return nullptr;
}


/**
* Given base address, return the register's value.
* Multi-byte registers are construed to be big-endian.
*
* @param base_addr The base address of the register in the device.
* @return The value stored by the register.
*/
unsigned int I2CDeviceWithRegisters::regValue(uint8_t base_addr) {
  DeviceRegister* reg = getRegisterByBaseAddress(base_addr);
  return (reg) ? reg->getVal() : 0;
}


/**
* Given base address, return the register's unread state.
*
* @param base_addr The base address of the register in the device.
* @return True if the register content is waiting to be acted upon.
*/
bool I2CDeviceWithRegisters::regUpdated(uint8_t base_addr) {
  DeviceRegister *nu = getRegisterByBaseAddress(base_addr);
  return (nu) ? nu->unread : false;
}

/**
* @param base_addr The base address of the register in the device.
* @return True if the register content is waiting to be written.
*/
bool I2CDeviceWithRegisters::regDirty(uint8_t base_addr) {
  DeviceRegister *nu = getRegisterByBaseAddress(base_addr);
  return (nu) ? nu->dirty : false;
}

/**
* Given base address, mark this register as having been read.
*
* @param base_addr The base address of the register in the device.
*/
void I2CDeviceWithRegisters::markRegRead(uint8_t base_addr) {
  DeviceRegister *nu = getRegisterByBaseAddress(base_addr);
  if (nu) {
    nu->unread = false;
  }
}


/**
* Multi-byte registers are construed to be big-endian.
* Override to support stacking up operations.
*
* @param base_addr The base address of the register in the device.
* @param val The desired value of the register.
* @return 0 on success. i2c error code on failure.
*/
int8_t I2CDeviceWithRegisters::writeIndirect(uint8_t base_addr, unsigned int val) {
  return writeIndirect(base_addr, val, false);
}


/**
* Multi-byte registers are construed to be big-endian.
* TODO: Needs a refactor. This is an experiment.
*
* @param base_addr The base address of the register in the device.
* @param val The desired value of the register.
* @param defer Pass true to defer the i/o. Used to stack operations.
* @return 0 on success. i2c error code on failure.
*/
int8_t I2CDeviceWithRegisters::writeIndirect(uint8_t base_addr, unsigned int val, bool defer) {
  DeviceRegister *nu = getRegisterByBaseAddress(base_addr);
  if (nu == nullptr) return I2C_ERR_SLAVE_UNDEFD_REG;
  if (!nu->writable) return I2C_ERR_SLAVE_REG_IS_RO;

  nu->set(val);
  if (!defer) {
    writeDirtyRegisters();
  }
  return I2C_ERR_SLAVE_NO_ERROR;
}


int8_t I2CDeviceWithRegisters::writeRegister(uint8_t base_addr) {
  return writeRegister(getRegisterByBaseAddress(base_addr));
}


int8_t I2CDeviceWithRegisters::writeRegister(DeviceRegister *reg) {
  if (reg == nullptr) {
    return I2C_ERR_SLAVE_UNDEFD_REG;
  }
  if (!reg->writable) {
    return I2C_ERR_SLAVE_REG_IS_RO;
  }

  if (!reg->op_pending && !writeX(reg->addr, reg->len, (uint8_t*) reg->val)) {
    #ifdef MANUVR_DEBUG
    Kernel::log("Bus error while writing device.\n");
    #endif
    return I2C_ERR_SLAVE_BUS_FAULT;
  }
  reg->op_pending = true;
  return I2C_ERR_SLAVE_NO_ERROR;
}


/**
* Read a register at the given base address.
*
* @param base_addr Device access will begin at the specified register index.
* @return 0 on success. i2c error code on failure.
*/
int8_t I2CDeviceWithRegisters::readRegister(uint8_t base_addr) {
  return readRegister(getRegisterByBaseAddress(base_addr));
}

int8_t I2CDeviceWithRegisters::readRegister(DeviceRegister* reg) {
  if (reg == nullptr) {
    return I2C_ERR_SLAVE_UNDEFD_REG;
  }
  if (!readX(reg->addr, reg->len, (uint8_t*) reg->val)) {
    #ifdef MANUVR_DEBUG
    Kernel::log("Bus error while reading device.\n");
    #endif
    return I2C_ERR_SLAVE_BUS_FAULT;
  }
  return I2C_ERR_SLAVE_NO_ERROR;
}


/*
* Dump the contents of this device to the logger.
*/
void I2CDeviceWithRegisters::printDebug(StringBuilder* temp) {
  if (temp) {
    I2CDevice::printDebug(temp);
    DeviceRegister *temp_reg = nullptr;
    unsigned int count = reg_defs.count();
    temp->concat("--- Device registers\n");
    for (unsigned int i = 0; i < count; i++) {
      temp_reg = reg_defs.get(i);
      temp->concatf("\tReg 0x%02x  (%p)  0x%02x", temp_reg->addr, temp_reg->val, *(temp_reg->val+0));
      switch (temp_reg->len) {
        case 1:
          temp->concat("      ");
          break;
        case 2:
          temp->concatf("%02x    ", *(temp_reg->val+1));
          break;
        case 4:
          temp->concatf("%02x%02x%02x", *(temp_reg->val+1), *(temp_reg->val+2), *(temp_reg->val+3));
          break;
        default:
          break;
      }
      temp->concatf(
        "\t%s\t%s\t%s\t%s\n",
        (temp_reg->dirty    ? "dirty"  : "clean"),
        (temp_reg->unread   ? "unread" : "known"),
        (temp_reg->writable ? "r/w"    : "ro"),
        (temp_reg->op_pending ? "i/o pending"    : "stable")
      );
    }
  }
}


/**
* When the class is freshly-instantiated, we need to call this fxn do bring the class's idea of
*   register values into alignment with "what really is" in the device.
*
* @return I2C_ERR_SLAVE_NO_ERROR on success.
*/
int8_t I2CDeviceWithRegisters::syncRegisters() {
  DeviceRegister *temp = nullptr;
  int8_t return_value = I2C_ERR_SLAVE_NO_ERROR;
  unsigned int count = reg_defs.count();
  for (unsigned int i = 0; i < count; i++) {
    temp = reg_defs.get(i);
    if (temp) {   // Safety-check that an out-of-bounds reg wasn't in the list...
      return_value = readRegister(temp);
      #ifdef MANUVR_DEBUG
      if (return_value != I2C_ERR_SLAVE_NO_ERROR) {
        StringBuilder output;
        output.concatf("Failed to read from register %d\n", temp->addr);
        Kernel::log(&output);
      }
      #endif
    }
    return_value = I2C_ERR_SLAVE_UNDEFD_REG;
  }
  return return_value;
}


/**
* Any registers marked dirty will be written to the device if the register is also marked wriatable.
*
* @return I2C_ERR_SLAVE_NO_ERROR on success.
*/
int8_t I2CDeviceWithRegisters::writeDirtyRegisters() {
  int8_t return_value = I2C_ERR_SLAVE_NO_ERROR;
  DeviceRegister *temp = nullptr;
  unsigned int count = reg_defs.count();
  for (unsigned int i = 0; i < count; i++) {
    temp = reg_defs.get(i);
    if (temp->dirty) {
      if (temp->writable) {
        return_value = writeRegister(temp);
        if (return_value != I2C_ERR_SLAVE_NO_ERROR) {
          #ifdef MANUVR_DEBUG
          StringBuilder output;
          output.concatf("Failed to write dirty register %d with code(%d). The dropped data was (%d). Aborting...\n", temp->addr, return_value, temp->val);
          Kernel::log(&output);
          #endif
          return return_value;
        }
      }
      else {
        #ifdef MANUVR_DEBUG
          StringBuilder output;
          output.concatf("Uh oh... register %d was marked dirty but it isn't writable. Marking clean with no write...\n", temp->addr);
          Kernel::log(&output);
        #endif
      }
    }
  }
  return return_value;
}


/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

int8_t I2CDeviceWithRegisters::io_op_callahead(BusOp* op) {
  // Default is to allow the transfer.
  return 0;
}


int8_t I2CDeviceWithRegisters::io_op_callback(BusOp* _op) {
  int8_t return_value = -1;
  I2CBusOp* completed = (I2CBusOp*) _op;

  if (completed) {
    DeviceRegister* reg = getRegisterByBaseAddress(completed->sub_addr);
    reg->op_pending = false;
    if (!completed->hasFault()) {
      if (reg) {
        switch (completed->get_opcode()) {
          case BusOpcode::RX:
            reg->unread = true;
            return_value = register_read_cb(reg);
            break;
          case BusOpcode::TX:
            reg->dirty = false;
            return_value = register_write_cb(reg);
            break;
          case BusOpcode::TX_CMD:
            break;
          default:
            break;
        }
      }
      else {
        #ifdef MANUVR_DEBUG
        Kernel::log("I2CDeviceWithRegisters::io_op_callback(): register lookup failed.\n");
        #endif
      }
    }
    else {
      #ifdef MANUVR_DEBUG
      Kernel::log("I2CDeviceWithRegisters::io_op_callback(): i2c operation errored.\n");
      #endif
    }
    /* Null the buffer so the bus adapter isn't tempted to free it.
      TODO: This is silly. Fix this in the API. */
    _op->buf     = nullptr;
    _op->buf_len = 0;
  }
  return return_value;
}

#endif  // MANUVR_SUPPORT_I2C
