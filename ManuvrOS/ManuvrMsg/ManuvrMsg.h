/*
File:   ManuvrMsg.h
Author: J. Ian Lindsay
Date:   2014.03.10

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


/*
*
*
*
* This class forms the foundation of internal events. It contains the identity of a given message and
*   its arguments. The idea here is to encapsulate notions of "method" and "argument", regardless of
*   the nature of its execution parameters.
*
*
*
*
*
*/

#ifndef __MANUVR_MESSAGE_H__
#define __MANUVR_MESSAGE_H__

#include "../EnumeratedTypeCodes.h"
#include "MessageDefs.h"    // This include file contains all of the message codes.

#include <DataStructures/StringBuilder.h>
#include <DataStructures/BufferPipe.h>
#include <DataStructures/PriorityQueue.h>
#include <DataStructures/LightLinkedList.h>
#include <DataStructures/Vector3.h>
#include <DataStructures/Quaternion.h>


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
    const char*           arg_semantics;  // For messages that have arguments, this defines their semantics.
} MessageTypeDef;



/*
* These are flag definitions for Message types.
*/
#define MSG_FLAG_IDEMPOTENT   0x0001      // Indicates that only one of the given message should be enqueue.
#define MSG_FLAG_EXPORTABLE   0x0002      // Indicates that the message might be sent between systems.
#define MSG_FLAG_DEMAND_ACK   0x0004      // Demands that a message be acknowledged if sent outbound.
#define MSG_FLAG_AUTH_ONLY    0x0008      // This flag indicates that only an authenticated session can use this message.
#define MSG_FLAG_EMITS        0x0010      // Indicates that this device might emit this message.
#define MSG_FLAG_LISTENS      0x0020      // Indicates that this device can accept this message.

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


class ManuvrXport;
class EventReceiver;
class ManuvrRunnable;

/* This is how we define arguments to messages. */
class Argument {
  public:
    /*
    * Hackery surrounding this member:
    * There is no point to storing a pointer to a heap ref to hold data that is not
    *   bigger than the pointer itself. So rather than malloc()/free() and populate
    *   this slot with things like int32, we will instead cast the value itself to a
    *   void* and store it in the pointer slot. When we do this, we need to be sure
    *   not to mark the pointer for reap.
    *
    * Glorious, glorious hackery. Keeping it. But do need to account for (and extend to)
    *   64-bit pointers.
    *        ---J. Ian Lindsay   Mon Oct 05 22:55:41 MST 2015
    */
    void*   target_mem;

    uint16_t len;         // This is sometimes not used. It is for data types that are not fixed-length.
    uint8_t  type_code;
    bool     reap;

    Argument();
    Argument(uint8_t  val) : Argument(pack_into_ptr_space(val), sizeof(val), UINT8_FM)  {};
    Argument(uint16_t val) : Argument(pack_into_ptr_space(val), sizeof(val), UINT16_FM) {};
    Argument(uint32_t val) : Argument(pack_into_ptr_space(val), sizeof(val), UINT32_FM) {};
    Argument(int8_t   val) : Argument(pack_into_ptr_space(val), sizeof(val), INT8_FM)   {};
    Argument(int16_t  val) : Argument(pack_into_ptr_space(val), sizeof(val), INT16_FM)  {};
    Argument(int32_t  val) : Argument(pack_into_ptr_space(val), sizeof(val), INT32_FM)  {};
    Argument(float    val) : Argument(pack_into_ptr_space(val), sizeof(val), FLOAT_FM)  {};

    Argument(uint8_t*  val) : Argument((void*) val, sizeof(val), UINT8_PTR_FM)  {};
    Argument(uint16_t* val) : Argument((void*) val, sizeof(val), UINT16_PTR_FM) {};
    Argument(uint32_t* val) : Argument((void*) val, sizeof(val), UINT32_PTR_FM) {};
    Argument(int8_t*   val) : Argument((void*) val, sizeof(val), INT8_PTR_FM)   {};
    Argument(int16_t*  val) : Argument((void*) val, sizeof(val), INT16_PTR_FM)  {};
    Argument(int32_t*  val) : Argument((void*) val, sizeof(val), INT32_PTR_FM)  {};
    Argument(float*    val) : Argument((void*) val, sizeof(val), FLOAT_PTR_FM)  {};

