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


Parts of this driver were taken from Atmel's Cryptoauth library.

\atmel_crypto_device_library_license_start
\page License

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

3. The name of Atmel may not be used to endorse or promote products derived
  from this software without specific prior written permission.

4. This software may only be redistributed and used in connection with an
  Atmel integrated circuit.

THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

\atmel_crypto_device_library_license_stop
*/

#include "ATECC508.h"
#include <Platform/Platform.h>
#include <Utilities.h>


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

static ATECC508* INSTANCE = nullptr;  // Singleton pattern for timer's sake.


#if defined(ATECC508_CAPABILITY_DEBUG)
const char* ATECC508::getZoneStr(ATECCZones x) {
  switch (x) {
    case ATECCZones::CONF:   return "CONF";
    case ATECCZones::OTP:    return "OTP";
    case ATECCZones::DATA:   return "DATA";
    default:                 return "UNDEF";
  }
}

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

const char* ATECC508::getHLOpStr(ATECCHLOps x) {
  switch (x) {
    case ATECCHLOps::READ_CONF:   return "READ_CONF";
    case ATECCHLOps::WRITE_CONF:  return "WRITE_CONF";
    case ATECCHLOps::READ_SLOT:   return "READ_SLOT";
    case ATECCHLOps::WRITE_SLOT:  return "WRITE_SLOT";
    case ATECCHLOps::READ_OTP:    return "READ_OTP";
    case ATECCHLOps::WRITE_OTP:   return "WRITE_OTP";
    case ATECCHLOps::LOCK_CONF:   return "LOCK_CONF";
    case ATECCHLOps::LOCK_SLOT:   return "LOCK_SLOT";
    case ATECCHLOps::LOCK_OTP:    return "LOCK_OTP";
    case ATECCHLOps::UNDEF:
    default:                      return "UNDEF";
  }
}

const char* ATECC508::getReturnStr(ATECCReturnCodes x) {
  switch (x) {
    case ATECCReturnCodes::SUCCESS:                return "SUCCESS";
    case ATECCReturnCodes::CONFIG_ZONE_LOCKED:     return "CONFIG_ZONE_LOCKED";
    case ATECCReturnCodes::DATA_ZONE_LOCKED:       return "DATA_ZONE_LOCKED";
    case ATECCReturnCodes::WAKE_FAILED:            return "WAKE_FAILED";
    case ATECCReturnCodes::CHECKMAC_VERIFY_FAILED: return "CHECKMAC_VERIFY_FAILED";
    case ATECCReturnCodes::PARSE_ERROR:            return "PARSE_ERROR";
    case ATECCReturnCodes::STATUS_CRC:             return "STATUS_CRC";
    case ATECCReturnCodes::STATUS_UNKNOWN:         return "STATUS_UNKNOWN";
    case ATECCReturnCodes::STATUS_ECC:             return "STATUS_ECC";
    case ATECCReturnCodes::FUNC_FAIL:              return "FUNC_FAIL";
    case ATECCReturnCodes::GEN_FAIL:               return "GEN_FAIL";
    case ATECCReturnCodes::BAD_PARAM:              return "BAD_PARAM";
    case ATECCReturnCodes::INVALID_ID:             return "INVALID_ID";
    case ATECCReturnCodes::INVALID_SIZE:           return "INVALID_SIZE";
    case ATECCReturnCodes::BAD_CRC:                return "BAD_CRC";
    case ATECCReturnCodes::RX_FAIL:                return "RX_FAIL";
    case ATECCReturnCodes::RX_NO_RESPONSE:         return "RX_NO_RESPONSE";
    case ATECCReturnCodes::RESYNC_WITH_WAKEUP:     return "RESYNC_WITH_WAKEUP";
    case ATECCReturnCodes::PARITY_ERROR:           return "PARITY_ERROR";
    case ATECCReturnCodes::TX_TIMEOUT:             return "TX_TIMEOUT";
    case ATECCReturnCodes::RX_TIMEOUT:             return "RX_TIMEOUT";
    case ATECCReturnCodes::COMM_FAIL:              return "COMM_FAIL";
    case ATECCReturnCodes::TIMEOUT:                return "TIMEOUT";
    case ATECCReturnCodes::BAD_OPCODE:             return "BAD_OPCODE";
    case ATECCReturnCodes::WAKE_SUCCESS:           return "WAKE_SUCCESS";
    case ATECCReturnCodes::EXECUTION_ERROR:        return "EXECUTION_ERROR";
    case ATECCReturnCodes::UNIMPLEMENTED:          return "UNIMPLEMENTED";
    case ATECCReturnCodes::ASSERT_FAILURE:         return "ASSERT_FAILURE";
    case ATECCReturnCodes::TX_FAIL:                return "TX_FAIL";
    case ATECCReturnCodes::NOT_LOCKED:             return "NOT_LOCKED";
    case ATECCReturnCodes::NO_DEVICES:             return "NO_DEVICES";
    default:                                       return "UNDEF";
  }
}
#endif  // ATECC508_CAPABILITY_DEBUG

/*
* Returns the expected worst-case delay for the given operation (in ms).
*
*/
unsigned int ATECC508::getOpTime(const ATECCOpcodes x) {
  switch (x) {
    case ATECCOpcodes::Pause:       return 3;
    case ATECCOpcodes::Nonce:       return 7;
    case ATECCOpcodes::SHA:         return 9;
    case ATECCOpcodes::UpdateExtra: return 10;
    case ATECCOpcodes::GenDig:      return 11;
    case ATECCOpcodes::CheckMac:    return 13;
    case ATECCOpcodes::MAC:         return 14;
    case ATECCOpcodes::Counter:     return 20;
    case ATECCOpcodes::HMAC:
    case ATECCOpcodes::Random:      return 23;
    case ATECCOpcodes::Write:       return 26;
    case ATECCOpcodes::Lock:        return 32;
    case ATECCOpcodes::PrivWrite:   return 48;
    case ATECCOpcodes::DeriveKey:
    case ATECCOpcodes::Sign:        return 50;
    case ATECCOpcodes::ECDH:
    case ATECCOpcodes::Verify:      return 58;
    case ATECCOpcodes::GenKey:      return 115;
    case ATECCOpcodes::Read:
    case ATECCOpcodes::Info:
    case ATECCOpcodes::UNDEF:
    default:                        return 1;
  }
}

#if defined(ATECC508_CAPABILITY_CONFIG_UNLOCK)
/* If we have the capability of burning configs then we supply the carve-up and
     birth certificate parameters. */

#define ATECC_KEY_REF_STRING "HW_PRI,FW_PRI,MVRAPI_PRI,MVRAPI_ECDH,UPRI0,UECDH0,UPRI1,UECDH1,B_CERT,PROV_SIG,MVRAPI0,MVRAPI1,FW_PUB,U1_UNLOCK,UPUB0,UPUB1"

