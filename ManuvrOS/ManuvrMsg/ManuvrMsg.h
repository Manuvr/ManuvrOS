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


This class forms the foundation of internal events. It contains the identity of a given message and
  its arguments. The idea here is to encapsulate notions of "method" and "argument", regardless of
  the nature of its execution parameters.
*/

#ifndef __MANUVR_MESSAGE_H__
#define __MANUVR_MESSAGE_H__

#include <map>

#include "MessageDefs.h"    // This include file contains all of the message codes.
#include <DataStructures/Argument.h>
#include <DataStructures/LightLinkedList.h>

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


/*
* This is the class that represents a message with an optional ordered set of Arguments.
*/
class ManuvrMsg {
  public:
    ManuvrMsg();
    ManuvrMsg(uint16_t code);
    virtual ~ManuvrMsg();

    virtual int8_t repurpose(uint16_t code);

    /**
    * Allows the caller to plan other allocations based on how many bytes this message's
    *   Arguments occupy.
    *
    * @return the length (in bytes) of the arguments for this message.
    */
    int argByteCount() {
      return ((nullptr == arg) ? 0 : arg->sumAllLengths());
    }

    /**
    * Allows the caller to count the Args attached to this message.
    *
    * @return the cardinality of the argument list.
    */
    inline int      argCount() {   return ((nullptr != arg) ? arg->argCount() : 0);   };

    inline uint16_t eventCode() {  return event_code;   };

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
    inline uint8_t getArgumentType() {   return getArgumentType(0);  };


    MessageTypeDef* getMsgDef();


    /*
    * Overrides for Argument constructor access. Prevents outside classes from having to care about
    *   implementation details of Arguments. Might look ugly, but takes the CPU burden off of runtime
    *   and forces the compiler to deal with it.
    */
    inline Argument* addArg(uint8_t val) {             return addArg(new Argument(val));   }
    inline Argument* addArg(uint16_t val) {            return addArg(new Argument(val));   }
    inline Argument* addArg(uint32_t val) {            return addArg(new Argument(val));   }
    inline Argument* addArg(int8_t val) {              return addArg(new Argument(val));   }
    inline Argument* addArg(int16_t val) {             return addArg(new Argument(val));   }
    inline Argument* addArg(int32_t val) {             return addArg(new Argument(val));   }
    inline Argument* addArg(float val) {               return addArg(new Argument(val));   }

    inline Argument* addArg(uint8_t *val) {            return addArg(new Argument(val));   }
    inline Argument* addArg(uint16_t *val) {           return addArg(new Argument(val));   }
    inline Argument* addArg(uint32_t *val) {           return addArg(new Argument(val));   }
    inline Argument* addArg(int8_t *val) {             return addArg(new Argument(val));   }
    inline Argument* addArg(int16_t *val) {            return addArg(new Argument(val));   }
    inline Argument* addArg(int32_t *val) {            return addArg(new Argument(val));   }
    inline Argument* addArg(float *val) {              return addArg(new Argument(val));   }

    inline Argument* addArg(Vector3ui16 *val) {        return addArg(new Argument(val));   }
    inline Argument* addArg(Vector3i16 *val) {         return addArg(new Argument(val));   }
    inline Argument* addArg(Vector3f *val) {           return addArg(new Argument(val));   }
    inline Argument* addArg(Vector4f *val) {           return addArg(new Argument(val));   }

    inline Argument* addArg(void *val, int len) {      return addArg(new Argument(val, len));   }
    inline Argument* addArg(const char *val) {         return addArg(new Argument(val));   }
    inline Argument* addArg(StringBuilder *val) {      return addArg(new Argument(val));   }
    inline Argument* addArg(BufferPipe *val) {         return addArg(new Argument(val));   }
    inline Argument* addArg(EventReceiver *val) {      return addArg(new Argument(val));   }
    inline Argument* addArg(ManuvrXport *val) {        return addArg(new Argument(val));   }
    inline Argument* addArg(ManuvrRunnable *val) {     return addArg(new Argument(val));   }

    Argument* addArg(Argument*);  // The only "real" implementation.