    Argument(Vector3ui16* val) : Argument((void*) val, 6,  VECT_3_UINT16) {};
    Argument(Vector3i16*  val) : Argument((void*) val, 6,  VECT_3_INT16)  {};
    Argument(Vector3f*    val) : Argument((void*) val, 12, VECT_3_FLOAT)  {};
    Argument(Vector4f*    val) : Argument((void*) val, 16, VECT_4_FLOAT)  {};

    /*
    * This is a constant character pointer. Do not reap it.
    */
    Argument(const char* val) : Argument((void*) val, (strlen(val)+1), STR_FM) {};

    /*
    * We typically want references to typeless swaths of memory be left alone at the end of
    *   the Argument's life cycle. We will specify otherwise when appropriate.
    */
    Argument(void* val, uint16_t len) : Argument(val, len, BINARY_FM) {};

    /*
    * These are system service pointers. Do not reap.
    */
    Argument(EventReceiver* val) : Argument((void*) val, sizeof(val), SYS_EVENTRECEIVER_FM) {};
    Argument(ManuvrXport* val)   : Argument((void*) val, sizeof(val), SYS_MANUVR_XPORT_FM)  {};
    Argument(BufferPipe* val)    : Argument((void*) val, sizeof(val), BUFFERPIPE_PTR_FM)    {};

    /*
    * We typically want ManuvrRunnable references to be left alone at the end of
    *   the Argument's life cycle. We will specify otherwise when appropriate.
    */
    Argument(ManuvrRunnable* val) : Argument((void*) val, sizeof(val), SYS_RUNNABLE_PTR_FM) {};

    // These are reapable.
    Argument(char* val);
    Argument(StringBuilder* val);


    ~Argument();

    int8_t serialize(StringBuilder*);
    int8_t serialize_raw(StringBuilder*);


    static char*    printBinStringToBuffer(unsigned char *str, int len, char *buffer);


  protected:
    Argument(void* ptr, int len, uint8_t code);  // Protected constructor to which we delegate.

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


    inline bool demandsACK() {
      if (NULL == message_def) message_def = lookupMsgDefByCode(event_code);
      return (message_def->msg_type_flags & MSG_FLAG_DEMAND_ACK);
    }


    inline bool isIdempotent() {
      if (NULL == message_def) message_def = lookupMsgDefByCode(event_code);
      return (message_def->msg_type_flags & MSG_FLAG_IDEMPOTENT);
    }


    int serialize(StringBuilder*);  // Returns the number of bytes resulting.
    uint8_t inflateArgumentsFromBuffer(unsigned char *str, int len);
    int8_t clearArgs();     // Clear all of the arguments attached to this message, reaping them if necessary.

    uint8_t getArgumentType(uint8_t);  // Given a position, return the type code for the Argument.
    uint8_t getArgumentType();         // Return the type code for Argument 0.

    MessageTypeDef* getMsgDef();


    /*
    * Overrides for Argument constructor access. Prevents outside classes from having to care about
    *   implementation details of Arguments. Might look ugly, but takes the CPU burden off of runtime
    *   and forces the compiler to deal with it.
    */
    inline int addArg(uint8_t val) {             return args.insert(new Argument(val));   }
    inline int addArg(uint16_t val) {            return args.insert(new Argument(val));   }
    inline int addArg(uint32_t val) {            return args.insert(new Argument(val));   }
    inline int addArg(int8_t val) {              return args.insert(new Argument(val));   }
    inline int addArg(int16_t val) {             return args.insert(new Argument(val));   }
    inline int addArg(int32_t val) {             return args.insert(new Argument(val));   }
    inline int addArg(float val) {               return args.insert(new Argument(val));   }

    inline int addArg(uint8_t *val) {            return args.insert(new Argument(val));   }
    inline int addArg(uint16_t *val) {           return args.insert(new Argument(val));   }
    inline int addArg(uint32_t *val) {           return args.insert(new Argument(val));   }
    inline int addArg(int8_t *val) {             return args.insert(new Argument(val));   }
    inline int addArg(int16_t *val) {            return args.insert(new Argument(val));   }
    inline int addArg(int32_t *val) {            return args.insert(new Argument(val));   }
    inline int addArg(float *val) {              return args.insert(new Argument(val));   }

    inline int addArg(Vector3ui16 *val) {        return args.insert(new Argument(val));   }
    inline int addArg(Vector3i16 *val) {         return args.insert(new Argument(val));   }
    inline int addArg(Vector3f *val) {           return args.insert(new Argument(val));   }
    inline int addArg(Vector4f *val) {           return args.insert(new Argument(val));   }

