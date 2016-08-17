#include "ManuvrOptions.h"


uintptr_t ManuvrOpt::get_const_from_char_ptr(char* test) {
  uintptr_t result = 0;
  return result;
}


ManuvrOpt::ManuvrOpt() : Argument() {
  _next   = nullptr;
  _flags  = 0;
  _key    = nullptr;
}


ManuvrOpt::ManuvrOpt(const char* k, void* ptr, int len, uint8_t code) : Argument(ptr, len, code) {
  _next   = nullptr;
  _flags  = 0;
  _key    = k;
}


ManuvrOpt::~ManuvrOpt() {
  if (nullptr != _next) delete _next;
}


/**
* @return [description]
*/
int8_t ManuvrOpt::add(ManuvrOpt* nu) {
  if (nullptr != _next) {
    nu->_next = _next;
  }
  _next = nu;
  return 0;
}


/*
* Warning: call is propagated across entire list.
*/
void ManuvrOpt::printDebug(StringBuilder* out) {
  out->concatf("\t%s\t", _key);

  out->concatf("%s\t%s", getTypeCodeString(typeCode()), (reapValue() ? "(reap)" : "\t"));
  uint8_t* buf     = (uint8_t*) pointer();
  uint16_t l_ender = ((len <= 16)) ? len : 16;
  for (int n = 0; n < l_ender; n++) out->concatf("%02x ", (uint8_t) *(buf + n));
  out->concat("\n");

  if (_next) _next->printDebug(out);
}
