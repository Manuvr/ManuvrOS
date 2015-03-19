#include "ManuvrMsg.h"
#include "FirmwareDefs.h"
#include <string.h>

extern inline double   parseDoubleFromchars(unsigned char *input);
extern inline float    parseFloatFromchars(unsigned char *input);
extern inline uint32_t parseUint32Fromchars(unsigned char *input);
extern inline uint16_t parseUint16Fromchars(unsigned char *input);


/****************************************************************************************************
* Static initializers                                                                               *
****************************************************************************************************/
PriorityQueue<const MessageTypeDef*> ManuvrMsg::message_defs_extended;

const unsigned char ManuvrMsg::MSG_ARGS_NONE[] = {0};      // Generic argument def for a message with no args.
  
// Generics for messages that have one arg of a single type arg, or no args.
const unsigned char ManuvrMsg::MSG_ARGS_U8[]  = {UINT8_FM,  0, 0}; 
const unsigned char ManuvrMsg::MSG_ARGS_U16[] = {UINT16_FM, 0, 0}; 
const unsigned char ManuvrMsg::MSG_ARGS_U32[] = {UINT32_FM, 0, 0}; 
const unsigned char ManuvrMsg::MSG_ARGS_STR_BUILDER[] = {STR_BUILDER_FM, 0, 0}; 

// Generics for messages that have Two types.
const unsigned char ManuvrMsg::MSG_ARGS_U8_U8[]   = {UINT8_FM, UINT8_FM,  0, UINT8_FM, 0, 0};
const unsigned char ManuvrMsg::MSG_ARGS_U8_U32[]  = {UINT8_FM, UINT32_FM, 0, UINT8_FM, 0, 0};

// Generics for messages that have three types.
const unsigned char ManuvrMsg::MSG_ARGS_U8_U32_I16[]  = {
  UINT8_FM, 0,
  UINT8_FM, UINT32_FM, 0, 
  UINT8_FM, UINT32_FM, INT16_FM, 0, 
  0};

/* 
* This is the argument form for messages that either...
*   a) Need a free-form type that we don't support natively
*   b) Are special-cases (COUNTERPARTY_REPLY) that will have their args cast after validation.
*/
const unsigned char ManuvrMsg::MSG_ARGS_BINBLOB[] = {
  BINARY_FM, 0, 
  0}; 


                                     
const unsigned char ManuvrMsg::MSG_ARGS_POWER_MODE[] = {
  UINT8_FM, 0,             // 1 bytes: Statement about current power profile.
  0};                      // 0 bytes: Request for present power profile.
                            
  



