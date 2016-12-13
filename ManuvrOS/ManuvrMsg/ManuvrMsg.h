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

#include <EnumeratedTypeCodes.h>
#include <MsgProfiler.h>


/*
* These are flag definitions that might apply to an instance of a Msg.
*/
//#define MANUVR_RUNNABLE_FLAG_MEM_MANAGED     0x0100  // Set to true to cause the Kernel to not free().
#define MANUVR_RUNNABLE_FLAG_AUTOCLEAR       0x0400  // If true, this schedule will be removed after its last execution.
#define MANUVR_RUNNABLE_FLAG_SCHED_ENABLED   0x0800  // Is the schedule running?
#define MANUVR_RUNNABLE_FLAG_SCHEDULED       0x1000  // Set to true to cause the Kernel to not free().
#define MANUVR_RUNNABLE_FLAG_PENDING_EXEC    0x2000  // This schedule is pending execution.


/*
* These are flag definitions that might apply to an instance of a Msg.
*/
#define MANUVR_MSG_FLAG_AUTOCLEAR       0x04000000  // If true, this schedule will be removed after its last execution.
#define MANUVR_MSG_FLAG_SCHED_ENABLED   0x08000000  // Is the schedule running?
#define MANUVR_MSG_FLAG_SCHEDULED       0x10000000  // Set to true to cause the Kernel to not free().
#define MANUVR_MSG_FLAG_PENDING_EXEC    0x20000000  // This schedule is pending execution.

#define MANUVR_MSG_FLAG_PRIORITY_MASK   0x0000FF00
#define MANUVR_MSG_FLAG_REF_COUNT_MASK  0x0000007F


