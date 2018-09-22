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
This is not Atmel's general driver. It is a Manuvr-specific driver that imposes
  a usage pattern specific to Manuvr and uses the following slot carveup:

  0: H/W private key     // HW_PRI      Used to sign nonce for proof of original hardware.
  1: Firmware Auth Priv  // FW_PRI      Optionally used to bind firmware to hardware.
  2: API private key     // MVRAPI_PRI  Identifies this device to Manuvr's API.
  3: API ECDH key        // MVRAPI_ECDH Generated for API comm.
  4: <USER private key>  // UPRI0       Left open for the end-user.
  5: <USER ECDH key>     // UECDH0      Left open for the end-user.
  6: <USER private key>  // UPRI1       Left open for the end-user.
  7: <USER ECDH key>     // UECDH1      Left open for the end-user.
  8: Birth cert          // B_CERT      Placed at provisioning time.
  9: Provisioning sig    // PROV_SIG    The sig over the birth cert.
  A: Manuvr API Pub 0    // MVRAPI0     API key
  B: Manuvr API Pub 1    // MVRAPI1     Reserve API key
  C: Firmware Auth Pub   // FW_PUB      Used to unlock slots [1, 3].
  D: User unlock         // U1_UNLOCK   Used to unlock slots 6, 7, E.
  E: <USER>              // UPUB0       Left open for the end-user.
  F: <USER>              // UPUB1       Left open for the end-user.

  Monotonic counters and OTP region are unused, and are available for user code.

  All keys marked <USER> are left open to application manipulation following
    config locking. Users can use these to setup their own API hosts/services.

  The B_CERT slot is filled with a data structure that contains:
    * struct version                    1  A version of this data structure.
    * carve-up version                  1  The carve-up version.
    * hardware code                     1  Manufacturer-specific hardware code.
    * hardware version                  1  PCB revision code.
    * lot number                        4  Identifies a specific production run.
    * manufacture date                  8  Date of the unit's production.
    * hardware UUID                    16  The hardware's unique UUID.
    * manufacturer and model strings   48
    * manufacturer-specified data     116  Optional manufacturer data.

    * provisioner UUID                 16  The UUID of the provisioner.
    * provisioner public key           64  The provisioner's public key.
    * provisioning date                 8  The date this data was written.
    * string mappings to each slot    128  An array of strings naming the slots.
        HW_PRI
        FW_PRI
        MVRAPI_PRI
        MVRAPI_ECDH
        UPRI0
        UECDH0
        UPRI1
        UECDH1
        B_CERT
        PROV_SIG
        MVRAPI0
        MVRAPI1
        FW_PUB
        U1_UNLOCK
        UPUB0
        UPUB1
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

enum class ATECCReturnCodes : uint8_t {
  SUCCESS      = 0x00,
  MISCOMPARE   = 0x01,
  PARSE_ERR    = 0x03,
  ECC_FAULT    = 0x05,
  EXEC_ERR     = 0x0F,
  FRESH_WAKE   = 0x11,
  INSUF_TIME   = 0xEE,
  CRC_COMM_ERR = 0xFF
};


enum class ATECCPktCodes : uint8_t {
  RESET   = 0x00,
  SLEEP   = 0x01,
  IDLE    = 0x02,
  COMMAND = 0x03
};


enum class ATECCZones : uint8_t {
  CONF = 0x00,  // 128 bytes across 2 slots.
  OTP  = 0x01,  // 64 bytes across 2 slots.
  DATA = 0x02   // 1208 bytes across 16 slots.
};

enum class ATECCDataSize : uint8_t {
  L4  = 0x04,  // Data is this long.
  L32 = 0x20   // Data is this long.
};


enum class ATECCOpcodes : uint8_t {
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


typedef struct atecc_birth_cert_t {
  uint8_t  s_ver;
  uint8_t  c_ver;
  uint8_t  hw_code;
  uint8_t  hw_ver;
  uint8_t  lot_num[4];
  uint8_t  make_str[24];
  uint8_t  model_str[24];
  uint64_t manu_date;
  UUID     hw_id;
  UUID     prov_id;
  uint8_t  prov_pub[64];
  uint64_t prov_date;
  uint8_t  key_refs[128];
  uint8_t  manu_blob[116];
} ATECBirthCert;


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
  union {
    struct {
      uint16_t priv:         1;
      uint16_t pubinfo:      1;
      uint16_t key_type:     3;
      uint16_t lockabe:      1;
      uint16_t req_rand:     1;
      uint16_t req_auth:     1;
      uint16_t auth_key:     4;
      uint16_t RFU:          1;
      uint16_t intrusn_prot: 1;
      uint16_t x509_id:      2;
    };
    uint16_t val;
  } key;
} SlotDef;


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
      inline const char* const consoleName() { return getReceiverName();  };
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
    void printBirthCert(StringBuilder*);

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
    SlotDef       _slot[16];              // Two bytes per slot.
    ATECBirthCert _birth_cert;            // The device's birth certificate.

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
    int config_write();

    /*
    * OTP zone convenience fxns.
    */
    int otp_read();
    int otp_write(uint8_t* buf, uint16_t len);
};

#endif   // __ATECC508_SEC_DRIVER_H__
