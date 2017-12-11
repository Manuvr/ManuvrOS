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

NOTE: Some members, definitions, and capabilities are cased-off in the
  pre-processor for the sake of culling code that is useless in production and
  the presence of which might represent needless risk and dead-weight binary.

ATECC508_CAPABILITY_DEBUG
ATECC508_CAPABILITY_OTP_RW
ATECC508_CAPABILITY_CONFIG_UNLOCK

-- System constraints imposed by this class:
--------------------------------------------------------------------------------
  1) Only useful for i2c devices, although this limitation is fairly soft, and
      could be extended to the one-wire parts by replacing the BusQueue.
  2) Makes architectural assumptions that limit the system to one ATECC.

-- Use-case assumptions:
--------------------------------------------------------------------------------

*/

#ifndef __ATECC508_SEC_DRIVER_H__
#define __ATECC508_SEC_DRIVER_H__

#include <inttypes.h>
#include <stdint.h>
#include "Platform/Peripherals/I2C/I2CAdapter.h"
#ifdef MANUVR_CONSOLE_SUPPORT
  #include "XenoSession/Console/ManuvrConsole.h"
#endif


/* TODO: These ought to be migrated into config. */
#define ATECC508_CAPABILITY_DEBUG


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
  CONF = 0x00,  // 128 bytes across 2 slots.
  OTP  = 0x01,  // 64 bytes across 2 slots.
  DATA = 0x02   // 1208 bytes across 16 slots.
};

enum class ATECCDataSize {
  L4  = 0x04,  // Data is this long.
  L32 = 0x20   // Data is this long.
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


typedef struct atecc_slot_capabilities_t {
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
} SlotCapabilities;


/*
* Pin defs for this module.
* Set pin def to 255 to mark it as unused.
*/
class ATECC508Opts {
  public:
    // If valid, will use a gpio operation to pull the SDA pin low to wake device.
    const uint8_t pull_pin;

    ATECC508Opts(const ATECC508Opts* o) :
      pull_pin(o->pull_pin) {};

    ATECC508Opts(uint8_t _pull_pin) :
      pull_pin(_pull_pin) {};

    bool useGPIOWake() const {
      return (255 != pull_pin);
    };


  private:
};



// TODO: We are only extending EventReceiver while the driver is written.
class ATECC508 : public EventReceiver,
  #ifdef MANUVR_CONSOLE_SUPPORT
    public ConsoleInterface,
  #endif
    I2CDevice {
  public:
    ATECC508(const ATECC508Opts* o, const uint8_t addr);
    ATECC508(const ATECC508Opts* o) : ATECC508(o, ATECC508_I2CADDR) {};
    virtual ~ATECC508();

    int8_t init();

    #ifdef MANUVR_CONSOLE_SUPPORT
      /* Overrides from ConsoleInterface */
      uint consoleGetCmds(ConsoleCommand**);
      inline const char* consoleName() { return getReceiverName();  };
      void consoleCmdProc(StringBuilder* input);
    #endif  //MANUVR_CONSOLE_SUPPORT

    /* Overrides from I2CDevice... */
    int8_t io_op_callahead(BusOp*);
    int8_t io_op_callback(BusOp*);
    void printDebug(StringBuilder*);

    /* Overrides from EventReceiver */
    //int8_t notify(ManuvrMsg*);
    int8_t callback_proc(ManuvrMsg*);

    void printSlotInfo(uint8_t slot, StringBuilder*);

    #if defined(ATECC508_CAPABILITY_DEBUG)
    static const char* getPktTypeStr(ATECCPktCodes);
    static const char* getOpStr(ATECCOpcodes);
    static const char* getReturnStr(ATECCReturnCodes);
    #endif  // ATECC508_CAPABILITY_DEBUG
    static const unsigned int getOpTime(ATECCOpcodes);
    static ATECCReturnCodes   validateRC(ATECCOpcodes oc, uint8_t* buf);



  protected:
    int8_t attached();


  private:
    const unsigned long WD_TIMEOUT  = 1300;  // Chip goes to sleep after this many ms.
    unsigned long _last_action_time = 0;  // Tracks the last time the device was known to be awake.
    unsigned long _last_wake_sent   = 0;  // Tracks the last time we sent wake sequence.
    uint32_t      _atecc_flags      = 0;  // Flags related to maintaining the state machine.
    uint16_t      _addr_counter     = 0;  // Mirror of the device's internal address counter.
    uint16_t      _slot_locks       = 0;  // One bit per slot.
    SlotConf      _slot_conf[16];         // Two bytes per slot.
    const ATECC508Opts _opts;
    ATECCOpcodes  _current_op = ATECCOpcodes::UNDEF;


    inline bool slot_locked(uint8_t slot_number) {
      return (0x01 & (_slot_locks >> (slot_number & 0x0F)));
    };

    void internal_reset();

    /* Packet operations. */
    int read_op_buffer(ATECCDataSize);
    int dispatch_packet(ATECCPktCodes, uint8_t* buf, uint16_t len);
    int command_operation(uint8_t* buf, uint16_t len);
    inline int idle_operation() {    return dispatch_packet(ATECCPktCodes::IDLE,  nullptr, 0);  };
    inline int sleep_operation() {   return dispatch_packet(ATECCPktCodes::SLEEP, nullptr, 0);  };
    inline int reset_operation() {   return dispatch_packet(ATECCPktCodes::RESET, nullptr, 0);  };

    bool need_wakeup();  // Wakeup related.
    int send_wakeup();   // Wakeup related.

    /*
    * Members for zone access and management.
    */
    int zone_lock();
    int zone_read(ATECCZones, ATECCDataSize, uint16_t addr);
    int zone_write(ATECCZones, uint8_t* buf, ATECCDataSize, uint16_t addr);


    /*
    * Slot zone convenience fxns.
    */
    int slot_read(uint8_t s);
    int slot_write(uint8_t s);
    int slot_lock(uint8_t s);

    /*
    * Config zone convenience fxns.
    */
    int config_read();
    int config_write(uint8_t* buf, uint16_t len);

    /*
    * OTP zone convenience fxns.
    */
    int otp_read();
    int otp_write(uint8_t* buf, uint16_t len);
};

#endif   // __ATECC508_SEC_DRIVER_H__