/*
* These are the hard-coded message types that the program knows about.
* This is where we decide what kind of arguments each message is capable of carrying.
*
* Until someone does something smarter, there is no enforcemnt of types in this definition. Only 
*   cardinality. Trying to get the type as something incompatible should return an error, but this
*   leaves type definition as a negotiable-matter between the code that wrote the message, and the
*   code reading it. We can also add a type string, as I had in BridgeBox, but this can also be 
*   dynamically built.
*/
const MessageTypeDef ManuvrMsg::message_defs[] = {


/* Reserved codes */
  {  MANUVR_MSG_UNDEFINED            , 0x0000,               "<UNDEFINED>"          , MSG_ARGS_NONE }, // This should be the first entry for failure cases.

  // Protocol mechanisms for QoS, and fault-tolerence.
  {  MANUVR_MSG_REPLY_PARSE_FAIL     , 0x0000,               "REPLY_PARSE_FAIL"     , MSG_ARGS_NONE }, //  This reply denotes that the packet failed to parse (despite passing checksum).
  {  MANUVR_MSG_REPLY_RETRY          , 0x0000,               "REPLY_RETRY"          , MSG_ARGS_NONE }, //  This reply asks for a reply of the given Unique ID.
  {  MANUVR_MSG_REPLY                , 0x0000,               "REPLY"                , MSG_ARGS_NONE }, //  This reply is for success-case.

/* System codes */                                               
  {  MANUVR_MSG_SYS_BOOTLOADER       , 0x0000,               "SYS_BOOTLOADER"       , MSG_ARGS_NONE }, // Reboots into the STM32F4 bootloader.
  {  MANUVR_MSG_SYS_REBOOT           , 0x0000,               "SYS_REBOOT"           , MSG_ARGS_NONE }, // Reboots into THIS program.
  {  MANUVR_MSG_SYS_SHUTDOWN         , 0x0000,               "SYS_SHUTDOWN"         , MSG_ARGS_NONE }, // Raised when the system is pending complete shutdown.

  {  MANUVR_MSG_PROGRAM_START        , 0x0000,               "PROGRAM_START"        , MSG_ARGS_STR_BUILDER }, // Starting an application on the receiver. Needs a string. 
  
  {  MANUVR_MSG_LEGEND_TYPES         , 0x0000,               "LEGEND_TYPES"         , MSG_ARGS_NONE }, // No args? Asking for this legend. One arg: Legend provided.
  {  MANUVR_MSG_LEGEND_MESSAGES      , 0x0000,               "LEGEND_MESSAGES"      , MSG_ARGS_BINBLOB }, // No args? Asking for this legend. One arg: Legend provided.

  {  MANUVR_MSG_SYS_DATETIME_CHANGED , 0x0000,               "SYS_DATETIME_CHANGED" , MSG_ARGS_NONE }, // Raised when the system time changes.
  {  MANUVR_MSG_SYS_SET_DATETIME     , 0x0000,               "SYS_SET_DATETIME"     , MSG_ARGS_NONE }, //
  {  MANUVR_MSG_SYS_REPORT_DATETIME  , 0x0000,               "SYS_REPORT_DATETIME"  , MSG_ARGS_NONE }, //

  {  MANUVR_MSG_SYS_POWER_MODE       , 0x0000,               "SYS_POWER_MODE"       , MSG_ARGS_POWER_MODE }, // 
  {  MANUVR_MSG_SYS_LOG_VERBOSITY    , 0x0000,               "SYS_LOG_VERBOSITY"    , MSG_ARGS_U8 },   // This tells client classes to adjust their log verbosity.

  {  MANUVR_MSG_SYS_BOOT_COMPLETED   , 0x0000,               "SYS_BOOT_COMPLETED"   , MSG_ARGS_NONE }, // Raised when bootstrap is finished.
  {  MANUVR_MSG_SYS_PREALLOCATION    , 0x0000,               "SYS_PREALLOCATION"    , MSG_ARGS_NONE }, // Any classes that do preallocation should listen for this.

  {  MANUVR_MSG_SYS_ISSUE_LOG_ITEM   , 0x0000,               "SYS_ISSUE_LOG_ITEM"   , MSG_ARGS_STR_BUILDER }, // Classes emit this to get their log data saved/sent.

  {  MANUVR_MSG_USER_BUTTON_PRESS     , 0x0000,               "USER_BUTTON_PRESS",    MSG_ARGS_U8 },   // The user pushed a button with the given integer code.
  {  MANUVR_MSG_USER_BUTTON_RELEASE   , 0x0000,               "USER_BUTTON_RELEASE",  MSG_ARGS_U8 },   // The user released a button with the given integer code.
  
  // USB VCP stuff...
  {  MANUVR_MSG_SYS_USB_CONNECT      , 0x0000,               "SYS_USB_CONNECT"      , MSG_ARGS_NONE }, // 
  {  MANUVR_MSG_SYS_USB_DISCONNECT   , 0x0000,               "SYS_USB_DISCONNECT"   , MSG_ARGS_NONE }, // 
  {  MANUVR_MSG_SYS_USB_CONFIGURED   , 0x0000,               "SYS_USB_CONFIGURED"   , MSG_ARGS_NONE }, // 
  {  MANUVR_MSG_SYS_USB_SUSPEND      , 0x0000,               "SYS_USB_SUSPEND"      , MSG_ARGS_NONE }, // 
  {  MANUVR_MSG_SYS_USB_RESUME       , 0x0000,               "SYS_USB_RESUME"       , MSG_ARGS_NONE }, // 
  {  MANUVR_MSG_SYS_USB_RESET        , 0x0000,               "SYS_USB_RESET"        , MSG_ARGS_NONE }, // 

  
  {  MANUVR_MSG_USER_DEBUG_INPUT     , 0x0000,               "USER_DEBUG_INPUT"     , MSG_ARGS_STR_BUILDER }, // 

  {  MANUVR_MSG_SCHED_ENABLE_BY_PID  , 0x0000,               "SCHED_ENABLE_BY_PID"  , MSG_ARGS_NONE }, // The given PID is being enabled.
  {  MANUVR_MSG_SCHED_DISABLE_BY_PID , 0x0000,               "SCHED_DISABLE_BY_PID" , MSG_ARGS_NONE }, // The given PID is being disabled.
  {  MANUVR_MSG_SCHED_PROFILER_START , 0x0000,               "SCHED_PROFILER_START" , MSG_ARGS_NONE }, // We want to profile the given PID.
  {  MANUVR_MSG_SCHED_PROFILER_STOP  , 0x0000,               "SCHED_PROFILER_STOP"  , MSG_ARGS_NONE }, // We want to stop profiling the given PID.
  {  MANUVR_MSG_SCHED_PROFILER_DUMP  , 0x0000,               "SCHED_PROFILER_DUMP"  , MSG_ARGS_NONE }, // Dump the profiler data for all PIDs (no args) or given PIDs.
  {  MANUVR_MSG_SCHED_DUMP_META      , 0x0000,               "SCHED_DUMP_META"      , MSG_ARGS_NONE }, // Tell the Scheduler to dump its meta metrics.
  {  MANUVR_MSG_SCHED_DUMP_SCHEDULES , 0x0000,               "SCHED_DUMP_SCHEDULES" , MSG_ARGS_NONE }, // Tell the Scheduler to dump schedules.
  {  MANUVR_MSG_SCHED_WIPE_PROFILER  , 0x0000,               "SCHED_WIPE_PROFILER"  , MSG_ARGS_NONE }, // Tell the Scheduler to wipe its profiler data. Pass PIDs to be selective.
  {  MANUVR_MSG_SCHED_DEFERRED_EVENT , 0x0000,               "SCHED_DEFERRED_EVENT" , MSG_ARGS_NONE }, // Tell the Scheduler to broadcast the attached Event so many ms into the future.

  /* Roving Networks Bluetooth radio codes */
  {  MANUVR_MSG_BT_CONNECTION_LOST   , 0x0000,               "BT_CONNECTION_LOST"   , MSG_ARGS_NONE }, // 
  {  MANUVR_MSG_BT_CONNECTION_GAINED , 0x0000,               "BT_CONNECTION_GAINED" , MSG_ARGS_NONE }, // 
  {  MANUVR_MSG_BT_QUEUE_READY       , MSG_FLAG_IDEMPOTENT,  "BT_QUEUE_READY"       , MSG_ARGS_NONE }, // There is action possible in the bluetooth queue.
  {  MANUVR_MSG_BT_RX_BUF_NOT_EMPTY  , 0x0000,               "BT_RX_BUF_NOT_EMPTY"  , MSG_ARGS_NONE }, // The host sent us data without indication of an end.
  {  MANUVR_MSG_BT_ENTERED_CMD_MODE  , 0x0000,               "BT_ENTERED_CMD_MODE"  , MSG_ARGS_NONE }, // The module entered command mode.
  {  MANUVR_MSG_BT_EXITED_CMD_MODE   , 0x0000,               "BT_EXITED_CMD_MODE"   , MSG_ARGS_NONE }, // The module exited command mode.

  {  MANUVR_MSG_I2C_QUEUE_READY      , MSG_FLAG_IDEMPOTENT,  "I2C_QUEUE_READY"      , MSG_ARGS_NONE }, // The i2c queue is ready for attention.
  {  MANUVR_MSG_I2C_DUMP_DEBUG       , 0x0000,               "I2C_DUMP_DEBUG"       , MSG_ARGS_NONE }, // Debug dump for i2c.
  {  MANUVR_MSG_SPI_QUEUE_READY      , MSG_FLAG_IDEMPOTENT,  "SPI_QUEUE_READY"      , MSG_ARGS_NONE }, // SPI queue for the CPLD is ready for attention.
  {  MANUVR_MSG_SPI_CB_QUEUE_READY   , MSG_FLAG_IDEMPOTENT,  "SPI_CALLBACK_READY"   , MSG_ARGS_NONE }, // One or more completed SPI jobs is waiting for callback service.

  {  MANUVR_MSG_RNG_BUFFER_EMPTY     , 0x0000,               "RNG_BUFFER_EMPTY"     , MSG_ARGS_NONE }, // The RNG couldn't keep up with our entropy demands.
  {  MANUVR_MSG_INTERRUPTS_MASKED    , 0x0000,               "INTERRUPTS_MASKED"    , MSG_ARGS_NONE }, // Anything that depends on interrupts is now broken.
  {  MANUVR_MSG_SENSOR_INA219        , 0x0000,               "SENSOR_INA219"        , MSG_ARGS_NONE }, // The current sensor has something to say.
  {  MANUVR_MSG_SENSOR_ISL29033      , 0x0000,               "SENSOR_ISL29033"      , MSG_ARGS_NONE }, // The light sensor has something to say.
  {  MANUVR_MSG_SENSOR_LPS331        , 0x0000,               "SENSOR_LPS331"        , MSG_ARGS_NONE }, // The baro sensor has something to say.
  {  MANUVR_MSG_SENSOR_SI7021        , 0x0000,               "SENSOR_SI7021"        , MSG_ARGS_NONE }, // The humidity sensor has something to say.
  {  MANUVR_MSG_SENSOR_TMP006        , 0x0000,               "SENSOR_TMP006"        , MSG_ARGS_NONE }, // The thermopile has something to say.
  {  MANUVR_MSG_DIRTY_FRAME_BUF      , 0x0000,               "DIRTY_FRAME_BUF"      , MSG_ARGS_NONE }, // Something changed the framebuffer and we need to redraw.
                                              
  {  MANUVR_MSG_SD_EJECTED           , 0x0000,               "SD_EJECTED"           , MSG_ARGS_NONE }, // The SD card was just ejected.
  {  MANUVR_MSG_SD_INSERTED          , 0x0000,               "SD_INSERTED"          , MSG_ARGS_NONE }, // An SD card was inserted.
  {  MANUVR_MSG_SESS_ESTABLISHED     , 0x0000,               "SESS_ESTABLISHED"     , MSG_ARGS_NONE }, // Session established.
  {  MANUVR_MSG_SESS_HANGUP          , 0x0000,               "SESS_HANGUP"          , MSG_ARGS_NONE }, // Session hangup.
  {  MANUVR_MSG_SESS_AUTH_CHALLENGE  , 0x0000,               "SESS_AUTH_CHALLENGE"  , MSG_ARGS_NONE }, // A code for challenge-response authentication.
  {  MANUVR_MSG_SESS_SUBCRIBE        , 0x0000,               "SESS_SUBCRIBE"        , MSG_ARGS_NONE }, // Used to subscribe this session to other events.
  {  MANUVR_MSG_SESS_UNSUBCRIBE      , 0x0000,               "SESS_UNSUBCRIBE"      , MSG_ARGS_NONE }, // Used to unsubscribe this session from other events.
  {  MANUVR_MSG_SESS_DUMP_DEBUG      , 0x0000,               "SESS_DUMP_DEBUG"      , MSG_ARGS_NONE }, // 
  {  MANUVR_MSG_SESS_ORIGINATE_MSG   , 0x0000,               "SESS_ORIGINATE_MSG"   , MSG_ARGS_NONE }, // 
};





