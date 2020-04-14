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


TLC5947::TLC5947(uint8_t count, SPIAdapter* b, uint8_t cs, uint8_t oe) {
  _cs_pin    = cs;
  _oe_pin    = oe;
  _bus       = b;
  _chain_len = count;
  if (_chain_len) {
    _buffer = (uint8_t*) malloc(bufLen());
    bzero(_buffer, bufLen());
  }
  gpioDefine(_cs_pin, GPIOMode::OUTPUT);
  gpioDefine(_oe_pin, GPIOMode::OUTPUT);
  setPin(_cs_pin, false);
  blank(false);
}


TLC5947::~TLC5947() {
  // Recall outstanding bus operations.
  // Turn off the chip's outputs.
  // Free buffer memory.
  _bus->purge_queued_work_by_dev(this);
  blank(true);
  if (_buffer) {
    free(_buffer);
    _buffer = nullptr;
  }
}


void TLC5947::refresh() {
  SPIBusOp* op = _bus->new_op(BusOpcode::TX, this);
  op->setBuffer(_buffer, bufLen());
  queue_io_job(op);
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
        *(_buffer + (byte_offset + 1)) = (uint8_t) (temp | (uint8_t) (val << 4));
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


/*
* Ultimately, all bus access this class does passes to this function as its last-stop
*   before becoming folded into the SPI bus queue.
*/
int8_t TLC5947::queue_io_job(BusOp* _op) {
  if (nullptr == _op) return -1;   // This should never happen.
  SPIBusOp* op = (SPIBusOp*) _op;
  op->callback = (BusOpCallback*) this;         // Notify us of the results.
  op->setCSPin(_cs_pin);
  return _bus->queue_io_job(op);     // Pass it to the adapter for bus access.
}


/**
* Called prior to the given bus operation beginning.
* Returning 0 will allow the operation to continue.
* Returning anything else will fail the operation with IO_RECALL.
*   Operations failed this way will have their callbacks invoked as normal.
*
* @param  _op  The bus operation that was completed.
* @return 0 to run the op, or non-zero to cancel it.
*/
int8_t TLC5947::io_op_callahead(BusOp* _op) {
  // Bus adapters don't typically do anything here, other
  //   than permit the transfer.
  return 0;
}


/*
* All notifications of bus activity enter the class here. This is probably where
*   we should act on data coming in.
*/
int8_t TLC5947::io_op_callback(BusOp* _op) {
  SPIBusOp* op = (SPIBusOp*) _op;
  int8_t return_value = BUSOP_CALLBACK_NOMINAL;

  // There is zero chance this object will be a null pointer unless it was done on purpose.
  if (op->hasFault()) {
    StringBuilder local_log;
    local_log.concat("~~~~~~~~TLC5947::io_op_callback   (ERROR CASE -1)\n");
    op->printDebug(&local_log);
    Kernel::log(&local_log);

    // TODO: Should think carefully, and...   return_value = BUSOP_CALLBACK_RECYCLE;   // Re-run the job.
    return BUSOP_CALLBACK_ERROR;
  }

  /* Our first choice is: Did we just finish a WRITE or a READ? */
  /* READ Case-offs */
  if (BusOpcode::RX == op->get_opcode()) {
  }
  /* WRITE Case-offs */
  else if (BusOpcode::TX == op->get_opcode()) {
    _buf_dirty = false;
  }

  return return_value;
}
