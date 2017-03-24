/*
File:   ATECC508.cpp
Author: J. Ian Lindsay
Date:   2017.03.23

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

#include "ATECC508.h"
#include <Platform/Platform.h>

/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/
/*
* Constructor. Takes pin numbers as arguments.
*/
ATECC508::ATECC508(const ATECC508Opts* o, const uint8_t addr) : EventReceiver("ATECC508"), I2CDevice(addr), _opts(o) {
}


/*
* Destructor.
*/
ATECC508::~ATECC508() {
}


int8_t ATECC508::init() {
  for (int i = 0; i < 16; i++) _slot_conf[i].conf.val = 0;   // Zero the conf mirror.
  return 0;
}


/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

int8_t ATECC508::io_op_callback(BusOp* _op) {
  I2CDevice::io_op_callback(_op);

  if (!_op->hasFault()) {
    // The device is awake.
    _last_action_time = millis();   // Note the time.
    _er_set_flag(ATECC508_FLAG_AWAKE, true);
  }

  if (BusOpcode::TX_CMD != _op->get_opcode()) {
    I2CBusOp* completed = (I2CBusOp*) _op;
    switch ((ATECCOpcodes) completed->sub_addr) {
      // Look at the first byte to decide what operation we ran.
      case ATECCOpcodes::CheckMac:
        break;
      case ATECCOpcodes::Counter:
        break;
      case ATECCOpcodes::DeriveKey:
        break;
      case ATECCOpcodes::ECDH:
        break;
      case ATECCOpcodes::GenDig:
        break;
      case ATECCOpcodes::GenKey:
        break;
      case ATECCOpcodes::HMAC:
        break;
      case ATECCOpcodes::Info:
        break;
      case ATECCOpcodes::Lock:
        break;
      case ATECCOpcodes::MAC:
        break;
      case ATECCOpcodes::Nonce:
        break;
      case ATECCOpcodes::Pause:
        break;
      case ATECCOpcodes::PrivWrite:
        break;
      case ATECCOpcodes::Random:
        break;
      case ATECCOpcodes::Read:
        break;
      case ATECCOpcodes::Sign:
        break;
      case ATECCOpcodes::SHA:
        break;
      case ATECCOpcodes::UpdateExtra:
        break;
      case ATECCOpcodes::Verify:
        break;
      case ATECCOpcodes::Write:
        break;
      default:
        local_log.concatf("ATECC508::io_op_callback(): Unknown opcode 0x%02x\n", (uint8_t) completed->sub_addr);
        break;
    }
  }
  flushLocalLog();
  return 0;
}


/*
* Dump this item to the dev log.
*/
void ATECC508::printDebug(StringBuilder* output) {
  EventReceiver::printDebug(output);
  I2CDevice::printDebug(output);
  output->concatf("\n\t Awake:        %c\n", _er_flag(ATECC508_FLAG_AWAKE) ? 'y' :'n');
  output->concatf("\t Syncd:        %c\n", _er_flag(ATECC508_FLAG_SYNCD) ? 'y' :'n');
  output->concatf("\t OTP Locked:   %c\n", _er_flag(ATECC508_FLAG_OTP_LOCKED) ? 'y' :'n');
  output->concatf("\t Conf Locked:  %c\n", _er_flag(ATECC508_FLAG_CONF_LOCKED) ? 'y' :'n');
  output->concatf("\t Data Locked:  %c\n", _er_flag(ATECC508_FLAG_DATA_LOCKED) ? 'y' :'n');
}



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
int8_t ATECC508::attached() {
  if (EventReceiver::attached()) {
    return 1;
  }
  return 0;
}


/**
* If we find ourselves in this fxn, it means an event that this class built (the argument)
*   has been serviced and we are now getting the chance to see the results. The argument
*   to this fxn will never be NULL.
*
* Depending on class implementations, we might choose to handle the completed Event differently. We
*   might add values to event's Argument chain and return RECYCLE. We may also free() the event
*   ourselves and return DROP. By default, we will return REAP to instruct the Kernel
*   to either free() the event or return it to it's preallocate queue, as appropriate. If the event
*   was crafted to not be in the heap in its own allocation, we will return DROP instead.
*
* @param  event  The event for which service has been completed.
* @return A callback return code.
*/
int8_t ATECC508::callback_proc(ManuvrMsg* event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = (0 == event->refCount()) ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->eventCode()) {
    default:
      break;
  }

  return return_value;
}


#if defined(MANUVR_CONSOLE_SUPPORT)

void ATECC508::procDirectDebugInstruction(StringBuilder *input) {
  const char* str = (char *) input->position(0);
  char c    = *str;
  int temp_int = 0;

  if (input->count() > 1) {
    // If there is a second token, we proceed on good-faith that it's an int.
    temp_int = input->position_as_int(1);
  }

  switch (c) {
    case 'i':   // Debug prints.
      switch (temp_int) {
        case 1:   // We want the channel stats.
          break;
        default:
          printDebug(&local_log);
          break;
      }
      break;
    case 'p':   // Ping device
      if (0 == sendWakeup()) {
        _bus->ping_slave_addr(_dev_addr);
      }
      break;

    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }

  flushLocalLog();
}
#endif  //MANUVR_CONSOLE_SUPPORT



/****************************************************************************************************
* Functions specific to this class....                                                              *
****************************************************************************************************/

int ATECC508::sendWakeup() {
  if (true) {  // TODO: Currently assumes bus-driven wake-up.
    // Form a ping packet to address 0x00, in the hopes that 8 bit times adds up
    //   to at least 60uS. This will only work if the bus frequency is lower
    //   than ~133KHz.
    // We rely on slop in the software chain to enforce the 1500uS wake-up delay.
    // TODO: Rigorously treat these problems.
    I2CBusOp* nu = _bus->new_op(BusOpcode::TX_CMD, this);
    nu->dev_addr = 0;
    nu->sub_addr = -1;
    nu->buf      = nullptr;
    nu->buf_len  = 0;
    if (0 == _bus->queue_io_job(nu)) {
      return 0;   // Success.
    }
  }

  local_log.concat("ATECC508 drive failed to send wake-up.\n");
  flushLocalLog();
  return -1;
}
