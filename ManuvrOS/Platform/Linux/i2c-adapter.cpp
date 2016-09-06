
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
    if (dev >= 0) {
      #ifdef __MANUVR_DEBUG
      Kernel::log("Closing the open i2c bus...\n");
      #endif
      close(dev);
    }
    while (dev_list.hasNext()) {
      dev_list.get()->disassignBusInstance();
      dev_list.remove();
    }

    /* TODO: The work_queue destructor will take care of its own cleanup, but
       We should abort any open transfers prior to deleting this list. */
}



// TODO: Inline this.
int8_t I2CAdapter::generateStart() {
  return 0;
}


// TODO: Inline this.
int8_t I2CAdapter::generateStop() {
  return 0;
}
