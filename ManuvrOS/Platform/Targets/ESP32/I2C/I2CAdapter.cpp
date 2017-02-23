#include <Platform/Peripherals/I2C/I2CAdapter.h>

#if defined(MANUVR_SUPPORT_I2C)
#include "driver/i2c.h"

#define ACK_CHECK_EN   0x1     /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS  0x0     /*!< I2C master will not check ack from slave */


void i2c_worker_thread(void* arg) {
  while (1) {
    vTaskDelay(10000 / portTICK_RATE_MS);
  }
}



int8_t I2CAdapter::bus_init() {
  i2c_config_t conf;
  conf.mode             = I2C_MODE_MASTER;
  conf.sda_io_num       = (gpio_num_t) _bus_opts.sda_pin;
  conf.scl_io_num       = (gpio_num_t) _bus_opts.scl_pin;
  conf.sda_pullup_en    = GPIO_PULLUP_ENABLE;
  conf.scl_pullup_en    = GPIO_PULLUP_ENABLE;
  conf.master.clk_speed = 100000;

  switch (getAdapterId()) {
    case 0:
      i2c_param_config(I2C_NUM_0, &conf);
      i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
      xTaskCreate(i2c_worker_thread, "i2c_thread", 1024 * 2, (void* ) 0, 10, NULL);
      busOnline(true);
      break;

    case 1:
      i2c_param_config(I2C_NUM_1, &conf);
      i2c_driver_install(I2C_NUM_1, conf.mode, 0, 0, 0);
      xTaskCreate(i2c_worker_thread, "i2c_thread", 1024 * 2, (void* ) 0, 10, NULL);
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

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  int wire_status = -1;
  int i = 0;
  if (0 == (i2c_port_t) device->getAdapterId()) {
    i2c_master_start(cmd);

    switch (get_opcode()) {
      case BusOpcode::RX:
        i2c_master_write_byte(cmd, ((uint8_t) (dev_addr & 0x00FF) << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
        if (need_to_send_subaddr()) {
          i2c_master_write_byte(cmd, (uint8_t) (sub_addr & 0x00FF), ACK_CHECK_EN);
        }
        i2c_master_read(cmd, buf, (size_t) buf_len, ACK_CHECK_EN);
        break;
      case BusOpcode::TX:
        i2c_master_write_byte(cmd, ((uint8_t) (dev_addr & 0x00FF) << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
        if (need_to_send_subaddr()) {
          i2c_master_write_byte(cmd, (uint8_t) (sub_addr & 0x00FF), ACK_CHECK_EN);
        }
        i2c_master_write(cmd, buf, (size_t) buf_len, ACK_CHECK_EN);
        break;
      case BusOpcode::TX_CMD:
        i2c_master_write_byte(cmd, ((uint8_t) (dev_addr & 0x00FF) << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
        break;
      default:
        i2c_cmd_link_delete(cmd);
        abort(XferFault::BAD_PARAM);
        return XferFault::BAD_PARAM;
    }
    i2c_master_stop(cmd);

    int ret = i2c_master_cmd_begin((i2c_port_t) device->getAdapterId(), cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_FAIL) {
      abort(XferFault::BUS_FAULT);
      return XferFault::BUS_FAULT;
    }
    else {
      markComplete();
    }
  }
  else {
    abort(XferFault::BAD_PARAM);
    return XferFault::BAD_PARAM;
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
