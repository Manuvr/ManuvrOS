#include <Platform/Peripherals/I2C/I2CAdapter.h>

#if defined(MANUVR_SUPPORT_I2C)
#include <i2c_t3/i2c_t3.h>

I2CAdapter::I2CAdapter(uint8_t dev_id, uint8_t sda, uint8_t scl) : EventReceiver(), BusAdapter(12) {
  __class_initializer();
  dev = dev_id;

  switch (dev_id) {
    case 0:
      if (((18 == sda) && (19 == scl)) || ((17 == sda) && (16 == scl))) {
        Wire.begin(I2C_MASTER, 0x00,
          (17 == sda) ? I2C_PINS_16_17 : I2C_PINS_18_19,
          I2C_PULLUP_INT, I2C_RATE_400, I2C_OP_MODE_ISR);
        Wire.setDefaultTimeout(10000);   // We are willing to wait up to 10mS before failing an operation.
        busOnline(true);
        sda_pin = sda;
        scl_pin = scl;
      }
      break;
    #if defined(__MK20DX256__)
    case 1:
      if ((30 == sda) && (29 == scl)) {
        Wire1.begin(I2C_MASTER, 0x00, I2C_PINS_29_30, I2C_PULLUP_INT, I2C_RATE_400, I2C_OP_MODE_ISR);
        Wire1.setDefaultTimeout(10000);   // We are willing to wait up to 10mS before failing an operation.
        busOnline(true);
        sda_pin = sda;
        scl_pin = scl;
      }
      break;
    #endif  //__MK20DX256__
    default:
      Kernel::log("I2CAdapter unsupported.\n");
      break;
  }
}


I2CAdapter::I2CAdapter(uint8_t dev_id) : I2CAdapter(dev_id, 18, 17) {
}


I2CAdapter::~I2CAdapter() {
  __class_teardown();
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



  int8_t I2CAdapter::dispatchOperation(I2CBusOp* op) {
    // TODO: This is awful. Need to ultimately have a direct ref to the class that *is* the adapter.
    if (0 == dev) {
      Wire.beginTransmission((uint8_t) (op->dev_addr & 0x00FF));
      if (op->need_to_send_subaddr()) {
        Wire.write((uint8_t) (op->sub_addr & 0x00FF));
        op->advance_operation(1);
      }

      if (op->get_opcode() == BusOpcode::RX) {
        Wire.endTransmission(I2C_NOSTOP);
        Wire.requestFrom(op->dev_addr, op->buf_len, I2C_STOP, 10000);
        int i = 0;
        while(Wire.available()) {
          *(op->buf + i++) = (uint8_t) Wire.readByte();
        }
      }
      else if (op->get_opcode() == BusOpcode::TX) {
        for(int i = 0; i < op->buf_len; i++) Wire.write(*(op->buf+i));
        Wire.endTransmission(I2C_STOP, 10000);   // 10ms timeout
      }
      else if (op->get_opcode() == BusOpcode::TX_CMD) {
        Wire.endTransmission(I2C_STOP, 10000);   // 10ms timeout
      }

      switch (Wire.status()) {
        case I2C_WAITING:
          op->markComplete();
          break;
        case I2C_ADDR_NAK:
          op->abort(XferFault::DEV_NOT_FOUND);
          break;
        case I2C_DATA_NAK:
          op->abort(XferFault::DEV_NOT_FOUND);
          break;
        case I2C_ARB_LOST:
          op->abort(XferFault::BUS_BUSY);
          break;
        case I2C_TIMEOUT:
          op->abort(XferFault::TIMEOUT);
          break;
      }
    }
#if defined(__MK20DX256__)
  else if (1 == dev) {
    Wire1.beginTransmission((uint8_t) (op->dev_addr & 0x00FF));
    if (op->need_to_send_subaddr()) {
      Wire1.write((uint8_t) (op->sub_addr & 0x00FF));
      op->advance_operation(1);
    }

    if (op->get_opcode() == BusOpcode::RX) {
      Wire1.endTransmission(I2C_NOSTOP);
      Wire1.requestFrom(op->dev_addr, op->buf_len, I2C_STOP, 10000);
      int i = 0;
      while(Wire1.available()) {
        *(op->buf + i++) = (uint8_t) Wire1.readByte();
      }
    }
    else if (op->get_opcode() == BusOpcode::TX) {
      for(int i = 0; i < op->buf_len; i++) Wire1.write(*(op->buf+i));
      Wire1.endTransmission(I2C_STOP, 10000);   // 10ms timeout
    }
    else if (op->get_opcode() == BusOpcode::TX_CMD) {
      Wire1.endTransmission(I2C_STOP, 10000);   // 10ms timeout
    }

    switch (Wire1.status()) {
      case I2C_WAITING:
        op->markComplete();
        break;
      case I2C_ADDR_NAK:
        op->abort(XferFault::DEV_NOT_FOUND);
        break;
      case I2C_DATA_NAK:
        op->abort(XferFault::DEV_NOT_FOUND);
        break;
      case I2C_ARB_LOST:
        op->abort(XferFault::BUS_BUSY);
        break;
      case I2C_TIMEOUT:
        op->abort(XferFault::TIMEOUT);
        break;
    }
  }
#endif
  else {
  }
  return 0;
}



/*
* Private function that will switch the addressed i2c device via ioctl. This
*   function is meaningless on anything but a linux system, in which case it
*   will always return true;
* On a linux system, this will only return true if the ioctl call succeeded.
*/
bool I2CAdapter::switch_device(uint8_t nu_addr) {
  return true;
}

#endif  // MANUVR_SUPPORT_I2C
