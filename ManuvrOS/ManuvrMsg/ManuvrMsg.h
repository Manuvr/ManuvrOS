/*



Some documentation is called for... 
This class forms the foundation of our major messaging component: ManuvrEvent

This is the data structure that contains the identity of a given message and its arguments. The idea here
  is to allow for a smooth transition between XenoMessages and ManuvrEvents. The counterparty ought to be 
  able to subscribe itself to any class of message and be thereby notified about what is happening in the
  system.

The message codes defined by this file are dual-purposed across the EventAnd Host message classes to keep
  continuity between message types.
*/


#ifndef __MANUVR_MESSAGE_H__
#define __MANUVR_MESSAGE_H__

#include "../EnumeratedTypeCodes.h"
#include "MessageDefs.h"    // This include file contains all of the message codes.

#include "StringBuilder/StringBuilder.h"
#include "DataStructures/PriorityQueue.h"
#include "DataStructures/LightLinkedList.h"
#include "DataStructures/Vector3.h"

#define DIG_MSG_ERROR_NO_ERROR        0
#define DIG_MSG_ERROR_INVALID_ARG     -1
#define DIG_MSG_ERROR_INVALID_TYPE    -2



/*
* Messages are defined by this struct. Note that this amounts to nothing more than definition.
* Actual instances of messages are held by ManuvrMsg.
* For memory-constrained devices, the list of message definitions should be declared as a const
* array of this type. See the cpp file for the static initiallizer.
*/
typedef struct msg_defin_t {
    uint16_t              msg_type_code;  // This field identifies the message class.
    uint16_t              msg_type_flags; // Optional flags to describe nuances of this message type.
    const char*           debug_label;    // This is a pointer to a const that represents this message code as a string.
    const unsigned char*  arg_modes;      // For messages that have arguments, this defines their possible types.
} MessageTypeDef;



/*
* These are flag definitions for Message types.
*/
#define MSG_FLAG_IDEMPOTENT   0x0001      // Indicates that only one of the given message should be enqueue.
#define MSG_FLAG_EXPORTABLE   0x0002      // Indicates that the message might be sent between systems.

#define MSG_FLAG_RESERVED_D   0x0004      // Reserved flag.
#define MSG_FLAG_RESERVED_C   0x0008      // Reserved flag.
#define MSG_FLAG_RESERVED_B   0x0010      // Reserved flag.
#define MSG_FLAG_RESERVED_A   0x0020      // Reserved flag.
#define MSG_FLAG_RESERVED_9   0x0040      // Reserved flag.
#define MSG_FLAG_RESERVED_8   0x0080      // Reserved flag.
#define MSG_FLAG_RESERVED_7   0x0100      // Reserved flag.
#define MSG_FLAG_RESERVED_6   0x0200      // Reserved flag.
#define MSG_FLAG_RESERVED_5   0x0400      // Reserved flag.
#define MSG_FLAG_RESERVED_4   0x0800      // Reserved flag.
#define MSG_FLAG_RESERVED_3   0x1000      // Reserved flag.
#define MSG_FLAG_RESERVED_2   0x2000      // Reserved flag.
#define MSG_FLAG_RESERVED_1   0x4000      // Reserved flag.
#define MSG_FLAG_RESERVED_0   0x8000      // Reserved flag.



class StaticHub;
class Scheduler;
class ManuvrEvent;

/* This is how we define arguments to messages. */
class Argument {
  public:
    uint8_t  type_code;
    uint16_t len;         // This is sometimes not used. It is for data types that are not fixed-length.
    bool    reap;
    
    /*
    * Hackery surrounding this member:
    * There is no point to storing a pointer to a heap ref to hold data that is not
    *   bigger than the pointer itself. So rather than malloc()/free() and populate
    *   this slot with things like int32, we will instead cast the value itself to a
    *   void* and store it in the pointer slot. When we do this, we need to be sure 
    *   not to mark the pointer for reap.
    */
    void*   target_mem;
    
    Argument();
    Argument(uint8_t);
    Argument(uint16_t);
    Argument(uint32_t);
    Argument(int8_t);
    Argument(int16_t);
    Argument(int32_t);
    Argument(float);
    
