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

I2CDeviceWithRegisters::I2CDeviceWithRegisters(uint8_t addr) : I2CDevice(addr) {
  multi_access_support = false;
}


I2CDeviceWithRegisters::~I2CDeviceWithRegisters() {
  DeviceRegister *temp = nullptr;
  while (reg_defs.hasNext()) {
    temp = reg_defs.get();
    reg_defs.remove();
    //if (nullptr == pooled_registers) {   // If we don't have pooled registers...
    //  if (temp->val != nullptr) {        // ...and our buffer slot is populated...
    //    free(temp->val);              // Free the buffer space.
    //  }
    //}
    delete temp;      // Delete the register definition.
  }

  //if (nullptr != pooled_registers) {   // If we DO have pooled registers...
  //  free(pooled_registers);         // ..free them at this point.
  //  pooled_registers = nullptr;
  //}
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
  DeviceRegister *nu = new DeviceRegister();
  nu->addr     = _addr;
  nu->len      = 1;
  nu->val      = (uint8_t*) malloc(1);
  *(nu->val)   = val;
  nu->dirty    = dirty;
  nu->unread   = unread;
  nu->writable = writable;

  reg_defs.insert(nu);
  return true;
}

bool I2CDeviceWithRegisters::defineRegister(uint16_t _addr, uint16_t val, bool dirty, bool unread, bool writable) {
  DeviceRegister *nu = new DeviceRegister();
  nu->addr     = _addr;
  nu->val      = (uint8_t*) malloc(2);
  nu->len      = 2;
  *(nu->val)     = (uint8_t) ((val >> 8) & 0x00FF);
  *(nu->val + 1) = (uint8_t) (val & 0x00FF);
  nu->dirty    = dirty;
  nu->unread   = unread;
  nu->writable = writable;

  reg_defs.insert(nu);
  return true;
}

