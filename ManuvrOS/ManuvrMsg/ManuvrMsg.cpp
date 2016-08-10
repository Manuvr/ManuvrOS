/*
File:   ManuvrMsg.cpp
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

// Generics for messages that have one arg of a single type, or no args.
const unsigned char ManuvrMsg::MSG_ARGS_U8[]  = {UINT8_FM,  0};
const unsigned char ManuvrMsg::MSG_ARGS_U16[] = {UINT16_FM, 0};
const unsigned char ManuvrMsg::MSG_ARGS_U32[] = {UINT32_FM, 0};

const unsigned char ManuvrMsg::MSG_ARGS_STR_BUILDER[] = {STR_BUILDER_FM, 0};

const unsigned char ManuvrMsg::MSG_ARGS_BUFFERPIPE[]  = {BUFFERPIPE_PTR_FM, 0};
const unsigned char ManuvrMsg::MSG_ARGS_XPORT[]       = {SYS_MANUVR_XPORT_FM, 0};

/*
* This is the argument form for messages that either...
*   a) Need a free-form type that we don't support natively
*   b) Are special-cases (REPLY) that will have their args cast after validation.
*/
const unsigned char ManuvrMsg::MSG_ARGS_BINBLOB[] = {  BINARY_FM, 0};

const unsigned char ManuvrMsg::MSG_ARGS_SELF_DESC[] = {
  UINT32_FM, UINT32_FM, STR_FM, STR_FM, STR_FM, STR_FM, STR_FM, STR_FM, 0, // All fields.
};                                                                         // 0 bytes: Request for self-description.