    Argument(uint8_t*);  // The only possible reason to do this is because the target
    Argument(uint16_t*); // of the pointer is not intended to be reaped.
    Argument(uint32_t*);
    Argument(int8_t*);
    Argument(int16_t*);
    Argument(int32_t*);
    Argument(float*);
    
    Argument(Vector3ui16*);
    Argument(Vector3i16*);
    Argument(Vector3f*);
    
    Argument(const char* val);
    Argument(void*, uint16_t buf_len);
    Argument(StringBuilder *);
    Argument(StaticHub *);
    Argument(Scheduler *);
    Argument(ManuvrEvent *);
    
    ~Argument();
    
    int8_t serialize(StringBuilder*);
    int8_t serialize_raw(StringBuilder*);
	
	
    //static double   parseDoubleFromchars(unsigned char *input);
    static char*    printBinStringToBuffer(unsigned char *str, int len, char *buffer);
    

  private:
    void wipe();
};


/*
* This is the class that represents a message with an optional ordered set of Arguments.
*/
class ManuvrMsg {
  public:
    LinkedList<Argument*> args;     // The optional list of arguments associated with this event.
    uint16_t event_code;            // The identity of the event (or command).

    
    ManuvrMsg(void);
    ManuvrMsg(uint16_t code);
    virtual ~ManuvrMsg(void);
    
    virtual int8_t repurpose(uint16_t code);
    
    int argCount();
    int argByteCount();

   
    inline bool isExportable() {
      if (NULL == message_def) message_def = lookupMsgDefByCode(event_code);
      return (message_def->msg_type_flags & MSG_FLAG_EXPORTABLE);
    }

    
    inline bool isIdempotent() {
      if (NULL == message_def) message_def = lookupMsgDefByCode(event_code);
      return (message_def->msg_type_flags & MSG_FLAG_IDEMPOTENT);
    }


    int serialize(StringBuilder*);  // Returns the number of bytes resulting.
    uint8_t inflateArgumentsFromBuffer(unsigned char *str, int len);
    uint8_t inflateArgumentsFromRawBuffer(unsigned char *str, int len);
    int8_t clearArgs();     // Clear all of the arguments attached to this message, reaping them if necessary.
    
    uint8_t getArgumentType(uint8_t);  // Given a position, return the type code for the Argument.
    uint8_t getArgumentType();         // Return the type code for Argument 0.
    
    MessageTypeDef* getMsgDef();
    
    /* These accessors imply the first argument which is retained after the call. */
    int8_t getArgAs(uint8_t *dat); 
    int8_t getArgAs(uint16_t *dat);
    int8_t getArgAs(uint32_t *dat);
    int8_t getArgAs(int8_t *dat);  
    int8_t getArgAs(int16_t *dat); 
    int8_t getArgAs(int32_t *dat); 
    int8_t getArgAs(float *dat);   
    int8_t getArgAs(Vector3f **dat);
    int8_t getArgAs(Vector3ui16 **dat);
    int8_t getArgAs(Vector3i16 **dat);
    int8_t getArgAs(const char **dat);
    int8_t getArgAs(StringBuilder **dat);
    int8_t getArgAs(StaticHub **dat);
    int8_t getArgAs(Scheduler **dat);
    int8_t getArgAs(ManuvrEvent **dat);
    
    
    /* These accessors imply the first argument and reaps it if the return succeeds. */
    int8_t consumeArgAs(uint8_t *dat);   
    int8_t consumeArgAs(uint16_t *dat);
    int8_t consumeArgAs(uint32_t *dat);
    int8_t consumeArgAs(int8_t *dat);  
    int8_t consumeArgAs(int16_t *dat); 
    int8_t consumeArgAs(int32_t *dat); 
    int8_t consumeArgAs(float *dat);   
    int8_t consumeArgAs(const char **dat);
    int8_t consumeArgAs(StringBuilder **dat);
    int8_t consumeArgAs(StaticHub **dat);
    int8_t consumeArgAs(Scheduler **dat);
    int8_t consumeArgAs(ManuvrEvent **dat);

