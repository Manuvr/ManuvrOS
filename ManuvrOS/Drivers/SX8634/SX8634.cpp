/*
File:   MCP3918.cpp
Author: J. Ian Lindsay
*/

#include "SX8634.h"

volatile static SX8634* INSTANCE = nullptr;


/*
* This is an ISR.
*/
void sx8634_isr() {
  //((SX8634*) INSTANCE)->isr_fired = true;
}


/*
* Constructor
*/
SX8634::SX8634(const SX8634Opts* o) : I2CDevice(o->i2c_addr),  _opts{o} {
  INSTANCE = this;
  memset(_registers, 0, 128);
  memset(_io_buffer, 0, 128);
  //_ll_pin_init();
}

/*
* Destructor
*/
SX8634::~SX8634() {
}



int8_t SX8634::reset() {
  return -1;
}


int8_t SX8634::init() {
  return -1;
}


bool SX8634::buttonPressed(uint8_t) {
  return false;
}


bool SX8634::buttonReleased(uint8_t) {
  return false;
}



int8_t  SX8634::setGPIOState(uint8_t pin, uint8_t value) {
  return -1;
}


uint8_t SX8634::getGPIOState(uint8_t pin) {
  return 0;
}


int8_t SX8634::_clear_registers() {
  return -1;
}


int8_t SX8634::_read_full_spm() {
  return -1;
}


int8_t SX8634::_ll_pin_init() {
  return -1;
}