static const uint8_t config_def[128] = {
  // The first 16-bytes are write-never.
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

  // No keys have usage limitations.
  0xC0,        // We only support i2c, and we leave the default.
  0x00, 0x55,  // Reserved, OTP consumption mode.
  0x02,        // Open selector, Vcc-ref'd inputs, 1.3s watchdog.

  /* Slot Config  [7-0][15-8] */
  0b10000111, 0b00110000,  // HW_PRI         No write, open gen, open derive
  0b10010111, 0b00110000,  // FW_PRI         No write, open gen, open derive
  0b10001100, 0b00110000,  // MVRAPI_PRI     No write, open gen, open derive
  0b11010011, 0b00110000,  // MVRAPI_ECDH    No write, open gen, open derive
  0b10001100, 0b00110000,  // UPRI0          No write, open gen, open derive
  0b11010011, 0b00110000,  // UECDH0         No write, open gen, open derive
  0b10001100, 0b00110000,  // UPRI1          No write, open gen, open derive
  0b11010011, 0b00110000,  // UECDH1         No write, open gen, open derive
  0b00000000, 0b00000000,  // B_CERT         Plaintext. Used as OTP.
  0b00001111, 0b00000000,  // PROV_SIG
  0b00001111, 0b00000000,  // MVRAPI0
  0b00001111, 0b00000000,  // MVRAPI1
  0b00001111, 0b00000000,  // FW_PUB
  0b00001111, 0b00000000,  // U1_UNLOCK
  0b00001111, 0b00000000,  // UPUB0
  0b00001111, 0b00000000,  // UPUB1

  /* We don't use the secure counter capabilities. */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Counter 0
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Counter 1
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // LastKeyUse
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // LastKeyUse

  0x00, 0x00, 0x00, 0x00,  // UserExtra, Selector, LockValue
  0x00, 0x00, 0x00, 0x00,  // No X.509 usage

  /* Key Config  [7-0][15-8] */
  0b01010001, 0b00000000,  // HW_PRI
  0b01110001, 0b00000000,  // FW_PRI
  0b01010001, 0b00000000,  // MVRAPI_PRI
  0b01000000, 0b00000000,  // MVRAPI_ECDH
  0b01110001, 0b00000000,  // UPRI0
  0b01100000, 0b00000000,  // UECDH0
  0b01110001, 0b00000000,  // UPRI1
  0b01100000, 0b00000000,  // UECDH1
  0b00100000, 0b00000000,  // B_CERT
  0b00000000, 0b00000000,  // PROV_SIG
  0b00010010, 0b00000000,  // MVRAPI0
  0b00010010, 0b00000000,  // MVRAPI1
  0b00110010, 0b00000000,  // FW_PUB
  0b00110010, 0b00000000,  // U1_UNLOCK
  0b00110010, 0b00000000,  // UPUB0
  0b00110010, 0b00000000   // UPUB1
};
#endif  // ATECC508_CAPABILITY_CONFIG_UNLOCK


/*
* Given opts, this function returns a Zone encoding byte.
*/
uint8_t zoneBytePack(ATECCZones zone, ATECCDataSize ds) {
  uint8_t ret = (uint8_t) zone;
  //if (ATECCDataSize::L32 == ds) { ret += 0x40; }
  //if (encrypted) {                ret += 0x80; }
  //ret &= 0xC3;  // Paranoia
  if (ATECCDataSize::L32 == ds) { ret += 0x80; }
  ret &= (ATECCZones::OTP == zone) ? 0x03 : 0x83;
  return ret;
}


/**
* Taken from Atmel Cryptoauth and modified.
* This function calculates CRC given raw data, puts the CRC to given pointer.
*
* @param length size of data not including the CRC byte positions
* @param[in] data pointer to the data over which to compute the CRC
* @param[out] crc pointer to the place where the two-bytes of CRC will be placed
* @return The 16-bit result.
*/
static uint16_t atCRC(uint8_t length, uint8_t* data) {
  const uint16_t POLYNOM = 0x8005;
  uint16_t crc_register = 0;
  uint8_t shift_register;
  uint8_t data_bit, crc_bit;

  for (uint8_t counter = 0; counter < length; counter++) {
    for (shift_register = 0x01; shift_register > 0x00; shift_register <<= 1) {
      data_bit = (data[counter] & shift_register) ? 1 : 0;
      crc_bit = crc_register >> 15;
      crc_register <<= 1;
      if (data_bit != crc_bit) {
        crc_register ^= POLYNOM;
      }
    }
  }
  return crc_register;
}


static void atCRCBuf(uint8_t length, uint8_t* data, uint8_t* crc_ptr) {
  const uint16_t crc_register = atCRC(length, data);
  *(crc_ptr + 0) = (uint8_t)(crc_register & 0x00FF);
  *(crc_ptr + 1) = (uint8_t)(crc_register >> 8);
}


/**
* Taken from Atmel Cryptoauth and modified.
* checks for basic error frame in data
* Performs CRC check on packet.
*
* @param data pointer to received data - expected to be in the form of a CA device response frame
* @return ATCA_STATUS indicating type of error or no error
*/
static ATECCReturnCodes validateRC(uint8_t* buf, uint16_t len) {
  const uint8_t GOOD[4] = { 0x04, 0x00, 0x03, 0x40 };
  if (memcmp(buf, GOOD, 4) == 0) {
    return ATECCReturnCodes::SUCCESS;
  }

  if (0x04 == buf[0]) {    // error packets are always 4 bytes long
    switch (buf[1]) {
      case 0x01:              // checkmac or verify failed
        return ATECCReturnCodes::CHECKMAC_VERIFY_FAILED;
      case 0x03: // command received byte length, opcode or parameter was illegal
        return ATECCReturnCodes::BAD_OPCODE;
      case 0x0F: // chip can't execute the command
        return ATECCReturnCodes::EXECUTION_ERROR;
      case 0x11: // chip was successfully woken up
        return ATECCReturnCodes::WAKE_SUCCESS;
      case 0xFF: // bad crc found or other comm error
        return ATECCReturnCodes::STATUS_CRC;
      default:
        return ATECCReturnCodes::GEN_FAIL;
    }
  }
  else {
    // TODO: Broken CRC check.
    //uint16_t crc = atCRC(len-2, buf);
    //if ((crc & 0xFF) == *(buf + (len-1))) {
    //  if (((crc>>8) & 0xFF) == *(buf + (len-2))) {
        return ATECCReturnCodes::SUCCESS;
    //  }
    //}
    //return ATECCReturnCodes::STATUS_CRC;
  }
}


