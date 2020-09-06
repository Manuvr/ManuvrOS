#include <I2CAdapter.h>
#include <Platform/Platform.h>

#if defined(CONFIG_MANUVR_I2C)
#include "driver/i2c.h"
#include "esp_task_wdt.h"

#define ACK_CHECK_EN   0x01     /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS  0x00     /*!< I2C master will not check ack from slave */

static volatile I2CBusOp* _threaded_op[2] = {nullptr, nullptr};

TaskHandle_t static_i2c_thread_id = 0;

static void* IRAM_ATTR i2c_worker_thread(void* arg) {
  while (!platform.nominalState()) {
    sleep_ms(20);
  }
  I2CAdapter* BUSPTR = (I2CAdapter*) arg;
  uint8_t anum = BUSPTR->adapterNumber();
  while (1) {
    if (nullptr != _threaded_op[anum]) {
      I2CBusOp* op = (I2CBusOp*) _threaded_op[anum];
      op->advance(0);
      _threaded_op[anum] = nullptr;
      yieldThread();
    }
    else {
      suspendThread();
      //ulTaskNotifyTake(pdTRUE, 10000 / portTICK_RATE_MS);
    }
  }
  return nullptr;
}




/*******************************************************************************
* ___     _                                  This is a template class for
*  |   / / \ o    /\   _|  _. ._ _|_  _  ._  defining arbitrary I/O adapters.
* _|_ /  \_/ o   /--\ (_| (_| |_) |_ (/_ |   Adapters must be instanced with
*                             |              a BusOp as the template param.
*******************************************************************************/

int8_t I2CAdapter::bus_init() {
  i2c_config_t conf;
  conf.mode             = I2C_MODE_MASTER;  // TODO: We only support master mode right now.
  conf.sda_io_num       = (gpio_num_t) _bus_opts.sda_pin;
  conf.scl_io_num       = (gpio_num_t) _bus_opts.scl_pin;
  conf.sda_pullup_en    = _bus_opts.sdaPullup() ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
  conf.scl_pullup_en    = _bus_opts.sclPullup() ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
  conf.master.clk_speed = _bus_opts.freq;
  int a_id = adapterNumber();
  switch (a_id) {
    case 0:
    case 1:
      if (ESP_OK == i2c_param_config(((0 == a_id) ? I2C_NUM_0 : I2C_NUM_1), &conf)) {
        if (ESP_OK == i2c_driver_install(((0 == a_id) ? I2C_NUM_0 : I2C_NUM_1), conf.mode, 0, 0, 0)) {
          ManuvrThreadOptions topts;
          topts.thread_name = "I2C";
          topts.stack_sz    = 2048;
          unsigned long _thread_id = 0;
          createThread(&_thread_id, nullptr, i2c_worker_thread, (void*) this, &topts);
          static_i2c_thread_id = (TaskHandle_t) _thread_id;
          busOnline(true);
        }
      }
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
  output->concatf("-- I2C%d (%sline)\n", adapterNumber(), (_adapter_flag(I2C_BUS_FLAG_BUS_ONLINE)?"on":"OFF"));
}


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
    switch (device->adapterNumber()) {
      case 0:
      case 1:
        if (nullptr == _threaded_op[device->adapterNumber()]) {
          if ((nullptr == callback) || (0 == callback->io_op_callahead(this))) {
            set_state(XferState::INITIATE);
            _threaded_op[device->adapterNumber()] = this;
            vTaskResume(static_i2c_thread_id);
            return XferFault::NONE;
          }
          else {
            abort(XferFault::IO_RECALL);
          }
        }
        else {
          abort(XferFault::DEV_NOT_FOUND);
        }
        break;
      default:
        abort(XferFault::BAD_PARAM);
        break;
    }
  }
  else {
    abort(XferFault::BUS_BUSY);
  }
  return getFault();
}


/*
* FreeRTOS doesn't have a concept of interrupt, but we might call this
*   from an I/O thread.
*/
XferFault I2CBusOp::advance(uint32_t status_reg) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  if (ESP_OK == i2c_master_start(cmd)) {
    switch (get_opcode()) {
      case BusOpcode::RX:
        if (need_to_send_subaddr()) {
          i2c_master_write_byte(cmd, ((uint8_t) (dev_addr & 0x00FF) << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
          i2c_master_write_byte(cmd, (uint8_t) (sub_addr & 0x00FF), ACK_CHECK_EN);
          set_state(XferState::ADDR);
          i2c_master_start(cmd);
        }
        i2c_master_write_byte(cmd, ((uint8_t) (dev_addr & 0x00FF) << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
        i2c_master_read(cmd, _buf, (size_t) _buf_len, I2C_MASTER_LAST_NACK);
        set_state(XferState::RX_WAIT);
        break;
      case BusOpcode::TX:
        i2c_master_write_byte(cmd, ((uint8_t) (dev_addr & 0x00FF) << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
        if (need_to_send_subaddr()) {
          i2c_master_write_byte(cmd, (uint8_t) (sub_addr & 0x00FF), ACK_CHECK_EN);
          set_state(XferState::ADDR);
        }
        i2c_master_write(cmd, _buf, (size_t) _buf_len, ACK_CHECK_EN);
        set_state(XferState::TX_WAIT);
        break;
      case BusOpcode::TX_CMD:
        i2c_master_write_byte(cmd, ((uint8_t) (dev_addr & 0x00FF) << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
        set_state(XferState::TX_WAIT);
        break;
      default:
        abort(XferFault::BAD_PARAM);
        break;
    }

    if (XferFault::NONE == getFault()) {
      if (ESP_OK == i2c_master_stop(cmd)) {
        set_state(XferState::STOP);
        int ret = i2c_master_cmd_begin((i2c_port_t) device->adapterNumber(), cmd, 1000 / portTICK_RATE_MS);
        switch (ret) {
          case ESP_OK:
            markComplete();
            break;
          case ESP_ERR_INVALID_ARG:
            abort(XferFault::BAD_PARAM);
            break;
          case ESP_ERR_INVALID_STATE:
            abort(XferFault::ILLEGAL_STATE);
            break;
          case ESP_ERR_TIMEOUT:
            abort(XferFault::TIMEOUT);
            break;
          case ESP_FAIL:
            abort(XferFault::DEV_FAULT);
            break;
          default:
            abort();
            break;
        }
      }
      else {
        abort();
      }
    }
  }
  else {
    abort(XferFault::BUS_FAULT);
  }
  i2c_cmd_link_delete(cmd);  // Cleanup.

  return getFault();
}

#endif  // CONFIG_MANUVR_I2C
