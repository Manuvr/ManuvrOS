#ifndef __OFFBOARD_REGISTER_DEV_H__
  #define __OFFBOARD_REGISTER_DEV_H__ 1

  #include "StringBuilder/StringBuilder.h"
  #include "DeviceRegister.h"

  /*
  * This class is a representation of a generic offboard device that contains
  *   its own registers.
  */
  class DeviceWithRegisters {
    public:
      DeviceWithRegisters(uint8_t r_count);
      ~DeviceWithRegisters();

    protected:
      uint8_t        reg_count;  // This is how many registers this device has.
      DeviceRegister *reg_defs;  // Here is where they will be enumerated.
      uint8_t*  register_pool        = NULL;
      bool      multi_access_support = false;   // TODO: Make this happen! Depends on pooled_registers.
  
      unsigned int regValue(uint8_t idx);
      bool reg_valid(uint8_t idx);
      
      void mark_it_zero(uint8_t);   // Zeroes the register without marking it dirty or unread.
      void mark_it_zero();          // Zeroes all registers.

      /* An implementation must override these with functions appropriate for its bus/transport. */      
      virtual int8_t writeRegister(DeviceRegister *reg) = 0;
      virtual int8_t readRegister(DeviceRegister *reg)  = 0;

      /* Debug stuff... */
      void dumpDevRegs(StringBuilder*);
  };

#endif