class EventReceiver;

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
* They are constant for a given message type, and are not related to those
*   stored in the _flags member.
*/
#define MSG_FLAG_RESERVED_F   0x0001      // Indicates that only one of the given message should be enqueue.
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
    EventReceiver*  specific_target = nullptr;  // If the runnable is meant for a single class, put a pointer to it here.

    ManuvrMsg();
    ManuvrMsg(uint16_t code);
    ManuvrMsg(uint16_t msg_code, EventReceiver* _origin);
    ManuvrMsg(int16_t recurrence, uint32_t sch_period, bool ac, FxnPointer sch_callback);
    ManuvrMsg(int16_t recurrence, uint32_t sch_period, bool ac, EventReceiver* _origin);
    ~ManuvrMsg();


    int8_t repurpose(uint16_t code, EventReceiver* cb);
    inline int8_t repurpose(uint16_t code) { return repurpose(code, nullptr); };

    #if defined(__MANUVR_DEBUG)
      void printDebug(StringBuilder*);
    #endif

    /**
    * For outside callers to read the private message code.
    *
    * @return the 16-bit message code.
    */
    inline uint16_t eventCode() {  return _code;   };

    /**
    * Is this message exportable over the network?
    *
    * @return true if so.
    */
    inline bool isExportable() {
      if (NULL == message_def) message_def = lookupMsgDefByCode(_code);
      return (message_def->msg_type_flags & MSG_FLAG_EXPORTABLE);
    }

    /**
    * Does this message demand a response from a counterparty across xport?
    *
    * @return true if so.
    */
    inline bool demandsACK() {
      if (NULL == message_def) message_def = lookupMsgDefByCode(_code);
      return (message_def->msg_type_flags & MSG_FLAG_DEMAND_ACK);
    }

    /**
    * Allows the caller to plan other allocations based on how many bytes this message's
    *   Arguments occupy.
    *
    * @return the length (in bytes) of the arguments for this message.
    */
    inline int argByteCount() {
      return ((nullptr == _args) ? 0 : _args->sumAllLengths());
    }

    /**
    * Allows the caller to count the Args attached to this message.
    *
    * @return the cardinality of the argument list.
    */
    inline int argCount() {   return ((nullptr != _args) ? _args->argCount() : 0);   };


    int serialize(StringBuilder*);  // Returns the number of bytes resulting.
    uint8_t inflateArgumentsFromBuffer(uint8_t* buffer, int len);

    uint8_t getArgumentType(uint8_t);  // Given a position, return the type code for the Argument.
    inline uint8_t getArgumentType() {   return getArgumentType(0);  };

    /* Members for getting definition on the message. */
    const char* getMsgTypeString();
    const char* getArgTypeString(uint8_t idx);
    MessageTypeDef* getMsgDef();


    /*
    * Overrides for Argument constructor access. Prevents outside classes from having to care about
    *   implementation details of Arguments. Might look ugly, but takes the CPU burden off of runtime
    *   and forces the compiler to deal with it.
    */
    inline Argument* addArg(uint8_t val) {         return addArg(new Argument(val));  }
    inline Argument* addArg(uint16_t val) {        return addArg(new Argument(val));  }
    inline Argument* addArg(uint32_t val) {        return addArg(new Argument(val));  }
    inline Argument* addArg(int8_t val) {          return addArg(new Argument(val));  }
    inline Argument* addArg(int16_t val) {         return addArg(new Argument(val));  }
    inline Argument* addArg(int32_t val) {         return addArg(new Argument(val));  }
    inline Argument* addArg(float val) {           return addArg(new Argument(val));  }

    inline Argument* addArg(uint8_t *val) {        return addArg(new Argument(val));  }
    inline Argument* addArg(uint16_t *val) {       return addArg(new Argument(val));  }
    inline Argument* addArg(uint32_t *val) {       return addArg(new Argument(val));  }
    inline Argument* addArg(int8_t *val) {         return addArg(new Argument(val));  }
    inline Argument* addArg(int16_t *val) {        return addArg(new Argument(val));  }
    inline Argument* addArg(int32_t *val) {        return addArg(new Argument(val));  }
    inline Argument* addArg(float *val) {          return addArg(new Argument(val));  }

    inline Argument* addArg(Vector3ui16 *val) {    return addArg(new Argument(val));  }
    inline Argument* addArg(Vector3i16 *val) {     return addArg(new Argument(val));  }
    inline Argument* addArg(Vector3f *val) {       return addArg(new Argument(val));  }
    inline Argument* addArg(Vector4f *val) {       return addArg(new Argument(val));  }

    inline Argument* addArg(void *val, int len) {  return addArg(new Argument(val, len));  }
    inline Argument* addArg(const char *val) {     return addArg(new Argument(val));  }
    inline Argument* addArg(StringBuilder *val) {  return addArg(new Argument(val));  }
    inline Argument* addArg(BufferPipe *val) {     return addArg(new Argument(val));  }
    inline Argument* addArg(EventReceiver *val) {  return addArg(new Argument(val));  }
    inline Argument* addArg(ManuvrXport *val) {    return addArg(new Argument(val));  }

    Argument* addArg(Argument*);  // The only "real" implementation.
    int8_t    clearArgs();        // Clear all arguments attached to us, reaping if necessary.
    Argument* takeArgs();
    int8_t    markArgForReap(uint8_t idx, bool reap);
    inline Argument* getArgs() {   return _args;  };

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

    inline int8_t getArgAs(uint8_t idx, uint8_t  *trg_buf) {        return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, uint16_t *trg_buf) {        return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, uint32_t *trg_buf) {        return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, int8_t   *trg_buf) {        return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, int16_t  *trg_buf) {        return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, int32_t  *trg_buf) {        return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, float    *trg_buf) {        return getArgAs(idx, (void*) trg_buf);  }

    // These accessors treat the (void*) as a pointer. These functions are essentially type-casts.
    inline int8_t getArgAs(Vector3f **trg_buf) {                    return getArgAs(0, (void*) trg_buf);  }
    inline int8_t getArgAs(Vector3ui16 **trg_buf) {                 return getArgAs(0, (void*) trg_buf);  }
    inline int8_t getArgAs(Vector3i16 **trg_buf) {                  return getArgAs(0, (void*) trg_buf);  }
    inline int8_t getArgAs(const char **trg_buf) {                  return getArgAs(0, (void*) trg_buf);  }
    inline int8_t getArgAs(StringBuilder **trg_buf) {               return getArgAs(0, (void*) trg_buf);  }
    inline int8_t getArgAs(BufferPipe **trg_buf) {                  return getArgAs(0, (void*) trg_buf);  }
    inline int8_t getArgAs(EventReceiver **trg_buf) {               return getArgAs(0, (void*) trg_buf);  }
    inline int8_t getArgAs(ManuvrXport **trg_buf) {                 return getArgAs(0, (void*) trg_buf);  }
    inline int8_t getArgAs(ManuvrMsg* *trg_buf) {                   return getArgAs(0, (void*) trg_buf);  }

    inline int8_t getArgAs(uint8_t idx, Vector3f  **trg_buf) {      return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, Vector3ui16  **trg_buf) {   return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, Vector3i16  **trg_buf) {    return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, const char  **trg_buf) {    return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, StringBuilder  **trg_buf) { return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, BufferPipe  **trg_buf) {    return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, EventReceiver  **trg_buf) { return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, ManuvrXport  **trg_buf) {   return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, ManuvrMsg  **trg_buf) {     return getArgAs(idx, (void*) trg_buf);  }

    /**
    * If the parameters of the event are such that this class can handle its
    *   execution unaided, there is a performance and encapsulation advantage
    *   to allowing it to do so.
    *
    * @return true if the Kernel should call execute() in lieu of broadcast.
    */
    inline bool singleTarget() { return (schedule_callback || specific_target); };

    /* If singleTarget is true, the kernel calls this to proc the event. */
    int8_t execute();

    /* If this ManuvrMsg is scheduled, aborts it. Returns true if aborted. */
    bool abort();

    /* Applies time to the schedule, bringing it closer to execution. */
    int8_t applyTime(uint32_t ms);

    /* Called from the kernel to inform the class that it has completed. */
    int8_t callbackOriginator();

    /**
    * Accessors for the originoator member.
    */
    inline void setOriginator(EventReceiver* er) { _origin = er; };
    inline bool isOriginator(EventReceiver* er) { return (er == _origin); };

    //TODO: /* These are accessors to concurrency-sensitive members. */
    /* These are accessors to formerly-public members of ScheduleItem. */
    inline uint32_t schedulePeriod() { return _sched_period; };
    bool alterScheduleRecurrence(int16_t recurrence);
    bool alterSchedulePeriod(uint32_t nu_period);
    bool alterSchedule(FxnPointer sch_callback);
    bool alterSchedule(uint32_t sch_period, int16_t recurrence, bool auto_clear, FxnPointer sch_callback);
    bool enableSchedule(bool enable);      // Re-enable a previously-disabled schedule.
    bool willRunAgain();                   // Returns true if the indicated schedule will fire again.
    bool delaySchedule(uint32_t by_ms);    // Set the schedule's TTW to the given value this execution only.
    inline bool delaySchedule() {         return delaySchedule(_sched_period);  }  // Reset the given schedule to its period and enable it.


    /**
    * If this ManuvrMsg is scheduled, call this to proc it on the "next-tick".
    * All schedule members are treated normally, so if the schedule recurs,
    *   it will be re-timed after next-tick, with a decremented recur counter.
    */
    inline void fireNow() { shouldFire(true); };

    /**
    * Is the schedule pending execution ahread of schedule (next tick)?
    *
    * @return true if the schedule will execute ahread of schedule.
    */
    inline bool shouldFire() { return (_flags & MANUVR_RUNNABLE_FLAG_PENDING_EXEC); };

    /**
    * Is this schedule being observed? If yes, it will execute in the future
    *   unless action is taken to stop it.
    *
    * @return true if the schedule is enabled.
    */
    inline bool scheduleEnabled() { return (_flags & MANUVR_RUNNABLE_FLAG_SCHED_ENABLED); };

    /**
    * When this schedule completes normally, will it be dropped from the
    *   scheduler queue? This causes some memory thrash, but saves CPU cycles
    *   expended on keeping the schedule current.
    *
    * @return true if the schedule will be dropped from the schedule queue.
    */
    inline bool autoClear() { return (_flags & MANUVR_RUNNABLE_FLAG_AUTOCLEAR); };
    inline void autoClear(bool en) {
      _flags = (en) ? (_flags | MANUVR_RUNNABLE_FLAG_AUTOCLEAR) : (_flags & ~(MANUVR_RUNNABLE_FLAG_AUTOCLEAR));
    };

    /**
    * Is the kernel holding a lock on us? We are in the scheduler queue, and
    *   we need to be careful about tear-down.
    *
    * @return true if the kernel has us in the scheduler queue.
    */
    inline bool isScheduled() { return (_flags & MANUVR_RUNNABLE_FLAG_SCHEDULED); };
    inline void isScheduled(bool en) {
      _flags = (en) ? (_flags | MANUVR_RUNNABLE_FLAG_SCHEDULED) : (_flags & ~(MANUVR_RUNNABLE_FLAG_SCHEDULED));
    };


    inline uint8_t refCount() {  return (_flags & MANUVR_MSG_FLAG_REF_COUNT_MASK); };
    inline bool    decRefs() {   return (0 == --_flags);  };
    inline void    incRefs() {   _flags++;  };

    inline uint8_t priority() {  return ((_flags & MANUVR_MSG_FLAG_PRIORITY_MASK) >> 8); };
    inline void    priority(uint8_t pri) {
      _flags = (_flags & ~(MANUVR_MSG_FLAG_PRIORITY_MASK)) + (pri << 8);
    };


    #if defined(__MANUVR_EVENT_PROFILER)
      /**
      * Asks if this schedule is being profiled...
      *
      * @return true if so, and false if not.
      */
      inline bool profilingEnabled() {       return (prof_data != nullptr); };

      void printProfilerData(StringBuilder*);
      void profilingEnabled(bool enabled);
      void clearProfilingData();           // Clears profiling data associated with the given schedule.

      /* Function for pinging the profiler data. */
      void noteExecutionTime(uint32_t start, uint32_t stop);
    #endif


    /* Required argument forms */
    static const unsigned char  MSG_ARGS_NONE[];
    static const MessageTypeDef message_defs[];
    static const uint16_t       TOTAL_MSG_DEFS;


    static int8_t getMsgLegend(StringBuilder *output);

    static const MessageTypeDef* lookupMsgDefByCode(uint16_t msg_code);
    static const MessageTypeDef* lookupMsgDefByLabel(char* label);
    static const char*           getMsgTypeString(uint16_t msg_code);

    // TODO: All of this sucks. there-has-to-be-a-better-way.jpg
    static int8_t registerMessage(MessageTypeDef*);
    static int8_t registerMessage(uint16_t, uint16_t, const char*, const unsigned char*, const char*);
    static int8_t registerMessages(const MessageTypeDef[], int len);
    static bool   isExportable(const MessageTypeDef*);



  private:
    const MessageTypeDef* message_def  = nullptr;  // The definition for the message (once it is associated).
    FxnPointer     schedule_callback   = nullptr;  // Pointers to the schedule service function.
    EventReceiver* _origin             = nullptr;  // This is an optional ref to the class that raised this runnable.
    Argument*      _args               = nullptr;  // The optional list of arguments associated with this event.
    uint32_t       _flags              = 0;        // Optional flags that might be important for a runnable.
    uint16_t       _code  = MANUVR_MSG_UNDEFINED;  // The identity of the event (or command).
    int16_t        _sched_recurs       = 0;        // See Note 2.
    uint32_t       _sched_period       = 0;        // How often does this schedule execute?
    uint32_t       _sched_ttw          = 0;        // How much longer until the schedule fires?

    #if defined(__MANUVR_EVENT_PROFILER)
    TaskProfilerData* prof_data = nullptr;  // If this schedule is being profiled, the ref will be here.
    #endif

    int8_t getArgAs(uint8_t idx, void *dat);
    int8_t writePointerArgAs(uint8_t idx, void *trg_buf);

    char* is_valid_argument_buffer(int len);
    int   collect_valid_grammatical_forms(int, LinkedList<char*>*);

    inline void scheduleEnabled(bool en) {
      _flags = (en) ? (_flags | MANUVR_RUNNABLE_FLAG_SCHED_ENABLED) : (_flags & ~(MANUVR_RUNNABLE_FLAG_SCHED_ENABLED));
    };
    inline void shouldFire(bool en) {
      _flags = (en) ? (_flags | MANUVR_RUNNABLE_FLAG_PENDING_EXEC) : (_flags & ~(MANUVR_RUNNABLE_FLAG_PENDING_EXEC));
    };



    // Where runtime-loaded message defs go.
    static std::map<uint16_t, const MessageTypeDef*> message_defs_extended;
};

#endif
