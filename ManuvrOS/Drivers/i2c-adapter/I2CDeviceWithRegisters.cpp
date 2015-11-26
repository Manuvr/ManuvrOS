#include "i2c-adapter.h"



I2CDeviceWithRegisters::I2CDeviceWithRegisters(void) : I2CDevice() {
  multi_access_support = false;
}


I2CDeviceWithRegisters::~I2CDeviceWithRegisters(void) {
  DeviceRegister *temp = NULL;
  while (reg_defs.hasNext()) {
    temp = reg_defs.get();
    reg_defs.remove();
    //if (NULL == pooled_registers) {   // If we don't have pooled registers...
    //  if (temp->val != NULL) {        // ...and our buffer slot is populated...
    //    free(temp->val);              // Free the buffer space.
    //  }
    //}
    delete temp;      // Delete the register definition.
  }
  
  //if (NULL != pooled_registers) {   // If we DO have pooled registers...
  //  free(pooled_registers);         // ..free them at this point.
  //  pooled_registers = NULL;
  //}
}



bool I2CDeviceWithRegisters::sync(void) {
	bool return_value = (syncRegisters() == I2C_ERR_CODE_NO_ERROR);
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
  while (nu != NULL) {
    if (nu->addr == b_addr) {
      return nu;
    }
    i++;
    nu = reg_defs.get(i);
  }
  return NULL;
}


