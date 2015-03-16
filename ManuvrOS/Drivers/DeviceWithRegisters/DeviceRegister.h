#ifndef __OFFBOARD_REGISTER_DEFS_H__
  #define __OFFBOARD_REGISTER_DEFS_H__ 1
  
  #include <inttypes.h>
  #include <stdlib.h>

  /*
  * Some classes (espescially those that drive hardware) may want to access registers in the device.
  *   These structures provide a uniform means of defining availible registers.
  * Software sensors can use these as well to control various aspects of their operation.
  */
  class DeviceRegister {
    public:
      DeviceRegister();
      DeviceRegister(uint16_t, uint8_t val,  uint8_t* buf, bool dirty, bool unread, bool writable);
      DeviceRegister(uint16_t, uint16_t val, uint8_t* buf, bool dirty, bool unread, bool writable);
      DeviceRegister(uint16_t, uint32_t val, uint8_t* buf, bool dirty, bool unread, bool writable);
      
      uint16_t addr;       // The internal address of the register.
      uint8_t  len;        // How large is the register?
      uint8_t  *val;       // A buffer that mirrors the register.
      bool     dirty;      // Marked true by the class when the value needs to be written to device.
      bool     unread;     // Marked true by automated read functions if a value changed.
      bool     writable;   // Is this register writable.
      
      void set(unsigned int nu_val);   // Causes the given value to be written to the register.
  };

#endif