/*
* This is a FxnPointer to be fed to the scheduler.
*/
void _atec_global_callback() {
  INSTANCE->timed_service();
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
ATECC508::ATECC508(const ATECC508Opts* o, const uint8_t addr) : I2CDevice(addr), _opts(o) {
  INSTANCE = this;
  memset((void*) &_birth_cert, 0, sizeof(ATECBirthCert));
  _wipe_config();
  // Setup the schedule for marching bus operations through the chip with the
  //   proper delays.
  _atec_service.incRefs();
  _atec_service.repurpose(MANUVR_MSG_DEFERRED_FXN);
  _atec_service.alterSchedule(1000, 0, false, _atec_global_callback);
  _atec_service.enableSchedule(false);
  platform.kernel()->addSchedule(&_atec_service);
}

/*
* Destructor.
*/
ATECC508::~ATECC508() {
  _atec_service.enableSchedule(false);
  platform.kernel()->removeSchedule(&_atec_service);
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
  _atec_set_flag(ATECC508_FLAG_OPS_RUNNING);
  if (BusOpcode::TX_CMD == _op->get_opcode()) {
    _atec_set_flag(ATECC508_FLAG_PENDING_WAKE, true);
    _last_wake_sent = millis();
  }
  // TODO: Check that we have enough time to run the proposed operation.
  return 0;
}


int8_t ATECC508::io_op_callback(BusOp* _op) {
  uint32_t next_timer_period = 1;
  if (!_op->hasFault()) {
    // The device is awake.
    _last_action_time = millis();   // Note the time.
  }

  I2CBusOp* completed = (I2CBusOp*) _op;
  switch (completed->get_opcode()) {
    case BusOpcode::TX:
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
          //   of the next transaction.
          _last_action_time = 0;
          break;
        case ATECCPktCodes::COMMAND:
          next_timer_period = getOpTime((ATECCOpcodes) *(completed->buf));
          break;
        default:
          local_log.concatf("ATECC508::io_op_callback(): Unknown packet type 0x%02x\n", completed->sub_addr);
          break;
      }
      break;

    case BusOpcode::TX_CMD:
      // Hopefully, this is a successful ping response.
      _atec_set_flag(ATECC508_FLAG_PENDING_WAKE, false);
      next_timer_period = 15;
      break;

    case BusOpcode::RX:
      {
        uint16_t chip_ret_len = *(completed->buf) & 0xFF;
        if (chip_ret_len == (completed->buf_len + 3)) {
          ATECCReturnCodes ret = validateRC(completed->buf, completed->buf_len);
          switch (ret) {
            case ATECCReturnCodes::WAKE_FAILED:
            _atec_set_flag(ATECC508_FLAG_AWAKE, false);
            break;
            case ATECCReturnCodes::WAKE_SUCCESS:
            _atec_set_flag(ATECC508_FLAG_AWAKE, true);
            break;
            case ATECCReturnCodes::SUCCESS:
            case ATECCReturnCodes::CONFIG_ZONE_LOCKED:
            case ATECCReturnCodes::DATA_ZONE_LOCKED:
            case ATECCReturnCodes::CHECKMAC_VERIFY_FAILED:
            case ATECCReturnCodes::PARSE_ERROR:
            case ATECCReturnCodes::STATUS_CRC:
            case ATECCReturnCodes::STATUS_UNKNOWN:
            case ATECCReturnCodes::STATUS_ECC:
            case ATECCReturnCodes::FUNC_FAIL:
            case ATECCReturnCodes::GEN_FAIL:
            case ATECCReturnCodes::BAD_PARAM:
            case ATECCReturnCodes::INVALID_ID:
            case ATECCReturnCodes::INVALID_SIZE:
            case ATECCReturnCodes::BAD_CRC:
            case ATECCReturnCodes::RX_FAIL:
            case ATECCReturnCodes::RX_NO_RESPONSE:
            case ATECCReturnCodes::RESYNC_WITH_WAKEUP:
            case ATECCReturnCodes::PARITY_ERROR:
            case ATECCReturnCodes::TX_TIMEOUT:
            case ATECCReturnCodes::RX_TIMEOUT:
            case ATECCReturnCodes::COMM_FAIL:
            case ATECCReturnCodes::TIMEOUT:
            case ATECCReturnCodes::BAD_OPCODE:
            case ATECCReturnCodes::EXECUTION_ERROR:
            case ATECCReturnCodes::UNIMPLEMENTED:
            case ATECCReturnCodes::ASSERT_FAILURE:
            case ATECCReturnCodes::TX_FAIL:
            case ATECCReturnCodes::NOT_LOCKED:
            case ATECCReturnCodes::NO_DEVICES:
            default:
              local_log.concatf("ATECC508::io_op_callback(): %s\n", getReturnStr(ret));
              local_log.concatf("\t ATECC508 readback: (%u)\n\t", chip_ret_len);
              for (int i = 0; i < strict_min(chip_ret_len, completed->buf_len); i++) {
                local_log.concatf("%02x ", *(completed->buf + i));
              }
              break;
          }
        }
        else {
          local_log.concatf("ATECC508::io_op_callback(): Return len is != bus_op len (%u vs %u)\n", chip_ret_len, completed->buf_len);
        }
      }
      break;

    default:
      break;
  }

  if (!completed->hasFault()) {
    if (nullptr != _current_grp) {
      if (0 == _current_grp->ops_remaining()) {
        // We are at the end of our current io_group. We should clean it up and
        //   proc the callback for the higher-level operation.
        local_log.concat("Group cleanup and advance\n");
        _current_grp->op_state = 1;
        _clean_current_op_group();
      }
    }
  }
  else if (BusOpcode::TX_CMD != completed->get_opcode()) {
    // A failed op means we fail the remaining operations in the present group.
    if (nullptr != _current_grp) {
      local_log.concat("Failing the entire op_group.\n");
      _current_grp->op_state = -1;
      _clean_current_op_group();
    }
  }
  else {
    // We don't fail the op_group on failed pings to the zero address.
  }

  if (_have_pending_ops()) {
    local_log.concatf("Pending ops cause timer activation in %ums\n", next_timer_period);
    _atec_service.alterSchedulePeriod(next_timer_period);
    _atec_service.enableSchedule(true);
  }

  flushLocalLog();
  return 0;
}


/*
* Dump this item to the dev log.
*/
void ATECC508::printDebug(StringBuilder* output) {
  output->concat("\n==< ATECC508a >======================\n");
  output->concat("\t Serial:");
  StringBuilder::printBuffer(output, serialNumber(), 9, "\t\t");
  output->concat("\t Conf:\n");
  StringBuilder::printBuffer(output, _config, 128, "\t\t");
  output->concatf("\t OTP Mode:     0x%02x\n", _otp_mode);
  output->concatf("\t Chip Mode:    0x%02x\n", _chip_mode);
  output->concatf("\t Awake:        %c\n", _atec_flag(ATECC508_FLAG_AWAKE) ? 'y' :'n');
  output->concatf("\t Syncd:        %c\n", _atec_flag(ATECC508_FLAG_SYNCD) ? 'y' :'n');
  output->concatf("\t OTP Locked:   %c\n", _atec_flag(ATECC508_FLAG_OTP_LOCKED) ? 'y' :'n');
  output->concatf("\t Conf Locked:  %c\n", _atec_flag(ATECC508_FLAG_CONF_LOCKED) ? 'y' :'n');
  output->concatf("\t Data Locked:  %c\n", _atec_flag(ATECC508_FLAG_DATA_LOCKED) ? 'y' :'n');
  //output->concatf("\t Need WAKE:    %c\n", need_wakeup() ? 'y' :'n');
  output->concatf("\t Pending WAKE: %c\n", _atec_flag(ATECC508_FLAG_PENDING_WAKE) ? 'y' :'n');
  if (nullptr != _current_grp) {
    output->concat("\t _current_grp\n");
    output->concatf("\t\t hl_op:        %s\n", getHLOpStr(_current_grp->hl_op));
    output->concatf("\t\t Pending ops:  %u\n", _current_grp->ops_remaining());
  }
  output->concatf("\t Pending op_groups:  %u\n", _op_grps.size());
}



/*******************************************************************************
* Bus I/O grouping fxns
*******************************************************************************/

/*
* This ought to only be called from the private scheduler member.
*/
void ATECC508::timed_service() {
  if (nullptr != _current_grp) {
    I2CBusOp* nxt = _current_grp->nextBusOp();
    if (nullptr != nxt) {
      local_log.concat("Taking next bus_op for ATEC job group.\n");
      _bus->queue_io_job(nxt);
    }
    else {
      // Clean up the operation group and advance.
      local_log.concat("Group cleanup and advance\n");
      delete _current_grp;
      _op_group_advance();
    }
  }
  else {
    // No current group. Start the next group if one exists.
    local_log.concat("Starting new op_group.\n");
    _op_group_advance();
  }

  flushLocalLog();
}


int8_t ATECC508::_op_group_advance() {
  _current_grp = _op_grps.dequeue();
  if (nullptr != _current_grp) {
    I2CBusOp* nxt = _current_grp->nextBusOp();
    if (nullptr != nxt) {
      return _bus->queue_io_job(nxt);
    }
    else {
      return -1;
    }
  }
  else {
    _atec_clear_flag(ATECC508_FLAG_OPS_RUNNING);
  }
  return 0;
}


/*
* Fail the group, and clean up all remaining operations that might
*   be inside it.
*/
int8_t ATECC508::_clean_current_op_group() {
  if (nullptr != _current_grp) {
    I2CBusOp* nxt = _current_grp->nextBusOp();
    while (nullptr != nxt) {
      _bus->return_op_to_pool(nxt);
      nxt = _current_grp->nextBusOp();
    }
    _op_group_callback(_current_grp);
    delete _current_grp;
    _current_grp = nullptr;
  }
  return 0;
}


