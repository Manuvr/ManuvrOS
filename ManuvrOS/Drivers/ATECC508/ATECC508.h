/*
File:   ATECC508.h
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

#ifndef __ATECC508_SEC_DRIVER_H__
#define __ATECC508_SEC_DRIVER_H__

#include <inttypes.h>
#include <stdint.h>
#include <Kernel.h>
#include "Platform/Peripherals/I2C/I2CAdapter.h"


#define ATECC508_FLAG_AWAKE        0x01  // The part is believed to be awake.
#define ATECC508_FLAG_SYNCD        0x02  // The part is present and has been read.
#define ATECC508_FLAG_OTP_LOCKED   0x04  // The OTP zone is locked.
#define ATECC508_FLAG_CONF_LOCKED  0x08  // The conf zone is locked.
#define ATECC508_FLAG_DATA_LOCKED  0x10  // The data zone is locked.
#define ATECC508_FLAG_PENDING_WAKE 0x80  // There is a wake sequence outstanding.


#define ATECC508_I2CADDR           0x60

enum class ATECCReturnCodes {
  SUCCESS      = 0x00,
  MISCOMPARE   = 0x01,
  PARSE_ERR    = 0x03,
  ECC_FAULT    = 0x05,
  EXEC_ERR     = 0x0F,
  FRESH_WAKE   = 0x11,
  INSUF_TIME   = 0xEE,
  CRC_COMM_ERR = 0xFF
};


enum class ATECCPktCodes {
  RESET   = 0x00,
  SLEEP   = 0x01,
  IDLE    = 0x02,
  COMMAND = 0x03
};


enum class ATECCZones {
  CONF,
  DATA,
  OTP
};


enum class ATECCOpcodes {
  UNDEF       = 0x00,
  Pause       = 0x01,
  Read        = 0x02,
  MAC         = 0x08,
  HMAC        = 0x11,
  Write       = 0x12,
  GenDig      = 0x15,
  Nonce       = 0x16,
  Lock        = 0x17,
  Random      = 0x1B,
  DeriveKey   = 0x1C,
  UpdateExtra = 0x20,
  Counter     = 0x24,
  CheckMac    = 0x28,
  Info        = 0x30,
  GenKey      = 0x40,
  Sign        = 0x41,
  ECDH        = 0x43,
  Verify      = 0x45,
  PrivWrite   = 0x46,
  SHA         = 0x47
};


typedef struct atecc_slot_conf_t {
  union {
    struct {
      uint16_t read_key:     4;
      uint16_t no_mac:       1;
      uint16_t limited_use:  1;
      uint16_t encrypt_read: 1;
      uint16_t is_secret:    1;
      uint16_t write_key:    4;
      uint16_t write_config: 4;
    };
    uint16_t val;
  } conf;
} SlotConf;


/*
* Pin defs for this module.
* Set pin def to 255 to mark it as unused.
*/
class ATECC508Opts {
  public:
    const uint8_t x;

    ATECC508Opts(const ATECC508Opts* o) :
      x(o->x) {};

    ATECC508Opts(uint8_t _x) :
      x(_x) {};


  private:
};



// TODO: We are only extending EventReceiver while the driver is written.
class ATECC508 : public EventReceiver, I2CDevice {
  public:
    ATECC508(const ATECC508Opts* o, const uint8_t addr);
    ATECC508(const ATECC508Opts* o) : ATECC508(o, ATECC508_I2CADDR) {};
    virtual ~ATECC508();

    int8_t init();

    /* Overrides from I2CDevice... */
    int8_t io_op_callahead(BusOp*);
    int8_t io_op_callback(BusOp*);
    void printDebug(StringBuilder*);

    /* Overrides from EventReceiver */
    //int8_t notify(ManuvrMsg*);
    int8_t callback_proc(ManuvrMsg*);
    #if defined(MANUVR_CONSOLE_SUPPORT)
      void procDirectDebugInstruction(StringBuilder*);
    #endif  //MANUVR_CONSOLE_SUPPORT

    void printSlotInfo(StringBuilder*);

    static const char* getPktTypeStr(ATECCPktCodes);
    static const char* getOpStr(ATECCOpcodes);
    static const char* getReturnStr(ATECCReturnCodes);
    static const unsigned int getOpTime(ATECCOpcodes);


  protected:
    int8_t attached();


  private:
    const unsigned long WD_TIMEOUT  = 1300;  // Chip goes to sleep after this many ms.
    unsigned long _last_action_time = 0;  // Tracks the last time the device was known to be awake.
    unsigned long _last_wake_sent   = 0;  // Tracks the last time we sent wake sequence.
    uint16_t      _addr_counter     = 0;  // Mirror of the device's internal address counter.
    uint16_t      _slot_locks       = 0;  // One bit per slot.
    SlotConf      _slot_conf[16];         // Two bytes per slot.
    const ATECC508Opts _opts;
    ATECCOpcodes  _current_op = ATECCOpcodes::UNDEF;


    inline bool slot_locked(uint8_t slot_number) {
      return (0x01 & (_slot_locks >> (slot_number & 0x0F)));
    };

    void internal_reset();
    int read_conf();

    int lock_zone();
    int lock_slot(uint8_t);

    int read_op_buffer();
    int start_operation(ATECCOpcodes, uint8_t* buf, uint16_t len);

    int send_wakeup();
};

#endif   // __ATECC508_SEC_DRIVER_H__
