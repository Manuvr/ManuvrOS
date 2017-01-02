/*
File:   I2CBusOp.cpp
Author: J. Ian Lindsay
Date:   2014.03.10

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

*/

#if defined(MANUVR_SUPPORT_I2C)

#include <Platform/Peripherals/I2C/I2CAdapter.h>


/*
* It is worth re-iterating here, that this class ought to never malloc() or free() the buf member. That should be
*   under the exclusive control of the caller.
*/
I2CBusOp::I2CBusOp() {
  xfer_state   = XferState::IDLE;
};


/*
* It is worth re-iterating here, that this class ought to never malloc() or free() the buf member. That should be
*   under the exclusive control of the caller.
*/
I2CBusOp::I2CBusOp(BusOpcode nu_op, BusOpCallback* requester) : I2CBusOp() {
  opcode       = nu_op;
  callback     = requester;
  subaddr_sent((sub_addr >= 0) ? false : true);
};


/*
* It is worth re-iterating here, that this class ought to never malloc() or free() the buf member. That should be
*   under the exclusive control of the caller.
*/
I2CBusOp::I2CBusOp(BusOpcode nu_op, uint8_t dev_addr, int16_t sub_addr, uint8_t *buf, uint8_t len) : I2CBusOp() {
  xfer_state   = XferState::IDLE;
  opcode       = nu_op;
  dev_addr     = dev_addr;
  sub_addr     = sub_addr;
  buf          = buf;
  buf_len      = len;
  subaddr_sent((sub_addr >= 0) ? false : true);
};


I2CBusOp::~I2CBusOp() {
}



/**
* Wipes this bus operation so it can be reused.
* Be careful not to blow away the flags that prevent us from being reaped.
*/
void I2CBusOp::wipe() {
  set_state(XferState::IDLE);
  // We need to preserve flags that deal with memory management.
  xfer_fault  = XferFault::NONE;
  opcode      = BusOpcode::UNDEF;
  buf_len     = 0;
  buf         = nullptr;
  callback    = nullptr;
}


/****************************************************************************************************
* These functions are for logging support.                                                          *
****************************************************************************************************/

/*
* Dump this item to the dev log.
*/
void I2CBusOp::printDebug() {
	StringBuilder temp;
	printDebug(&temp);
	Kernel::log(&temp);
}


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void I2CBusOp::printDebug(StringBuilder* output) {
  BusOp::printBusOp("I2COp", this, output);
  output->concatf("\t device          0x%02x\n", dev_addr);

  if (sub_addr >= 0x00) {
    output->concatf("\t subaddress      0x%02x (%ssent)\n", sub_addr, (subaddr_sent() ? "" : "un"));
  }
  output->concat("\n\n");
}



/****************************************************************************************************
* Job control functions. Calling these will affect control-flow during processing.                  *
****************************************************************************************************/

/* Call to mark something completed that may not be. Also sends a stop. */
int8_t I2CBusOp::abort(XferFault er) {
  markComplete();
  xfer_fault = er;
  return 0;
}

/*
*
*/
void I2CBusOp::markComplete() {
	xfer_state = XferState::COMPLETE;
	ManuvrMsg* q_rdy = Kernel::returnEvent(MANUVR_MSG_I2C_QUEUE_READY);
	q_rdy->specific_target = device;
  Kernel::isrRaiseEvent(q_rdy);   // Raise an event
}


#endif  // MANUVR_SUPPORT_I2C
