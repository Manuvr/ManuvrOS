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
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here.
*******************************************************************************/

const char* ATECC508::getPktTypeStr(ATECCPktCodes x) {
  switch (x) {
    case ATECCPktCodes::RESET:    return "RESET";
    case ATECCPktCodes::SLEEP:    return "SLEEP";
    case ATECCPktCodes::IDLE:     return "IDLE";
    case ATECCPktCodes::COMMAND:  return "COMMAND";
    default:                      return "UNDEF";
  }
}

const char* ATECC508::getOpStr(ATECCOpcodes x) {
  switch (x) {
    case ATECCOpcodes::Pause       return "Pause";
    case ATECCOpcodes::Read        return "Read";
    case ATECCOpcodes::MAC         return "MAC";
    case ATECCOpcodes::HMAC        return "HMAC";
    case ATECCOpcodes::Write       return "Write";
    case ATECCOpcodes::GenDig      return "GenDig";
    case ATECCOpcodes::Nonce       return "Nonce";
    case ATECCOpcodes::Lock        return "Lock";
    case ATECCOpcodes::Random      return "Random";
    case ATECCOpcodes::DeriveKey   return "DeriveKey";
    case ATECCOpcodes::UpdateExtra return "UpdateExtra";
    case ATECCOpcodes::Counter     return "Counter";
    case ATECCOpcodes::CheckMac    return "CheckMac";
    case ATECCOpcodes::Info        return "Info";
    case ATECCOpcodes::GenKey      return "GenKey";
    case ATECCOpcodes::Sign        return "Sign";
    case ATECCOpcodes::ECDH        return "ECDH";
    case ATECCOpcodes::Verify      return "Verify";
    case ATECCOpcodes::PrivWrite   return "PrivWrite";
    case ATECCOpcodes::SHA         return "SHA";
    case ATECCOpcodes::UNDEF:
    default:                       return "UNDEF";
  }
}

/*
* Returns the expected worst-case delay for the given operation.
*
*/
const unsigned int ATECC508::getOpTime(ATECCOpcodes x) {
  switch (x) {
    case ATECCOpcodes::Pause       return 3;
    case ATECCOpcodes::Read        return 1;
    case ATECCOpcodes::MAC         return 14;
    case ATECCOpcodes::HMAC        return 23;
    case ATECCOpcodes::Write       return 26;
    case ATECCOpcodes::GenDig      return 11;
    case ATECCOpcodes::Nonce       return 7;
    case ATECCOpcodes::Lock        return 32;
    case ATECCOpcodes::Random      return 23;
    case ATECCOpcodes::DeriveKey   return 50;
    case ATECCOpcodes::UpdateExtra return 10;
    case ATECCOpcodes::Counter     return 20;
    case ATECCOpcodes::CheckMac    return 13;
    case ATECCOpcodes::Info        return 1;
    case ATECCOpcodes::GenKey      return 115;
    case ATECCOpcodes::Sign        return 50;
    case ATECCOpcodes::ECDH        return 58;
    case ATECCOpcodes::Verify      return 58;
    case ATECCOpcodes::PrivWrite   return 48;
    case ATECCOpcodes::SHA         return 9;
    case ATECCOpcodes::UNDEF:
    default:                       return 1;
  }
}

const char* ATECC508::getReturnStr(ATECCReturnCodes x) {
  switch (x) {
    case ATECCReturnCodes::SUCCESS:      return "SUCCESS";
    case ATECCReturnCodes::MISCOMPARE:   return "MISCOMPARE";
    case ATECCReturnCodes::PARSE_ERR:    return "PARSE_ERR";
    case ATECCReturnCodes::ECC_FAULT:    return "ECC_FAULT";
    case ATECCReturnCodes::EXEC_ERR:     return "EXEC_ERR";
    case ATECCReturnCodes::FRESH_WAKE:   return "FRESH_WAKE";
    case ATECCReturnCodes::INSUF_TIME:   return "INSUF_TIME";
    case ATECCReturnCodes::CRC_COMM_ERR: return "CRC_COMM_ERR";
    default:                             return "UNDEF";
  }
}


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
  internal_reset();
  return 0;
}


/*******************************************************************************
* ___     _       _                      These members are mandatory overrides
*  |   / / \ o   | \  _     o  _  _      for implementing I/O callbacks. They
* _|_ /  \_/ o   |_/ (/_ \/ | (_ (/_     are also implemented by Adapters.
*******************************************************************************/

int8_t ATECC508::io_op_callahead(BusOp* _op) {
  return 0;
}


