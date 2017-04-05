/*
File:   SPIAdapter.cpp
Author: J. Ian Lindsay
Date:   2016.12.17

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


This is a peripheral wraspper around the Teensyduino SPI driver.
*/
#ifdef MANUVR_SUPPORT_SPI

#include <Platform/Peripherals/SPI/SPIAdapter.h>
#include <SPI.h>  // Teensyduino SPI


/*******************************************************************************
* ######## ##     ## ######## ##    ## ########  ######
* ##       ##     ## ##       ###   ##    ##    ##    ##
* ##       ##     ## ##       ####  ##    ##    ##
* ######   ##     ## ######   ## ## ##    ##     ######
* ##        ##   ##  ##       ##  ####    ##          ##
* ##         ## ##   ##       ##   ###    ##    ##    ##
* ########    ###    ######## ##    ##    ##     ######
*
* These are overrides from EventReceiver interface...
*******************************************************************************/

/**
* This is called when the kernel attaches the module.
* This is the first time the class can be expected to have kernel access.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t SPIAdapter::attached() {
  if (EventReceiver::attached()) {
    // We should init the SPI library...
    SPI.begin();
    SPI.setDataMode(SPI_MODE0);
    SPI.setClockDivider(SPI_CLOCK_DIV32);
    return 1;
  }
  return 0;
}


/**
* Calling this member will cause the bus operation to be started.
*
* @return 0 on success, or non-zero on failure.
*/
XferFault SPIBusOp::begin() {
  //time_began    = micros();
  //if (0 == _param_len) {
  //  // Obvious invalidity. We must have at least one transfer parameter.
  //  abort(XferFault::BAD_PARAM);
  //  return XferFault::BAD_PARAM;
  //}

  //if (SPI1->SR & SPI_FLAG_BSY) {
  //  Kernel::log("SPI op aborted before taking bus control.\n");
  //  abort(XferFault::BUS_BUSY);
  //  return XferFault::BUS_BUSY;
  //}

  set_state(XferState::INITIATE);  // Indicate that we now have bus control.

  _assert_cs(true);

  if (_param_len) {
    set_state(XferState::ADDR);
    for (int i = 0; i < _param_len; i++) {
      SPI.transfer(xfer_params[i]);
    }
  }

  if (buf_len) {
    set_state((opcode == BusOpcode::TX) ? XferState::TX_WAIT : XferState::RX_WAIT);
    for (int i = 0; i < buf_len; i++) {
      SPI.transfer(*(buf + i));
    }
  }

  markComplete();
  return XferFault::NONE;
}


/**
* Called from the ISR to advance this operation on the bus.
* Stay brief. We are in an ISR.
*
* @return 0 on success. Non-zero on failure.
*/
int8_t SPIBusOp::advance_operation(uint32_t status_reg, uint8_t data_reg) {
  //debug_log.concatf("advance_op(0x%08x, 0x%02x)\n\t %s\n\t status: 0x%08x\n", status_reg, data_reg, getStateString(), (unsigned long) hspi1.State);
  //Kernel::log(&debug_log);

  /* These are our transfer-size-invariant cases. */
  switch (xfer_state) {
    case XferState::COMPLETE:
      abort(XferFault::HUNG_IRQ);
      return 0;

    case XferState::TX_WAIT:
    case XferState::RX_WAIT:
      markComplete();
      return 0;

    case XferState::FAULT:
      return 0;

    case XferState::QUEUED:
    case XferState::ADDR:
    case XferState::STOP:
    case XferState::UNDEF:

    /* Below are the states that we shouldn't be in at this point... */
    case XferState::INITIATE:
    case XferState::IDLE:
      abort(XferFault::ILLEGAL_STATE);
      return 0;
  }

  return -1;
}


#endif   // MANUVR_SUPPORT_SPI
