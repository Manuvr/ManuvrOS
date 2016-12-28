/*
File:   SPIBusOp.cpp
Author: J. Ian Lindsay
Date:   2014.07.01

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


#include "SPIBusOp.h"
#include <Platform/Platform.h>

/*******************************************************************************
* Out-of-class                                                                 +
*******************************************************************************/
StringBuilder debug_log;   // TODO: Relocate this to a static member.


/*******************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here.
*******************************************************************************/
uint16_t SPIBusOp::spi_wait_timeout = 20; // In microseconds. Per-byte.
ManuvrMsg SPIBusOp::event_spi_queue_ready;


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

/**
* Vanilla constructor that calls wipe().
*/
SPIBusOp::SPIBusOp() {
  wipe();
}


/**
* Constructor that does setup by parameters.
*
* @param  nu_op The opcode that dictates the bus operation we use
* @param  requester  The object to be notified when the bus operation completes with success.
*/
SPIBusOp::SPIBusOp(BusOpcode nu_op, BusOpCallback* requester) : SPIBusOp() {
  opcode   = nu_op;
  callback = requester;
}


/**
* Constructor that does setup by parameters.
*
* @param  nu_op The opcode that dictates the bus operation we use
* @param  requester  The object to be notified when the bus operation completes with success.
* @param  cs         The pin number for the device's chip-select signal.
* @param  ah         True for an active-high chip-select.
*/
SPIBusOp::SPIBusOp(BusOpcode nu_op, BusOpCallback* requester, uint8_t cs, bool ah) : SPIBusOp(nu_op, requester) {
  _cs_pin = cs;
  csActiveHigh(ah);
}


/**
* Destructor
* Should be nothing to do here. If this is DMA'd, we expect the referenced buffer
*   to be managed by the class that creates these objects.
*
* Moreover, sometimes instances of this class will be preallocated, and never torn down.
*/
SPIBusOp::~SPIBusOp() {
  if (profile()) {
    debug_log.concat("Destroying an SPI job that was marked for profiling:\n");
    printDebug(&debug_log);
  }
  if (debug_log.length() > 0) Kernel::log(&debug_log);
}


/**
* Set the buffer parameters. Note the there is only ONE buffer, despite this
*   bus being full-duplex.
*
* @param  buf The transfer buffer.
* @param  len The length of the buffer.
*/
void SPIBusOp::setBuffer(uint8_t *buf, unsigned int len) {
  buf     = buf;
  buf_len = len;
}


/**
* Some devices require transfer parameters that are in non-contiguous memory
*   with-respect-to the payload buffer.
*
* @param  p0 The first transfer parameter.
* @param  p1 The second transfer parameter.
* @param  p2 The third transfer parameter.
* @param  p3 The fourth transfer parameter.
*/
void SPIBusOp::setParams(uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3) {
  _param_len     = 4;
  xfer_params[0] = p0;
  xfer_params[1] = p1;
  xfer_params[2] = p2;
  xfer_params[3] = p3;
}


/**
* Some devices require transfer parameters that are in non-contiguous memory
*   with-respect-to the payload buffer.
*
* @param  p0 The first transfer parameter.
* @param  p1 The second transfer parameter.
* @param  p2 The third transfer parameter.
*/
void SPIBusOp::setParams(uint8_t p0, uint8_t p1, uint8_t p2) {
  _param_len     = 3;
  xfer_params[0] = p0;
  xfer_params[1] = p1;
  xfer_params[2] = p2;
  xfer_params[3] = 0;
}

/**
* Some devices require transfer parameters that are in non-contiguous memory
*   with-respect-to the payload buffer.
*
* @param  p0 The first transfer parameter.
* @param  p1 The second transfer parameter.
*/
void SPIBusOp::setParams(uint8_t p0, uint8_t p1) {
  _param_len     = 2;
  xfer_params[0] = p0;
  xfer_params[1] = p1;
  xfer_params[2] = 0;
  xfer_params[3] = 0;
}


/**
* Some devices require transfer parameters that are in non-contiguous memory
*   with-respect-to the payload buffer.
*
* @param  p0 The first transfer parameter.
*/
void SPIBusOp::setParams(uint8_t p0) {
  _param_len     = 1;
  xfer_params[0] = p0;
  xfer_params[1] = 0;
  xfer_params[2] = 0;
  xfer_params[3] = 0;
}


/**
* Wipes this bus operation so it can be reused.
* Be careful not to blow away the flags that prevent us from being reaped.
*/
void SPIBusOp::wipe() {
  set_state(XferState::IDLE);
  // We need to preserve flags that deal with memory management.
  _flags      = _flags & (SPI_XFER_FLAG_NO_FREE | SPI_XFER_FLAG_PREALLOCATE_Q);
  xfer_fault  = XferFault::NONE;
  opcode      = BusOpcode::UNDEF;
  buf_len     = 0;
  buf         = nullptr;
  callback    = nullptr;
  _cs_pin     = 255;
  _param_len  = 0;
  xfer_params[0] = 0;
  xfer_params[1] = 0;
  xfer_params[2] = 0;
  xfer_params[3] = 0;

  profile(false);
}