/**
* Vanilla constructor.
*/
ManuvrMsg::ManuvrMsg(void) {
  event_code = MANUVR_MSG_UNDEFINED;
  __class_initializer();
}


/**
* Constructor that specifies the identity.
*
* @param code   The identity code for the new message.
*/
ManuvrMsg::ManuvrMsg(uint16_t code) {
  event_code = code;
  __class_initializer();
}


/**
* Destructor. Clears all Arguments. Memory management is accounted
*   for in each Argument, so no need to worry about that here.
*/
ManuvrMsg::~ManuvrMsg(void) {
  clearArgs();
}


/**
* This is here for compatibility with C++ standards that do not allow for definition and declaration
*   in the header file. Takes no parameters, and returns nothing.
*
* The nature of this class makes this a better design, anyhow. Since the same object is often
*   repurposed many times, doing any sort of init in the constructor should probably be avoided.
*/
void ManuvrMsg::__class_initializer() {
  clearArgs();
  message_def = lookupMsgDefByCode(event_code);
}


/**
* Call this member to repurpose this message for an unrelated task. This mechanism
*   is designed to prevent malloc()/free() thrash where it can be avoided.
* 
* @param code   The new identity code for the message.
*/
int8_t ManuvrMsg::repurpose(uint16_t code) {
  event_code = code;
  __class_initializer();
  return 0;
}


/**
* Allows the caller to count the Args attached to this message.
* TODO: Inlinable.
*
* @return the cardinality of the argument list.
*/
int ManuvrMsg::argCount() {
  return args.size();
}


/**
* Allows the caller to plan other allocations based on how many bytes this message's
*   Arguments occupy.
*
* @return the length (in bytes) of the arguments for this message.
*/
int ManuvrMsg::argByteCount() {
  int return_value = 0;
  for (int i = 0; i < args.size(); i++) {
    return_value += args.get(i)->len;
  }
  return return_value;
}