int8_t ATECC508::io_op_callback(BusOp* _op) {
  if (!_op->hasFault()) {
    // The device is awake.
    _last_action_time = millis();   // Note the time.
    _er_set_flag(ATECC508_FLAG_AWAKE, true);
  }

  I2CBusOp* completed = (I2CBusOp*) _op;
  // We switch on the subaddress to decide what sort of packet this was.
  switch ((ATECCPktCodes) completed->sub_addr) {
    case ATECCPktCodes::RESET:  // Address counter reset.
      _addr_counter     = 0;
      break;
    case ATECCPktCodes::SLEEP:  // Device went to sleep.
      internal_reset();
      break;
    case ATECCPktCodes::IDLE:   // Device is ignoring I/O until next wake.
      // TODO: Until this is carefully thought-out, we can't have multiple
      //   ATECCs on a single bus. For now, we will just force a wakeup ahead
      //   of the next trasnsaction.
      _last_action_time = 0;
      break;
    case ATECCPktCodes::COMMAND:
      break;
    default:
      local_log.concatf("ATECC508::io_op_callback(): Unknown packet type 0x%02x\n", completed->sub_addr);
      break;
  }

  switch (_op->get_opcode()) {
    case BusOpcode::TX:
      {
        // This block is concerned with
        _current_op = (ATECCOpcodes) *(completed->buf+0);
        // Which operation was just requested?
        switch (_current_op) {
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
      break;

    case BusOpcode::TX_CMD:
      // Hopefully, this is a successful ping response.
      if (!_op->hasFault()) {
        _last_wake_sent = _last_action_time;
      }
      _er_set_flag(ATECC508_FLAG_AWAKE, !_op->hasFault());
      _er_set_flag(ATECC508_FLAG_PENDING_WAKE, false);
      break;

    case BusOpcode::RX:
      // This is a buffer read. Adjust the counter appropriately, and cycle the
      //   operation back in to read more if necessary.
      break;
  }

  if (_op->buf) {
    free(_op->buf);
    _op->buf     = nullptr;
    _op->buf_len = 0;
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
  output->concatf("\t Pending WAKE: %c\n", _er_flag(ATECC508_FLAG_PENDING_WAKE) ? 'y' :'n');
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
      read_config();
      break;

    case 'p':   // Ping device
      send_wakeup();
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

int ATECC508::send_wakeup() {
  int return_value = -1;   // Fail by default.
  if (true) {  // TODO: Currently assumes bus-driven wake-up.
    // Form a ping packet to address 0x00, in the hopes that 8 bit times adds up
    //   to at least 60uS. This will only work if the bus frequency is lower
    //   than ~133KHz.
    // We rely on slop in the software chain to enforce the 1500uS wake-up delay.
    // TODO: Rigorously treat these problems.
    _bus->ping_slave_addr(0);  // We don't care about this return.
    // We care about this callback.
    I2CBusOp* nu = _bus->new_op(BusOpcode::TX_CMD, this);
    if (nu) {
      nu->dev_addr = _dev_addr;
      nu->sub_addr = (int16_t) oc;
      nu->buf      = buf;
      nu->buf_len  = len;
      return_value = _bus->queue_io_job(nu);
    }
  }

  if (return_value) {
    ATECC508_FLAG_PENDING_WAKE
    local_log.concat("ATECC508 drive failed to send wake-up.\n");
  }
  _er_set_flag(ATECC508_FLAG_PENDING_WAKE, (0 == return_value));
  flushLocalLog();
  return return_value;
}


int ATECC508::read_conf() {
  if (true) {  // TODO: Currently assumes bus-driven wake-up.
    _bus->ping_slave_addr(0);  // We don't care about this return.
    // We care about this callback because we must free this buffer.
    uint8_t* rbuf = (uint8_t*) malloc(32);
    I2CBusOp* nu = _bus->new_op(BusOpcode::TX_CMD, this);
    nu->dev_addr = _dev_addr;
    nu->sub_addr = (int16_t) ATECCPktCodes::COMMAND;
    nu->buf      = rbuf;
    nu->buf_len  = 0;
    if (0 == _bus->queue_io_job(nu)) {
      return 0;   // Success.
    }
  }

  local_log.concat("ATECC508 drive failed to send wake-up.\n");
  flushLocalLog();
  return -1;
}


/*
* Given parameters, builds i/o jobs representing a high-level operation and
*   dispatches it to the bus.
*/
int ATECC508::start_operation(ATECCOpcodes oc, uint8_t* buf, uint16_t len) {
  I2CBusOp* nu = _bus->new_op(BusOpcode::TX_CMD, this);
  if (nu) {
    nu->dev_addr = _dev_addr;
    nu->sub_addr = (int16_t) oc;
    nu->buf      = buf;
    nu->buf_len  = len;
    return _bus->queue_io_job(nu);
  }
  return -1;
}


void ATECC508::internal_reset() {
  _er_set_flag(0xFF, false);  // Wipe the flags.
  _addr_counter     = 0;
  _slot_locks       = 0;
  _last_action_time = 0;
  _last_wake_sent   = 0;
  for (int i = 0; i < 16; i++) _slot_conf[i].conf.val = 0;   // Zero the conf mirror.
}


int ATECC508::read_op_buffer() {
  return -1;
}


void ATECC508::printSlotInfo(uint8_t s, StringBuilder* output) {
  s &= 0x0F;  // Force-limit to 0-15.
  output->concatf("\t --- Slot %u (%sLOCKED) -------------\n", s, (slot_locked(s) ? "" : "UN"));
  output->concatf("\t read_key:     0x%02x\n", _slot_conf[s].read_key);
  output->concatf("\t no_mac:       %u\n", _slot_conf[s].no_mac);
  output->concatf("\t limited_use:  %u\n", _slot_conf[s].limited_use);
  output->concatf("\t encrypt_read: %u\n", _slot_conf[s].encrypt_read);
  output->concatf("\t is_secret:    %u\n", _slot_conf[s].is_secret);
  output->concatf("\t write_key:    0x%02x\n", _slot_conf[s].write_key);
  output->concatf("\t write_config: 0x%02x\n", _slot_conf[s].write_config);
}
