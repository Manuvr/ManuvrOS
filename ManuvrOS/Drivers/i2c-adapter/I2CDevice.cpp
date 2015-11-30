#include "i2c-adapter.h"

/*
* Constructor takes a pointer to the bus we are going to be using, and the slave
*   address of the implementing class.
*/
I2CDevice::I2CDevice(void) {
  _dev_addr = 0;
  _bus      = NULL;
};


/*
* Destructor doesn't have any memory to free, but we should tell the bus that we
*   are going away.
*/
I2CDevice::~I2CDevice(void) {
  if (_bus == NULL) {
    _bus->removeSlaveDevice(this);
    _bus = NULL;
  }
};



void I2CDevice::operationCompleteCallback(I2CQueuedOperation* completed) {
	StringBuilder temp;
	if (completed != NULL) {
	  #ifdef __MANUVR_DEBUG
		if (completed->opcode == I2C_OPERATION_READ) {
			temp.concatf("Default callback (I2CDevice)\tReceived %d bytes from i2c slave.\n", (completed->len - completed->remaining_bytes));
		}
		else {
			temp.concatf("Default callback (I2CDevice)\tSent %d/%d bytes to i2c slave.\n", (completed->len - completed->remaining_bytes), completed->len);
		}
		#endif
		completed->printDebug(&temp);
	}
	if (temp.length() > 0) StaticHub::log(&temp);
}

/* If your device needs something to happen immediately prior to bus I/O... */
bool I2CDevice::operationCallahead(I2CQueuedOperation* op) {
  // Default behavior is to return true, to tell the bus "Go Ahead".
  return true;
}


bool I2CDevice::assignBusInstance(volatile I2CAdapter *bus) { // Trivial override.
  return assignBusInstance((I2CAdapter *)bus);
};

// Needs to be called by the i2c class during insertion.
bool I2CDevice::assignBusInstance(I2CAdapter *adapter) {
  if (_bus == NULL) {
    _bus = adapter;
    return true;
  }
  return false;
};


// This is to be called from the adapter's unassignment function.
bool I2CDevice::disassignBusInstance(void) {
  _bus = NULL;
  return true;
};



/****************************************************************************************************
* These functions are protected bus-access fxns that the dev will use to do its thing.              *
****************************************************************************************************/

/*
  This function sends the MSB first.
*/
bool I2CDevice::write16(int sub_addr, uint16_t dat) {
    if (_bus == NULL) {
      #ifdef __MANUVR_DEBUG
      StaticHub::log(__PRETTY_FUNCTION__, 2, "No bus assignment (i2c addr 0x%02x).", _dev_addr);
      #endif
      return false;
    }
    uint8_t* temp = (uint8_t*) malloc(2);
    *(temp + 0) = (dat >> 8);
    *(temp + 1) = (uint8_t) (dat & 0x00FF);
    I2CQueuedOperation* nu = new I2CQueuedOperation(I2C_OPERATION_WRITE, _dev_addr, (int16_t) sub_addr, temp, 2);
    nu->requester = this;
    return _bus->insert_work_item(nu);
}


bool I2CDevice::write8(uint8_t dat) {
    if (_bus == NULL) {
      #ifdef __MANUVR_DEBUG
      StaticHub::log(__PRETTY_FUNCTION__, 2, "No bus assignment (i2c addr 0x%02x).", _dev_addr);
      #endif
      return false;
    }
    uint8_t* temp = (uint8_t*) malloc(1);
    *(temp + 0) = dat;
    I2CQueuedOperation* nu = new I2CQueuedOperation(I2C_OPERATION_WRITE, _dev_addr, (int16_t) -1, temp, 1);
    nu->requester = this;
    return _bus->insert_work_item(nu);
}


bool I2CDevice::write8(int sub_addr, uint8_t dat) {
    if (_bus == NULL) {
      #ifdef __MANUVR_DEBUG
      StaticHub::log(__PRETTY_FUNCTION__, 2, "No bus assignment (i2c addr 0x%02x).", _dev_addr);
      #endif
      return false;
    }
    uint8_t* temp = (uint8_t*) malloc(1);
    *(temp + 0) = dat;
    I2CQueuedOperation* nu = new I2CQueuedOperation(I2C_OPERATION_WRITE, _dev_addr, (int16_t) sub_addr, temp, 1);
    nu->requester = this;
    return _bus->insert_work_item(nu);
}