    inline int addArg(void *val, int len) {      return args.insert(new Argument(val, len));   }
    inline int addArg(const char *val) {         return args.insert(new Argument(val));   }
    inline int addArg(StringBuilder *val) {      return args.insert(new Argument(val));   }
    inline int addArg(BufferPipe *val) {         return args.insert(new Argument(val));   }
    inline int addArg(EventReceiver *val) {      return args.insert(new Argument(val));   }
    inline int addArg(ManuvrXport *val) {        return args.insert(new Argument(val));   }
    inline int addArg(ManuvrRunnable *val) {     return args.insert(new Argument(val));   }


    /*
    * Overrides for Argument retreival. Pass in pointer to the type the argument should be retreived as.
    * Returns an error code if the types don't match.
    */
    // These accessors treat the (void*) as 4 bytes of data storage.
    inline int8_t consumeArgAs(int8_t *trg_buf) {       return getArgAs(0, (void*) trg_buf, false);  }
    inline int8_t consumeArgAs(uint8_t *trg_buf) {      return getArgAs(0, (void*) trg_buf, false);  }
    inline int8_t consumeArgAs(int16_t *trg_buf) {      return getArgAs(0, (void*) trg_buf, false);  }
    inline int8_t consumeArgAs(uint16_t *trg_buf) {     return getArgAs(0, (void*) trg_buf, false);  }
    inline int8_t consumeArgAs(int32_t *trg_buf) {      return getArgAs(0, (void*) trg_buf, false);  }
    inline int8_t consumeArgAs(uint32_t *trg_buf) {     return getArgAs(0, (void*) trg_buf, false);  }
    inline int8_t consumeArgAs(float *trg_buf) {        return getArgAs(0, (void*) trg_buf, false);  }
    inline int8_t consumeArgAs(ManuvrRunnable **trg_buf) {  return getArgAs(0, (void*) trg_buf, false);  }
    inline int8_t consumeArgAs(StringBuilder **trg_buf) {   return getArgAs(0, (void*) trg_buf, false);  }

    inline int8_t getArgAs(int8_t *trg_buf) {           return getArgAs(0, (void*) trg_buf, true);  }
    inline int8_t getArgAs(uint8_t *trg_buf) {          return getArgAs(0, (void*) trg_buf, true);  }
    inline int8_t getArgAs(int16_t *trg_buf) {          return getArgAs(0, (void*) trg_buf, true);  }
    inline int8_t getArgAs(uint16_t *trg_buf) {         return getArgAs(0, (void*) trg_buf, true);  }
    inline int8_t getArgAs(int32_t *trg_buf) {          return getArgAs(0, (void*) trg_buf, true);  }
    inline int8_t getArgAs(uint32_t *trg_buf) {         return getArgAs(0, (void*) trg_buf, true);  }
    inline int8_t getArgAs(float *trg_buf) {            return getArgAs(0, (void*) trg_buf, true);  }

    inline int8_t getArgAs(uint8_t idx, uint8_t  *trg_buf) {     return getArgAs(idx, (void*) trg_buf, true);  }
    inline int8_t getArgAs(uint8_t idx, uint16_t *trg_buf) {     return getArgAs(idx, (void*) trg_buf, true);  }
    inline int8_t getArgAs(uint8_t idx, uint32_t *trg_buf) {     return getArgAs(idx, (void*) trg_buf, true);  }
    inline int8_t getArgAs(uint8_t idx, int8_t   *trg_buf) {     return getArgAs(idx, (void*) trg_buf, true);  }
    inline int8_t getArgAs(uint8_t idx, int16_t  *trg_buf) {     return getArgAs(idx, (void*) trg_buf, true);  }
    inline int8_t getArgAs(uint8_t idx, int32_t  *trg_buf) {     return getArgAs(idx, (void*) trg_buf, true);  }
    inline int8_t getArgAs(uint8_t idx, float  *trg_buf) {       return getArgAs(idx, (void*) trg_buf, true);  }

    // These accessors treat the (void*) as a pointer. These functions are essentially type-casts.
    inline int8_t getArgAs(Vector3f **trg_buf) {                 return getArgAs(0, (void*) trg_buf, true);  }
    inline int8_t getArgAs(Vector3ui16 **trg_buf) {              return getArgAs(0, (void*) trg_buf, true);  }
    inline int8_t getArgAs(Vector3i16 **trg_buf) {               return getArgAs(0, (void*) trg_buf, true);  }
    inline int8_t getArgAs(const char **trg_buf) {               return getArgAs(0, (void*) trg_buf, true);  }
    inline int8_t getArgAs(StringBuilder **trg_buf) {            return getArgAs(0, (void*) trg_buf, true);  }
    inline int8_t getArgAs(BufferPipe **trg_buf) {               return getArgAs(0, (void*) trg_buf, true);  }
    inline int8_t getArgAs(EventReceiver **trg_buf) {            return getArgAs(0, (void*) trg_buf, true);  }
    inline int8_t getArgAs(ManuvrXport **trg_buf) {              return getArgAs(0, (void*) trg_buf, true);  }
    inline int8_t getArgAs(ManuvrRunnable **trg_buf) {           return getArgAs(0, (void*) trg_buf, true);  }