    /* These accessors specify an argument position, and retain the argument after the call. */
    int8_t getArgAs(uint8_t idx, uint8_t *dat);
    int8_t getArgAs(uint8_t idx, uint16_t *dat);
    int8_t getArgAs(uint8_t idx, uint32_t *dat);
    int8_t getArgAs(uint8_t idx, int8_t *dat);
    int8_t getArgAs(uint8_t idx, int16_t *dat);
    int8_t getArgAs(uint8_t idx, int32_t *dat);
    int8_t getArgAs(uint8_t idx, float *dat);
    
    int8_t getArgAs(uint8_t idx, Vector3f **dat);
    int8_t getArgAs(uint8_t idx, Vector3ui16 **dat);
    int8_t getArgAs(uint8_t idx, Vector3i16 **dat);
    int8_t getArgAs(uint8_t idx, const char **dat);
    int8_t getArgAs(uint8_t idx, StringBuilder **dat);
    int8_t getArgAs(uint8_t idx, StaticHub **dat);
    int8_t getArgAs(uint8_t idx, Scheduler **dat);
    int8_t getArgAs(uint8_t idx, ManuvrEvent **dat);
    
    /* 
    * Protip: Think on the stack...
    * markForReap(addArg(new StringBuilder("Sample data to reap on destruction.")), true);
    */
    int8_t markArgForReap(int idx, bool reap);
    
	
    /* These are pass-throughs to an appropriate Argument constructor. */
    int addArg(uint8_t);
    int addArg(uint16_t);
    int addArg(uint32_t);
    int addArg(int8_t);
    int addArg(int16_t);                           
    int addArg(int32_t);
    int addArg(float);
    
    int addArg(uint8_t*);
    int addArg(uint16_t*);
    int addArg(uint32_t*);
    int addArg(int8_t*);
    int addArg(int16_t*);                           
    int addArg(int32_t*);
    int addArg(float*);
    
    int addArg(Vector3ui16*);
    int addArg(Vector3i16*);
    int addArg(Vector3f*);

    int addArg(void*, int len);
    int addArg(const char*);
    int addArg(StringBuilder*);
    int addArg(StaticHub*);
    int addArg(Scheduler*);
    int addArg(ManuvrEvent*);
    //int addArg(double);
    //int addArg(void* nu, uint8_t type);

    //uint8_t  get_uint8(void);
    //uint16_t get_uint16(void);
    //uint32_t get_uint32(void);
    //int8_t   get_int8(void);
    //int16_t  get_int16(void);
    //int32_t  get_int32(void);
    //double   get_double(void);
    //float    get_float(void);
    
    void printDebug(StringBuilder *);
    
    const char* getMsgTypeString();
    const char* getArgTypeString(uint8_t idx);

    static const MessageTypeDef* lookupMsgDefByCode(uint16_t msg_code);
    static const MessageTypeDef* lookupMsgDefByLabel(char* label);
    static const char* getMsgTypeString(uint16_t msg_code);
    
    static int8_t getMsgLegend(StringBuilder *output);
    
    static const MessageTypeDef message_defs[];
    static PriorityQueue<const MessageTypeDef*> message_defs_extended;  // Where runtime-loaded message defs go.

    
    static bool isExportable(const MessageTypeDef* message_def) {
      return (message_def->msg_type_flags & MSG_FLAG_EXPORTABLE);
    }


    /* Required argument forms */
    static const unsigned char MSG_ARGS_NONE[];
    static const unsigned char MSG_ARGS_U8[];
    static const unsigned char MSG_ARGS_U16[];
    static const unsigned char MSG_ARGS_U32[];
    static const unsigned char MSG_ARGS_STR_BUILDER[];
    static const unsigned char MSG_ARGS_U8_U8[];
    static const unsigned char MSG_ARGS_U8_U32[];
    static const unsigned char MSG_ARGS_U8_U32_I16[];
    static const unsigned char MSG_ARGS_BINBLOB[];
    static const unsigned char MSG_ARGS_POWER_MODE[];
    
    
    
    
    
    

  protected:
    const MessageTypeDef*  message_def;             // The definition for the message (once it is associated).
    
    int8_t getArgAs(uint8_t idx, void *dat, bool preserve);


  private:    
    void __class_initializer();

    int8_t writePointerArgAs(uint32_t dat); 
    int8_t writePointerArgAs(uint8_t idx, void *trg_buf);
    
    char* is_valid_argument_buffer(int len);

};


#endif
