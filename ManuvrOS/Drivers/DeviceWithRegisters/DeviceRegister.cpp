#include "DeviceRegister.h"


DeviceRegister::DeviceRegister(uint16_t nu_addr, uint8_t nu_val, uint8_t* buf, bool d, bool u, bool w) {
  len      = 1;
  addr     = nu_addr;
  val      = buf;
  *(val)   = nu_val;
  dirty    = d;
  unread   = u;
  writable = w;
}

DeviceRegister::DeviceRegister(uint16_t nu_addr, uint16_t nu_val, uint8_t* buf, bool d, bool u, bool w) {
  len      = 2;
  addr     = nu_addr;
  val      = buf;
  *((uint16_t*)val)   = nu_val;
  dirty    = d;
  unread   = u;
  writable = w;
}


DeviceRegister::DeviceRegister(uint16_t nu_addr, uint32_t nu_val, uint8_t* buf, bool d, bool u, bool w) {
  len      = 4;
  addr     = nu_addr;
  val      = buf;
  *((uint32_t*)val)   = nu_val;
  dirty    = d;
  unread   = u;
  writable = w;
}


DeviceRegister::DeviceRegister() {
  len      = 0;
  addr     = 0;
  val      = NULL;
  dirty    = false;
  unread   = false;
  writable = false;
}




void DeviceRegister::set(unsigned int nu_val) {
  switch (len) {
    case 1:
      *((uint8_t*) val) = (uint8_t) nu_val & 0xFF;
      break;
    case 2:
      *((uint16_t*) val) = (uint16_t) nu_val & 0xFFFF;
      break;
    case 4:
      *((uint32_t*) val) = (uint32_t) nu_val & 0xFFFFFFFF;
      break;
    default:
      // TODO: Log
      return;
  }
  dirty    = true;
}