    inline int8_t getArgAs(uint8_t idx, Vector3f  **trg_buf) {          return getArgAs(idx, (void*) trg_buf, true);  }
    inline int8_t getArgAs(uint8_t idx, Vector3ui16  **trg_buf) {       return getArgAs(idx, (void*) trg_buf, true);  }
    inline int8_t getArgAs(uint8_t idx, Vector3i16  **trg_buf) {        return getArgAs(idx, (void*) trg_buf, true);  }
    inline int8_t getArgAs(uint8_t idx, const char  **trg_buf) {        return getArgAs(idx, (void*) trg_buf, true);  }
    inline int8_t getArgAs(uint8_t idx, StringBuilder  **trg_buf) {     return getArgAs(idx, (void*) trg_buf, true);  }
    inline int8_t getArgAs(uint8_t idx, BufferPipe  **trg_buf) {        return getArgAs(idx, (void*) trg_buf, true);  }
    inline int8_t getArgAs(uint8_t idx, EventReceiver  **trg_buf) {     return getArgAs(idx, (void*) trg_buf, true);  }
    inline int8_t getArgAs(uint8_t idx, ManuvrXport  **trg_buf) {       return getArgAs(idx, (void*) trg_buf, true);  }
    inline int8_t getArgAs(uint8_t idx, ManuvrRunnable  **trg_buf) {    return getArgAs(idx, (void*) trg_buf, true);  }


    /*
    * Protip: Think on the stack...
    * markForReap(addArg(new StringBuilder("Sample data to reap on destruction.")), true);
    */
    int8_t markArgForReap(int idx, bool reap);


    void printDebug(StringBuilder *);

    const char* getMsgTypeString();
    const char* getArgTypeString(uint8_t idx);

    static const MessageTypeDef* lookupMsgDefByCode(uint16_t msg_code);
    static const MessageTypeDef* lookupMsgDefByLabel(char* label);
    static const char* getMsgTypeString(uint16_t msg_code);

    static int8_t getMsgLegend(StringBuilder *output);

    #if defined (__ENABLE_MSG_SEMANTICS)
    static int8_t getMsgSemantics(MessageTypeDef*, StringBuilder *output);
    #endif

    static int8_t registerMessage(MessageTypeDef*);
    static int8_t registerMessage(uint16_t, uint16_t, const char*, const unsigned char*, const char*);

    static int8_t registerMessages(const MessageTypeDef[], int len);

    static const MessageTypeDef message_defs[];


    static bool isExportable(const MessageTypeDef* message_def) {
      return (message_def->msg_type_flags & MSG_FLAG_EXPORTABLE);
    }

    /* Required argument forms */
    static const unsigned char MSG_ARGS_NONE[];
    static const unsigned char MSG_ARGS_U8[];
    static const unsigned char MSG_ARGS_U16[];
    static const unsigned char MSG_ARGS_U32[];
    static const unsigned char MSG_ARGS_STR_BUILDER[];
    static const unsigned char MSG_ARGS_BINBLOB[];

    static const unsigned char MSG_ARGS_SELF_DESC[];
    static const unsigned char MSG_ARGS_MSG_FORWARD[];

    static const unsigned char MSG_ARGS_BUFFERPIPE[];
    static const unsigned char MSG_ARGS_XPORT[];


  protected:
    const MessageTypeDef*  message_def;             // The definition for the message (once it is associated).

    int8_t getArgAs(uint8_t idx, void *dat, bool preserve);


  private:
    void __class_initializer();

    int8_t writePointerArgAs(uint32_t dat);
    int8_t writePointerArgAs(uint8_t idx, void *trg_buf);

    char* is_valid_argument_buffer(int len);
    int   collect_valid_grammatical_forms(int, LinkedList<char*>*);

    static PriorityQueue<const MessageTypeDef*> message_defs_extended;  // Where runtime-loaded message defs go.
};


#endif