/*
* This code is used for forwarding messages between Manuvrables.
* Implementation of this code is not required, but the ability to understand it is.
*/
const unsigned char ManuvrMsg::MSG_ARGS_MSG_FORWARD[] = {  STR_FM, BINARY_FM, 0};



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
  {  MANUVR_MSG_UNDEFINED            , 0x0000,               "<UNDEF>"          , MSG_ARGS_NONE }, // This should be the first entry for failure cases.

  /* Protocol basics. */
  #if defined(MANUVR_OVER_THE_WIRE)
  {  MANUVR_MSG_REPLY_FAIL           , MSG_FLAG_EXPORTABLE,               "REPLY_FAIL"           , MSG_ARGS_NONE }, //  This reply denotes that the packet failed to parse (despite passing checksum).
  {  MANUVR_MSG_REPLY_RETRY          , MSG_FLAG_EXPORTABLE,               "REPLY_RETRY"          , MSG_ARGS_NONE }, //  This reply asks for a reply of the given Unique ID.
  {  MANUVR_MSG_REPLY                , MSG_FLAG_EXPORTABLE,               "REPLY"                , MSG_ARGS_NONE }, //  This reply is for success-case.
  {  MANUVR_MSG_SELF_DESCRIBE        , MSG_FLAG_DEMAND_ACK | MSG_FLAG_EXPORTABLE,  "SELF_DESCRIBE"        , MSG_ARGS_SELF_DESC }, // Starting an application on the receiver. Needs a string.
  {  MANUVR_MSG_SYNC_KEEPALIVE       , MSG_FLAG_DEMAND_ACK | MSG_FLAG_EXPORTABLE,  "KA"                  , MSG_ARGS_NONE }, //  A keep-alive message to be ack'd.
  {  MANUVR_MSG_LEGEND_TYPES         , MSG_FLAG_DEMAND_ACK | MSG_FLAG_EXPORTABLE,  "LEGEND_TYPES"         , MSG_ARGS_BINBLOB }, // No args? Asking for this legend. One arg: Legend provided.
  {  MANUVR_MSG_LEGEND_MESSAGES      , MSG_FLAG_DEMAND_ACK | MSG_FLAG_EXPORTABLE,  "LEGEND_MESSAGES"      , MSG_ARGS_BINBLOB }, // No args? Asking for this legend. One arg: Legend provided.
  {  MANUVR_MSG_LEGEND_SEMANTIC      , MSG_FLAG_DEMAND_ACK | MSG_FLAG_EXPORTABLE,  "LEGEND_SEMANTIC"      , MSG_ARGS_BINBLOB }, // No args? Asking for this legend. One arg: Legend provided.
  #endif

  {  MANUVR_MSG_SESS_ESTABLISHED     , MSG_FLAG_DEMAND_ACK | MSG_FLAG_EXPORTABLE,  "SESS_ESTABLISHED"     , MSG_ARGS_NONE }, // Session established.
  {  MANUVR_MSG_SESS_HANGUP          , MSG_FLAG_EXPORTABLE,                        "SESS_HANGUP"          , MSG_ARGS_NONE }, // Session hangup.
  {  MANUVR_MSG_SESS_AUTH_CHALLENGE  , MSG_FLAG_DEMAND_ACK | MSG_FLAG_EXPORTABLE,  "SESS_AUTH_CHALLENGE"  , MSG_ARGS_NONE }, // A code for challenge-response authentication.

  {  MANUVR_MSG_MSG_FORWARD          , MSG_FLAG_DEMAND_ACK | MSG_FLAG_EXPORTABLE,  "MSG_FORWARD"          , MSG_ARGS_MSG_FORWARD }, // No args? Asking for this legend. One arg: Legend provided.

  /*
    For messages that have arguments, we have the option of defining inline lables for each parameter.
    This is advantageous for debugging and writing front-ends. We case-off here to make this choice at
    compile time.
  */
  #if defined (__ENABLE_MSG_SEMANTICS)
  #else
  #endif
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
* This function is for the exclusive purpose of inflating an argument from a place where a
*   pointer doesn't make sense. This means that an argument mode that contains a non-exportable
*   pointer type will not produce the expected result. It might even asplode.
*
*
* @param   a buffer containing the byte-stream containing the data.
* @param   int length of the buffer
* @return  the number of Arguments extracted and instantiated from the buffer.
*/
uint8_t ManuvrMsg::inflateArgumentsFromBuffer(unsigned char *buffer, int len) {
  int return_value = 0;
  if ((NULL == buffer) || (0 == len)) {
    return 0;
  }

  char* arg_mode = NULL;
  LinkedList<char*> possible_forms;
  int r = 0;
  switch (collect_valid_grammatical_forms(len, &possible_forms)) {
    case 0:
      // No valid forms. This is a problem.
      //Kernel::log("No valid argument forms...\n");
      return 0;
      break;
    case 1:
      // Only one possibility.
      arg_mode = possible_forms.get(r++);
      break;
    default:
      // There are a few possible forms. We need to isolate which is correct.
      while (possible_forms.size() > 0) {
        arg_mode = possible_forms.get(r++);
      }
      break;
  }




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
      case VECT_4_FLOAT:
        nu_arg = new Argument(new Vector4f(parseFloatFromchars(buffer + 0), parseFloatFromchars(buffer + 4), parseFloatFromchars(buffer + 8), parseFloatFromchars(buffer + 12)));
        len = len - 16;
        buffer += 16;
        nu_arg->reap = true;
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

      // Variable-length types...
      case STR_FM:
        nu_arg = new Argument(strdup((const char*) buffer));
        buffer = buffer + nu_arg->len;
        len    = len - nu_arg->len;
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
        *((uint8_t*) trg_buf) = *((uint8_t*)&arg->target_mem);
        break;
      case INT16_FM:    // This frightens the compiler. Its fears are unfounded.
      case UINT16_FM:   // This frightens the compiler. Its fears are unfounded.
        return_value = DIG_MSG_ERROR_NO_ERROR;
        *((uint16_t*) trg_buf) = *((uint16_t*)&arg->target_mem);
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

      case VECT_4_FLOAT:
      case VECT_3_FLOAT:
      case VECT_3_UINT16:
      case VECT_3_INT16:

      case STR_BUILDER_FM:          // This is a pointer to some StringBuilder. Presumably this is on the heap.
      case STR_FM:                  // This is a pointer to a string constant. Presumably this is stored in flash.
      case BUFFERPIPE_PTR_FM:       // This is a pointer to a BufferPipe/.
      case SYS_RUNNABLE_PTR_FM:     // This is a pointer to ManuvrRunnable.
      case SYS_EVENTRECEIVER_FM:    // This is a pointer to an EventReceiver.
      case SYS_MANUVR_XPORT_FM:     // This is a pointer to a transport.
        return_value = DIG_MSG_ERROR_NO_ERROR;
        *((uint32_t*) trg_buf) = *((uint32_t*)&arg->target_mem);
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
  total_elements = message_defs_extended.size();
  for (int i = 0; i < total_elements; i++) {
    temp_type_def = message_defs_extended.get(i);
    if (temp_type_def->msg_type_code == code) {
      return temp_type_def->debug_label;
    }
  }
  // If we've come this far, we don't know what the caller is asking for. Return the default.
  return ManuvrMsg::message_defs[0].debug_label;
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
  total_elements = message_defs_extended.size();
  for (int i = 0; i < total_elements; i++) {
    temp_type_def = message_defs_extended.get(i);
    if (temp_type_def->msg_type_code == code) {
      return temp_type_def;
    }
  }
  // If we've come this far, we don't know what the caller is asking for. Return the default.
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
    if (strstr(label, ManuvrMsg::message_defs[i].debug_label)) {
      return &ManuvrMsg::message_defs[i];
    }
  }

  // Didn't find it there. Search in the extended defs...
  const MessageTypeDef* temp_type_def;
  total_elements = message_defs_extended.size();
  for (int i = 1; i < total_elements; i++) {
    temp_type_def = message_defs_extended.get(i);
    if (strstr(label, temp_type_def->debug_label)) {
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
      temp->concatf("\t\t %d\t(%s)\t%s ", i, (working_arg->reap ? "reap" : "no reap"), getArgTypeString(i));
      uint8_t* buf = (uint8_t*) working_arg->target_mem;
      uint16_t l_ender = (working_arg->len < 16 ? working_arg->len : 16);
      for (uint8_t n = 0; n < l_ender; n++) {
        temp->concatf("0x%02x ", (uint8_t) *(buf + n));
      }
      temp->concat("\n");
    }
  }
  else {
    temp->concat("\t No arguments.\n");
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


/**
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
*
* @param  output  The buffer to write results to.
* @return 0 on failure. 1 on success.
*/
int8_t ManuvrMsg::getMsgLegend(StringBuilder *output) {
  if (NULL == output) return 0;

  int total_elements = sizeof(message_defs) / sizeof(MessageTypeDef);
  const MessageTypeDef *temp_def = NULL;
  for (int i = 1; i < total_elements; i++) {
    temp_def = (const MessageTypeDef *) &(message_defs[i]);

    if (isExportable(temp_def)) {
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
  }

  total_elements = message_defs_extended.size();
  for (int i = 1; i < total_elements; i++) {
    temp_def = message_defs_extended.get(i);
    if (isExportable(temp_def)) {
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
  }
  return 1;
}



/**
* This is called when a counterparty wants annotations to our message map.
* Format is binary with a variable length.
* Broken out into a format that a greedy parser can cope with:
*   |--------------|  /---------------\ |-----|
*   | Message Code |  | Several NTSs  | |  0  |
*   |--------------|  \---------------/ |-----|
*           2                 y            1     <---- Bytes
*
* Since the counterparty can't know how big any given definition might be, we need to
*   take care to bake our null-terminators into the output so that the parser on the
*   other side can take the condition "a zero-length string" to signify the end of a message
*   definition, and can move on to the next entry.
*
* Because these definitions might be much larger than their corrosponding MESSAGE_LEGEND entries,
*   it is expected that they will be sent piece-wise to the counterparty so that neither side is
*   expected to hold the entire definition set at once in a single message.
*
* @param  output  The buffer to write results to.
* @return 0 on failure. 1 on success.
*/
#if defined (__ENABLE_MSG_SEMANTICS)
int8_t ManuvrMsg::getMsgSemantics(MessageTypeDef* def, StringBuilder* output) {
  // TODO: def parameter is being ignored for now.
  int8_t return_value = 1;
  if ((NULL != def) && (NULL != output)) {

    int total_elements = sizeof(message_defs) / sizeof(MessageTypeDef);
    const MessageTypeDef *temp_def = NULL;
    for (int i = 1; i < total_elements; i++) {
      temp_def = (const MessageTypeDef *) &(message_defs[i]);

      if (isExportable(temp_def)) {
        //int _set_size = strlen(temp_def->arg_semantics)+1;
        output->concat((unsigned char*) temp_def, 2);

        // Now to capture the argument modes...
        // Capture the null-terminator in the concats. Otherwise, the counterparty can't see where strings end.
        unsigned char* mode = (unsigned char*) temp_def->arg_semantics;
        int arg_mode_len = strlen((const char*) mode);
        while (arg_mode_len > 0) {
          output->concat((unsigned char*) mode, arg_mode_len+1);
          mode += arg_mode_len + 1;
          arg_mode_len = strlen((const char*) mode);
        }
        output->concat((unsigned char*) "\0", 1);   // This is the obligatory "NO ARGUMENT" mode.
      }
    }

    total_elements = message_defs_extended.size();
    for (int i = 1; i < total_elements; i++) {
      temp_def = message_defs_extended.get(i);
      if (isExportable(temp_def)) {
        //int _set_size = strlen(temp_def->arg_semantics)+1;
        output->concat((unsigned char*) temp_def, 2);

        // Now to capture the argument modes...
        // Capture the null-terminator in the concats. Otherwise, the counterparty can't see where strings end.
        unsigned char* mode = (unsigned char*) temp_def->arg_semantics;
        int arg_mode_len = strlen((const char*) mode);
        while (arg_mode_len > 0) {
          output->concat((unsigned char*) mode, arg_mode_len+1);
          mode += arg_mode_len + 1;
          arg_mode_len = strlen((const char*) mode);
        }
        output->concat((unsigned char*) "\0", 1);   // This is the obligatory "NO ARGUMENT" mode.
      }
    }
  }
  return return_value;
}
#endif



/**
* Isolate all of the possible grammatical forms for an argument buffer of the given length.
* Returns 0 if...
*   a) there is no valid argument sequence with the given length
*   b) there is no record of the message type represented by this object.
*
* @param  len  The length of the argument buffer.
* @return The number of possibly-valid grammatical forms the given length buffer.
*/
int ManuvrMsg::collect_valid_grammatical_forms(int len, LinkedList<char*>* return_modes) {
  if (NULL == message_def) getMsgDef();
  if (NULL == message_def) return 0;     // Case (b)

  int return_value = 0;
  char* mode = (char*) message_def->arg_modes;
  int arg_mode_len = strlen((const char*) mode);
  int temp = 0;

    temp = getMinimumSizeByTypeString(mode);
    if (len == temp) {
      return_modes->insert(mode);
      return_value++;
    }
    else if (len > temp) {
      if (containsVariableLengthArgument(mode)) {
        return_modes->insert(mode);
        return_value++;
      }
    }

    mode += arg_mode_len + 1;
    arg_mode_len = strlen((const char*) mode);

  return return_value;
}


/**
* Returns a pointer to the only possible argument sequence for a given length.
* Returns NULL if...
*   a) there is no valid argument sequence with the given length
*   b) there is more than one valid sequence
*   c) there is no record of the message type represented by this object.
*
* @param  len  The length of the input buffer.
* @return A pointer to the type-code sequence, or NULL on failure.
*/
char* ManuvrMsg::is_valid_argument_buffer(int len) {
  if (NULL == message_def) getMsgDef();
  if (NULL == message_def) return NULL;

  char* return_value = NULL;
  char* mode = (char*) message_def->arg_modes;
  int arg_mode_len = strlen((const char*) mode);
  int temp = 0;
  while (arg_mode_len > 0) {
    temp = getMinimumSizeByTypeString(mode);
    if (len == temp) {
      if (NULL != return_value) {
        // This is case (b) above. If this happens, it means the grammatical forms
        //   for this argument were poorly chosen. There is nothing we can do to help
        //   this at runtime. Failure...
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



int8_t ManuvrMsg::registerMessages(const MessageTypeDef defs[], int mes_count) {
  for (int i = 0; i < mes_count; i++) {
    message_defs_extended.insert(&defs[i]);
  }
  return 0;
}


int8_t ManuvrMsg::registerMessage(MessageTypeDef* nu_def) {
  return message_defs_extended.insert(nu_def);
}


int8_t ManuvrMsg::registerMessage(uint16_t tc, uint16_t tf, const char* lab, const unsigned char* forms, const char* sem) {
  MessageTypeDef *nu_def = (MessageTypeDef*) malloc(sizeof(MessageTypeDef));
  if (nu_def) {
    nu_def->msg_type_code  = tc;
    nu_def->msg_type_flags = tf;
    nu_def->debug_label    = lab;
    nu_def->arg_modes      = forms;
    nu_def->arg_semantics  = sem;
    return registerMessage(nu_def);
  }
  else {
    // Misfortune.
  }
  return -1;
}
