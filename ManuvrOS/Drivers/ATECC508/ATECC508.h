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
#define ATECC508_CAPABILITY_CONFIG_UNLOCK

#define ATECC508_S_VER             0x01  // Version of the birth-cert serialization format.
#define ATECC508_C_VER             0x01  // Version of the ATEC carveup.


#define ATECC508_FLAG_OPS_RUNNING  0x00200000  // Are there running bus operations?
#define ATECC508_FLAG_BCERT_VALID  0x00400000  // Is the birth cert valid?
#define ATECC508_FLAG_BCERT_LOADED 0x00800000  // Is the birth cert loaded?
#define ATECC508_FLAG_AWAKE        0x01000000  // The part is believed to be awake.
#define ATECC508_FLAG_SYNCD        0x02000000  // The part is present and has been read.
#define ATECC508_FLAG_OTP_LOCKED   0x04000000  // The OTP zone is locked.
#define ATECC508_FLAG_CONF_LOCKED  0x08000000  // The conf zone is locked.
#define ATECC508_FLAG_DATA_LOCKED  0x10000000  // The data zone is locked.
#define ATECC508_FLAG_CONF_READ    0x20000000  // The config member is valid.
#define ATECC508_FLAG_PENDING_WAKE 0x80000000  // There is a wake sequence outstanding.


#define ATECC508_I2CADDR           0x60