/**
* This fxn is called when we have a slew of bytes from which we need to build Arguments.
* Because it came from outside our walls, this function should never encounter pointer types.
*
* @param  buffer   The pointer to the data.
* @param  len      How long is that buffer?
* @return the number of Arguments extracted and instantiated from the buffer.
*/
uint8_t ManuvrMsg::inflateArgumentsFromBuffer(unsigned char *buffer, int len) {
  int return_value = 0;
  Argument *nu_arg = NULL;
  while ((buffer != NULL) & (len > 1)) {
    //uint8_t arg_len = *(buffer + 1);
    switch (*buffer) {
      case INT8_FM:
        nu_arg = new Argument((int8_t) *(buffer+2));
        len = len - 3;
        buffer += 3;
        break;
      case UINT8_FM:
        nu_arg = new Argument((uint8_t) *(buffer+2));
        len = len - 3;
        buffer += 3;
        break;
      case INT16_FM:
        nu_arg = new Argument((int16_t) parseUint16Fromchars(buffer+2));
        len = len - 4;
        buffer += 4;
        break;
      case UINT16_FM:
        nu_arg = new Argument(parseUint16Fromchars(buffer+2));
        len = len - 4;
        buffer += 4;
        break;
      case INT32_FM:
        nu_arg = new Argument((int32_t) parseUint32Fromchars(buffer+2));
        len = len - 6;
        buffer += 6;
        break;
      case UINT32_FM:
        nu_arg = new Argument(parseUint32Fromchars(buffer+2));
        len = len - 6;
        buffer += 6;
        break;
      case FLOAT_FM:
        nu_arg = new Argument((float) parseUint32Fromchars(buffer+2));
        len = len - 6;
        buffer += 6;
        break;
      default:
        break;
    }

    if (nu_arg != NULL) {
      args.insert(nu_arg);
      nu_arg = NULL;
      return_value++;
    }
  }
  return return_value;
}


/*
* This function is for the exclusive purpose of inflating an argument from a place where a
*   pointer doesn't make sense. This means that an argument mode that contains a non-exportable 
*   pointer type will not produce the expected result. It might even asplode.
* Returns the number of Arguments extracted from the buffer.
*/
uint8_t ManuvrMsg::inflateArgumentsFromRawBuffer(unsigned char *buffer, int len) {
  int return_value = 0;
  if (NULL == buffer) return 0;
  if (0 == len)       return 0;
  
  char* arg_mode  = is_valid_argument_buffer(len);
  
  if (NULL == arg_mode) return 0;

  Argument *nu_arg = NULL;
  while ((NULL != arg_mode) & (len > 0)) {
    switch ((unsigned char) *arg_mode) {
      case INT8_FM:
        nu_arg = new Argument((int8_t) *(buffer));
        len--;
        buffer++;
        break;
      case UINT8_FM:
        nu_arg = new Argument((uint8_t) *(buffer));
        len--;
        buffer++;
        break;
      case INT16_FM:
        nu_arg = new Argument((int16_t) parseUint16Fromchars(buffer));
        len = len - 2;
        buffer += 2;
        break;
      case UINT16_FM:
        nu_arg = new Argument(parseUint16Fromchars(buffer));
        len = len - 2;
        buffer += 2;
        break;
      case INT32_FM:
        nu_arg = new Argument((int32_t) parseUint32Fromchars(buffer));
        len = len - 4;
        buffer += 4;
        break;
      case UINT32_FM:
        nu_arg = new Argument(parseUint32Fromchars(buffer));
        len = len - 4;
        buffer += 4;
        break;
      case FLOAT_FM:
        nu_arg = new Argument((float) parseUint32Fromchars(buffer));
        len = len - 4;
        buffer += 4;
        break;
      case VECT_3_FLOAT:
        nu_arg = new Argument(new Vector3f(parseFloatFromchars(buffer + 0), parseFloatFromchars(buffer + 4), parseFloatFromchars(buffer + 8)));
        len = len - 12;
        buffer += 12;
        nu_arg->reap = true;
        break;
      case VECT_3_UINT16:
        nu_arg = new Argument(new Vector3ui16(parseUint16Fromchars(buffer + 0), parseUint16Fromchars(buffer + 2), parseUint16Fromchars(buffer + 4)));
        len = len - 6;
        buffer += 6;
        nu_arg->reap = true;
        break;
      case VECT_3_INT16:
        nu_arg = new Argument(new Vector3i16((int16_t) parseUint16Fromchars(buffer + 0), (int16_t) parseUint16Fromchars(buffer + 2), (int16_t) parseUint16Fromchars(buffer + 4)));
        len = len - 6;
        buffer += 6;
        nu_arg->reap = true;
        break;
      default:
        // Abort parse. We encountered a type we can't deal with.
        args.clear();
        return 0;
        break;
    }

    if (nu_arg != NULL) {
      args.insert(nu_arg);
      nu_arg = NULL;
      return_value++;
    }
    arg_mode++;
  }
  return return_value;
}



/**
* Marks the given Argument as reapable or not. This is useful to avoid a malloc/copy/free cycle
*   for constants or flash-resident strings.
*
* @param  idx   The Argument position
* @param  reap  True if the Argument should be reaped with the message, false if it should be retained.
* @return 1 on success, 0 on failure.
*/
int8_t ManuvrMsg::markArgForReap(int idx, bool reap) {
  if (args.size() > idx) {
    args.get(idx)->reap = reap;
    return 1;
  }
  return 0;
}


/****************************************************************************************************
* These are only here for the sake of convenience and comprehensibility elsewhere.                  *
* Prevents client classes from having to do casting and type manipulation.                          *
* TODO: Would like to see these inlined. Not sure how without throwing the linker into a rage.      *
****************************************************************************************************/

