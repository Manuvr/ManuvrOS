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

#if defined(ATECC508_CAPABILITY_DEBUG)
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
    case ATECCOpcodes::Pause:       return "Pause";
    case ATECCOpcodes::Read:        return "Read";
    case ATECCOpcodes::MAC:         return "MAC";
    case ATECCOpcodes::HMAC:        return "HMAC";
    case ATECCOpcodes::Write:       return "Write";
    case ATECCOpcodes::GenDig:      return "GenDig";
    case ATECCOpcodes::Nonce:       return "Nonce";
    case ATECCOpcodes::Lock:        return "Lock";
    case ATECCOpcodes::Random:      return "Random";
    case ATECCOpcodes::DeriveKey:   return "DeriveKey";
    case ATECCOpcodes::UpdateExtra: return "UpdateExtra";
    case ATECCOpcodes::Counter:     return "Counter";
    case ATECCOpcodes::CheckMac:    return "CheckMac";
    case ATECCOpcodes::Info:        return "Info";
    case ATECCOpcodes::GenKey:      return "GenKey";
    case ATECCOpcodes::Sign:        return "Sign";
    case ATECCOpcodes::ECDH:        return "ECDH";
    case ATECCOpcodes::Verify:      return "Verify";
    case ATECCOpcodes::PrivWrite:   return "PrivWrite";
    case ATECCOpcodes::SHA:         return "SHA";
    case ATECCOpcodes::UNDEF:
    default:                        return "UNDEF";
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
#endif  // ATECC508_CAPABILITY_DEBUG