/*
* When the high-level operation is resolved, this function handles
*   the consequences.
*/
int8_t ATECC508::_op_group_callback(ATECC508OpGroup* op_grp) {
  if (nullptr != op_grp) {
    if (1 == op_grp->op_state) {
      switch (op_grp->hl_op) {
        case ATECCHLOps::WAKEUP:
          local_log.concat("ATEC reports wakeup.\n");
          break;
        case ATECCHLOps::READ_CONF:
          local_log.concat("ATEC conf read.\n");
          memcpy(&_sn[0], (op_grp->op_buf+8),  4);  // Copy serial number
          memcpy(&_sn[4], (op_grp->op_buf+16), 5);
          _otp_mode  = *(op_grp->op_buf+26);  // OTP mode byte
          _chip_mode = *(op_grp->op_buf+27);  // Chip mode byte
          // Skip 8, read 32, skip 9, read 32, skip 9, read 32, skip 9, read 32
          memcpy(&_config[0],  (op_grp->op_buf+8),   32);
          memcpy(&_config[32], (op_grp->op_buf+49),  32);
          memcpy(&_config[64], (op_grp->op_buf+92),  32);
          memcpy(&_config[96], (op_grp->op_buf+134), 32);
          _slot_set(0,  (op_grp->op_buf+28), (op_grp->op_buf+134));
          _slot_set(1,  (op_grp->op_buf+30), (op_grp->op_buf+136));
          _slot_set(2,  (op_grp->op_buf+32), (op_grp->op_buf+138));
          _slot_set(3,  (op_grp->op_buf+34), (op_grp->op_buf+140));
          _slot_set(4,  (op_grp->op_buf+36), (op_grp->op_buf+142));
          _slot_set(5,  (op_grp->op_buf+38), (op_grp->op_buf+144));
          _slot_set(6,  (op_grp->op_buf+40), (op_grp->op_buf+146));
          _slot_set(7,  (op_grp->op_buf+49), (op_grp->op_buf+148));
          _slot_set(8,  (op_grp->op_buf+51), (op_grp->op_buf+150));
          _slot_set(9,  (op_grp->op_buf+53), (op_grp->op_buf+152));
          _slot_set(10, (op_grp->op_buf+55), (op_grp->op_buf+154));
          _slot_set(11, (op_grp->op_buf+57), (op_grp->op_buf+156));
          _slot_set(12, (op_grp->op_buf+59), (op_grp->op_buf+158));
          _slot_set(13, (op_grp->op_buf+61), (op_grp->op_buf+160));
          _slot_set(14, (op_grp->op_buf+63), (op_grp->op_buf+162));
          _slot_set(15, (op_grp->op_buf+65), (op_grp->op_buf+164));
          _atec_set_flag(ATECC508_FLAG_CONF_READ);
          break;
        case ATECCHLOps::WRITE_CONF:
        case ATECCHLOps::READ_SLOT:
        case ATECCHLOps::WRITE_SLOT:
        case ATECCHLOps::READ_OTP:
        case ATECCHLOps::WRITE_OTP:
        case ATECCHLOps::LOCK_CONF:
        case ATECCHLOps::LOCK_SLOT:
        case ATECCHLOps::LOCK_OTP:
        default:
          local_log.concatf("_op_group_callback(%s): end_state %d\n", getHLOpStr(op_grp->hl_op), op_grp->op_state);
          StringBuilder::printBuffer(&local_log, op_grp->op_buf, op_grp->op_buf_len, "\t");
          break;
      }
    }
    else {
      local_log.concatf("ATEC HLOp failed: %s end_state %d\n", getHLOpStr(op_grp->hl_op), op_grp->op_state);
    }
  }
  flushLocalLog();
  return 0;
}


/*
* Takes the group of bus operations related to a high-level action and sends it
*   to the bus. If any operation in a group fails, the subsequent operations in
*   that group are also failed in the order in which they occur.
* If no operations are pending, the first operation in the group is started
*   immediately.
*/
int8_t ATECC508::_dispatch_op_grp(ATECC508OpGroup* nu_grp) {
  send_wakeup();
  _op_grps.insert(nu_grp);
  if (!_atec_service.scheduleEnabled()) {
    // Schedule will be enabled in the i2c callback if there is more to do.
    _atec_service.fireNow();
  }
  return 0;
}



#if defined(MANUVR_CONSOLE_SUPPORT)
/*******************************************************************************
* Console I/O
*******************************************************************************/

static const ConsoleCommand console_cmds[] = {
  { "i", "Info" },
  { "i1", "I2CDevice debug" },
  { "i2", "Print birth cert" },
  { "b", "Read birth cert" },
  { "c", "Read device config" },
  #if defined(ATECC508_CAPABILITY_CONFIG_UNLOCK)
  { "C", "Write config" },
  { "(", "Lock config" },
  #endif   // ATECC508_CAPABILITY_CONFIG_UNLOCK
  { "o", "Read OTP" },
  { "S", "Read slot" },
  { "s", "Dump slot" },
  { "p", "Send wakeup" }
};


uint ATECC508::consoleGetCmds(ConsoleCommand** ptr) {
  *ptr = (ConsoleCommand*) &console_cmds[0];
  return sizeof(console_cmds) / sizeof(ConsoleCommand);
}


void ATECC508::consoleCmdProc(StringBuilder* input) {
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
        case 1:   // We want the slot stats.
          I2CDevice::printDebug(&local_log);
          break;
        case 2:   // Print the birth cert.
          printBirthCert(&local_log);
          break;
        default:
          printDebug(&local_log);
          break;
      }
      break;

    case 'c':   // Read device config
      config_read();
      break;

    #if defined(ATECC508_CAPABILITY_CONFIG_UNLOCK)
    case 'C':   // Write device config
      config_write((uint8_t*) config_def);
      break;
    case '(':   // Lock config
      {
        zone_lock(ATECCZones::CONF, 0, atCRC(128, _config));
      }
      break;
    #endif   // ATECC508_CAPABILITY_CONFIG_UNLOCK

    case '+':
      {
        I2CBusOp* nu_bus_op = _rx_packet(ATECCDataSize::L32, &_config[0]);
        if (nu_bus_op) {
          _bus->queue_io_job(nu_bus_op);
        }
      }
      break;

    case 'b':   // Read birth certificate
      slot_read(8);
      break;
    case 'B':   // Generate birth certificate
      local_log.concatf("generateBirthCert() returns %d\n", generateBirthCert());
      break;
    case 'L':   // Look up a slot by its label
      if (1 < input->count()) {
        char* label = input->position(1);
        local_log.concatf("Searching for slot by label %s... ", label);
        int8_t ret = getSlotByName(label);
        if (0 <= ret) {
          local_log.concat("\n");
          printSlotInfo(ret & 0x0F, &local_log);
        }
        else {
          local_log.concat("Not found.\n");
        }
      }
      break;

    case 'o':   // Read OTP region
      otp_read();
      break;
    case 'S':   // Read slot.
      slot_read(temp_int & 0x0F);
      break;
    case 's':   // Dump slot.
      printSlotInfo(temp_int & 0x0F, &local_log);
      break;

    case 'p':   // Ping device
      send_wakeup();
      break;

    #if defined(ATECC508_CAPABILITY_CONFIG_UNLOCK)
    case '*':   // Debug for testing config struct parse/pack
      for (uint8_t i = 0; i < 16; i++) {
        //_slot[i].conf.val = config_def_1[i*2] + ((uint16_t) config_def_1[(i*2)+1] << 8);
        //_slot[i].key.val  = config_def_4[i*2] + ((uint16_t) config_def_4[(i*2)+1] << 8);
      }
      local_log.concat("Wrote test conf\n");
      break;
    #endif

    default:
      break;
  }

  flushLocalLog();
}
#endif  //MANUVR_CONSOLE_SUPPORT



