#include <Drivers/i2c-adapter/i2c-adapter.h>

#if defined(MANUVR_SUPPORT_I2C)
#include <Wire/Wire.h>

I2CAdapter::I2CAdapter(uint8_t dev_id) : EventReceiver() {
  __class_initializer();
  dev = dev_id;

  if (dev_id == 1) {
    //Wire.begin(I2C_MASTER, 0x00, I2C_PINS_29_30, I2C_PULLUP_INT, I2C_RATE_400);
    busOnline(true);
  }
}


I2CAdapter::~I2CAdapter() {
    busOnline(false);
    while (dev_list.hasNext()) {
      dev_list.get()->disassignBusInstance();
      dev_list.remove();
    }

    /* TODO: The work_queue destructor will take care of its own cleanup, but
       We should abort any open transfers prior to deleting this list. */
}


// TODO: Inline this.
int8_t I2CAdapter::generateStart() {
  #ifdef __MANUVR_DEBUG
  if (getVerbosity() > 6) Kernel::log("I2CAdapter::generateStart()\n");
  #endif
  if (! busOnline()) return -1;
  //Wire1.sendTransmission(I2C_STOP);
  //Wire1.finish(900);   // We allow for 900uS for timeout.

  return 0;
}

// TODO: Inline this.
int8_t I2CAdapter::generateStop() {
  #ifdef __MANUVR_DEBUG
  if (getVerbosity() > 6) Kernel::log("I2CAdapter::generateStop()\n");
  #endif
  if (! busOnline()) return -1;
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