enum class ATECCReturnCodes : uint8_t {
  SUCCESS                = 0x00, // Function succeeded.
  CONFIG_ZONE_LOCKED     = 0x01,
  DATA_ZONE_LOCKED       = 0x02,
  WAKE_FAILED            = 0xD0, // response status byte indicates CheckMac failure (status byte = 0x01)
  CHECKMAC_VERIFY_FAILED = 0xD1, // response status byte indicates CheckMac failure (status byte = 0x01)
  PARSE_ERROR            = 0xD2, // response status byte indicates parsing error (status byte = 0x03)
  STATUS_CRC             = 0xD4, // response status byte indicates CRC error (status byte = 0xFF)
  STATUS_UNKNOWN         = 0xD5, // response status byte is unknown
  STATUS_ECC             = 0xD6, // response status byte is ECC fault (status byte = 0x05)
  FUNC_FAIL              = 0xE0, // Function could not execute due to incorrect condition / state.
  GEN_FAIL               = 0xE1, // unspecified error
  BAD_PARAM              = 0xE2, // bad argument (out of range, null pointer, etc.)
  INVALID_ID             = 0xE3, // invalid device id, id not set
  INVALID_SIZE           = 0xE4, // Count value is out of range or greater than buffer size.
  BAD_CRC                = 0xE5, // incorrect CRC received
  RX_FAIL                = 0xE6, // Timed out while waiting for response. Number of bytes received is > 0.
  RX_NO_RESPONSE         = 0xE7, // Not an error while the Command layer is polling for a command response.
  RESYNC_WITH_WAKEUP     = 0xE8, // Re-synchronization succeeded, but only after generating a Wake-up
  PARITY_ERROR           = 0xE9, // for protocols needing parity
  TX_TIMEOUT             = 0xEA, // for Atmel PHY protocol, timeout on transmission waiting for master
  RX_TIMEOUT             = 0xEB, // for Atmel PHY protocol, timeout on receipt waiting for master
  COMM_FAIL              = 0xF0, // Communication with device failed. Same as in hardware dependent modules.
  TIMEOUT                = 0xF1, // Timed out while waiting for response. Number of bytes received is 0.
  BAD_OPCODE             = 0xF2, // opcode is not supported by the device
  WAKE_SUCCESS           = 0xF3, // received proper wake token
  EXECUTION_ERROR        = 0xF4, // chip was in a state where it could not execute the command, response status byte indicates command execution error (status byte = 0x0F)
  UNIMPLEMENTED          = 0xF5, // Function or some element of it hasn't been implemented yet
  ASSERT_FAILURE         = 0xF6, // Code failed run-time consistency check
  TX_FAIL                = 0xF7, // Failed to write
  NOT_LOCKED             = 0xF8, // required zone was not locked
  NO_DEVICES             = 0xF9, // For protocols that support device discovery (kit protocol), no devices were found
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
  L0  = 0x00,  // Data is this long.
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


/*
* This enum defines the different high-level operations supported by the driver.
* They allow us to tag the op_group with some context so that (maybe) dozens of
*   discrete asyncronous bus operations can be delt with in a sensible way.
*/
enum class ATECCHLOps : uint8_t {
  UNDEF       = 0x00,
  WAKEUP,
  READ_CONF,
  WRITE_CONF,
  READ_SLOT,
  WRITE_SLOT,
  READ_OTP,
  WRITE_OTP,
  LOCK_CONF,
  LOCK_SLOT,
  LOCK_OTP
};


/* This is stored in slot 8. It's size must be 416 bytes. */
typedef struct atecc_birth_cert_t {
  uint8_t  s_ver;           // Serialization version of this data
  uint8_t  c_ver;           // Carve-up version used on this ATECC
  uint8_t  hw_fam;          // Hardware family code
  uint8_t  hw_ver;          // Hardware version code
  uint8_t  lot_num[4];      // Manufacturer lot number
  uint8_t  make_str[24];    // Null-terminated string
  uint8_t  model_str[24];   // Null-terminated string
  uint64_t manu_date;       // Epoch time stamp of manufacture
  uint64_t prov_date;       // Epoch time stamp of provisioning
  UUID     hw_id;           // The UUID of the device this cert belongs to
  UUID     prov_id;         // UUID of the provisioning system
  uint8_t  prov_pub[64];    // The public key of the provisioner
  uint8_t  key_refs[128];   // Key ID strings
  uint8_t  manu_blob[116];  // Optional manufacturer blob
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
class ATECC508OpGroup {
  public:
    const ATECCHLOps hl_op;
    // TODO: Enum.
    //   -2:  mem failure
    //   -1:  bus failure
    //    0:  Unresolved
    //    1:  Success
    int8_t   op_state   = 0;
    uint16_t op_buf_len = 0;
    uint8_t* op_buf = nullptr;

    ATECC508OpGroup(const ATECCHLOps oc) : hl_op(oc) {};

    ATECC508OpGroup(const ATECCHLOps oc, const unsigned int _buf_sz) : ATECC508OpGroup(oc) {
      if (0 < _buf_sz) {
        op_buf_len = _buf_sz;
        op_buf = (uint8_t*) malloc(op_buf_len);
        op_state = (nullptr == op_buf) ? -2 : 0;
      }
    };

    ~ATECC508OpGroup() {
      if (nullptr != op_buf) {
        free(op_buf);
        op_buf = nullptr;
      }
    };

    inline int addBusOp(I2CBusOp* o) {
      return _op_q.insert(o);
    };

    inline I2CBusOp* nextBusOp() {
      return _op_q.dequeue();
    };

    inline int ops_remaining() {
      return _op_q.size();
    };


  private:
    PriorityQueue<I2CBusOp*> _op_q;
};



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
class ATECC508 :
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
      inline const char* consoleName() { return "ATECC508a";  };
      void consoleCmdProc(StringBuilder* input);
    #endif  //MANUVR_CONSOLE_SUPPORT

    /* Overrides from I2CDevice... */
    int8_t io_op_callahead(BusOp*);
    int8_t io_op_callback(BusOp*);
    void printDebug(StringBuilder*);

    void printSlotInfo(uint8_t slot, StringBuilder*);
    void printBirthCert(StringBuilder*);

    // TODO: Has potential to expose class contents to outside callers.
    inline uint8_t* serialNumber() {   return &_sn[0];  };

    void timed_service();

    #if defined(ATECC508_CAPABILITY_DEBUG)
    static const char* getZoneStr(ATECCZones);
    static const char* getPktTypeStr(ATECCPktCodes);
    static const char* getOpStr(ATECCOpcodes);
    static const char* getHLOpStr(ATECCHLOps);
    static const char* getReturnStr(ATECCReturnCodes);
    #endif  // ATECC508_CAPABILITY_DEBUG
    static unsigned int getOpTime(const ATECCOpcodes);


  protected:


  private:
    const ATECC508Opts _opts;
    StringBuilder local_log;
    ManuvrMsg _atec_service;
    PriorityQueue<ATECC508OpGroup*> _op_grps;
    ATECC508OpGroup* _current_grp   = nullptr;
    const unsigned long WD_TIMEOUT  = 1300;  // Chip goes to sleep after this many ms.
    unsigned long _last_action_time = 0;  // Tracks the last time the device was known to be awake.
    unsigned long _last_wake_sent   = 0;  // Tracks the last time we sent wake sequence.
    uint32_t      _atecc_flags      = 0;  // Flags related to maintaining the state machine.
    uint16_t      _addr_counter     = 0;  // Mirror of the device's internal address counter.
    uint16_t      _slot_locks       = 0;  // One bit per slot.

    uint8_t       _sn[9];                 // Serial number
    uint8_t       _otp_mode;              // OTP mode byte from config.
    uint8_t       _chip_mode;             // Chip mode byte from config.
    uint8_t       _config[128];           // Device configuration zone.

    SlotDef       _slot[16];              // Two bytes per slot.
    ATECBirthCert _birth_cert;            // The device's birth certificate.


    inline uint32_t _atec_flags() {                return _atecc_flags;            };
    inline bool _atec_flag(uint32_t _flag) {       return (_atecc_flags & _flag);  };
    inline void _atec_flip_flag(uint32_t _flag) {  _atecc_flags ^= _flag;          };
    inline void _atec_clear_flag(uint32_t _flag) { _atecc_flags &= ~_flag;         };
    inline void _atec_set_flag(uint32_t _flag) {   _atecc_flags |= _flag;          };
    inline void _atec_set_flag(uint32_t _flag, bool nu) {
      if (nu) _atecc_flags |= _flag;
      else    _atecc_flags &= ~_flag;
    };

    inline bool slot_locked(uint8_t slot_number) {
      return (0x01 & (_slot_locks >> (slot_number & 0x0F)));
    };

    void flushLocalLog();
    void internal_reset();

    /* Packet operations. */
    I2CBusOp* _rx_packet(ATECCDataSize, uint8_t*);
    I2CBusOp* _tx_packet(ATECCPktCodes, uint16_t, uint8_t*);
    inline I2CBusOp* _tx_packet(ATECCPktCodes pc, ATECCDataSize ds, uint8_t* buf) {
      return _tx_packet(pc, (uint16_t) ds, buf);
    };

    /*
    * I/O grouping functions
    */
    int8_t  _dispatch_op_grp(ATECC508OpGroup*);
    int8_t  _op_group_advance();
    int8_t  _clean_current_op_group();
    int8_t  _op_group_callback(ATECC508OpGroup*);
    inline bool _have_pending_ops() {
      return ((nullptr != _current_grp) || (0 < _op_grps.size()));
    };

    inline I2CBusOp* reset_operation() {   return _tx_packet(ATECCPktCodes::RESET, ATECCDataSize::L0, nullptr);  };
    inline I2CBusOp* sleep_operation() {   return _tx_packet(ATECCPktCodes::SLEEP, ATECCDataSize::L0, nullptr);  };
    inline I2CBusOp* idle_operation() {    return _tx_packet(ATECCPktCodes::IDLE,  ATECCDataSize::L0, nullptr);  };

    /*
    * Members for birth cert validation and management.
    */
    int8_t _read_birth_cert();
    int8_t generateBirthCert();
    int8_t getSlotByName(const char*);
    bool  _birth_cert_valid();

    /*
    * Slot zone convenience fxns.
    */
    int slot_read(uint8_t s);
    int slot_write(uint8_t s);
    int slot_lock(uint8_t s);
    void _slot_set(uint8_t idx, uint8_t* slot_conf, uint8_t* key_conf);

    /*
    * Config zone convenience fxns.
    */
    int config_read();
    int config_write(uint8_t new_config[128]);
    inline void _wipe_config() {    memset(_config, 0, 128);    };

    /*
    * OTP zone convenience fxns.
    */
    int otp_read();
    int otp_write(uint8_t* buf, uint16_t len);

    /*
    * Members for zone access and management.
    */
    int zone_lock(const ATECCZones, uint8_t slot, uint16_t crc);
    int zone_read(ATECC508OpGroup*, ATECCZones, uint16_t len, uint16_t addr);
    int zone_write(ATECCZones, uint8_t* buf, ATECCDataSize, uint16_t addr);

    /*
    * Low-level wakeup fxns.
    */
    bool need_wakeup();  // Wakeup related.
    int send_wakeup();   // Wakeup related.
    I2CBusOp* _wake_packet0();  // Return a BusOp for a wake packet.
    I2CBusOp* _wake_packet1();  // Return a BusOp for a wake packet.
};

#endif   // __ATECC508_SEC_DRIVER_H__
