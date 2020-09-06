#include <stdlib.h>
#include <unistd.h>

#include "I2CAdapter.h"
#include <Platform/Platform.h>
#include <Kernel.h>

#if defined(CONFIG_MANUVR_I2C)
#include <linux/i2c-dev.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <fcntl.h>
#include <inttypes.h>
#include <ctype.h>


int open_bus_handle = -1;        //TODO: This is a hack. Re-work it.
int8_t  last_used_bus_addr = 0;  //TODO: This is a hack. Re-work it.
pthread_t _thread_id = 0;


I2CBusOp* _threaded_op = nullptr;

void* i2c_worker_thread(void* arg) {
  //I2CAdapter* adapter = (I2CAdapter*) arg;
  while (!platform.nominalState()) {
    sleep_millis(80);
  }
  while (platform.nominalState()) {
    if (_threaded_op) {
      _threaded_op->advance(0);
      _threaded_op = nullptr;
      yieldThread();
    }
    else {
      suspendThread();
    }
  }
  return nullptr;
}


/*
* Private function that will switch the addressed i2c device via ioctl. This
*   function is meaningless on anything but a linux system, in which case it
*   will always return true;
* On a linux system, this will only return true if the ioctl call succeeded.
*/
bool switch_device(I2CAdapter* adapter, uint8_t nu_addr) {
  bool return_value = false;
  unsigned short timeout = 10000;
  if (nu_addr != last_used_bus_addr) {
    if (open_bus_handle < 0) {
      // If the bus is either uninitiallized or not idle, decline
      // to switch the device. Return false;
      #ifdef MANUVR_DEBUG
      Kernel::log("i2c bus is not online, so won't switch device. Failing....\n");
      #endif
      return return_value;
    }
    else {
      while (adapter->busError() && (timeout > 0)) { timeout--; }
      if (adapter->busError()) {
        #ifdef MANUVR_DEBUG
        Kernel::log("i2c bus was held for too long. Failing....\n");
        #endif
        return return_value;
      }

      if (ioctl(open_bus_handle, I2C_SLAVE, nu_addr) >= 0) {
        last_used_bus_addr = nu_addr;
        return_value = true;
      }
      else {
        #ifdef MANUVR_DEBUG
        StringBuilder local_log;
        local_log.concatf("Failed to acquire bus access and/or talk to slave at %d.\n", nu_addr);
        Kernel::log(&local_log);
        #endif
        adapter->busError(true);
      }
    }
  }
  else {
    return_value = true;
  }
  return return_value;
}


/*******************************************************************************
* ___     _                                  This is a template class for
*  |   / / \ o    /\   _|  _. ._ _|_  _  ._  defining arbitrary I/O adapters.
* _|_ /  \_/ o   /--\ (_| (_| |_) |_ (/_ |   Adapters must be instanced with
*                             |              a BusOp as the template param.
*******************************************************************************/

int8_t I2CAdapter::bus_init() {
  char *filename = (char *) alloca(24);
  *filename = 0;
  if (sprintf(filename, "/dev/i2c-%d", adapterNumber()) > 0) {
    open_bus_handle = open(filename, O_RDWR);
    if (open_bus_handle < 0) {
      // TODO?
      // http://stackoverflow.com/questions/15337799/configure-linux-i2c-speed
      #ifdef MANUVR_DEBUG
      if (getVerbosity() > 2) {
        local_log.concatf("Failed to open the i2c bus represented by %s.\n", filename);
        Kernel::log(&local_log);
      }
      #endif
    }
    else {
      createThread(&_thread_id, nullptr, i2c_worker_thread, (void*) this, nullptr);
      busOnline(true);
    }
  }
  #if defined(MANUVR_DEBUG)
  else if (getVerbosity() > 2) {
    local_log.concatf("Somehow we failed to sprintf and build a filename to open i2c bus %d.\n", adapterNumber());
    Kernel::log(&local_log);
  }
  #endif
  return (busOnline() ? 0:-1);
}


int8_t I2CAdapter::bus_deinit() {
  if (open_bus_handle >= 0) {
    #ifdef MANUVR_DEBUG
    Kernel::log("Closing the open i2c bus...\n");
    #endif
    close(open_bus_handle);
  }
  return 0;
}


void I2CAdapter::printHardwareState(StringBuilder* output) {
  output->concatf("-- I2C%d (%sline)\n", adapterNumber(), (_adapter_flag(I2C_BUS_FLAG_BUS_ONLINE)?"on":"OFF"));
}


// TODO: Inline this.
int8_t I2CAdapter::generateStart() {
  return busOnline() ? 0 : -1;
}


// TODO: Inline this.
int8_t I2CAdapter::generateStop() {
  return busOnline() ? 0 : -1;
}



/*******************************************************************************
* ___     _                              These members are mandatory overrides
*  |   / / \ o     |  _  |_              from the BusOp class.
* _|_ /  \_/ o   \_| (_) |_)
*******************************************************************************/

XferFault I2CBusOp::begin() {
  if (nullptr == _threaded_op) {
    if (device) {
      switch (device->adapterNumber()) {
        case 0:
        case 1:
          if ((nullptr == callback) || (0 == callback->io_op_callahead(this))) {
            set_state(XferState::INITIATE);
            _threaded_op = this;
            //device->wake();  // TODO: Forgot what this was responsible for.
            return XferFault::NONE;
          }
          else {
            abort(XferFault::IO_RECALL);
          }
          break;
        default:
          abort(XferFault::BAD_PARAM);
          break;
      }
    }
    else {
      abort(XferFault::DEV_NOT_FOUND);
    }
  }
  else {
    abort(XferFault::BUS_BUSY);
  }

  return getFault();
}


/*
* Linux doesn't have a concept of interrupt, but we might call this
*   from an I/O thread.
*/
XferFault I2CBusOp::advance(uint32_t status_reg) {
  set_state(XferState::ADDR);
  if (device->generateStart()) {
    // Failure to generate START condition.
    abort(XferFault::BUS_BUSY);
    return getFault();
  }

  if (!switch_device(device, dev_addr)) {
    abort(XferFault::BUS_FAULT);
    return getFault();
  }

  if (get_opcode() == BusOpcode::RX) {
    uint8_t sa = (uint8_t) (sub_addr & 0x00FF);

    if (write(open_bus_handle, &sa, 1) == 1) {
      if (read(open_bus_handle, _buf, _buf_len) == _buf_len) {
        markComplete();
      }
      else {
        abort(XferFault::BUS_FAULT);
      }
    }
    else {
      abort(XferFault::BUS_FAULT);
    }
  }
  else if (get_opcode() == BusOpcode::TX) {
    uint8_t buffer[_buf_len + 1];
    buffer[0] = (uint8_t) (sub_addr & 0x00FF);

    for (int i = 0; i < _buf_len; i++) buffer[i + 1] = *(_buf + i);

    if (write(open_bus_handle, &buffer, _buf_len+1) == _buf_len+1) {
      markComplete();
    }
    else {
      abort(XferFault::BUS_FAULT);
    }
  }
  else if (get_opcode() == BusOpcode::TX_CMD) {
    uint8_t buffer[_buf_len + 1];
    buffer[0] = (uint8_t) (sub_addr & 0x00FF);
    if (write(open_bus_handle, &buffer, 1) == 1) {
      markComplete();
    }
    else {
      abort(XferFault::BUS_FAULT);
    }
  }
  else {
    abort(XferFault::BUS_FAULT);
  }

  return getFault();
}

#endif  // CONFIG_MANUVR_I2C