/*******************************************************************************
* Functions specific to this class....                                         *
*******************************************************************************/

/**
* If the local_log is not empty, forward the logs to the Kernel.
* This alieviates us of the responsibility of freeing the log.
*/
void ATECC508::flushLocalLog() {
  if (local_log.length() > 0) Kernel::log(&local_log);
}


void ATECC508::printSlotInfo(uint8_t s, StringBuilder* output) {
  s &= 0x0F;  // Force-limit to 0-15.
  output->concatf("\t --- Slot %u (%sLOCKED) -------------\n", s, (slot_locked(s) ? "" : "UN"));
  output->concatf("\t read_key:     0x%02x\n", _slot[s].conf.read_key);
  output->concatf("\t no_mac:       %c\n",     _slot[s].conf.no_mac ? 'y':'n');
  output->concatf("\t limited_use:  %c\n",     _slot[s].conf.limited_use ? 'y':'n');
  output->concatf("\t encrypt_read: %c\n",     _slot[s].conf.encrypt_read ? 'y':'n');
  output->concatf("\t is_secret:    %c\n",     _slot[s].conf.is_secret ? 'y':'n');
  output->concatf("\t write_key:    0x%02x\n", _slot[s].conf.write_key);
  output->concatf("\t write_config: 0x%02x\n", _slot[s].conf.write_config);
  output->concatf("\t priv:         %c\n",     _slot[s].key.priv ? 'y':'n');
  output->concatf("\t pubinfo:      %c\n",     _slot[s].key.pubinfo ? 'y':'n');
  output->concatf("\t key_type:     0x%02x\n", _slot[s].key.key_type);
  output->concatf("\t lockabe:      %c\n",     _slot[s].key.lockabe ? 'y':'n');
  output->concatf("\t req_rand:     %c\n",     _slot[s].key.req_rand ? 'y':'n');
  output->concatf("\t req_auth:     %c\n",     _slot[s].key.req_auth ? 'y':'n');
  output->concatf("\t auth_key:     0x%02x\n", _slot[s].key.auth_key);
  output->concatf("\t intrusn_prot: %c\n",     _slot[s].key.intrusn_prot ? 'y':'n');
  output->concatf("\t x509_id:      0x%02x\n", _slot[s].key.x509_id);
}


int8_t ATECC508::getSlotByName(const char* label) {
  if (_birth_cert_valid()) {
    // Only do the lookup if we know the birth cert is valid.
    // TODO: This sucks. Lots of heap thrash. Might-should store a lookup table.
    StringBuilder _key_ref_parser(_birth_cert.key_refs, sizeof(_birth_cert.key_refs));
    _key_ref_parser.split(",");
    if (16 == _key_ref_parser.count()) {
      for (uint8_t ksi = 0; ksi < 16; ksi++) {
        if (0 == StringBuilder::strcasestr(label, (char*) _key_ref_parser.position(0))) {
          return ksi;
        }
      }
    }
  }
  return -1;
}


/*
* TODO: This check should verify the hash of B_CERT slot content against the sig
*   stored in the PROV_SIG slot.
*/
bool ATECC508::_birth_cert_valid() {
  bool is_valid = (_birth_cert.s_ver == ATECC508_S_VER);
  _atec_set_flag(ATECC508_FLAG_BCERT_VALID, is_valid);
  return is_valid;
}


/*
* This is a provisioning-only fxn.
* Will clobber our existing _birth_cert
*/
int8_t ATECC508::generateBirthCert() {
  memset((void*) &_birth_cert, 0, sizeof(ATECBirthCert));
  IdentityUUID* iuuid = (IdentityUUID*) platform.selfIdentity()->getIdentity(IdentFormat::UUID);
  if (nullptr == iuuid) {
    Kernel::log("ATECC508 failed to find a UUID self identity.\n");
    return -1;
  }
  _birth_cert.s_ver = ATECC508_S_VER;
  _birth_cert.c_ver = ATECC508_C_VER;
  #if defined(HW_FAMILY_CODE)
    _birth_cert.hw_fam  = HW_FAMILY_CODE & 0xFF;
  #endif
  #if defined(HW_VERSION_CODE)
    _birth_cert.hw_ver  = HW_VERSION_CODE & 0xFF;
  #endif

  uint32_t str_sz = strict_min(sizeof(_birth_cert.make_str)-1, strlen(MANUFACTURER_NAME));
  memcpy(_birth_cert.make_str, MANUFACTURER_NAME, str_sz);
  str_sz = strict_min(sizeof(_birth_cert.model_str)-1, strlen(FIRMWARE_NAME));
  memcpy(_birth_cert.model_str, FIRMWARE_NAME, str_sz);

  iuuid->copyRaw((uint8_t*) &_birth_cert.hw_id);

  _birth_cert.prov_date = epochTime();

  str_sz = strict_min(sizeof(_birth_cert.key_refs)-1, strlen(ATECC_KEY_REF_STRING));
  memcpy(_birth_cert.key_refs, ATECC_KEY_REF_STRING, str_sz);

  // These values must come from outside.
  //_birth_cert.manu_date
  //_birth_cert.lot_num
  //_birth_cert.prov_id;
  //_birth_cert.prov_pub[64];
  //_birth_cert.manu_blob[116]
  return 0;
}


void ATECC508::printBirthCert(StringBuilder* output) {
  if (_birth_cert.s_ver == ATECC508_S_VER) {
    output->concat("\n--- ATECC-resident birth cert  (");
    uuid_to_sb(&_birth_cert.hw_id, output);
    output->concat(")  -------------\n");
    output->concatf("\t Make/Model:    %s %s\n", _birth_cert.make_str, _birth_cert.model_str);
    output->concatf("\t s/c_ver:       0x%02x / 0x%02x\n", _birth_cert.s_ver, _birth_cert.c_ver);
    output->concatf("\t hw fam/ver:    0x%02x / 0x%02x\n", _birth_cert.hw_fam, _birth_cert.hw_ver);
    output->concatf("\t lot_num:       0x%02x%02x%02x%02x\n", _birth_cert.lot_num[0], _birth_cert.lot_num[1], _birth_cert.lot_num[2], _birth_cert.lot_num[3]);
    output->concatf("\t manu_date:     %lu\n", _birth_cert.manu_date);

    output->concat("\t prov_id:       ");
    uuid_to_sb(&_birth_cert.prov_id, output);
    output->concatf("\n\t prov_date:     %lu\n", _birth_cert.prov_date);

    StringBuilder _key_ref_parser(_birth_cert.key_refs, sizeof(_birth_cert.key_refs));
    _key_ref_parser.split(",");
    if (16 == _key_ref_parser.count()) {
      for (uint8_t ksi = 0; ksi < 16; ksi++) {
        output->concatf("\t Slot %u:\t%s\n", ksi, _key_ref_parser.position(0));
        _key_ref_parser.drop_position(0);
      }
    }
  }
  else {
    output->concat("\nATECC does not have a valid birth cert.\n");
  }
  //uint8_t prov_pub[64];
  //uint8_t manu_blob[116];
}


/*
* Returns true if a wakeup needs to be sent.
* Updates the ATECC508_FLAG_AWAKE.
*/
bool ATECC508::need_wakeup() {
  _atec_set_flag(ATECC508_FLAG_AWAKE, (wrap_accounted_delta(millis(), _last_wake_sent) < WD_TIMEOUT));
  return (!_atec_flag(ATECC508_FLAG_PENDING_WAKE | ATECC508_FLAG_AWAKE));
}


