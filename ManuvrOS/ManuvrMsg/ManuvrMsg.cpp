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


#include <CommonConstants.h>
#include <DataStructures/Argument.h>
#include "ManuvrMsg.h"
#include <string.h>
#include <Kernel.h>

extern inline double   parseDoubleFromchars(unsigned char *input);
extern inline float    parseFloatFromchars(unsigned char *input);
extern inline uint32_t parseUint32Fromchars(unsigned char *input);
extern inline uint16_t parseUint16Fromchars(unsigned char *input);


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
// Runtime manifest of Msg definitions.
std::map<uint16_t, const MessageTypeDef*> ManuvrMsg::message_defs_extended;

// Generic argument def for a message with no args.
const unsigned char ManuvrMsg::MSG_ARGS_NONE[] = {0};

/**
* This function is needed as a static.
* Is this message exportable over the network?
*
* @return true if so.
*/
bool ManuvrMsg::isExportable(const MessageTypeDef* message_def) {
  return (message_def->msg_type_flags & MSG_FLAG_EXPORTABLE);
}

/**
* Called by other classes to add their event definitions to the runtime
*   manifest.
*
* @param  MessageTypeDef[]  An array of MessageTypeDefs to add.
* @param  int  The number of MessageTypeDefs in the array.
* @return 0 on success. Non-zero otherwise.
*/
int8_t ManuvrMsg::registerMessages(const MessageTypeDef defs[], int mes_count) {
  for (int i = 0; i < mes_count; i++) {
    message_defs_extended[defs[i].msg_type_code] = &defs[i];
  }
  return 0;
}

/**
* Called by other classes to add their event definitions to the runtime
*   manifest.
*
* @param  MessageTypeDef*  A single MessageTypeDef to add.
* @return 0 on success. Non-zero otherwise.
*/
int8_t ManuvrMsg::registerMessage(MessageTypeDef* nu_def) {
  message_defs_extended[nu_def->msg_type_code] = nu_def;
  return 0;
}

/**
* Called by other classes to add their event definitions to the runtime
*   manifest.
*
* @param  uint16_t     Msg code
* @param  uint16_t     Msg flags
* @param  const char*  Msg label
* @param  const unsigned char*  Valid grammatical forms for the Msg.
* @param  const char*  Semantic markings for the Msg.
* @return 0 on success. Non-zero otherwise.
*/
int8_t ManuvrMsg::registerMessage(uint16_t tc, uint16_t tf, const char* lab, const unsigned char* forms, const char* sem) {
  MessageTypeDef *nu_def = (MessageTypeDef*) malloc(sizeof(MessageTypeDef));
  if (nu_def) {
    nu_def->msg_type_code  = tc;
    nu_def->msg_type_flags = tf;
    nu_def->debug_label    = lab;
    nu_def->arg_modes      = forms;
    return registerMessage(nu_def);
  }
  else {
    // Misfortune.
  }
  return -1;
}


/**
* Given a Msg code, return the corrosponding label.
*
* @param  uint16_t  The message identity code in question.
* @return a pointer to the human-readable label for this Msg code. Never nullptr.
*/
const char* ManuvrMsg::getMsgTypeString(uint16_t code) {
  for (int i = 0; i < TOTAL_MSG_DEFS; i++) {
    if (ManuvrMsg::message_defs[i].msg_type_code == code) {
      return ManuvrMsg::message_defs[i].debug_label;
    }
  }

  // Didn't find it there. Search in the extended defs...
  const MessageTypeDef* temp_type_def = message_defs_extended[code];
  if ( temp_type_def) {
    return temp_type_def->debug_label;
  }
  // If we've come this far, we don't know what the caller is asking for. Return the default.
  return ManuvrMsg::message_defs[0].debug_label;
}


/**
* This will never return NULL. It will at least return the defintion for the UNDEFINED class.
* This is the static version of this method.
*
* @param  uint16_t  The message identity code in question.
* @return a pointer to the MessageTypeDef for this Msg code. Never nullptr.
*/
const MessageTypeDef* ManuvrMsg::lookupMsgDefByCode(uint16_t code) {
  for (int i = 0; i < TOTAL_MSG_DEFS; i++) {
    if (ManuvrMsg::message_defs[i].msg_type_code == code) {
      return &ManuvrMsg::message_defs[i];
    }
  }
  // Didn't find it there. Search in the extended defs...
  const MessageTypeDef* temp_type_def = message_defs_extended[code];
  if (temp_type_def) {
    return temp_type_def;
  }
  // If we've come this far, we don't know what the caller is asking for. Return the default.
  return &ManuvrMsg::message_defs[0];
}


