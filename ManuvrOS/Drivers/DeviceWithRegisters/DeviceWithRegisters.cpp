#include "DeviceWithRegisters.h"


DeviceWithRegisters::DeviceWithRegisters(uint8_t r_count) {
  reg_count = r_count;
  reg_defs = (reg_count > 0) ? (DeviceRegister*) malloc(sizeof(DeviceRegister) * reg_count) : NULL;
}

DeviceWithRegisters::~DeviceWithRegisters() {
  if (NULL != reg_defs) free(reg_defs);
  reg_defs = NULL;
  // TODO: We should purge the bus of any of our operations and mask our IRQs prior to doing this.
}


/*
* Convenience fxn. Returns 0 if register index is out of bounds.
*/
unsigned int DeviceWithRegisters::regValue(uint8_t idx) {
  if (idx >= reg_count) return 0;
  switch (reg_defs[idx].len) {
    case 2:
      return (uint16_t) *((uint16_t*) reg_defs[idx].val);
    case 4:
      return (uint32_t) *((uint32_t*) reg_defs[idx].val);
    case 1:
    default:
      return (uint8_t) *((uint8_t*) reg_defs[idx].val);
  }
}


/**
*
* @return true if the register's contents are valid. IE, there are no pending writes.
*/
bool DeviceWithRegisters::reg_valid(uint8_t idx) {
  if (idx >= reg_count) return false;  // Techincally, this is factual.
  if (reg_defs[idx].dirty) {  // Is there a pending write?
    return false;
  }
  return true;
}


/**
* Zeroes the register without marking it dirty or unread.
*
* @return true if the register's contents are valid. IE, there are no pending writes.
*/
void DeviceWithRegisters::mark_it_zero(uint8_t idx) {
  switch (reg_defs[idx].len) {
    case 4:
      *((uint32_t*) reg_defs[idx].val) = (uint32_t) 0;
      break;
    case 2:
      *((uint16_t*) reg_defs[idx].val) = (uint16_t) 0;
      break;
    case 1:
    default:
      *((uint8_t*)  reg_defs[idx].val) = (uint8_t) 0;
      break;
  }
  reg_defs[idx].dirty  = false;
  reg_defs[idx].unread = false;
}


/**
* Zeroes all registers.
*
* @return true if the register's contents are valid. IE, there are no pending writes.
*/
void DeviceWithRegisters::mark_it_zero() {
  for (uint8_t i = 0; i < reg_count; i++) {
    mark_it_zero(i);
  }
}



void DeviceWithRegisters::dumpDevRegs(StringBuilder* output) {
  if (NULL == output) return;
  for (int i = 0; i < reg_count; i++) {
    switch (reg_defs[i].len) {
      case 4:
        output->concatf("\t Reg 0x%02x: 0x%08x %s %s %s\n", reg_defs[i].addr, *((uint32_t*) reg_defs[i].val), (reg_defs[i].dirty ? "\tdirty":"\tclean"), (reg_defs[i].unread ? "\tunread":"\tknown"), (reg_defs[i].writable ? "\twritable":"\treadonly"));
        break;
      case 2:
        output->concatf("\t Reg 0x%02x: 0x%04x %s %s %s\n", reg_defs[i].addr, *((uint16_t*) reg_defs[i].val), (reg_defs[i].dirty ? "\tdirty":"\tclean"), (reg_defs[i].unread ? "\tunread":"\tknown"), (reg_defs[i].writable ? "\twritable":"\treadonly"));
        break;
      case 1:
      default:
        output->concatf("\t Reg 0x%02x: 0x%02x %s %s %s\n", reg_defs[i].addr, *(reg_defs[i].val), (reg_defs[i].dirty ? "\tdirty":"\tclean"), (reg_defs[i].unread ? "\tunread":"\tknown"), (reg_defs[i].writable ? "\twritable":"\treadonly"));
        break;
    }
  }
  output->concatf("\n");

}