    /*
    * Overrides for Argument retreival. Pass in pointer to the type the argument should be retreived as.
    * Returns an error code if the types don't match.
    */
    // These accessors treat the (void*) as 4 bytes of data storage.
    inline int8_t getArgAs(int8_t *trg_buf) {     return getArgAs(0, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t *trg_buf) {    return getArgAs(0, (void*) trg_buf);  }
    inline int8_t getArgAs(int16_t *trg_buf) {    return getArgAs(0, (void*) trg_buf);  }
    inline int8_t getArgAs(uint16_t *trg_buf) {   return getArgAs(0, (void*) trg_buf);  }
    inline int8_t getArgAs(int32_t *trg_buf) {    return getArgAs(0, (void*) trg_buf);  }
    inline int8_t getArgAs(uint32_t *trg_buf) {   return getArgAs(0, (void*) trg_buf);  }
    inline int8_t getArgAs(float *trg_buf) {      return getArgAs(0, (void*) trg_buf);  }

    inline int8_t getArgAs(uint8_t idx, uint8_t  *trg_buf) {          return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, uint16_t *trg_buf) {          return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, uint32_t *trg_buf) {          return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, int8_t   *trg_buf) {          return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, int16_t  *trg_buf) {          return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, int32_t  *trg_buf) {          return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, float  *trg_buf) {            return getArgAs(idx, (void*) trg_buf);  }

    // These accessors treat the (void*) as a pointer. These functions are essentially type-casts.
    inline int8_t getArgAs(Vector3f **trg_buf) {                      return getArgAs(0, (void*) trg_buf);  }
    inline int8_t getArgAs(Vector3ui16 **trg_buf) {                   return getArgAs(0, (void*) trg_buf);  }
    inline int8_t getArgAs(Vector3i16 **trg_buf) {                    return getArgAs(0, (void*) trg_buf);  }
    inline int8_t getArgAs(const char **trg_buf) {                    return getArgAs(0, (void*) trg_buf);  }
    inline int8_t getArgAs(StringBuilder **trg_buf) {                 return getArgAs(0, (void*) trg_buf);  }
    inline int8_t getArgAs(BufferPipe **trg_buf) {                    return getArgAs(0, (void*) trg_buf);  }
    inline int8_t getArgAs(EventReceiver **trg_buf) {                 return getArgAs(0, (void*) trg_buf);  }
    inline int8_t getArgAs(ManuvrXport **trg_buf) {                   return getArgAs(0, (void*) trg_buf);  }
    inline int8_t getArgAs(ManuvrRunnable **trg_buf) {                return getArgAs(0, (void*) trg_buf);  }

    inline int8_t getArgAs(uint8_t idx, Vector3f  **trg_buf) {        return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, Vector3ui16  **trg_buf) {     return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, Vector3i16  **trg_buf) {      return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, const char  **trg_buf) {      return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, StringBuilder  **trg_buf) {   return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, BufferPipe  **trg_buf) {      return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, EventReceiver  **trg_buf) {   return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, ManuvrXport  **trg_buf) {     return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, ManuvrRunnable  **trg_buf) {  return getArgAs(idx, (void*) trg_buf);  }


    int8_t markArgForReap(int idx, bool reap);

    void printDebug(StringBuilder*);

    const char* getMsgTypeString();
    const char* getArgTypeString(uint8_t idx);


    static const MessageTypeDef* lookupMsgDefByCode(uint16_t msg_code);
    static const MessageTypeDef* lookupMsgDefByLabel(char* label);
    static const char* getMsgTypeString(uint16_t msg_code);

    static int8_t getMsgLegend(StringBuilder *output);

    #if defined (__ENABLE_MSG_SEMANTICS)
    static int8_t getMsgSemantics(MessageTypeDef*, StringBuilder *output);
    #endif

    // TODO: All of this sucks. there-has-to-be-a-better-way.jpg
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
    int8_t getArgAs(uint8_t idx, void *dat);



  private:
    const MessageTypeDef*  message_def;  // The definition for the message (once it is associated).
    Argument* arg       = nullptr;       // The optional list of arguments associated with this event.
    uint16_t event_code = MANUVR_MSG_UNDEFINED; // The identity of the event (or command).

    int8_t writePointerArgAs(uint8_t idx, void *trg_buf);

    char* is_valid_argument_buffer(int len);
    int   collect_valid_grammatical_forms(int, LinkedList<char*>*);


    // Where runtime-loaded message defs go.
    static std::map<uint16_t, const MessageTypeDef*> message_defs_extended;
};


#endif