/**
* This will never return NULL. It will at least return the defintion for the
*   UNDEFINED Msg.
*
* @param  char*  The message label by which to lookup the message identity.
* @return a pointer to the MessageTypeDef for this Msg code. Never nullptr.
*/
const MessageTypeDef* ManuvrMsg::lookupMsgDefByLabel(char* label) {
  for (int i = 1; i < TOTAL_MSG_DEFS; i++) {
    // TODO: Yuck... have to import string.h for JUST THIS. Re-implement inline....
    if (strstr(label, ManuvrMsg::message_defs[i].debug_label)) {
      return &ManuvrMsg::message_defs[i];
    }
  }

  // Didn't find it there. Search in the extended defs...
  const MessageTypeDef* temp_type_def;
	std::map<uint16_t, const MessageTypeDef*>::iterator it;
	for (it = message_defs_extended.begin(); it != message_defs_extended.end(); it++) {
    temp_type_def = it->second;
    if (strstr(label, temp_type_def->debug_label)) {
      return temp_type_def;
    }
  }
  // If we've come this far, we don't know what the caller is asking for. Return the default.
  return &ManuvrMsg::message_defs[0];
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
* @param  StringBuilder*  The buffer to write results to.
* @return 0 on failure. 1 on success.
*/
int8_t ManuvrMsg::getMsgLegend(StringBuilder* output) {
  if (nullptr == output) return 0;

  const MessageTypeDef *temp_def = nullptr;
  for (int i = 1; i < TOTAL_MSG_DEFS; i++) {
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

	std::map<uint16_t, const MessageTypeDef*>::iterator it;
	for (it = message_defs_extended.begin(); it != message_defs_extended.end(); it++) {
    temp_def = it->second;

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


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

/**
* Vanilla constructor.
*/
ManuvrMsg::ManuvrMsg() {
  priority(EVENT_PRIORITY_DEFAULT);  // Set the default priority for this Msg
  _origin           = nullptr;
  specific_target   = nullptr;
  schedule_callback = nullptr;
}

/**
* Second-order constructor that specifies the identity.
*
* @param uint16_t   The identity code for the new message.
*/
ManuvrMsg::ManuvrMsg(uint16_t code) : ManuvrMsg() {
  repurpose(code);
}

/**
* Second-order constructor that specifies the message code, and a callback.
*
* @param uint16_t  The message id code.
* @param EventReceiver*  A pointer to the EventReceiver to be notified about completion of this event.
*/
ManuvrMsg::ManuvrMsg(uint16_t code, EventReceiver* cb) : ManuvrMsg() {
  repurpose(code, cb);
}

/**
* Third-order constructor. Takes a void fxn(void) as a callback (Legacy).
*
* @param int16_t    How many times should this schedule run?
* @param uint32_t   How often should this schedule run (in milliseconds).
* @param bool       Should the scheduler autoclear this schedule when it finishes running?
* @param FxnPointer A FxnPointer to the service routine. Useful for some general things.
*/
ManuvrMsg::ManuvrMsg(int16_t recurrence, uint32_t sch_period, bool ac, FxnPointer sch_callback) : ManuvrMsg(MANUVR_MSG_DEFERRED_FXN) {
  autoClear(ac);
  _sched_recurs  = recurrence;
  _sched_period  = sch_period;
  _sched_ttw     = sch_period;
  schedule_callback = sch_callback;    // This constructor uses the legacy callback.
}

/**
* Third-order constructor. Takes an EventReceiver* as an _origin.
* We need to deal with memory management of the Event. For a recurring schedule, we can't allow the
*   Kernel to reap the event. So it is very important to mark the event appropriately.
*
* @param int16_t         How many times should this schedule run?
* @param uint32_t        How often should this schedule run (in milliseconds).
* @param bool            Should the scheduler autoclear this schedule when it finishes running?
* @param EventReceiver*  A pointer to an Event that we will periodically raise.
*/
ManuvrMsg::ManuvrMsg(int16_t recurrence, uint32_t sch_period, bool ac, EventReceiver* ori) : ManuvrMsg(MANUVR_MSG_DEFERRED_FXN) {
  autoClear(ac);
  _sched_recurs  = recurrence;
  _sched_period  = sch_period;
  _sched_ttw     = sch_period;
  _origin        = ori;
}


/**
* Destructor. Clears all Arguments. Memory management is accounted
*   for in each Argument, so no need to worry about that here.
*/
ManuvrMsg::~ManuvrMsg() {
  #if defined(MANUVR_EVENT_PROFILER)
  clearProfilingData();
  #endif
  clearArgs();
}


/**
* Call this member to repurpose this message for an unrelated task. This mechanism
*   is designed to prevent malloc()/free() thrash where it can be avoided.
*
* @param uint16_t The new identity code for the message.
* @param EventReceiver* The EventReceiver that should be called-back on completion.
* @return 0 on success, or appropriate failure code.
*/
int8_t ManuvrMsg::repurpose(uint16_t code, EventReceiver* cb) {
  // These things have implications for memory management, which is why repurpose() doesn't touch them.
  uint32_t _persist_mask = MANUVR_RUNNABLE_FLAG_SCHEDULED;
  _flags            = _flags & _persist_mask;
  _origin           = cb;
  specific_target   = nullptr;
  schedule_callback = nullptr;
  priority(EVENT_PRIORITY_DEFAULT);
  _code             = code;
  message_def       = lookupMsgDefByCode(_code);
  return 0;
}


/*******************************************************************************
* Argument manipulation...                                                     *
*******************************************************************************/

/**
* Call this member to repurpose this message for an unrelated task. This mechanism
*   is designed to prevent malloc()/free() thrash where it can be avoided.
*
* @param Argument* The fully-formed argument to attach to this Msg.
* @return nullptr on failure, or Argument passed as a parameter on success.
*/
Argument* ManuvrMsg::addArg(Argument* nu) {
  if (_args) {
    return _args->link(nu);
  }
  _args = nu;
  return nu;
};


/**
* This function is for the exclusive purpose of inflating an argument from a place where a
*   pointer doesn't make sense. This means that an argument mode that contains a non-exportable
*   pointer type will not produce the expected result. It might even asplode.
*
* @param  uint8_t* A buffer containing the byte-stream containing the data.
* @param  int Length of the buffer
* @return  the number of Arguments extracted and instantiated from the buffer.
*/
uint8_t ManuvrMsg::inflateArgumentsFromBuffer(uint8_t* buffer, int len) {
  int return_value = 0;
  if ((nullptr == buffer) || (0 == len)) {
    return 0;
  }

  char* arg_mode = nullptr;
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

  Argument *nu_arg = nullptr;
  while ((nullptr != arg_mode) & (len > 0)) {
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
        nu_arg->reapValue(true);
        break;
      case VECT_3_FLOAT:
        nu_arg = new Argument(new Vector3f(parseFloatFromchars(buffer + 0), parseFloatFromchars(buffer + 4), parseFloatFromchars(buffer + 8)));
        len = len - 12;
        buffer += 12;
        nu_arg->reapValue(true);
        break;
      case VECT_3_UINT16:
        nu_arg = new Argument(new Vector3ui16(parseUint16Fromchars(buffer + 0), parseUint16Fromchars(buffer + 2), parseUint16Fromchars(buffer + 4)));
        len = len - 6;
        buffer += 6;
        nu_arg->reapValue(true);
        break;
      case VECT_3_INT16:
        nu_arg = new Argument(new Vector3i16((int16_t) parseUint16Fromchars(buffer + 0), (int16_t) parseUint16Fromchars(buffer + 2), (int16_t) parseUint16Fromchars(buffer + 4)));
        len = len - 6;
        buffer += 6;
        nu_arg->reapValue(true);
        break;

      // Variable-length types...
      case STR_FM:
        nu_arg = new Argument(strdup((const char*) buffer));
        buffer = buffer + nu_arg->length();
        len    = len - nu_arg->length();
        break;

      default:
        // Abort parse. We encountered a type we can't deal with.
        clearArgs();
        return 0;
        break;
    }

    addArg(nu_arg);
    nu_arg = nullptr;
    return_value++;

    arg_mode++;
  }
  return return_value;
}



/**
* Marks the given Argument as reapable or not. This is useful to avoid a malloc/copy/free cycle
*   for constants or flash-resident strings.
*
* @param  uint8_t The Argument position
* @param  bool  True if the Argument should be reaped with the message, false if it should be retained.
* @return 1 on success, 0 on failure.
*/
int8_t ManuvrMsg::markArgForReap(uint8_t idx, bool reap) {
  if (_args) {
    Argument* tmp = _args->retrieveArgByIdx(idx);
    tmp->reapValue(reap);
    return 1;
  }
  return 0;
}


/**
* All of the type-specialized getArgAs() fxns boil down to this. Which is private.
* The boolean preserve parameter will leave the argument attached (if true), or destroy it (if false).
*
* @param  uint8_t The Argument position
* @param  void*  A pointer to the place where we should write the result.
* @return 0 or appropriate failure code.
*/
int8_t ManuvrMsg::getArgAs(uint8_t idx, void* trg_buf) {
  int8_t return_value = -1;
  if (_args) {
    return ((0 == idx) ? _args->getValueAs(trg_buf) : _args->getValueAs(idx, trg_buf));
  }
  return return_value;
}


/**
* All of the type-specialized writePointerArgAs() fxns boil down to this.
* Calls to this fxn are only valid for pointer types.
*
* @param  uint8_t  The Argument position
* @param  trg_buf  A pointer to the place where we should read the Argument from.
* @return 0 or appropriate failure code.
*/
int8_t ManuvrMsg::writePointerArgAs(uint8_t idx, void* trg_buf) {
  int8_t return_value = -1;
  if (_args) {
    switch (_args->typeCode()) {
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
        return_value = 0;
        *((uintptr_t*) _args->target_mem) = *((uintptr_t*) trg_buf);
        break;
      default:
        return_value = -2;
        break;
    }
  }

  return return_value;
}


/**
* Clears the Arguments attached to this Message and reaps their contents, if
*   the need is indicated. Many subtle memory-related bugs can be caught here.
*
* @return 0 or appropriate failure code.
*/
int8_t ManuvrMsg::clearArgs() {
  if (_args) {
    Argument* tmp = _args;
    _args = nullptr;
    delete tmp;
  }
  return 0;
}


/**
* Returns the Argument* we carry, and then we placidly forget about it.
*
* @return nullptr if there were no Arguments, or the Arguments if there were.
*/
Argument* ManuvrMsg::takeArgs() {
  Argument* ret = _args;
  _args = nullptr;
  return ret;
}


/**
* Given idx, find the Argument and return its type.
*
* @param  uint8_t The Argument position
* @return NOTYPE_FM if the Argument isn't found, and its type code if it is.
*/
uint8_t ManuvrMsg::getArgumentType(uint8_t idx) {
  if (_args) {
    Argument* a = _args->retrieveArgByIdx(idx);
    if (a) {
      return a->typeCode();
    }
  }
  return NOTYPE_FM;
}


/**
* At some point, some downstream code will care about the definition of the
*   messages represented by this message. This will fetch the definition if
*   needed and return it.
*
* @return a pointer to the MessageTypeDef that identifies this class of Message. Never NULL.
*/
MessageTypeDef* ManuvrMsg::getMsgDef() {
  if (nullptr == message_def) {
    message_def = lookupMsgDefByCode(_code);
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
* Given idx, find the Argument and return its type.
*
* @param  uint8_t The Argument index.
* @return The human-readable label for the type of the Argument at given index.
*/
const char* ManuvrMsg::getArgTypeString(uint8_t idx) {
  if (nullptr == _args) return "<INVALID INDEX>";
  Argument* a = _args->retrieveArgByIdx(idx);
  if (nullptr == a) return "<INVALID INDEX>";
  return getTypeCodeString(a->typeCode());
}


/**
* This fxn takes all of the Arguments attached to this Message and writes them, along
*   with their types, to the provided StringBuilder as an unsigned charater stream.
*
* @param  StringBuilder* The buffer into which we should write our output.
* @return the number of Arguments so written, or a negative value on failure.
*/
int ManuvrMsg::serialize(StringBuilder* output) {
  if (output == nullptr) return -1;
  int return_value = 0;
  int delta_len = 0;
  Argument* current_arg = _args;
  while (current_arg) {
    delta_len = current_arg->serialize_raw(output);
    if (delta_len < 0) {
      return delta_len;
    }
    else {
      return_value++;
    }
  }
  return return_value;
}


/**
* Isolate all of the possible grammatical forms for an argument buffer of the given length.
* Returns 0 if...
*   a) there is no valid argument sequence with the given length
*   b) there is no record of the message type represented by this object.
*
* @param  int  The length of the argument buffer.
* @return The number of possibly-valid grammatical forms the given length buffer.
*/
int ManuvrMsg::collect_valid_grammatical_forms(int len, LinkedList<char*>* return_modes) {
  if (nullptr == message_def) getMsgDef();
  if (nullptr == message_def) return 0;     // Case (b)

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
* @param  int  The length of the input buffer.
* @return A pointer to the type-code sequence, or NULL on failure.
*/
char* ManuvrMsg::is_valid_argument_buffer(int len) {
  if (nullptr == message_def) getMsgDef();
  if (nullptr == message_def) return nullptr;

  char* return_value = nullptr;
  char* mode = (char*) message_def->arg_modes;
  int arg_mode_len = strlen((const char*) mode);
  int temp = 0;
  while (arg_mode_len > 0) {
    temp = getMinimumSizeByTypeString(mode);
    if (len == temp) {
      if (return_value) {
        // This is case (b) above. If this happens, it means the grammatical forms
        //   for this argument were poorly chosen. There is nothing we can do to help
        //   this at runtime. Failure...
        return nullptr;
      }
      else {
        return_value = mode;
      }
    }
    else if (len > temp) {
      if (nullptr != return_value) {
        // TODO: incorrect. No variable-length types...
        return nullptr;
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


#if defined(MANUVR_DEBUG)
/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void ManuvrMsg::printDebug(StringBuilder *output) {
  const MessageTypeDef* type_obj = getMsgDef();

  if (&ManuvrMsg::message_defs[0] == type_obj) {
    output->concatf("    ---< UNDEFINED (0x%04x) >-------------\n", _code);
  }
  else {
    output->concatf("    ---< %s >-----------------------------\n", getMsgTypeString());
  }

  if (_args) {
    output->concatf("\t %d Arguments:\n", _args->argCount());
    _args->printDebug(output);
    output->concat("\n");
  }
  else {
    output->concat("\t No arguments.\n");
  }

  output->concatf(
    "\t Profiling:\t%s\n\t Priority:\t%u\n\t Refs:\t%u\n",
    (profilingEnabled() ? YES_STR : NO_STR),
    priority(),
    refCount()
  );

  output->concatf("\t Originator:           %s\n", (nullptr == _origin ? NUL_STR : _origin->getReceiverName()));

  if (specific_target) {
    output->concatf("\t specific_target:      %s\n", (nullptr == specific_target ? NUL_STR : specific_target->getReceiverName()));
  }
  else {
    output->concat("\t Broadcast\n");
  }

  if (isScheduled()) {
    output->concatf("\t [%p] Schedule \n\t --------------------------------\n", this);
    output->concatf("\t Enabled       \t%s\n", (scheduleEnabled() ? YES_STR : NO_STR));
    output->concatf("\t Time-till-fire\t%u\n", _sched_ttw);
    output->concatf("\t Period        \t%u\n", _sched_period);
    output->concatf("\t Recurs?       \t%d\n", _sched_recurs);
    output->concatf("\t Exec pending: \t%s\n", (shouldFire() ? YES_STR : NO_STR));
    output->concatf("\t Autoclear     \t%s\n", (autoClear() ? YES_STR : NO_STR));
  }

  if (schedule_callback) {
    output->concat("\t Legacy callback\n");
  }
  output->concat("\n");
}
#endif  // MANUVR_DEBUG


/*******************************************************************************
* Functions dealing with profiling this particular Msg.                   *
*******************************************************************************/
#if defined(MANUVR_EVENT_PROFILER)

/**
* Prints details about the profiler data.
*
* @param  StringBuilder* The buffer to output into.
*/
void ManuvrMsg::printProfilerData(StringBuilder* output) {
  if (prof_data) output->concatf("\t %p  %9u  %9u  %9u  %9u  %9u  %9u %s\n", this, prof_data->executions, prof_data->run_time_total, prof_data->run_time_average, prof_data->run_time_worst, prof_data->run_time_best, prof_data->run_time_last, (isScheduled() ? " " : "(INACTIVE)"));
}

/**
* Any schedule that has a TaskProfilerData object in the appropriate slot will be profiled.
*  So to begin profiling a schedule, simply instance the appropriate struct into place.
*
* @param  bool  Enables or disables Msg profiling.
*/
void ManuvrMsg::profilingEnabled(bool enabled) {
  if (nullptr == prof_data) {
    // Profiler data does not exist. If enabled == false, do nothing.
    if (enabled) {
      prof_data = new TaskProfilerData();
      prof_data->profiling_active  = true;
    }
  }
  else {
    // Profiler data exists.
    prof_data->profiling_active  = enabled;
  }
}


/**
* Destroys whatever profiling data might be stored in this Msg.
*/
void ManuvrMsg::clearProfilingData() {
  if (prof_data) {
    prof_data->profiling_active = false;
    delete prof_data;
    prof_data = nullptr;
  }
}


/**
* Function for pinging the profiler data.
*
* @param  uint32_t  The time (in uS) that the Msg entered execution.
* @param  uint32_t  The time (in uS) that the Msg exited execution.
*/
void ManuvrMsg::noteExecutionTime(uint32_t profile_start_time, uint32_t profile_stop_time) {
  if (prof_data) {
    profile_stop_time = micros();
    prof_data->run_time_last    = wrap_accounted_delta(profile_start_time, profile_stop_time);  // Rollover invarient.
    prof_data->run_time_best    = strict_min(prof_data->run_time_best,  prof_data->run_time_last);
    prof_data->run_time_worst   = strict_max(prof_data->run_time_worst, prof_data->run_time_last);
    prof_data->run_time_total  += prof_data->run_time_last;
    prof_data->run_time_average = prof_data->run_time_total / ((prof_data->executions) ? prof_data->executions : 1);
    prof_data->executions++;
  }
}
#endif // MANUVR_EVENT_PROFILER



/*******************************************************************************
* Pertaining to deferred execution and scheduling....                          *
*******************************************************************************/
/**
* Safely aborts a schedule that is queued to execute.
*
* @return  true if the schedule was aborted.
*/
bool ManuvrMsg::abort() {
  return Kernel::abortEvent(this);
}


/**
* @param  uint32_t  The desired period (in mS) for this schedule.
* @return  true if the schedule alteraction succeeded.
*/
bool ManuvrMsg::alterSchedulePeriod(uint32_t nu_period) {
  bool return_value  = false;
  if (nu_period > 1) {
    _sched_period       = nu_period;
    _sched_ttw = nu_period;
    return_value  = true;
  }
  return return_value;
}


/**
* @param  int16_t  The desired repetition conut for this schedule.
* @return  true if the schedule alteraction succeeded.
*/
bool ManuvrMsg::alterScheduleRecurrence(int16_t recurrence) {
  shouldFire(false);
  _sched_recurs = recurrence;
  return true;
}


/**
* Call this function to alter a given schedule. Set with the given period, a given number of times, with a given function call.
*  Returns true on success or false if the given PID is not found, or there is a problem with the parameters.
*
* Will not set the schedule active, but will clear any pending executions for this schedule, as well as reset the timer for it.
*
* @param  uint32_t  The desired period (in mS) for this schedule.
* @param  int16_t   The desired repetition conut for this schedule.
* @param  bool      Should the scheduler autoclear this schedule when it finishes running?
* @param FxnPointer A FxnPointer to the service routine. Useful for some general things.
* @return  true if the schedule alteraction succeeded.
*/
bool ManuvrMsg::alterSchedule(uint32_t sch_p, int16_t sch_r, bool ac, FxnPointer sch_cb) {
  bool return_value  = false;
  if (sch_p > 1) {
    if (sch_cb) {
      shouldFire(false);
      autoClear(ac);
      _sched_recurs     = sch_r;
      _sched_period     = sch_p;
      _sched_ttw        = sch_p;
      schedule_callback = sch_cb;
      return_value      = true;
    }
  }
  return return_value;
}


/**
* Call this function to alter a given schedule. Set with the given period, a given number of times, with a given function call.
*  Returns true on success or false if the given PID is not found, or there is a problem with the parameters.
*
* Will not set the schedule active, but will clear any pending executions for this schedule, as well as reset the timer for it.
*
* @param FxnPointer A FxnPointer to the service routine. Useful for some general things.
* @return  true if the schedule alteraction succeeded.
*/
bool ManuvrMsg::alterSchedule(FxnPointer sch_cb) {
  bool return_value  = false;
  if (sch_cb) {
    schedule_callback = sch_cb;
    return_value      = true;
  }
  return return_value;
}


/**
* Returns true if...
* A) The schedule exists
*    AND
* B) The schedule is enabled, and has at least one more runtime before it *might* be auto-reaped.
*
* @return  true if the above conditions are met. False otherwise.
*/
bool ManuvrMsg::willRunAgain() {
  if (isScheduled() & scheduleEnabled()) {
    return (0 != _sched_recurs);
  }
  return false;
}


/**
* Applies time to the schedule, bringing it closer to execution.
*
* @param  uint32_t The number of milliseconds to drop from the schedule.
* @return  an integer code directing the kernel how to procede.
*/
int8_t ManuvrMsg::applyTime(uint32_t mse) {
  int8_t return_value = 0;
  if (scheduleEnabled()) {
    if ((_sched_ttw > mse) && (!shouldFire())) {
      _sched_ttw -= mse;
    }
    else {
      // The schedule should execute this time.
      uint32_t adjusted_ttw = (mse - _sched_ttw);
      if (adjusted_ttw > _sched_period) {
        // TODO: Possible error-case? Too many clicks passed. We have schedule jitter...
        // For now, we'll just throw away the difference.
        adjusted_ttw = 0;
        Kernel::lagged_schedules++;
      }
      _sched_ttw = _sched_period - adjusted_ttw;
      switch (_sched_recurs) {
        default:
          // If we are on a fixed execution-count, but will run again.
          _sched_recurs--;
        case -1:
          // We run until stopped.
          return_value = 1;
          break;
        case 0:
          // We aren't supposed to run again.
          scheduleEnabled(false);
          return_value = autoClear() ? -1 : 1;
          break;
      }
      shouldFire(false);    // ...mark it as serviced.
    }
  }
  return return_value;  // Kernel will take no action.
}


/**
* Call to (en/dis)able a given schedule.
* Will reset the time_to_wait such that if the schedule is re-enabled,
*   it doesn't fire sooner than expected.
*
* @param   bool Should this schedule be enabled?
* @return  true, always.
*/
bool ManuvrMsg::enableSchedule(bool en) {
  shouldFire(en);
  if (en) {
    _sched_ttw = _sched_period;
  }
  scheduleEnabled(en);
  return true;
}


/**
* Causes a given schedule's TTW (time-to-wait) to be set to the value we provide (this time only).
* If the schedule wasn't enabled before, it will be when we return.
*
* @param   uint32_t The number of ms to delay (this time only).
* @return  true, always.
*/
bool ManuvrMsg::delaySchedule(uint32_t by_ms) {
  _sched_ttw = by_ms;
  scheduleEnabled(true);
  return true;
}



/*******************************************************************************
* Actually execute this runnable.                                              *
*******************************************************************************/
/**
* Causes this Message to inform its _origin (if any) that it has fully-traversed
*   the kernel's broadcast cycle.
*
* @return  An event callback return code.
*/
int8_t ManuvrMsg::callbackOriginator() {
  if (_origin) {
    /* If the event has a valid _origin, do the callback dance and take instruction
       from the return value. */
    return _origin->callback_proc(this);
  }
  else {
    /* If there is no _origin specified for the Event, we rely on the flags in the Event itself to
     decide if it should be reaped. If its memory is being managed by some other class, the reclaim_event()
     fxn will simply remove it from the exec_queue and consider the matter closed. */
  }
  return EVENT_CALLBACK_RETURN_UNDEFINED;
}


/**
* If the parameters of the event are such that this class can handle its
*   execution unaided, there is a performance and encapsulation advantage
*   to allowing it to do so.
* If singleTarget is true, the kernel calls this to proc the event.
*
* @return the notify() equivilent int reflecting how many actions were taken.
*/
int8_t ManuvrMsg::execute() {
  if (schedule_callback) {
    // TODO: This is hold-over from the scheduler. Need to modernize it.
    //active_runnable->printDebug(&local_log);
    ((FxnPointer) schedule_callback)();   // Call the schedule's service function.
    return 1;
  }
  else if (specific_target) {
    return specific_target->notify(this);
  }
  return 0;
}