bool I2CDeviceWithRegisters::defineRegister(uint16_t _addr, uint32_t val, bool dirty, bool unread, bool writable) {
  DeviceRegister *nu = new DeviceRegister();
  nu->addr     = _addr;
  nu->val      = (uint8_t*) malloc(4);
  nu->len      = 4;
  *(nu->val)     = (uint8_t) ((val >> 24) & 0x000000FF);
  *(nu->val + 1) = (uint8_t) ((val >> 16) & 0x000000FF);
  *(nu->val + 2) = (uint8_t) ((val >> 8)  & 0x000000FF);
  *(nu->val + 3) = (uint8_t) (val & 0x000000FF);
  nu->dirty    = dirty;
  nu->unread   = unread;
  nu->writable = writable;

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


/*
* Given base address, return the register's value
*/
unsigned int I2CDeviceWithRegisters::regValue(uint8_t base_addr) {
  DeviceRegister *reg = getRegisterByBaseAddress(base_addr);
  if (reg == nullptr) {
    return 0;
  }
  else {
    unsigned int return_value = 0;
    for (uint8_t x = 0; x < reg->len; x++) {
      return_value = (return_value << 8) + *(reg->val + x);
    }

    // Use the register's length to truncate the value...
    unsigned int cull_val = (1 == reg->len) ? 0xFF : ((2 == reg->len) ? 0xFFFF : 0xFFFFFFFF);
    return (return_value & cull_val);
  }
}


/*
* Given base address, return the register's unread state.
*/
bool I2CDeviceWithRegisters::regUpdated(uint8_t base_addr) {
  DeviceRegister *nu = getRegisterByBaseAddress(base_addr);
  if (nu == nullptr) {
    return false;
  }
  else {
    return nu->unread;
  }
}

/*
* Given base address, mark this register as having been read.
*/
void I2CDeviceWithRegisters::markRegRead(uint8_t base_addr) {
  DeviceRegister *nu = getRegisterByBaseAddress(base_addr);
  if (nu) {
    nu->unread = false;
  }
}


// Override to support stacking up operations.
int8_t I2CDeviceWithRegisters::writeIndirect(uint8_t base_addr, unsigned int val) {
  return writeIndirect(base_addr, val, false);
}

// TODO: Needs a refactor. This is an experiment.
int8_t I2CDeviceWithRegisters::writeIndirect(uint8_t base_addr, unsigned int val, bool defer) {
  DeviceRegister *nu = getRegisterByBaseAddress(base_addr);
  if (nu == nullptr) return I2C_ERR_SLAVE_UNDEFD_REG;
  if (!nu->writable) return I2C_ERR_SLAVE_REG_IS_RO;
  switch (nu->len) {
    case 4:
      *((uint32_t*) nu->val) = val & 0xFFFFFFFF;
      break;
    case 2:
      *((uint16_t*) nu->val) = val & 0xFFFF;
      break;
    case 1:
      *((uint8_t*)  nu->val) = val & 0xFF;
      break;
    default:
      return I2C_ERR_SLAVE_REG_IS_RO;
  }
  nu->dirty = true;
  if (!defer) {
    writeDirtyRegisters();
  }
  return I2C_ERR_SLAVE_NO_ERROR;
}


int8_t I2CDeviceWithRegisters::writeRegister(uint8_t base_addr) {
  DeviceRegister *nu = getRegisterByBaseAddress(base_addr);
  if (nu == nullptr) return I2C_ERR_SLAVE_UNDEFD_REG;
  if (!nu->writable) return I2C_ERR_SLAVE_REG_IS_RO;

  else if (!writeX(nu->addr, nu->len, nu->val)) {
    #ifdef MANUVR_DEBUG
    Kernel::log("Bus error while writing device.\n");
    #endif
    return I2C_ERR_SLAVE_BUS_FAULT;
  }
  return I2C_ERR_SLAVE_NO_ERROR;
}

int8_t I2CDeviceWithRegisters::writeRegister(DeviceRegister *reg) {
  if (reg == nullptr) {
    return I2C_ERR_SLAVE_UNDEFD_REG;
  }
  if (!reg->writable) {
    return I2C_ERR_SLAVE_REG_IS_RO;
  }
  else if (!writeX(reg->addr, reg->len, reg->val)) {
    #ifdef MANUVR_DEBUG
    Kernel::log("Bus error while writing device.\n");
    #endif
    return I2C_ERR_SLAVE_BUS_FAULT;
  }
  return I2C_ERR_SLAVE_NO_ERROR;
}



/*
* base_addr     Device access will begin at the specified register index.
*/
int8_t I2CDeviceWithRegisters::readRegister(uint8_t base_addr) {
  DeviceRegister *reg = getRegisterByBaseAddress(base_addr);
  if (reg == nullptr) {
    return I2C_ERR_SLAVE_UNDEFD_REG;
  }
  if (!readX(reg->addr, reg->len, reg->val)) {
    #ifdef MANUVR_DEBUG
    Kernel::log("Bus error while reading device.\n");
    #endif
    return I2C_ERR_SLAVE_BUS_FAULT;
  }
  return I2C_ERR_SLAVE_NO_ERROR;
}

int8_t I2CDeviceWithRegisters::readRegister(DeviceRegister *reg) {
  if (reg == nullptr) {
    return I2C_ERR_SLAVE_UNDEFD_REG;
  }
  if (!readX(reg->addr, reg->len, reg->val)) {
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
    temp->concat("---<  Device registers  >---\n");
    for (int i = 0; i < reg_defs.size(); i++) {
      temp_reg = reg_defs.get(i);
      switch (temp_reg->len) {
        case 1:
          temp->concatf("\tReg 0x%02x   contains 0x%02x        %s %s %s\n", temp_reg->addr, *(temp_reg->val), (temp_reg->dirty ? "\tdirty":"\tclean"), (temp_reg->unread ? "\tunread":"\tknown"), (temp_reg->writable ? "\twritable":"\treadonly"));
          break;
        case 2:
          temp->concatf("\tReg 0x%02x   contains 0x%04x      %s %s %s\n", temp_reg->addr, *(temp_reg->val), (temp_reg->dirty ? "\tdirty":"\tclean"), (temp_reg->unread ? "\tunread":"\tknown"), (temp_reg->writable ? "\twritable":"\treadonly"));
          break;
        case 4:
        default:
          temp->concatf("\tReg 0x%02x   contains 0x%08x  %s %s %s\n", temp_reg->addr, *(temp_reg->val), (temp_reg->dirty ? "\tdirty":"\tclean"), (temp_reg->unread ? "\tunread":"\tknown"), (temp_reg->writable ? "\twritable":"\treadonly"));
          break;
      }
    }
  }
}


/*
* When the class is freshly-instantiated, we need to call this fxn do bring the class's idea of
*   register values into alignment with "what really is" in the device.
*/
int8_t I2CDeviceWithRegisters::syncRegisters(void) {
  DeviceRegister *temp = nullptr;
  int8_t return_value = I2C_ERR_SLAVE_NO_ERROR;
  uint8_t count = reg_defs.size();
  for (int i = 0; i < count; i++) {

    temp = reg_defs.get(i);
    if (temp == nullptr) {
      // Safety-check that an out-of-bounds reg wasn't in the list...
      return I2C_ERR_SLAVE_UNDEFD_REG;
    }

    return_value = readRegister(temp);
    if (return_value != I2C_ERR_SLAVE_NO_ERROR) {
      #ifdef MANUVR_DEBUG
      StringBuilder output;
      output.concatf("Failed to read from register %d\n", temp->addr);
      Kernel::log(&output);
      #endif
    }
  }

  return return_value;
}


/*
* Any registers marked dirty will be written to the device if the register is also marked wriatable.
*/
int8_t I2CDeviceWithRegisters::writeDirtyRegisters(void) {
  int8_t return_value = I2C_ERR_SLAVE_NO_ERROR;
  DeviceRegister *temp = nullptr;
  for (int i = 0; i < reg_defs.size(); i++) {
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
      //temp->dirty = false;  This ought to be set in the callback.  ---J. Ian Lindsay   Mon Dec 15 13:07:33 MST 2014
    }
  }
  return return_value;
}


int8_t I2CDeviceWithRegisters::io_op_callahead(BusOp* op) {
  // Default is to allow the transfer.
  return 0;
}


int8_t I2CDeviceWithRegisters::io_op_callback(BusOp* _op) {
  I2CBusOp* completed = (I2CBusOp*) _op;

  if (completed) {
    if (!completed->hasFault()) {
      DeviceRegister *nu = getRegisterByBaseAddress(completed->sub_addr);
      if (nu) {
        switch (completed->get_opcode()) {
          case BusOpcode::RX:
            nu->unread = true;
            //temp.concat("Buffer contents (read):  ");
            break;
          case BusOpcode::TX:
            nu->dirty = false;
            //temp.concat("Buffer contents (write):  ");
            break;
          case BusOpcode::TX_CMD:
            //temp.concat("Ping received.\n");
            break;
          default:
            break;
        }
      }
      else {
        #ifdef MANUVR_DEBUG
        Kernel::log("I2CDeviceWithRegisters::io_op_callback(): register lookup failed.\n");
        #endif
        return -1;
      }
    }
    else {
      #ifdef MANUVR_DEBUG
      Kernel::log("I2CDeviceWithRegisters::io_op_callback(): i2c operation errored.\n");
      #endif
      return -1;
    }
  }
  return 0;
}

#endif  // MANUVR_SUPPORT_I2C
