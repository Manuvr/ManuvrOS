#include <Platform/Peripherals/I2C/I2CAdapter.h>

#if defined(MANUVR_SUPPORT_I2C)
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <fcntl.h>
#include <inttypes.h>
#include <ctype.h>


I2CAdapter::I2CAdapter(uint8_t dev_id) : EventReceiver() {
  __class_initializer();
  dev = dev_id;

  char *filename = (char *) alloca(24);
  *filename = 0;
  if (sprintf(filename, "/dev/i2c-%d", dev_id) > 0) {
    dev = open(filename, O_RDWR);
    if (dev < 0) {
      #ifdef __MANUVR_DEBUG
      if (getVerbosity() > 2) {
        local_log.concatf("Failed to open the i2c bus represented by %s.\n", filename);
        Kernel::log(&local_log);
      }
      #endif
    }
    else {
      busOnline(true);
    }
  }
  else {
    #ifdef __MANUVR_DEBUG
    if (getVerbosity() > 2) {
      local_log.concatf("Somehow we failed to sprintf and build a filename to open i2c bus %d.\n", dev_id);
      Kernel::log(&local_log);
    }
    #endif
  }
}



I2CAdapter::~I2CAdapter() {
  __class_teardown();
    if (dev >= 0) {
      #ifdef __MANUVR_DEBUG
      Kernel::log("Closing the open i2c bus...\n");
      #endif
      close(dev);
    }
}



// TODO: Inline this.
int8_t I2CAdapter::generateStart() {
  return busOnline() ? 0 : -1;
}


// TODO: Inline this.
int8_t I2CAdapter::generateStop() {
  return busOnline() ? 0 : -1;
}


/*
* Private function that will switch the addressed i2c device via ioctl. This
*   function is meaningless on anything but a linux system, in which case it
*   will always return true;
* On a linux system, this will only return true if the ioctl call succeeded.
*/
bool I2CAdapter::switch_device(uint8_t nu_addr) {
  bool return_value = false;
  unsigned short timeout = 10000;
  if (nu_addr != last_used_bus_addr) {
    if (dev < 0) {
      // If the bus is either uninitiallized or not idle, decline
      // to switch the device. Return false;
      #ifdef __MANUVR_DEBUG
      if (getVerbosity() > 1) {
        Kernel::log("i2c bus is not online, so won't switch device. Failing....\n");
      }
      #endif
      return return_value;
    }
    else {
      while (busError() && (timeout > 0)) { timeout--; }
      if (busError()) {
        #ifdef __MANUVR_DEBUG
        if (getVerbosity() > 1) {
          Kernel::log("i2c bus was held for too long. Failing....\n");
        }
        #endif
        return return_value;
      }

      if (ioctl(dev, I2C_SLAVE, nu_addr) >= 0) {
        last_used_bus_addr = nu_addr;
        return_value = true;
      }
      else {
        #ifdef __MANUVR_DEBUG
        if (getVerbosity() > 1) {
          local_log.concatf("Failed to acquire bus access and/or talk to slave at %d.\n", nu_addr);
          Kernel::log(&local_log);
        }
        #endif
        busError(true);
      }
    }
  }
  else {
      return_value = true;
  }
  return return_value;
}


#endif  // MANUVR_SUPPORT_I2C
