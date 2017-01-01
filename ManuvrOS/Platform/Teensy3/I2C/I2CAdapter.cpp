#include <Platform/Peripherals/I2C/I2CAdapter.h>

#if defined(MANUVR_SUPPORT_I2C)
#include <i2c_t3/i2c_t3.h>


int8_t I2CAdapter::bus_init() {
  switch (dev) {
    case 0:
      if (((18 == sda_pin) && (19 == scl_pin)) || ((17 == sda_pin) && (16 == scl_pin))) {
        Wire.begin(I2C_MASTER, 0x00,
          (17 == sda_pin) ? I2C_PINS_16_17 : I2C_PINS_18_19,
          I2C_PULLUP_INT, I2C_RATE_400, I2C_OP_MODE_ISR
        );
        Wire.setDefaultTimeout(10000);   // We are willing to wait up to 10mS before failing an operation.
        busOnline(true);
      }
      break;
    #if defined(__MK20DX256__)
    case 1:
      if ((30 == sda_pin) && (29 == scl_pin)) {
        Wire1.begin(I2C_MASTER, 0x00, I2C_PINS_29_30, I2C_PULLUP_INT, I2C_RATE_400, I2C_OP_MODE_ISR);
        Wire1.setDefaultTimeout(10000);   // We are willing to wait up to 10mS before failing an operation.
        busOnline(true);
      }
      break;
    #endif  //__MK20DX256__
    default:
      Kernel::log("I2CAdapter unsupported.\n");
      break;
  }
  return (busOnline() ? 0:-1);
}


int8_t I2CAdapter::bus_deinit() {
  // TODO: This.
  return 0;
}



void I2CAdapter::printHardwareState(StringBuilder* output) {
  output->concatf("-- I2C%d (%sline) --------------------\n", dev, (_er_flag(I2C_BUS_FLAG_BUS_ONLINE)?"on":"OFF"));
}


int8_t I2CAdapter::generateStart() {
  return busOnline() ? 0 : -1;
}


int8_t I2CAdapter::generateStop() {
  return busOnline() ? 0 : -1;
}



XferFault I2CBusOp::begin() {
  if (nullptr == device) {
    abort(XferFault::DEV_NOT_FOUND);
    return XferFault::DEV_NOT_FOUND;
  }

  if ((nullptr != callback) && !((I2CDevice*)callback)->operationCallahead(this)) {
    abort(XferFault::IO_RECALL);
    return XferFault::IO_RECALL;
  }

  xfer_state = XferState::ADDR;
  if (device->generateStart()) {
    // Failure to generate START condition.
    abort(XferFault::BUS_BUSY);
    return XferFault::BUS_BUSY;
  }


  int wire_status = -1;
  int i = 0;
  if (0 == device->getDevId()) {
    Wire.beginTransmission((uint8_t) (dev_addr & 0x00FF));
    if (need_to_send_subaddr()) {
      Wire.write((uint8_t) (sub_addr & 0x00FF));
      advance_operation(1);
    }

    switch (get_opcode()) {
      case BusOpcode::RX:
        Wire.endTransmission(I2C_NOSTOP);
        Wire.requestFrom(dev_addr, buf_len, I2C_STOP, 10000);
        while(Wire.available()) {
          *(buf + i++) = (uint8_t) Wire.readByte();
        }
        break;
      case BusOpcode::TX:
        for(int i = 0; i < buf_len; i++) Wire.write(*(buf+i));
        Wire.endTransmission(I2C_STOP, 10000);   // 10ms timeout
        break;
      case BusOpcode::TX_CMD:
        Wire.endTransmission(I2C_STOP, 10000);   // 10ms timeout
        break;
      default:
        break;
    }

    wire_status = Wire.status();
  }
  #if defined(__MK20DX256__)
  else if (1 == device->getDevId()) {
    Wire1.beginTransmission((uint8_t) (dev_addr & 0x00FF));
    if (need_to_send_subaddr()) {
      Wire1.write((uint8_t) (sub_addr & 0x00FF));
      advance_operation(1);
    }

    switch (get_opcode()) {
      case BusOpcode::RX:
        Wire1.endTransmission(I2C_NOSTOP);
        Wire1.requestFrom(dev_addr, buf_len, I2C_STOP, 10000);
        while(Wire1.available()) {
          *(buf + i++) = (uint8_t) Wire1.readByte();
        }
        break;
      case BusOpcode::TX:
        for(int i = 0; i < buf_len; i++) Wire1.write(*(buf+i));
        Wire1.endTransmission(I2C_STOP, 10000);   // 10ms timeout
        break;
      case BusOpcode::TX_CMD:
        Wire1.endTransmission(I2C_STOP, 10000);   // 10ms timeout
        break;
      default:
        break;
    }

    wire_status = Wire1.status();
  }
  #endif  // __MK20DX256__
  else {
    abort(XferFault::BAD_PARAM);
    return XferFault::BAD_PARAM;
  }

  switch (wire_status) {
    case I2C_WAITING:
      markComplete();
      break;
    case I2C_ADDR_NAK:
      abort(XferFault::DEV_NOT_FOUND);
      return XferFault::DEV_NOT_FOUND;
    case I2C_DATA_NAK:
      abort(XferFault::DEV_NOT_FOUND);
      return XferFault::DEV_NOT_FOUND;
    case I2C_ARB_LOST:
      abort(XferFault::BUS_BUSY);
      return XferFault::BUS_BUSY;
    case I2C_TIMEOUT:
      abort(XferFault::TIMEOUT);
      return XferFault::TIMEOUT;
    default:
      Kernel::log("Trasnsfer failed with an unforseen error code.\n");
      abort(XferFault::NO_REASON);
      return XferFault::NO_REASON;
  }
  return XferFault::NONE;
}


/*
* Linux doesn't have a concept of interrupt, but we might call this
*   from an I/O thread.
*/
int8_t I2CBusOp::advance_operation(uint32_t status_reg) {
  switch (status_reg) {
    case 1:
      subaddr_sent(true);
      break;
  }
  return 0;
}

#endif  // MANUVR_SUPPORT_I2C