/*
* Overrides for Argument constructor access. Prevents outside classes from having to care about
*   implementation details of Arguments. Might look ugly, but takes the CPU burden off of runtime
*   and forces the compiler to deal with it.
*/
int ManuvrMsg::addArg(uint8_t val) {             return args.insert(new Argument(val));   }
int ManuvrMsg::addArg(uint16_t val) {            return args.insert(new Argument(val));   }
int ManuvrMsg::addArg(uint32_t val) {            return args.insert(new Argument(val));   }
int ManuvrMsg::addArg(int8_t val) {              return args.insert(new Argument(val));   }
int ManuvrMsg::addArg(int16_t val) {             return args.insert(new Argument(val));   }
int ManuvrMsg::addArg(int32_t val) {             return args.insert(new Argument(val));   }
int ManuvrMsg::addArg(float val) {               return args.insert(new Argument(val));   }

int ManuvrMsg::addArg(uint8_t *val) {            return args.insert(new Argument(val));   }
int ManuvrMsg::addArg(uint16_t *val) {           return args.insert(new Argument(val));   }
int ManuvrMsg::addArg(uint32_t *val) {           return args.insert(new Argument(val));   }
int ManuvrMsg::addArg(int8_t *val) {             return args.insert(new Argument(val));   }
int ManuvrMsg::addArg(int16_t *val) {            return args.insert(new Argument(val));   }
int ManuvrMsg::addArg(int32_t *val) {            return args.insert(new Argument(val));   }
int ManuvrMsg::addArg(float *val) {              return args.insert(new Argument(val));   }

int ManuvrMsg::addArg(Vector3ui16 *val) {        return args.insert(new Argument(val));   }
int ManuvrMsg::addArg(Vector3i16 *val) {         return args.insert(new Argument(val));   }
int ManuvrMsg::addArg(Vector3f *val) {           return args.insert(new Argument(val));   }

int ManuvrMsg::addArg(void *val, int len) {      return args.insert(new Argument(val, len));   }
int ManuvrMsg::addArg(const char *val) {         return args.insert(new Argument(val));   }
int ManuvrMsg::addArg(StringBuilder *val) {      return args.insert(new Argument(val));   }
int ManuvrMsg::addArg(StaticHub *val) {          return args.insert(new Argument(val));   }
int ManuvrMsg::addArg(Scheduler *val) {          return args.insert(new Argument(val));   }
int ManuvrMsg::addArg(ManuvrEvent *val) {   return args.insert(new Argument(val));   }


/*
* Overrides for Argument retreival. Pass in pointer to the type the argument should be retreived as.
* Returns an error code if the types don't match.
*/
// These accessors treat the (void*) as 4 bytes of data storage.
int8_t ManuvrMsg::consumeArgAs(int8_t *trg_buf) {       return getArgAs(0, (void*) trg_buf, false);  }
int8_t ManuvrMsg::consumeArgAs(uint8_t *trg_buf) {      return getArgAs(0, (void*) trg_buf, false);  }
int8_t ManuvrMsg::consumeArgAs(int16_t *trg_buf) {      return getArgAs(0, (void*) trg_buf, false);  }
int8_t ManuvrMsg::consumeArgAs(uint16_t *trg_buf) {     return getArgAs(0, (void*) trg_buf, false);  }
int8_t ManuvrMsg::consumeArgAs(int32_t *trg_buf) {      return getArgAs(0, (void*) trg_buf, false);  }
int8_t ManuvrMsg::consumeArgAs(uint32_t *trg_buf) {     return getArgAs(0, (void*) trg_buf, false);  }
int8_t ManuvrMsg::consumeArgAs(float *trg_buf) {        return getArgAs(0, (void*) trg_buf, false);  }
int8_t ManuvrMsg::consumeArgAs(ManuvrEvent **trg_buf) {  return getArgAs(0, (void*) trg_buf, false);  }

int8_t ManuvrMsg::getArgAs(int8_t *trg_buf) {           return getArgAs(0, (void*) trg_buf, true);  }
int8_t ManuvrMsg::getArgAs(uint8_t *trg_buf) {          return getArgAs(0, (void*) trg_buf, true);  }
int8_t ManuvrMsg::getArgAs(int16_t *trg_buf) {          return getArgAs(0, (void*) trg_buf, true);  }
int8_t ManuvrMsg::getArgAs(uint16_t *trg_buf) {         return getArgAs(0, (void*) trg_buf, true);  }
int8_t ManuvrMsg::getArgAs(int32_t *trg_buf) {          return getArgAs(0, (void*) trg_buf, true);  }
int8_t ManuvrMsg::getArgAs(uint32_t *trg_buf) {         return getArgAs(0, (void*) trg_buf, true);  }
int8_t ManuvrMsg::getArgAs(float *trg_buf) {            return getArgAs(0, (void*) trg_buf, true);  }

int8_t ManuvrMsg::getArgAs(uint8_t idx, uint8_t  *trg_buf) {     return getArgAs(idx, (void*) trg_buf, true);  }
int8_t ManuvrMsg::getArgAs(uint8_t idx, uint16_t *trg_buf) {     return getArgAs(idx, (void*) trg_buf, true);  }
int8_t ManuvrMsg::getArgAs(uint8_t idx, uint32_t *trg_buf) {     return getArgAs(idx, (void*) trg_buf, true);  }
int8_t ManuvrMsg::getArgAs(uint8_t idx, int8_t   *trg_buf) {     return getArgAs(idx, (void*) trg_buf, true);  }
int8_t ManuvrMsg::getArgAs(uint8_t idx, int16_t  *trg_buf) {     return getArgAs(idx, (void*) trg_buf, true);  }
int8_t ManuvrMsg::getArgAs(uint8_t idx, int32_t  *trg_buf) {     return getArgAs(idx, (void*) trg_buf, true);  }
int8_t ManuvrMsg::getArgAs(uint8_t idx, float  *trg_buf) {       return getArgAs(idx, (void*) trg_buf, true);  }