/*
* Given base address, return the register's value
*/
unsigned int I2CDeviceWithRegisters::regValue(uint8_t base_addr) {
  DeviceRegister *reg = getRegisterByBaseAddress(base_addr);
  if (reg == NULL) {
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
  if (nu == NULL) {
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
  if (nu != NULL) {
    nu->unread = false;
  }
}


// Override to support stacking up operations.
int8_t I2CDeviceWithRegisters::writeIndirect(uint8_t base_addr, uint8_t val) {
  return writeIndirect(base_addr, val, false);
}

// TODO: Needs a refactor. This is an experiment.
int8_t I2CDeviceWithRegisters::writeIndirect(uint8_t base_addr, uint8_t val, bool defer) {
  DeviceRegister *nu = getRegisterByBaseAddress(base_addr);
  if (nu == NULL)    return I2C_ERR_SLAVE_UNDEFD_REG;
  if (!nu->writable) return I2C_ERR_SLAVE_REG_IS_RO;
  
  *(nu->val) = val;
  nu->dirty = true;
  if (!defer) {
    writeDirtyRegisters();
  }
  return I2C_ERR_CODE_NO_ERROR;
}


int8_t I2CDeviceWithRegisters::writeRegister(uint8_t base_addr) {
  DeviceRegister *nu = getRegisterByBaseAddress(base_addr);
  if (nu == NULL)    return I2C_ERR_SLAVE_UNDEFD_REG;
  if (!nu->writable) return I2C_ERR_SLAVE_REG_IS_RO;

  else if (!writeX(nu->addr, nu->len, nu->val)) {
    StaticHub::log(__PRETTY_FUNCTION__, 1, "Bus error while writing device.");
    return I2C_ERR_CODE_BUS_FAULT;
  }
  return I2C_ERR_CODE_NO_ERROR;
}

int8_t I2CDeviceWithRegisters::writeRegister(DeviceRegister *reg) {
  if (reg == NULL) {
    return I2C_ERR_SLAVE_UNDEFD_REG;
  }
  if (!reg->writable) {
    return I2C_ERR_SLAVE_REG_IS_RO;
  }
  else if (!writeX(reg->addr, reg->len, reg->val)) {
    StaticHub::log(__PRETTY_FUNCTION__, 1, "Bus error while writing device.");
    return I2C_ERR_CODE_BUS_FAULT;
  }
  return I2C_ERR_CODE_NO_ERROR;
}



/*
* base_addr     Device access will begin at the specified register index.
*/
int8_t I2CDeviceWithRegisters::readRegister(uint8_t base_addr) {
  DeviceRegister *reg = getRegisterByBaseAddress(base_addr);
  if (reg == NULL) {
    return I2C_ERR_SLAVE_UNDEFD_REG;
  }
  if (!readX(reg->addr, reg->len, reg->val)) {
    StaticHub::log(__PRETTY_FUNCTION__, 1, "Bus error while reading device.");
    return I2C_ERR_CODE_BUS_FAULT;
  }
  return I2C_ERR_CODE_NO_ERROR;
}

int8_t I2CDeviceWithRegisters::readRegister(DeviceRegister *reg) {
  if (reg == NULL) {
    return I2C_ERR_SLAVE_UNDEFD_REG;
  }
  if (!readX(reg->addr, reg->len, reg->val)) {
    StaticHub::log(__PRETTY_FUNCTION__, 1, "Bus error while reading device.");
    return I2C_ERR_CODE_BUS_FAULT;
  }
  return I2C_ERR_CODE_NO_ERROR;
}


/*
* Dump the contents of this device to the logger.
*/
void I2CDeviceWithRegisters::printDebug(StringBuilder* temp) {
  if (temp != NULL) {
    I2CDevice::printDebug(temp);
    DeviceRegister *temp_reg = NULL;
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
	DeviceRegister *temp = NULL;
	int8_t return_value = I2C_ERR_CODE_NO_ERROR;
	uint8_t count = reg_defs.size();
	for (int i = 0; i < count; i++) {
	  
		temp = reg_defs.get(i);
		if (temp == NULL) {
			// Safety-check that an out-of-bounds reg wasn't in the list...
			return I2C_ERR_SLAVE_UNDEFD_REG;
		}
		
		return_value = readRegister(temp);
		if (return_value != I2C_ERR_CODE_NO_ERROR) {
			StringBuilder output;
			output.concatf("Failed to read from register %d\n", temp->addr);
			StaticHub::log(&output);
		}
	}

	return return_value;
}


/*
* Any registers marked dirty will be written to the device if the register is also marked wriatable.
*/
int8_t I2CDeviceWithRegisters::writeDirtyRegisters(void) {
	int8_t return_value = I2C_ERR_CODE_NO_ERROR;
	DeviceRegister *temp = NULL;
	for (int i = 0; i < reg_defs.size(); i++) {
		temp = reg_defs.get(i);
		if (temp->dirty) {
			if (temp->writable) {
				return_value = writeRegister(temp);
				if (return_value != I2C_ERR_CODE_NO_ERROR) {
					StaticHub::log(__PRETTY_FUNCTION__, 2, "Failed to write dirty register %d with code(%d). The dropped data was (%d). Aborting...", temp->addr, return_value, temp->val);
					return return_value;
				}
			}
			else {
				StaticHub::log(__PRETTY_FUNCTION__, 3, "Uh oh... register %d was marked dirty but it isn't writable. Marking clean with no write...", temp->addr);
			}
			//temp->dirty = false;  This ought to be set in the callback.  ---J. Ian Lindsay   Mon Dec 15 13:07:33 MST 2014
		}
	}
	return return_value;
}


void I2CDeviceWithRegisters::operationCompleteCallback(I2CQueuedOperation* completed) {
	StringBuilder temp; //("Default callback for registers!\n");
	if (completed != NULL) {
		if (completed->err_code == I2C_ERR_CODE_NO_ERROR) {
			DeviceRegister *nu = getRegisterByBaseAddress(completed->sub_addr);
			if (nu != NULL) {
				switch (completed->opcode) {
				  case I2C_OPERATION_READ:
				    nu->unread = true;
				    //temp.concat("Buffer contents (read):  ");
				    break;
				  case I2C_OPERATION_WRITE:
				    nu->dirty = false;
				    //temp.concat("Buffer contents (write):  ");
				    break;
				  case I2C_OPERATION_PING:
				    //temp.concat("Ping received.\n");
				    break;
				  default:
				    break;
				}
			}
			else {
				temp.concatf("Failed to lookup the register for a callback operation.\n");
				completed->printDebug(&temp);
			}
		}
		else {
			temp.concatf("i2c operation errored.\n");
			completed->printDebug(&temp);
		}
	}

	if (temp.length() > 0) {    StaticHub::log(&temp);  }
}


/* If your device needs something to happen immediately prior to bus I/O... */
bool I2CDeviceWithRegisters::operationCallahead(I2CQueuedOperation* op) {
  // Default behavior is to return true, to tell the bus "Go Ahead".
  return true;
}