/*
* Returns the expected worst-case delay for the given operation.
*
*/
const unsigned int ATECC508::getOpTime(ATECCOpcodes x) {
  switch (x) {
    case ATECCOpcodes::Pause:       return 3;
    case ATECCOpcodes::Read:        return 1;
    case ATECCOpcodes::MAC:         return 14;
    case ATECCOpcodes::HMAC:        return 23;
    case ATECCOpcodes::Write:       return 26;
    case ATECCOpcodes::GenDig:      return 11;
    case ATECCOpcodes::Nonce:       return 7;
    case ATECCOpcodes::Lock:        return 32;
    case ATECCOpcodes::Random:      return 23;
    case ATECCOpcodes::DeriveKey:   return 50;
    case ATECCOpcodes::UpdateExtra: return 10;
    case ATECCOpcodes::Counter:     return 20;
    case ATECCOpcodes::CheckMac:    return 13;
    case ATECCOpcodes::Info:        return 1;
    case ATECCOpcodes::GenKey:      return 115;
    case ATECCOpcodes::Sign:        return 50;
    case ATECCOpcodes::ECDH:        return 58;
    case ATECCOpcodes::Verify:      return 58;
    case ATECCOpcodes::PrivWrite:   return 48;
    case ATECCOpcodes::SHA:         return 9;
    case ATECCOpcodes::UNDEF:
    default:                        return 1;
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

  switch (_op->get_opcode()) {
    case BusOpcode::TX:
      {
        I2CBusOp* completed = (I2CBusOp*) _op;
        // We switch on the subaddress to decide what sort of packet this was.
        switch ((ATECCPktCodes) completed->sub_addr) {
          case ATECCPktCodes::RESET:  // Address counter reset. Not chip reset.
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
                case ATECCOpcodes::UNDEF:
                default:
                  local_log.concatf("ATECC508::io_op_callback(): Unknown opcode 0x%02x\n", (uint8_t) _current_op);
                  break;
              }
            }
            break;
          default:
            local_log.concatf("ATECC508::io_op_callback(): Unknown packet type 0x%02x\n", completed->sub_addr);
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
      local_log.concatf("\t ATECC508 readback:\n\t");
      if (_op->hasFault()) {
      }
      for (int i = 0; i < _op->buf_len; i++) {
        local_log.concatf("%02x ", *(_op->buf + i));
      }
      break;

    default:
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
    case 'c':   // Read device config
      config_read();
      break;
    case 'o':   // Read OTP region
      otp_read();
      break;
    case 's':   // Read slot.
      slot_read(temp_int & 0x0F);
      break;
    case 'r':   // Read result buffer.
      read_op_buffer(ATECCDataSize::L32);
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

void ATECC508::printSlotInfo(uint8_t s, StringBuilder* output) {
  s &= 0x0F;  // Force-limit to 0-15.
  output->concatf("\t --- Slot %u (%sLOCKED) -------------\n", s, (slot_locked(s) ? "" : "UN"));
  output->concatf("\t read_key:     0x%02x\n", _slot_conf[s].conf.read_key);
  output->concatf("\t no_mac:       %u\n", _slot_conf[s].conf.no_mac);
  output->concatf("\t limited_use:  %u\n", _slot_conf[s].conf.limited_use);
  output->concatf("\t encrypt_read: %u\n", _slot_conf[s].conf.encrypt_read);
  output->concatf("\t is_secret:    %u\n", _slot_conf[s].conf.is_secret);
  output->concatf("\t write_key:    0x%02x\n", _slot_conf[s].conf.write_key);
  output->concatf("\t write_config: 0x%02x\n", _slot_conf[s].conf.write_config);
}



/*
* Returns true if a wakeup needs to be sent.
* Updates the ATECC508_FLAG_AWAKE.
*/
bool ATECC508::need_wakeup() {
  if (!_er_flag(ATECC508_FLAG_PENDING_WAKE)) {
    if (millis() - _last_wake_sent >= (WD_TIMEOUT)) {
      _er_set_flag(ATECC508_FLAG_AWAKE, false);
    }
  }

  return (!_er_flag(ATECC508_FLAG_AWAKE));
}


int ATECC508::send_wakeup() {
  int return_value = -1;   // Fail by default.
  if (need_wakeup()) {
    if (!_opts.useGPIOWake()) {  // If bus-driven wake-up...
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
        nu->sub_addr = 0;
        nu->buf      = nullptr;
        nu->buf_len  = 0;
        return_value = _bus->queue_io_job(nu);
      }
      _er_set_flag(ATECC508_FLAG_PENDING_WAKE, (0 == return_value));
    }
    else {
      // TODO
      local_log.concat("ATECC508 GPIO-driven wakeup not yet supported.\n");
    }

    if (return_value) {
      local_log.concat("ATECC508 driver failed to send wake-up.\n");
    }
  }
  flushLocalLog();
  return return_value;
}

/*
* Base-level packet dispatch fxn for transmissions.
*/
int ATECC508::dispatch_packet(ATECCPktCodes pc, uint8_t* io_buf, uint16_t len) {
  if (io_buf) {
    I2CBusOp* nu = _bus->new_op(BusOpcode::TX, this);
    if (nu) {
      nu->dev_addr = _dev_addr;
      nu->sub_addr = (uint16_t) pc;
      nu->buf      = io_buf;
      nu->buf_len  = len;
      return _bus->queue_io_job(nu);
    }
  }
  return -1;
}

/*
* Base-level packet dispatch fxn for reading results.
*/
int ATECC508::read_op_buffer(ATECCDataSize ds) {
  uint16_t len = (ATECCDataSize::L32 == ds) ? 32 : 4;
  uint8_t* io_buf = (uint8_t*) malloc(len);
  if (io_buf) {
    for (int i = 0; i < len; i++) *(io_buf + i) = 0;
    I2CBusOp* nu = _bus->new_op(BusOpcode::RX, this);
    if (nu) {
      nu->dev_addr = _dev_addr;
      nu->sub_addr = -1;
      nu->buf      = io_buf;
      nu->buf_len  = len;
      if (0 == _bus->queue_io_job(nu)) {
        return 0;
      }
    }
    free(io_buf);  // lol, jk
  }
  return -1;
}


/*
* Given parameters, builds i/o jobs representing a high-level operation and
*   dispatches it to the bus.
*/
int ATECC508::command_operation(uint8_t* buf, uint16_t len) {
  send_wakeup();
  if (true) {
    return dispatch_packet(ATECCPktCodes::COMMAND, buf, len);
  }
  return -1;
}

/*
* Given opts, this function returns a Zone encoding byte.
*/
uint8_t zoneBytePack(ATECCZones zone, ATECCDataSize ds, bool encrypted) {
  uint8_t ret = (uint8_t) zone;
  if (ATECCDataSize::L32 == ds) { ret += 0x40; }
  if (encrypted) {                ret += 0x80; }
  ret &= 0xC3;  // Paranoia
  return ret;
}


int ATECC508::zone_read(ATECCZones z, ATECCDataSize ds, uint16_t addr) {
  uint8_t* io_buf = (uint8_t*) malloc(4);
  if (io_buf) {
    *(io_buf + 0) = (uint8_t) ATECCOpcodes::Read;
    *(io_buf + 1) = zoneBytePack(z, ds, false);       // Encrypt must be zero.
    *(io_buf + 2) = (uint8_t) (addr & 0x00FF);        // Address LSB.
    *(io_buf + 3) = (uint8_t) ((addr >> 8) & 0x00FF); // Address MSB.
    if (0 == command_operation(io_buf, 4)) {
      if (0 == read_op_buffer(ds)) {  // Read the result.
        return 0;
      }
    }
    free(io_buf);  // lol, jk
  }
  return -1;
}


/*
* Internal parameter reset.
*/
void ATECC508::internal_reset() {
  _er_set_flag(0xFF, false);  // Wipe the flags.
  _addr_counter     = 0;
  _slot_locks       = 0;
  _last_action_time = 0;
  _last_wake_sent   = 0;
  for (int i = 0; i < 16; i++) _slot_conf[i].conf.val = 0;   // Zero the conf mirror.
}


/*
*/
int ATECC508::otp_read() {
  if (0 == zone_read(ATECCZones::OTP, ATECCDataSize::L32, 0)) {
    if (0 == zone_read(ATECCZones::OTP, ATECCDataSize::L32, 8)) {
      return 0;
    }
  }
  return -1;
}


/*
*/
int ATECC508::otp_write(uint8_t* buf, uint16_t len) {
  #if defined(ATECC508_CAPABILITY_OTP_RW)
  I2CBusOp* nu = _bus->new_op(BusOpcode::TX, this);
  if (nu) {
    nu->dev_addr = _dev_addr;
    nu->sub_addr = (int16_t) oc;
    nu->buf      = buf;
    nu->buf_len  = len;
    return _bus->queue_io_job(nu);
  }
  #endif  // ATECC508_CAPABILITY_OTP_RW
  return -1;
}



int ATECC508::config_read() {
  // We care about this callback because we must free this buffer.
  if (0 == zone_read(ATECCZones::CONF, ATECCDataSize::L32, 0)) {
    if (0 == zone_read(ATECCZones::CONF, ATECCDataSize::L32, 0)) {
      if (0 == zone_read(ATECCZones::CONF, ATECCDataSize::L32, 0)) {
        if (0 == zone_read(ATECCZones::CONF, ATECCDataSize::L32, 0)) {
          return 0;   // Success.
        }
      }
    }
  }

  local_log.concat("ATECC508 failed to read config.\n");
  flushLocalLog();
  return -1;
}


/*
* It is assumed that this buffer will be 108 bytes long.
*/
int ATECC508::config_write(uint8_t* buf, uint16_t len) {
  #if defined(ATECC508_CAPABILITY_CONFIG_UNLOCK)
  // TODO: Presently assuming buffer to be a contiguous
  /* Writing the entire config space writable by `Write` command will require...
        <skip config[0-15]>
        4 4-byte ops
        1 32-byte op
        5 4-bytes ops
        <skip config[84-87]>
        2 4-byte ops
        1 32-byte op
    13 i/o operations to write the config. Each of those i/o operations will
      have a different value for <Addr>
    The first 4 4-byte ops will have the values (in order, and given little-endian)....
      (0x04, 0x00), (0x05, 0x00), (0x06, 0x00), (0x07, 0x00)
    The first 32-bit write will have...
      (0x08, 0x00)
    The next 5 4-byte ops will be:
      (0x00, 0x01), (0x01, 0x01), (0x02, 0x01), (0x03, 0x01), (0x04, 0x01)
    The next 2 4-byte ops will be:
      (0x06, 0x01), (0x07, 0x01)
    The final 32-byte op will be:
      (0x00, 0x02)
  */
  I2CBusOp* nu = _bus->new_op(BusOpcode::TX, this);
  if (nu) {
    nu->dev_addr = _dev_addr;
    nu->sub_addr = (int16_t) oc;
    nu->buf      = buf;
    nu->buf_len  = len;
    return _bus->queue_io_job(nu);
  }
  #endif  // ATECC508_CAPABILITY_OTP_RW
  return -1;
}


int ATECC508::slot_read(uint8_t s) {
  s &= 0x0F;  // Range-bind the parameter by wrapping it.
  uint16_t addr = (s << 3);
  uint16_t remaining = 0;
  if (0 == zone_read(ATECCZones::DATA, ATECCDataSize::L32, addr)) {
    addr += 0x0100;
    switch (s) {
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
        remaining = 4;
        break;
      case 8:  // Data slot.
        remaining = 384;
        break;
      case 9:
      case 10:
      case 11:
      case 12:
      case 13:
      case 14:
      case 15:
        remaining = 36;
        break;
      default:
        return -1;
    }

    while (remaining > 4) {
      if (zone_read(ATECCZones::DATA, ATECCDataSize::L32, addr)) {
        return -1;
      }
      addr += 0x0100;
      remaining -= 32;
    }

    return zone_read(ATECCZones::DATA, ATECCDataSize::L4, addr);
  }
  return -1;
}
