#include <I2CAdapter.h>

#if defined(CONFIG_MANUVR_I2C)
#include <i2c_t3/i2c_t3.h>

#define MANUVR_I2C_WRAP_TO 1000000

/*******************************************************************************
* ___     _                                  This is a template class for
*  |   / / \ o    /\   _|  _. ._ _|_  _  ._  defining arbitrary I/O adapters.
* _|_ /  \_/ o   /--\ (_| (_| |_) |_ (/_ |   Adapters must be instanced with
*                             |              a BusOp as the template param.
*******************************************************************************/

int8_t I2CAdapter::bus_init() {
  switch (getAdapterId()) {
    case 0:
      if (((18 == _bus_opts.sda_pin) && (19 == _bus_opts.scl_pin)) || ((17 == _bus_opts.sda_pin) && (16 == _bus_opts.scl_pin))) {
        Wire.begin(I2C_MASTER, 0x00,
          (17 == _bus_opts.sda_pin) ? I2C_PINS_16_17 : I2C_PINS_18_19,
          _bus_opts.usePullups() ? I2C_PULLUP_INT : I2C_PULLUP_EXT,
          _bus_opts.freq, I2C_OP_MODE_DMA
        );
        Wire.setDefaultTimeout(MANUVR_I2C_WRAP_TO);   // We are willing to wait up to 10mS before failing an operation.
        busOnline(true);
      }
      break;
    #if defined(__MK20DX256__)
    case 1:
      if ((30 == _bus_opts.sda_pin) && (29 == _bus_opts.scl_pin)) {
        Wire1.begin(
          I2C_MASTER, 0x00,
          I2C_PINS_29_30,
          _bus_opts.usePullups() ? I2C_PULLUP_INT : I2C_PULLUP_EXT,
          _bus_opts.freq, I2C_OP_MODE_DMA
        );
        Wire1.setDefaultTimeout(MANUVR_I2C_WRAP_TO);   // We are willing to wait up to 10mS before failing an operation.
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


#if defined(MANUVR_DEBUG)
void I2CAdapter::printHardwareState(StringBuilder* output) {
  output->concatf("-- I2C%d (%sline)\n", getAdapterId(), (_er_flag(I2C_BUS_FLAG_BUS_ONLINE)?"on":"OFF"));
}
#endif


int8_t I2CAdapter::generateStart() {
  return busOnline() ? 0 : -1;
}


int8_t I2CAdapter::generateStop() {
  return busOnline() ? 0 : -1;
}


/*******************************************************************************
* ___     _                              These members are mandatory overrides
*  |   / / \ o     |  _  |_              from the BusOp class.
* _|_ /  \_/ o   \_| (_) |_)
*******************************************************************************/

XferFault I2CBusOp::begin() {
  if (device) {
    if ((nullptr == callback) || (0 == callback->io_op_callahead(this))) {
      set_state(XferState::INITIATE);
      // TODO: It would be better to actually use this in interrupt mode.
      advance(0);
    }
    else {
      abort(XferFault::IO_RECALL);
    }
  }
  else {
    abort(XferFault::DEV_NOT_FOUND);
  }
  return xfer_fault;
}


/*
*/
XferFault I2CBusOp::advance(uint32_t status_reg) {
  xfer_state = XferState::ADDR;
  if (device->generateStart()) {
    // Failure to generate START condition.
    abort(XferFault::BUS_BUSY);
    return xfer_fault;
  }

  int wire_status = -1;
  int i = 0;
  i2c_t3* wire_ref = nullptr;

  switch (device->getAdapterId()) {
    case 0:
      wire_ref = &Wire;
      break;

    #if defined(__MK20DX256__)
    case 1:
      wire_ref = &Wire1;
      break;
    #endif

    default:
      abort(XferFault::BAD_PARAM);
      return xfer_fault;
  }

  wire_ref->beginTransmission((uint8_t) (dev_addr & 0x00FF));
  if (need_to_send_subaddr()) {
    wire_ref->write((uint8_t) (sub_addr & 0x00FF));
    subaddr_sent(true);
  }

  switch (get_opcode()) {
    case BusOpcode::RX:
      wire_ref->endTransmission(I2C_NOSTOP);
      wire_ref->requestFrom((uint8_t) (dev_addr & 0x00FF), buf_len, I2C_STOP, MANUVR_I2C_WRAP_TO);
      while(wire_ref->available()) {
        *(buf + i++) = wire_ref->readByte();
      }
      break;
    case BusOpcode::TX:
      wire_ref->write(buf, buf_len);
      // NOTE: No break
    case BusOpcode::TX_CMD:
      wire_ref->endTransmission(I2C_STOP, MANUVR_I2C_WRAP_TO);   // 10ms timeout
      break;
    default:
      break;
  }

  // TODO: If I can't find a way to get IRQ callbacks, I will hard-fork the
  //   library. But I really want to avoid that.
  while (!wire_ref->done()) { }
  wire_status = wire_ref->status();

  switch (wire_status) {
    case I2C_WAITING:
      markComplete();
      break;
    case I2C_ADDR_NAK:
      Kernel::log("DEV_NOT_FOUND\n");
      abort(XferFault::DEV_NOT_FOUND);
      break;
    case I2C_DATA_NAK:
      Kernel::log("I2C_DATA_NAK\n");
      abort(XferFault::DEV_NOT_FOUND);
      break;
    case I2C_ARB_LOST:
      abort(XferFault::BUS_BUSY);
      break;
    case I2C_TIMEOUT:
      abort(XferFault::TIMEOUT);
      break;
    default:
      Kernel::log("Transfer failed with an unforseen error code.\n");
      abort(XferFault::NO_REASON);
      break;
  }
  return xfer_fault;
}

#endif  // CONFIG_MANUVR_I2C