I2CBusOp* ATECC508::_wake_packet0() {
  // Form a ping packet to address 0x00, in the hopes that 8 bit times adds up
  //   to at least 60uS. This will only work if the bus frequency is lower
  //   than ~133KHz.
  // TODO: Rigorously treat these problems.
  I2CBusOp* nu = _bus->new_op(BusOpcode::TX_CMD, this);
  if (nullptr != nu) {
    nu->dev_addr = 0;
    nu->sub_addr = -1;
    nu->buf      = nullptr;
    nu->buf_len  = 0;
  }
  return nu;
}


I2CBusOp* ATECC508::_wake_packet1() {
  // We care about this callback, since its success or failure informs us that
  //   the preceeding wakeup operation worked.
  I2CBusOp* nu = _bus->new_op(BusOpcode::TX_CMD, this);
  if (nu) {
    nu->dev_addr = _dev_addr;
    nu->sub_addr = 0;
    nu->buf      = nullptr;
    nu->buf_len  = 0;
  }
  return nu;
}


/*
* Base-level packet formation fxn for transmissions.
* Passed-in buffer is setup to allow for the length and checksum parameters,
*/
I2CBusOp* ATECC508::_tx_packet(ATECCPktCodes pc, uint16_t len, uint8_t* io_buf) {
  I2CBusOp* nu = _bus->new_op(BusOpcode::TX, this);
  if (ATECCPktCodes::COMMAND == pc) {
    *(io_buf + 0) = (uint8_t) len;
    atCRCBuf(len-2, io_buf, (io_buf + (len-2)));
  }
  if (nu) {
    nu->dev_addr = _dev_addr;
    nu->sub_addr = (uint16_t) pc;
    nu->buf      = io_buf;
    nu->buf_len  = len;
    return nu;
  }
  return nullptr;
}


/*
* Base-level packet formation fxn for reading results.
*/
I2CBusOp* ATECC508::_rx_packet(ATECCDataSize ds, uint8_t* io_buf) {
  uint16_t len = (ATECCDataSize::L32 == ds) ? 32 : 4;
  if (io_buf) {
    memset(io_buf, 0, len);
    I2CBusOp* nu = _bus->new_op(BusOpcode::RX, this);
    if (nu) {
      nu->dev_addr = _dev_addr;
      nu->sub_addr = -1;
      nu->buf      = io_buf;
      nu->buf_len  = len;
      return nu;
    }
  }
  return nullptr;
}


/*
* Internal parameter reset.
*/
void ATECC508::internal_reset() {
  _addr_counter     = 0;
  _slot_locks       = 0;
  _last_action_time = 0;
  _last_wake_sent   = 0;
  _atecc_flags      = 0;
  for (int i = 0; i < 16; i++) _slot[i].conf.val = 0;   // Zero the conf mirror.
}


/*
*/
int ATECC508::send_wakeup() {
  if (!need_wakeup()) {
    return 1;
  }
  if (_opts.useGPIOWake()) {  // If bus-driven wake-up...
    // TODO: ATECC508 GPIO-driven wakeup not yet supported.
    return -1;
  }

  ATECC508OpGroup* nu_op_grp = new ATECC508OpGroup(ATECCHLOps::WAKEUP, 4);
  if ((nullptr != nu_op_grp) && (nullptr != nu_op_grp->op_buf)) {
    uint8_t* buf = nu_op_grp->op_buf;

    I2CBusOp* nu_bus_op = _wake_packet0();
    if (nullptr != nu_bus_op) {
      nu_op_grp->addBusOp(nu_bus_op);
    }
    nu_bus_op = _rx_packet(ATECCDataSize::L4, buf);
    if (nullptr != nu_bus_op) {
      nu_op_grp->addBusOp(nu_bus_op);
    }

    // NOTE: Calling _dispatch_op_grp() here (as is normally done) will recurse
    //  endlessly. So we rely on some other request to start that process.
    _op_grps.insert(nu_op_grp);
  }
  else {
    local_log.concat("Could not allocate mem for wakeup.\n");
  }

  flushLocalLog();
  return 0;
}


/*
*/
int ATECC508::otp_read() {
  const unsigned int OP_SIZE = 2 * (4+32);  // Total memory required for this operation.
  ATECC508OpGroup* nu_op_grp = new ATECC508OpGroup(ATECCHLOps::READ_OTP, OP_SIZE);
  int ret = -1;

  if ((nullptr != nu_op_grp) && (nullptr != nu_op_grp->op_buf)) {
    uint8_t* buf = nu_op_grp->op_buf;
    unsigned int offset = 0;

    *(buf + offset++) = (uint8_t) ATECCOpcodes::Read;
    *(buf + offset++) = zoneBytePack(ATECCZones::OTP, ATECCDataSize::L32);  // Encrypt must be zero.
    *(buf + offset++) = (uint8_t) 0;   // Address LSB.
    *(buf + offset++) = (uint8_t) 0;   // Address MSB.
    I2CBusOp* nu_bus_op = _tx_packet(ATECCPktCodes::COMMAND, 4, (buf + (offset-4)));
    if (nu_bus_op) {
      nu_op_grp->addBusOp(nu_bus_op);
    }
    offset += 4;
    nu_bus_op = _rx_packet(ATECCDataSize::L32, (buf + offset));
    if (nu_bus_op) {
      nu_op_grp->addBusOp(nu_bus_op);
    }
    offset += 32;

    *(buf + offset++) = (uint8_t) ATECCOpcodes::Read;
    *(buf + offset++) = zoneBytePack(ATECCZones::OTP, ATECCDataSize::L32);  // Encrypt must be zero.
    *(buf + offset++) = (uint8_t) 32 >> 2;   // Address LSB.
    *(buf + offset++) = (uint8_t) 0;   // Address MSB.
    nu_bus_op = _tx_packet(ATECCPktCodes::COMMAND, 4, (buf + (offset-4)));
    if (nu_bus_op) {
      nu_op_grp->addBusOp(nu_bus_op);
    }
    offset += 4;
    nu_bus_op = _rx_packet(ATECCDataSize::L32, (buf + offset));
    if (nu_bus_op) {
      nu_op_grp->addBusOp(nu_bus_op);
    }
    _dispatch_op_grp(nu_op_grp);
  }
  else {
    local_log.concat("Could not allocate mem for OTP read.\n");
  }

  flushLocalLog();
  return ret;
}


/*
*/
int ATECC508::otp_write(uint8_t* buf, uint16_t len) {
  #if defined(ATECC508_CAPABILITY_OTP_RW)
  I2CBusOp* nu = _bus->new_op(BusOpcode::TX, this);
  if (nu) {
    nu->dev_addr = _dev_addr;
    //nu->sub_addr = (int16_t) oc;
    nu->buf      = buf;
    nu->buf_len  = len;
    return _bus->queue_io_job(nu);
  }
  #endif  // ATECC508_CAPABILITY_OTP_RW
  return -1;
}


int ATECC508::config_read() {
  // 128 bytes to read
  // Must be done with 4 read ops, each with...
  //  * 7 bytes of overhead on the command side
  //  * 3 bytes of overhead on the read side
  const unsigned int OP_SIZE = 128 + ((7 + 3) * 4);  // Total memory required for this operation.
  ATECC508OpGroup* nu_op_grp = new ATECC508OpGroup(ATECCHLOps::READ_CONF, OP_SIZE);
  int ret = zone_read(nu_op_grp, ATECCZones::CONF, 128, 0);
  if (0 == ret) {
    _dispatch_op_grp(nu_op_grp);
  }
  else {
    local_log.concatf("zone_read() call failed with code %d.\n", ret);
  }
  flushLocalLog();
  return ret;
}


