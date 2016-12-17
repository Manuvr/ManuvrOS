/*
File:   TLC5947.cpp
Author: J. Ian Lindsay
Date:   2016.12.14

Copyright 2016 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


Driver for 24-Channel 12-bit PWM chip.

Driver supports daisy-chaining by passing constructor parameter.
*/

#include "TLC5947.h"


TLC5947::TLC5947(uint8_t count, BusAdapter<SPIBusOp>* b, uint8_t cs, uint8_t oe) {
  _cs_pin    = cs;
  _oe_pin    = oe;
  _bus       = b;
  _chain_len = count;
  if (_chain_len) {
    _buffer = (uint8_t*) malloc(bufLen());
    bzero(_buffer, bufLen());
  }
  gpioDefine(_cs_pin, OUTPUT);
  gpioDefine(_oe_pin, OUTPUT);
  setPin(_cs_pin, false);
  blank(false);
}


TLC5947::~TLC5947() {
  // TODO: Recall any outstanding bus operations.
  blank(true);
  if (_buffer) {
    free(_buffer);
    _buffer = nullptr;
  }
}


void TLC5947::refresh() {
  //// Make the bus sing.
  //for (int i = 0; i < 36; i++) {
  //  SPI.transfer(*(_buffer + i));
  //}
  //setPin(_cs_pin, true);
  //setPin(_cs_pin, false);
  //_buf_dirty = false;
}


void TLC5947::zeroAll() {
  if (_buffer) {
    bzero(_buffer, bufLen());
    refresh();
  }
}


// NOTE: Channels stack across chained chips.
int TLC5947::setChannel(uint8_t c, uint16_t val) {
  if (_buffer) {
    val &= 0x0FFF;  // 12-bit PWM...
    size_t byte_offset = (CHAN_DEPTH * c) >> 3;
    if (byte_offset < bufLen()) {
      if (c & 0x01) {
        // Odd channels have 4-bit offsets.
        uint8_t temp = *(_buffer + byte_offset + 0) & 0xF0;
        *(_buffer + (byte_offset + 0)) = (uint8_t) (temp | ((val >> 8) & 0x00FF));
        *(_buffer + (byte_offset + 1)) = (uint8_t) (val & 0x00FF);
      }
      else {
        // Even channels start byte-aligned.
        uint8_t temp = *(_buffer + byte_offset + 1) & 0x0F;
        *(_buffer + (byte_offset + 0)) = (uint8_t) ((val >> 4) & 0x00FF);
        *(_buffer + (byte_offset + 1)) = (uint8_t) (temp | (uint8_t) (val << 8));
      }
      _buf_dirty = true;
      return 0;
    }
  }
  return -1;
};


// NOTE: Channels stack across chained chips.
uint16_t TLC5947::getChannel(uint8_t c) {
  if (_buffer) {
    size_t byte_offset = (CHAN_DEPTH * c) >> 3;
    if (byte_offset < bufLen()) {
      uint16_t return_value;
      if (c & 0x01) {
        // Odd channels have 4-bit offsets.
        return_value  = ((uint16_t) *(_buffer + byte_offset + 0) & 0x0F) << 8;
        return_value += *(_buffer + byte_offset + 1);
      }
      else {
        // Even channels start byte-aligned.
        return_value  = ((uint16_t) *(_buffer + byte_offset + 0) & 0xFF) << 4;
        return_value += ((uint16_t) *(_buffer + byte_offset + 1) & 0xF0) >> 4;
      }
      return return_value;
    }
  }
  return -1;
};