/*
*
* P A D | C L*   // P: Pin asserted (not logic level!)
* ------|-----   // A: Active high
* 0 0 0 | 0  1   // D: Desired assertion state
* 0 0 1 | 1  0   // C: Pin changed
* 0 1 0 | 0  0   // L: Pin logic level
* 0 1 1 | 1  1
* 1 0 0 | 1  1   // Therefore...
* 1 0 1 | 0  0   // L  = !(A ^ D)
* 1 1 0 | 1  0   // C  = (P ^ D)
* 1 1 1 | 0  1
*/
int8_t SPIBusOp::_assert_cs(bool asrt) {
  if (csAsserted() ^ asrt) {
    csAsserted(asrt);
    setPin(_cs_pin, !(asrt ^ csActiveHigh()));
    return 0;
  }
  return -1;
}

/*******************************************************************************
*     8                  eeeeee
*     8  eeeee eeeee     8    e eeeee eeeee eeeee eeeee  eeeee e
*     8e 8  88 8   8     8e     8  88 8   8   8   8   8  8  88 8
*     88 8   8 8eee8e    88     8   8 8e  8   8e  8eee8e 8   8 8e
* e   88 8   8 88   8    88   e 8   8 88  8   88  88   8 8   8 88
* 8eee88 8eee8 88eee8    88eee8 8eee8 88  8   88  88   8 8eee8 88eee
*******************************************************************************/


/**
* Marks this bus operation complete.
*
* Need to remember: this gets called in the event of ANY condition that ends this job. And
*   that includes abort() where the bus operation was never begun, and SOME OTHER job has
*   control of the bus.
*
* @return 0 on success. Non-zero on failure.
*/
int8_t SPIBusOp::markComplete() {
  if (csAsserted()) {
    // If this job has bus control, we need to release the bus and tidy up IRQs.
    _assert_cs(false);
  }

  //time_ended = micros();
  xfer_state = XferState::COMPLETE;
  step_queues();
  return 0;
}


/**
* This will mark the bus operation complete with a given error code.
*
* @param  cause A failure code to mark the operation with.
* @return 0 on success. Non-zero on failure.
*/
int8_t SPIBusOp::abort(XferFault cause) {
  xfer_fault = cause;
  debug_log.concatf("SPI job aborted at state %s. Cause: %s.\n", getStateString(), getErrorString());
  printDebug(&debug_log);
  return markComplete();
}


/*******************************************************************************
* Memory-management and cleanup support.                                       *
*******************************************************************************/

/**
* The client class calls this fxn to set this object's post-completion behavior.
* If this fxn is never called, the default behavior of the class is to allow itself to be free()'d.
*
* This flag is preserved by wipe().
*
* @param  nu_reap_state Pass false to cause the bus manager to leave this object alone.
* @return true if the bus manager class should free() this object. False otherwise.
*/
bool SPIBusOp::shouldReap(bool nu_reap_state) {
  _flags = (nu_reap_state) ? (_flags & (uint8_t) ~SPI_XFER_FLAG_NO_FREE) : (_flags | SPI_XFER_FLAG_NO_FREE);
  return ((_flags & SPI_XFER_FLAG_NO_FREE) == 0);
}

/**
* Call this on instantiation with a value of 'true' to disable reap, and indicate to the bus manager that
*   it ought to return this object to the preallocation queue following completion.
* If this fxn is never called, the default behavior of the class is to decline to be recycled
*   into the prealloc queue.
* Calling this fxn implies an opposite return value for calls to shouldReap().
*
* This flag is preserved by wipe().
*
* @param  nu_prealloc_state Pass true to inform the bus manager that this object was preallocated.
* @return true if the bus manager class should return this object to its preallocation queue.
*/
bool SPIBusOp::returnToPrealloc(bool nu_prealloc_state) {
  _flags = (!nu_prealloc_state) ? (_flags & (uint8_t) ~SPI_XFER_FLAG_PREALLOCATE_Q) : (_flags | SPI_XFER_FLAG_PREALLOCATE_Q);
  shouldReap(!nu_prealloc_state);
  return (_flags & SPI_XFER_FLAG_PREALLOCATE_Q);
}

/**
* This is a means for a client class to remind itself if the write operation advanced the
*   device's registers or not.
*
* This flag is cleared if the operation is wipe()'d.
*
* @param  nu_reap_state Pass false to cause the bus manager to leave this object alone.
* @return true if the bus manager class should free() this object. False otherwise.
*/
bool SPIBusOp::devRegisterAdvance(bool _reg_advance) {
  _flags = (_reg_advance) ? (_flags | SPI_XFER_FLAG_DEVICE_REG_INC) : (_flags & (uint8_t) ~SPI_XFER_FLAG_DEVICE_REG_INC);
  return ((_flags & SPI_XFER_FLAG_DEVICE_REG_INC) == 0);
}



/*******************************************************************************
* These functions are for logging support.                                     *
*******************************************************************************/

/**
* Debug support method. This fxn is only present in debug builds.
*
* @param  StringBuilder* The buffer into which this fxn should write its output.
*/
void SPIBusOp::printDebug(StringBuilder *output) {
  if (NULL == output) return;
  BusOp::printBusOp("SPIBusOp", this, output);
  if (shouldReap())       output->concat("\t Will reap\n");
  if (returnToPrealloc()) output->concat("\t Returns to prealloc\n");
  output->concatf("\t param_len         %d\n", _param_len);
  output->concat("\t params            ");

  for (uint8_t i = 0; i < _param_len; i++) {
    output->concatf("0x%02x ", xfer_params[i]);
  }

  output->concat("\n\n");
}