// These accessors treat the (void*) as a pointer. These functions are essentially type-casts.
int8_t ManuvrMsg::getArgAs(Vector3f **trg_buf) {           return getArgAs(0, (void*) trg_buf, true);  }
int8_t ManuvrMsg::getArgAs(Vector3ui16 **trg_buf) {        return getArgAs(0, (void*) trg_buf, true);  }
int8_t ManuvrMsg::getArgAs(Vector3i16 **trg_buf) {         return getArgAs(0, (void*) trg_buf, true);  }
int8_t ManuvrMsg::getArgAs(const char **trg_buf) {         return getArgAs(0, (void*) trg_buf, true);  }
int8_t ManuvrMsg::getArgAs(StringBuilder **trg_buf) {      return getArgAs(0, (void*) trg_buf, true);  }
int8_t ManuvrMsg::getArgAs(StaticHub **trg_buf) {          return getArgAs(0, (void*) trg_buf, true);  }
int8_t ManuvrMsg::getArgAs(Scheduler **trg_buf) {          return getArgAs(0, (void*) trg_buf, true);  }
int8_t ManuvrMsg::getArgAs(ManuvrEvent **trg_buf) {   return getArgAs(0, (void*) trg_buf, true);  }

int8_t ManuvrMsg::getArgAs(uint8_t idx, Vector3f  **trg_buf) {          return getArgAs(idx, (void*) trg_buf, true);  }
int8_t ManuvrMsg::getArgAs(uint8_t idx, Vector3ui16  **trg_buf) {       return getArgAs(idx, (void*) trg_buf, true);  }
int8_t ManuvrMsg::getArgAs(uint8_t idx, Vector3i16  **trg_buf) {        return getArgAs(idx, (void*) trg_buf, true);  }
int8_t ManuvrMsg::getArgAs(uint8_t idx, const char  **trg_buf) {        return getArgAs(idx, (void*) trg_buf, true);  }
int8_t ManuvrMsg::getArgAs(uint8_t idx, StringBuilder  **trg_buf) {     return getArgAs(idx, (void*) trg_buf, true);  }
int8_t ManuvrMsg::getArgAs(uint8_t idx, StaticHub  **trg_buf) {         return getArgAs(idx, (void*) trg_buf, true);  }
int8_t ManuvrMsg::getArgAs(uint8_t idx, Scheduler  **trg_buf) {         return getArgAs(idx, (void*) trg_buf, true);  }
int8_t ManuvrMsg::getArgAs(uint8_t idx, ManuvrEvent  **trg_buf) {  return getArgAs(idx, (void*) trg_buf, true);  }


/**
* All of the type-specialized getArgAs() fxns boil down to this. Which is private.
* The boolean preserve parameter will leave the argument attached (if true), or destroy it (if false).
*
* @param  idx      The Argument position
* @param  trg_buf  A pointer to the place where we should write the result.
* @param  preserve Should the Argument be removed from this Message following this operation?
* @return DIG_MSG_ERROR_NO_ERROR or appropriate failure code.
*/
int8_t ManuvrMsg::getArgAs(uint8_t idx, void *trg_buf, bool preserve) {
  int8_t return_value = DIG_MSG_ERROR_INVALID_ARG;
  Argument* arg = NULL;
  if (args.size() > idx) {
    arg = args.get(idx);
    switch (arg->type_code) {
      case INT8_FM:    // This frightens the compiler. Its fears are unfounded.
      case UINT8_FM:   // This frightens the compiler. Its fears are unfounded.
        return_value = DIG_MSG_ERROR_NO_ERROR;
        *((uint8_t*) trg_buf) = (uint8_t) (((uint32_t)arg->target_mem) & 0x000000FF);
        break;
      case INT16_FM:    // This frightens the compiler. Its fears are unfounded.
      case UINT16_FM:   // This frightens the compiler. Its fears are unfounded.
        return_value = DIG_MSG_ERROR_NO_ERROR;
        *((uint16_t*) trg_buf) = (uint16_t) (((uint32_t)arg->target_mem) & 0x0000FFFF);
        break;
      case INT32_FM:    // This frightens the compiler. Its fears are unfounded.
      case UINT32_FM:   // This frightens the compiler. Its fears are unfounded.
      case FLOAT_FM:    // This frightens the compiler. Its fears are unfounded.

      
      case UINT32_PTR_FM:  // These are *pointers* to the indicated types. They
      case UINT16_PTR_FM:  //   therefore take the whole 4 bytes of memory allocated
      case UINT8_PTR_FM:   //   and can be returned as such.
      case INT32_PTR_FM:
      case INT16_PTR_FM:
      case INT8_PTR_FM:

      case VECT_3_FLOAT:
      case VECT_3_UINT16:
      case VECT_3_INT16:

      case STR_BUILDER_FM:       // This is a pointer to some StringBuilder. Presumably this is on the heap.
      case STR_FM:               // This is a pointer to a string constant. Presumably this is stored in flash.
      case EVENT_PTR_FM:         // This is a pointer to ManuvrEvent.
      case SYS_SVC_STATICHUB_FM: // This is a pointer to a system service.
      case SYS_SVC_SCHEDULER_FM: // This is a pointer to a system service.
        return_value = DIG_MSG_ERROR_NO_ERROR;
        *((uint32_t*) trg_buf) = (uint32_t) arg->target_mem;
        break;
      default:
        return_value = DIG_MSG_ERROR_INVALID_TYPE;
        break;
    }

    // Only reap the Argument if the return succeeded.
    if ((DIG_MSG_ERROR_NO_ERROR == return_value) && (!preserve)) {
      args.remove(arg);
      delete arg;
    }
  }
  
  return return_value;
}


//int8_t ManuvrMsg::writePointerArgAs(uint8_t *dat); 
//int8_t ManuvrMsg::writePointerArgAs(uint16_t *dat);
int8_t ManuvrMsg::writePointerArgAs(uint32_t dat) {   return writePointerArgAs(0, &dat);   }
//int8_t ManuvrMsg::writePointerArgAs(int8_t *dat);  
//int8_t ManuvrMsg::writePointerArgAs(int16_t *dat); 
//int8_t ManuvrMsg::writePointerArgAs(int32_t *dat); 
//int8_t ManuvrMsg::writePointerArgAs(float *dat);