/*
*
*/
int ATECC508::config_write(uint8_t new_config[128]) {
  #if defined(ATECC508_CAPABILITY_CONFIG_UNLOCK)
  /* Writing the entire config space writable by `Write` command will require...
      <skip config[0-15]>
      1 4-byte op
      1 32-byte op  (SlotConfig)
      1 32-byte op  (Counter init)
      <skip config[84-87]>
      2 4-byte ops
      1 32-byte op  (KeyConfig)
  */
  const unsigned int OP_SIZE = (3*8) + (3*36) + (6*8);  // Total memory required for this operation.
  ATECC508OpGroup* nu_op_grp = new ATECC508OpGroup(ATECCHLOps::WRITE_CONF, OP_SIZE);
  int ret = -1;

  if ((nullptr != nu_op_grp) && (nullptr != nu_op_grp->op_buf)) {
    uint8_t* buf = nu_op_grp->op_buf;
    unsigned int offset = 0;

    *(buf + offset++) = (uint8_t) ATECCOpcodes::Write;
    *(buf + offset++) = zoneBytePack(ATECCZones::CONF, ATECCDataSize::L4);  // Encrypt must be zero.
    *(buf + offset++) = (uint8_t) 16 >> 2;   // Address LSB.
    *(buf + offset++) = (uint8_t) 0;   // Address MSB.
    memcpy((buf + offset+4), &new_config[16], 4);
    I2CBusOp* nu_bus_op = _tx_packet(ATECCPktCodes::COMMAND, ATECCDataSize::L4, (buf + offset-4));
    if (nu_bus_op) {
      nu_op_grp->addBusOp(nu_bus_op);
    }
    offset += 4;
    nu_bus_op = _rx_packet(ATECCDataSize::L4, (buf + offset));
    if (nu_bus_op) {
      nu_op_grp->addBusOp(nu_bus_op);
    }
    offset += 4;

    // Write slot configs...
    *(buf + offset++) = (uint8_t) ATECCOpcodes::Write;
    *(buf + offset++) = zoneBytePack(ATECCZones::CONF, ATECCDataSize::L32);  // Encrypt must be zero.
    *(buf + offset++) = (uint8_t) 20 >> 2;   // Address LSB.
    *(buf + offset++) = (uint8_t) 0;   // Address MSB.
    memcpy((buf + offset+4), &new_config[20], 32);
    nu_bus_op = _tx_packet(ATECCPktCodes::COMMAND, ATECCDataSize::L32, (buf + (offset-4)));
    if (nu_bus_op) {
      nu_op_grp->addBusOp(nu_bus_op);
    }
    offset += 32;
    nu_bus_op = _rx_packet(ATECCDataSize::L4, (buf + offset));
    if (nu_bus_op) {
      nu_op_grp->addBusOp(nu_bus_op);
    }
    offset += 4;

    // Counter Init...
    *(buf + offset++) = (uint8_t) ATECCOpcodes::Write;
    *(buf + offset++) = zoneBytePack(ATECCZones::CONF, ATECCDataSize::L32);  // Encrypt must be zero.
    *(buf + offset++) = (uint8_t) 52 >> 2;   // Address LSB.
    *(buf + offset++) = (uint8_t) 0;   // Address MSB.
    memcpy((buf + offset+4), &new_config[52], 32);
    nu_bus_op = _tx_packet(ATECCPktCodes::COMMAND, ATECCDataSize::L32, (buf + (offset-4)));
    if (nu_bus_op) {
      nu_op_grp->addBusOp(nu_bus_op);
    }
    offset += 32;
    nu_bus_op = _rx_packet(ATECCDataSize::L4, (buf + offset));
    if (nu_bus_op) {
      nu_op_grp->addBusOp(nu_bus_op);
    }
    offset += 4;

    *(buf + offset++) = (uint8_t) ATECCOpcodes::Write;
    *(buf + offset++) = zoneBytePack(ATECCZones::CONF, ATECCDataSize::L4);  // Encrypt must be zero.
    *(buf + offset++) = (uint8_t) 88 >> 2;   // Address LSB.
    *(buf + offset++) = (uint8_t) 0;   // Address MSB.
    memcpy((buf + offset+4), &new_config[88], 4);
    nu_bus_op = _tx_packet(ATECCPktCodes::COMMAND, ATECCDataSize::L4, (buf + offset-4));
    if (nu_bus_op) {
      nu_op_grp->addBusOp(nu_bus_op);
    }
    offset += 4;
    nu_bus_op = _rx_packet(ATECCDataSize::L4, (buf + offset));
    if (nu_bus_op) {
      nu_op_grp->addBusOp(nu_bus_op);
    }
    offset += 4;

    *(buf + offset++) = (uint8_t) ATECCOpcodes::Write;
    *(buf + offset++) = zoneBytePack(ATECCZones::CONF, ATECCDataSize::L4);  // Encrypt must be zero.
    *(buf + offset++) = (uint8_t) 92 >> 2;   // Address LSB.
    *(buf + offset++) = (uint8_t) 0;   // Address MSB.
    memcpy((buf + offset+4), &new_config[92], 4);
    nu_bus_op = _tx_packet(ATECCPktCodes::COMMAND, ATECCDataSize::L4, (buf + offset-4));
    if (nu_bus_op) {
      nu_op_grp->addBusOp(nu_bus_op);
    }
    offset += 4;
    nu_bus_op = _rx_packet(ATECCDataSize::L4, (buf + offset));
    if (nu_bus_op) {
      nu_op_grp->addBusOp(nu_bus_op);
    }
    offset += 4;

    // Write key configs...
    *(buf + offset++) = (uint8_t) ATECCOpcodes::Write;
    *(buf + offset++) = zoneBytePack(ATECCZones::CONF, ATECCDataSize::L32);  // Encrypt must be zero.
    *(buf + offset++) = (uint8_t) 96 >> 2;   // Address LSB.
    *(buf + offset++) = (uint8_t) 0;   // Address MSB.
    memcpy((buf + offset+4), &new_config[96], 32);
    nu_bus_op = _tx_packet(ATECCPktCodes::COMMAND, ATECCDataSize::L32, (buf + (offset-4)));
    if (nu_bus_op) {
      nu_op_grp->addBusOp(nu_bus_op);
    }
    offset += 32;
    nu_bus_op = _rx_packet(ATECCDataSize::L4, (buf + offset));
    if (nu_bus_op) {
      nu_op_grp->addBusOp(nu_bus_op);
    }
    // TODO: Make all the error-checking above more nested.
    _dispatch_op_grp(nu_op_grp);
  }
  else {
    local_log.concat("Could not allocate mem for conf read.\n");
  }
  flushLocalLog();
  return ret;
  #else
  return -1;
  #endif  // ATECC508_CAPABILITY_CONFIG_UNLOCK
}


