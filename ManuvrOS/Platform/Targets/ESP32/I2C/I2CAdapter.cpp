#include <Platform/Peripherals/I2C/I2CAdapter.h>

#if defined(MANUVR_SUPPORT_I2C)
#include "driver/i2c.h"


int8_t I2CAdapter::bus_init() {
  i2c_config_t conf;
  conf.mode             = I2C_MODE_MASTER;
  conf.sda_io_num       = _bus_opts.sda_pin;
  conf.scl_io_num       = _bus_opts.scl_pin;
  conf.sda_pullup_en    = GPIO_PULLUP_ENABLE;
  conf.scl_pullup_en    = GPIO_PULLUP_ENABLE;
  conf.master.clk_speed = 100000;

  switch (getAdapterId()) {
    case 0:
      i2c_param_config(I2C_NUM_0, &conf);
      i2c_driver_install(I2C_NUM_0, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
      busOnline(true);
      break;

    case 1:
      i2c_param_config(I2C_NUM_1, &conf);
      i2c_driver_install(I2C_NUM_1, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
      busOnline(true);
      break;

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
  output->concatf("-- I2C%d (%sline)\n", getAdapterId(), (_er_flag(I2C_BUS_FLAG_BUS_ONLINE)?"on":"OFF"));
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

  if ((nullptr != callback) && (callback->io_op_callahead(this))) {
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
  if (0 == device->getAdapterId()) {
  }
  else {
    abort(XferFault::BAD_PARAM);
    return XferFault::BAD_PARAM;
  }

  switch (wire_status) {
    case I2C_WAITING:
      markComplete();
      break;
    case I2C_ADDR_NAK:
      Kernel::log("DEV_NOT_FOUND\n");
      abort(XferFault::DEV_NOT_FOUND);
      return XferFault::DEV_NOT_FOUND;
    case I2C_DATA_NAK:
      Kernel::log("I2C_DATA_NAK\n");
      abort(XferFault::DEV_NOT_FOUND);
      return XferFault::DEV_NOT_FOUND;
    case I2C_ARB_LOST:
      abort(XferFault::BUS_BUSY);
      return XferFault::BUS_BUSY;
    case I2C_TIMEOUT:
      abort(XferFault::TIMEOUT);
      return XferFault::TIMEOUT;
    default:
      Kernel::log("Transfer failed with an unforseen error code.\n");
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
  return 0;
}

#endif  // MANUVR_SUPPORT_I2C