/**
* All of the type-specialized writePointerArgAs() fxns boil down to this.
* Calls to this fxn are only valid for pointer types.
*
* @param  idx      The Argument position
* @param  trg_buf  A pointer to the place where we should read the Argument from.
* @return DIG_MSG_ERROR_NO_ERROR or appropriate failure code.
*/
int8_t ManuvrMsg::writePointerArgAs(uint8_t idx, void *trg_buf) {
  int8_t return_value = DIG_MSG_ERROR_INVALID_ARG;
  Argument* arg = NULL;
  if (args.size() > idx) {
    arg = args.get(idx);
    switch (arg->type_code) {
      case INT8_PTR_FM:
      case INT16_PTR_FM:
      case INT32_PTR_FM:
      case UINT8_PTR_FM:
      case UINT16_PTR_FM:
      case UINT32_PTR_FM:
      case FLOAT_PTR_FM:
      case STR_BUILDER_FM:
      case STR_FM:
      case BINARY_FM:
        return_value = DIG_MSG_ERROR_NO_ERROR;
        *((uint32_t*) arg->target_mem) = *((uint32_t*) trg_buf);
        break;
      default:
        return_value = DIG_MSG_ERROR_INVALID_TYPE;
        break;
    }
  }
  
  return return_value;
}



/**
* Clears the Arguments attached to this Message and reaps their contents, if
*   the need is indicated. Many subtle memory-related bugs can be caught here.
*
* @return DIG_MSG_ERROR_NO_ERROR or appropriate failure code.
*/
int8_t ManuvrMsg::clearArgs() {
  Argument *arg = NULL;
  while (args.hasNext()) {
    arg = args.get();
    args.remove();
    delete arg;
  }
  return DIG_MSG_ERROR_NO_ERROR;
}



/**
* Given idx, find the Argument and return its type.
*
* @param  idx      The Argument position
* @return NOTYPE_FM if the Argument isn't found, and its type code if it is.
*/
uint8_t ManuvrMsg::getArgumentType(uint8_t idx) {
  if (args.size() > idx) {
    return args.get(idx)->type_code;
  }
  return NOTYPE_FM;
}


/**
* Same as above, but assumes Argument 0.
*
* @return NOTYPE_FM if the Argument isn't found, and its type code if it is.
*/
uint8_t ManuvrMsg::getArgumentType() {
  return getArgumentType(0);
}


/**
* At some point, some downstream code will care about the definition of the
*   messages represented by this message. This will fetch the definition if
*   needed and return it.
*
* @return a pointer to the MessageTypeDef that identifies this class of Message. Never NULL.
*/
MessageTypeDef* ManuvrMsg::getMsgDef() {
  if (NULL == message_def) {
    message_def = lookupMsgDefByCode(event_code);
  }
  return (MessageTypeDef*) message_def;
}


/**
* Debug support fxn.
* TODO: Debug stuff. Need to be able to case-off stuff like this in the pre-processor.
*
* @return a pointer to the human-readable label for this Message class. Never NULL. 
*/
const char* ManuvrMsg::getMsgTypeString() {
  MessageTypeDef* mes_type = getMsgDef();
  return mes_type->debug_label;
}


/**
* Debug support fxn. This is the static version of this method.
* TODO: Debug stuff. Need to be able to case-off stuff like this in the pre-processor.
*
* @param  code  The message identity code in question.
* @return a pointer to the human-readable label for this Message class. Never NULL. 
*/
const char* ManuvrMsg::getMsgTypeString(uint16_t code) {
  int total_elements = sizeof(ManuvrMsg::message_defs) / sizeof(MessageTypeDef);
  for (int i = 0; i < total_elements; i++) {
    if (ManuvrMsg::message_defs[i].msg_type_code == code) {
      return ManuvrMsg::message_defs[i].debug_label;
    }
  }

  // Didn't find it there. Search in the extended defs...
  const MessageTypeDef* temp_type_def;
  total_elements = ManuvrMsg::message_defs_extended.size();
  for (int i = 0; i < total_elements; i++) {
    temp_type_def = ManuvrMsg::message_defs_extended.get(i);
    if (temp_type_def->msg_type_code == code) {
      return temp_type_def->debug_label;
    }
  }
  // If we've come this far, we don't know what the caller is asking for. Return the default.
  printf("code = 0x%04x. This should not be possible.\n", code);
  return "UNDEFINED";
}


/**
* This will never return NULL. It will at least return the defintion for the UNDEFINED class.
* This is the static version of this method.
*
* @param  code  The message identity code in question.
* @return a pointer to the MessageTypeDef that identifies this class of Message. Never NULL.
*/
const MessageTypeDef* ManuvrMsg::lookupMsgDefByCode(uint16_t code) {
  int total_elements = sizeof(ManuvrMsg::message_defs) / sizeof(MessageTypeDef);
  for (int i = 0; i < total_elements; i++) {
    if (ManuvrMsg::message_defs[i].msg_type_code == code) {
      return &ManuvrMsg::message_defs[i];
    }
  }

  // Didn't find it there. Search in the extended defs...
  const MessageTypeDef* temp_type_def;
  total_elements = ManuvrMsg::message_defs_extended.size();
  for (int i = 0; i < total_elements; i++) {
    temp_type_def = ManuvrMsg::message_defs_extended.get(i);
    if (temp_type_def->msg_type_code == code) {
      return temp_type_def;
    }
  }
  // If we've come this far, we don't know what the caller is asking for. Return the default.
  printf("event->event_code = 0x%04x. This should not be possible.\n", code);
  return &ManuvrMsg::message_defs[0];
}