bool I2CDevice::writeX(int sub_addr, uint16_t byte_count, uint8_t *buf) {
    if (_bus == NULL) {
      #ifdef __MANUVR_DEBUG
      StaticHub::log(__PRETTY_FUNCTION__, 2, "No bus assignment (i2c addr 0x%02x).", _dev_addr);
      #endif
      return false;
    }
    I2CQueuedOperation* nu = new I2CQueuedOperation(I2C_OPERATION_WRITE, _dev_addr, (int16_t) sub_addr, buf, byte_count);
    nu->reap_buffer = false;
    nu->requester = this;
    return _bus->insert_work_item(nu);
}





bool I2CDevice::readX(int sub_addr, uint8_t len, uint8_t *buf) {
    if (_bus == NULL) {
      #ifdef __MANUVR_DEBUG
      StaticHub::log(__PRETTY_FUNCTION__, 2, "No bus assignment (i2c addr 0x%02x).", _dev_addr);
      #endif
      return false;
    }
    I2CQueuedOperation* nu = new I2CQueuedOperation(I2C_OPERATION_READ, _dev_addr, (int16_t) sub_addr, buf, len);
    nu->reap_buffer = false;
    nu->requester = this;
    return _bus->insert_work_item(nu);
}



bool I2CDevice::read8(int sub_addr) {
    if (_bus == NULL) {
      #ifdef __MANUVR_DEBUG
      StaticHub::log(__PRETTY_FUNCTION__, 2, "No bus assignment (i2c addr 0x%02x).", _dev_addr);
      #endif
      return false;
    }
    uint8_t* temp = (uint8_t*) malloc(1);
    *(temp + 0) = 0x00;
    I2CQueuedOperation* nu = new I2CQueuedOperation(I2C_OPERATION_READ, _dev_addr, (int16_t) sub_addr, temp, 1);
    nu->requester = this;
    return _bus->insert_work_item(nu);
}


bool I2CDevice::read8(void) {
    if (_bus == NULL) {
      #ifdef __MANUVR_DEBUG
      StaticHub::log(__PRETTY_FUNCTION__, 2, "No bus assignment (i2c addr 0x%02x).", _dev_addr);
      #endif
      return false;
    }
    uint8_t* temp = (uint8_t*) malloc(1);
    *(temp + 0) = 0x00;
    I2CQueuedOperation* nu = new I2CQueuedOperation(I2C_OPERATION_READ, _dev_addr, (int16_t) -1, temp, 1);
    nu->requester = this;
    return _bus->insert_work_item(nu);
}


/*
  This function assumes the MSB is being read first.
*/
bool I2CDevice::read16(int sub_addr) {
    if (_bus == NULL) {
      #ifdef __MANUVR_DEBUG
      StaticHub::log(__PRETTY_FUNCTION__, 2, "No bus assignment (i2c addr 0x%02x).", _dev_addr);
      #endif
      return false;
    }
    uint8_t* temp = (uint8_t*) malloc(2);
    *(temp + 0) = 0x00;
    *(temp + 1) = 0x00;
    I2CQueuedOperation* nu = new I2CQueuedOperation(I2C_OPERATION_READ, _dev_addr, (int16_t) sub_addr, temp, 2);
    nu->requester = this;
    return _bus->insert_work_item(nu);
}


/*
  This function assumes the MSB is being read first.
*/
bool I2CDevice::read16(void) {
    if (_bus == NULL) {
      #ifdef __MANUVR_DEBUG
      StaticHub::log(__PRETTY_FUNCTION__, 2, "No bus assignment (i2c addr 0x%02x).", _dev_addr);
      #endif
      return false;
    }
    uint8_t* temp = (uint8_t*) malloc(2);
    *(temp + 0) = 0x00;
    *(temp + 1) = 0x00;
    I2CQueuedOperation* nu = new I2CQueuedOperation(I2C_OPERATION_READ, _dev_addr, (int16_t) -1, temp, 2);
    nu->requester = this;
    return _bus->insert_work_item(nu);
}




/****************************************************************************************************
* These functions are for logging support.                                                          *
****************************************************************************************************/

/**
* Debug support method. This fxn is only present in debug builds. 
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void I2CDevice::printDebug(StringBuilder* temp) {
  if (temp != NULL) {
    temp->concatf("\n+++ I2CDevice  0x%02x ++++ Bus %sassigned +++++++++++++++++++++++++++\n", _dev_addr, (_bus == NULL ? "un" : ""));
  }
}