/*
* Read the indicated slot from the ATECC.
*/
int ATECC508::slot_read(uint8_t s) {
  s &= 0x0F;  // Range-bind the parameter by wrapping it.
  int ret = -1;
  uint16_t addr = (s << 3);
  uint8_t ops_4b  = 0;
  uint8_t ops_32b = 1;

  switch (s) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
      ops_4b  = 1;
      break;
    case 8:  // Data slot.
      ops_32b = 13;
      break;
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
      ops_32b = 2;
      ops_4b  = 2;
      break;
    default:
      return -1;
  }

  const unsigned int OP_SIZE = (ops_32b * (4+32)) + (ops_4b * (4+4));  // Total memory required for this operation.
  ATECC508OpGroup* nu_op_grp = new ATECC508OpGroup(ATECCHLOps::READ_SLOT, OP_SIZE);

  if ((nullptr != nu_op_grp) && (nullptr != nu_op_grp->op_buf)) {
    uint8_t* buf = nu_op_grp->op_buf;
    unsigned int offset = 0;
    I2CBusOp* nu_bus_op = nullptr;

    for (uint8_t i = 0; i < ops_32b; i++) {
      *(buf + offset++) = (uint8_t) ATECCOpcodes::Read;
      *(buf + offset++) = zoneBytePack(ATECCZones::DATA, ATECCDataSize::L32);
      *(buf + offset++) = (uint8_t) (addr >> 2)  & 0xFF;   // Address LSB.
      *(buf + offset++) = (uint8_t) (addr >> 10) & 0xFF;   // Address MSB.
      addr += 32;
      nu_bus_op = _tx_packet(ATECCPktCodes::COMMAND, ATECCDataSize::L4, (buf + (offset-4)));
      if (nu_bus_op) {
        nu_op_grp->addBusOp(nu_bus_op);
      }
      offset += 4;
      nu_bus_op = _rx_packet(ATECCDataSize::L32, (buf + offset));
      if (nu_bus_op) {
        nu_op_grp->addBusOp(nu_bus_op);
      }
      offset += 32;
    }

    for (uint8_t i = 0; i < ops_4b; i++) {
      *(buf + offset++) = (uint8_t) ATECCOpcodes::Read;
      *(buf + offset++) = zoneBytePack(ATECCZones::DATA, ATECCDataSize::L4);
      *(buf + offset++) = (uint8_t) (addr >> 2)  & 0xFF;   // Address LSB.
      *(buf + offset++) = (uint8_t) (addr >> 10) & 0xFF;   // Address MSB.
      addr += 4;
      nu_bus_op = _tx_packet(ATECCPktCodes::COMMAND, ATECCDataSize::L4, (buf + (offset-4)));
      if (nu_bus_op) {
        nu_op_grp->addBusOp(nu_bus_op);
      }
      offset += 4;
      nu_bus_op = _rx_packet(ATECCDataSize::L32, (buf + offset));
      if (nu_bus_op) {
        nu_op_grp->addBusOp(nu_bus_op);
      }
      offset += 4;
    }
    _dispatch_op_grp(nu_op_grp);
  }
  else {
    local_log.concat("Could not allocate mem for slot read.\n");
  }
  flushLocalLog();
  return ret;
}


/*
*/
void ATECC508::_slot_set(uint8_t idx, uint8_t* slot_conf, uint8_t* key_conf) {
  _slot[idx].conf.val = (*(slot_conf+1) << 8) + *(slot_conf);
  _slot[idx].key.val  = (*(key_conf+1) << 8) + *(key_conf);
}


/*
* WARNING: Locking the OTP zone also locks the DATA zone, and vice-versa.
* WARNING: Locking the CONF zone is irreversible. Be sure you are sure.
*/
int ATECC508::zone_lock(const ATECCZones zone, uint8_t slot, uint16_t crc) {
  int ret = -1;
  uint8_t mode_byte = 0x00;
  ATECCHLOps hl_op;

  switch (zone) {
    case ATECCZones::DATA:
      mode_byte += (slot << 3);
      mode_byte++;
      hl_op = ATECCHLOps::LOCK_SLOT;
      break;
    case ATECCZones::OTP:
      mode_byte++;
      hl_op = ATECCHLOps::LOCK_OTP;
      break;
    case ATECCZones::CONF:
      hl_op = ATECCHLOps::LOCK_CONF;
      break;
    default:
      return -1;
  }

  const unsigned int OP_SIZE = 8;  // Total memory required for this operation.
  ATECC508OpGroup* nu_op_grp = new ATECC508OpGroup(hl_op, OP_SIZE);
  if ((nullptr != nu_op_grp) && (nullptr != nu_op_grp->op_buf)) {
    uint8_t* buf = nu_op_grp->op_buf;
    unsigned int offset = 0;
    I2CBusOp* nu_bus_op = nullptr;

    *(buf + offset++) = (uint8_t) ATECCOpcodes::Lock;
    *(buf + offset++) = mode_byte;
    *(buf + offset++) = (crc >> 8) & 0xFF;
    *(buf + offset++) = crc & 0xFF;

    nu_bus_op = _tx_packet(ATECCPktCodes::COMMAND, ATECCDataSize::L4, (buf + (offset-4)));
    if (nu_bus_op) {
      nu_op_grp->addBusOp(nu_bus_op);
    }
    offset += 4;
    nu_bus_op = _rx_packet(ATECCDataSize::L4, (buf + offset));
    if (nu_bus_op) {
      nu_op_grp->addBusOp(nu_bus_op);
    }
    _dispatch_op_grp(nu_op_grp);
  }
  else {
    local_log.concat("Could not allocate mem for lock cmd.\n");
  }
  flushLocalLog();
  return ret;
}


// Intermediate-level call to translate a zone read into a ATECC508OpGroup*, which is returned.
int ATECC508::zone_read(ATECC508OpGroup* op_grp, ATECCZones zone, uint16_t len, uint16_t addr) {
  // Is the total length decomposable into 32 and 4 byte blocks? Fail if not.
  addr = addr >> 2;
  uint8_t reads_32 = len >> 5;   // Efficient division by 32.
  uint8_t reads_4  = (len - (32 * reads_32)) >> 2;
  if (len == ((reads_4 * 4) + (reads_32 * 32))) {
    //uint16_t overhead = (reads_4 + reads_32) * 10;
    if ((nullptr != op_grp) && (nullptr != op_grp->op_buf)) {
      uint8_t* buf = op_grp->op_buf;
      unsigned int offset = 0;

      for (int i = 0; i < reads_32; i++) {
        offset += 1;    // Length byte placeholder
        *(buf + offset++) = (uint8_t) ATECCOpcodes::Read;
        *(buf + offset++) = zoneBytePack(zone, ATECCDataSize::L32);
        *(buf + offset++) = (uint8_t) addr & 0xFF;       // Address LSB.
        *(buf + offset++) = (uint8_t) (addr>>8) & 0xFF;  // Address MSB.
        offset += 2;    // Checksum placeholder
        I2CBusOp* nu_bus_op = _tx_packet(ATECCPktCodes::COMMAND, 7, (buf + (offset-7)));
        if (nu_bus_op) {
          op_grp->addBusOp(nu_bus_op);
          nu_bus_op = _rx_packet(ATECCDataSize::L32, (buf + offset));
          if (nu_bus_op) {
            op_grp->addBusOp(nu_bus_op);
          }
          else {
            return -3;
          }
        }
        else {
          return -2;
        }
        offset += 35;
        addr += 8;  // Increment by eight 4-byte blocks.
      }
      for (int i = 0; i < reads_4; i++) {
        offset += 1;    // Length byte placeholder
        *(buf + offset++) = (uint8_t) ATECCOpcodes::Read;
        *(buf + offset++) = zoneBytePack(zone, ATECCDataSize::L4);
        *(buf + offset++) = (uint8_t) addr & 0xFF;       // Address LSB.
        *(buf + offset++) = (uint8_t) (addr>>8) & 0xFF;  // Address MSB.
        offset += 2;    // Checksum placeholder
        I2CBusOp* nu_bus_op = _tx_packet(ATECCPktCodes::COMMAND, 7, (buf + (offset-7)));
        if (nu_bus_op) {
          op_grp->addBusOp(nu_bus_op);
          nu_bus_op = _rx_packet(ATECCDataSize::L4, (buf + offset));
          if (nu_bus_op) {
            op_grp->addBusOp(nu_bus_op);
          }
          else {
            return -3;
          }
        }
        else {
          return -2;
        }
        offset += 7;
        addr += 1;  // Increment by one 4-byte block.
      }
      return 0;
    }
    else {
      local_log.concatf("Could not allocate mem for zone_read (%s).\n", getZoneStr(zone));
    }
  }
  else {
    local_log.concatf("Illegal len to zone_read(). len=%u\n", len);
  }
  return -1;
}