/**
* This will never return NULL. It will at least return the defintion for the UNDEFINED class.
* TODO: Debug stuff. Need to be able to case-off stuff like this in the pre-processor.
*
* @param  label  The message label by which to lookup the message identity.
* @return a pointer to the MessageTypeDef that identifies this class of Message. Never NULL.
*/
const MessageTypeDef* ManuvrMsg::lookupMsgDefByLabel(char* label) {
  int total_elements = sizeof(ManuvrMsg::message_defs) / sizeof(MessageTypeDef);
  for (int i = 1; i < total_elements; i++) {
    // TODO: Yuck... have to import string.h for JUST THIS. Re-implement inline....
    if (strcasestr(label, ManuvrMsg::message_defs[i].debug_label)) {
      return &ManuvrMsg::message_defs[i];
    }
  }

  // Didn't find it there. Search in the extended defs...
  const MessageTypeDef* temp_type_def;
  total_elements = ManuvrMsg::message_defs_extended.size();
  for (int i = 1; i < total_elements; i++) {
    temp_type_def = ManuvrMsg::message_defs_extended.get(i);
    if (strcasestr(label, temp_type_def->debug_label)) {
      return temp_type_def;
    }
  }
  // If we've come this far, we don't know what the caller is asking for. Return the default.
  return &ManuvrMsg::message_defs[0];
}


const char* ManuvrMsg::getArgTypeString(uint8_t idx) {
  if (idx < args.size()) {
    return getTypeCodeString(args.get(idx)->type_code);
  }
  return "<INVALID INDEX>";
}




/**
* Debug support method. This fxn is only present in debug builds. 
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void ManuvrMsg::printDebug(StringBuilder *temp) {
  if (NULL == temp) return; 
  const MessageTypeDef* type_obj = getMsgDef();
  
  if (&ManuvrMsg::message_defs[0] == type_obj) {
    temp->concatf("\t Message type:   <UNDEFINED (Code 0x%04x)>\n", event_code);
  }
  else {
    temp->concatf("\t Message type:   %s\n", getMsgTypeString());
  }
  int ac = args.size();
  if (ac > 0) {
    Argument *working_arg = NULL;
    temp->concatf("\t Arguments:      %d\n", ac);
    for (int i = 0; i < ac; i++) {
      working_arg = args.get(i);
      temp->concatf("\t\t %d\t%d\t(%s)\t%s\n", i, working_arg->len, (working_arg->reap ? "reap" : "no reap"), getArgTypeString(i));
    }
  }
}


/**
* This fxn takes all of the Arguments attached to this Message and writes them, along
*   with their types, to the provided StringBuilder as an unsigned charater stream.
*
* @param  output  The buffer into which we should write our output.
* @return the number of Arguments so written, or a negative value on failure.
*/
int ManuvrMsg::serialize(StringBuilder *output) {
  if (output == NULL) return -1;
  int delta_len = 0;
  int arg_count = 0;
  for (int i = 0; i < args.size(); i++) {
    delta_len = args.get(i)->serialize_raw(output);
    if (delta_len < 0) {
      return delta_len;
    }
    else {
      arg_count++;
    }
  }
  return arg_count;
}


/*
* This is usually called because some other system is needing to know our message map.
* Format is binary with a variable length.
* Broken out into a format that a greedy parser can cope with:
*   |-------------------------------------------------|  /---------------\ |-----|
*   | Message Code | Flags | Label (null-term string) |  | Several NTSs  | |  0  |
*   |-------------------------------------------------|  \---------------/ |-----|
*           2          2               x                         y            1     <---- Bytes
*
* Since the counterparty can't know how big any given definition might be, we need to
*   take care to bake our null-terminators into the output so that the parser on the
*   other side can take the condition "a zero-length string" to signify the end of a message
*   definition, and can move on to the next entry.
* Returns 0 on failure. 1 on success.
*/
int8_t ManuvrMsg::getMsgLegend(StringBuilder *output) {
  if (NULL == output) return 0;
  
  int total_elements = sizeof(message_defs) / sizeof(MessageTypeDef);
  MessageTypeDef *temp_def = NULL;
  for (int i = 1; i < total_elements; i++) {
    temp_def = (MessageTypeDef *) &(message_defs[i]);
    output->concat((unsigned char*) temp_def, 4);
    // Capture the null-terminator in the concats. Otherwise, the counterparty can't see where strings end.
    output->concat((unsigned char*) temp_def->debug_label, strlen(temp_def->debug_label)+1);

    // Now to capture the argument modes...
    unsigned char* mode = (unsigned char*) temp_def->arg_modes;
    int arg_mode_len = strlen((const char*) mode);
    while (arg_mode_len > 0) {
      output->concat((unsigned char*) mode, arg_mode_len+1);
      mode += arg_mode_len + 1;
      arg_mode_len = strlen((const char*) mode);
    }
    output->concat((unsigned char*) "\0", 1);   // This is the obligatory "NO ARGUMENT" mode.
  }
  return 1;
}


/*
* Returns an index to the only possible argument sequence for a given length.
* Returns NULL if...
*   a) there is no valid argument sequence with the given length
*   b) there is more than one valid sequence
*   c) there is no record of the message type represented by this object.
*/
char* ManuvrMsg::is_valid_argument_buffer(int len) {
  if (NULL == message_def) getMsgDef();
  if (NULL == message_def) return NULL;
  
  char* return_value = NULL;
  char* mode = (char*) message_def->arg_modes;
  int arg_mode_len = strlen((const char*) mode);
  int temp = 0;
  while (arg_mode_len > 0) {
    temp = getTotalSizeByTypeString(mode);
    if (len == temp) {
      if (NULL != return_value) {
        return NULL;
      }
      else {
        return_value = mode;
      }
    }
    else if (len > temp) {
      if (NULL != return_value) {
        // TODO: incorrect. No variable-length types...
        return NULL;
      }
      else {
        return_value = mode;
      }
    }
    mode += arg_mode_len + 1;
    arg_mode_len = strlen((const char*) mode);
  }
  
  return return_value;
}

