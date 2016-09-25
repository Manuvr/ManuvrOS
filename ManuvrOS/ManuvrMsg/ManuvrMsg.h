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
* These are flag definitions for Message types.
*/
#define MANUVR_RUNNABLE_FLAG_MEM_MANAGED     0x0100  // Set to true to cause the Kernel to not free().
#define MANUVR_RUNNABLE_FLAG_PREALLOCD       0x0200  // Set to true to cause the Kernel to return this runnable to its prealloc.
#define MANUVR_RUNNABLE_FLAG_AUTOCLEAR       0x0400  // If true, this schedule will be removed after its last execution.
#define MANUVR_RUNNABLE_FLAG_SCHED_ENABLED   0x0800  // Is the schedule running?
#define MANUVR_RUNNABLE_FLAG_SCHEDULED       0x1000  // Set to true to cause the Kernel to not free().
#define MANUVR_RUNNABLE_FLAG_PENDING_EXEC    0x2000  // This schedule is pending execution.

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
    const char*           arg_semantics;  // For messages that have arguments, this defines their semantics.
} MessageTypeDef;


/*
* These are flag definitions for Message types.
* They are constant for a given message type, and are not related to those
*   stored in the _flags member.
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
    ManuvrMsg(uint16_t msg_code, EventReceiver* originator);
    ManuvrMsg(int16_t recurrence, uint32_t sch_period, bool ac, FxnPointer sch_callback);
    ManuvrMsg(int16_t recurrence, uint32_t sch_period, bool ac, EventReceiver* originator);
    ~ManuvrMsg();


    int8_t repurpose(uint16_t code);
    int8_t repurpose(uint16_t code, EventReceiver* cb);

    inline uint16_t eventCode() {  return event_code;   };

    /**
    * Allows the caller to plan other allocations based on how many bytes this message's
    *   Arguments occupy.
    *
    * @return the length (in bytes) of the arguments for this message.
    */
    inline int argByteCount() {
      return ((nullptr == arg) ? 0 : arg->sumAllLengths());
    }

    /**
    * Allows the caller to count the Args attached to this message.
    *
    * @return the cardinality of the argument list.
    */
    inline int      argCount() {   return ((nullptr != arg) ? arg->argCount() : 0);   };

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
    inline int8_t getArgAs(ManuvrMsg **trg_buf) {                return getArgAs(0, (void*) trg_buf);  }

    inline int8_t getArgAs(uint8_t idx, Vector3f  **trg_buf) {        return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, Vector3ui16  **trg_buf) {     return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, Vector3i16  **trg_buf) {      return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, const char  **trg_buf) {      return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, StringBuilder  **trg_buf) {   return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, BufferPipe  **trg_buf) {      return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, EventReceiver  **trg_buf) {   return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, ManuvrXport  **trg_buf) {     return getArgAs(idx, (void*) trg_buf);  }
    inline int8_t getArgAs(uint8_t idx, ManuvrMsg  **trg_buf) {  return getArgAs(idx, (void*) trg_buf);  }

    inline Argument* getArgs() {   return arg;  };

    Argument* takeArgs();

    int8_t markArgForReap(int idx, bool reap);

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

    static bool isExportable(const MessageTypeDef* message_def) {
      return (message_def->msg_type_flags & MSG_FLAG_EXPORTABLE);
    }

    /* Required argument forms */
    static const unsigned char MSG_ARGS_NONE[];

    static const MessageTypeDef message_defs[];
    static const uint16_t TOTAL_MSG_DEFS;


//GRAFT
      EventReceiver*  specific_target;   // If the runnable is meant for a single class, put a pointer to it here.
      FxnPointer schedule_callback; // Pointers to the schedule service function.

      int32_t         priority;          // Set the default priority for this Runnable

      /**
      * If the memory isn't managed explicitly by some other class, this will tell the Kernel to delete
      *   the completed event.
      * Preallocation implies no reap.
      *
      * @return true if the Kernel ought to free() this Event. False otherwise.
      */
      inline bool kernelShouldReap() {
        return (0 == (MANUVR_RUNNABLE_FLAG_MEM_MANAGED | MANUVR_RUNNABLE_FLAG_PREALLOCD | MANUVR_RUNNABLE_FLAG_SCHEDULED));
      };

      int8_t execute();
      int8_t callbackOriginator();

      void printDebug();
      void printDebug(StringBuilder*);

      /* Functions for pinging the profiler data. */
      void noteExecutionTime(uint32_t start, uint32_t stop);


      //TODO: /* These are accessors to concurrency-sensitive members. */
      /* These are accessors to formerly-public members of ScheduleItem. */
      bool alterScheduleRecurrence(int16_t recurrence);
      bool alterSchedulePeriod(uint32_t nu_period);
      bool alterSchedule(FxnPointer sch_callback);
      bool alterSchedule(uint32_t sch_period, int16_t recurrence, bool auto_clear, FxnPointer sch_callback);
      bool enableSchedule(bool enable);      // Re-enable a previously-disabled schedule.
      bool willRunAgain();                   // Returns true if the indicated schedule will fire again.
      void fireNow(bool nu);
      inline void fireNow() {               fireNow(true);           }
      bool delaySchedule(uint32_t by_ms);    // Set the schedule's TTW to the given value this execution only.
      inline bool delaySchedule() {         return delaySchedule(_sched_period);  }  // Reset the given schedule to its period and enable it.

      /* Any required setup finished without problems? */
      inline bool shouldFire() { return (_flags & MANUVR_RUNNABLE_FLAG_PENDING_EXEC); };
      inline void shouldFire(bool en) {
        _flags = (en) ? (_flags | MANUVR_RUNNABLE_FLAG_PENDING_EXEC) : (_flags & ~(MANUVR_RUNNABLE_FLAG_PENDING_EXEC));
      };

      /* Any required setup finished without problems? */
      inline bool scheduleEnabled() { return (_flags & MANUVR_RUNNABLE_FLAG_SCHED_ENABLED); };

      /* Any required setup finished without problems? */
      inline bool autoClear() { return (_flags & MANUVR_RUNNABLE_FLAG_AUTOCLEAR); };
      inline void autoClear(bool en) {
        _flags = (en) ? (_flags | MANUVR_RUNNABLE_FLAG_AUTOCLEAR) : (_flags & ~(MANUVR_RUNNABLE_FLAG_AUTOCLEAR));
      };

      /* Any required setup finished without problems? */
      inline bool isScheduled() { return (_flags & MANUVR_RUNNABLE_FLAG_SCHEDULED); };
      inline void isScheduled(bool en) {
        _flags = (en) ? (_flags | MANUVR_RUNNABLE_FLAG_SCHEDULED) : (_flags & ~(MANUVR_RUNNABLE_FLAG_SCHEDULED));
      };

      /* Schedule member accessors. */
      inline int16_t  scheduleRecurs() { return _sched_recurs; };
      inline uint32_t schedulePeriod() { return _sched_period; };
      inline uint32_t scheduleTimeToWait() { return thread_time_to_wait; };
      inline void setTimeToWait(uint32_t nu) { thread_time_to_wait = nu; };  // TODO: Sloppy. Kill with fire.


      inline void setOriginator(EventReceiver* er) { originator = er; };
      inline bool isOriginator(EventReceiver* er) { return (er == originator); };

      /**
      * Was this event preallocated?
      * Preallocation implies no reap.
      *
      * @param  nu_val Pass true to cause this event to be marked as part of a preallocation pool.
      * @return true if the Kernel ought to return this event to its preallocation queue.
      */
      inline bool returnToPrealloc() { return (_flags & MANUVR_RUNNABLE_FLAG_PREALLOCD); };
      inline void returnToPrealloc(bool en) {
        _flags = (en) ? (_flags | MANUVR_RUNNABLE_FLAG_PREALLOCD) : (_flags & ~(MANUVR_RUNNABLE_FLAG_PREALLOCD));
      };

      /* If some other class is managing this memory, this should return 'true'. */
      inline bool isManaged() { return (_flags & MANUVR_RUNNABLE_FLAG_MEM_MANAGED); };
      inline void isManaged(bool en) {
        _flags = (en) ? (_flags | MANUVR_RUNNABLE_FLAG_MEM_MANAGED) : (_flags & ~(MANUVR_RUNNABLE_FLAG_MEM_MANAGED));
      };

      /* If this runnable is scheduled, aborts it. Returns true if the schedule was aborted. */
      bool abort();

//GRAFT
    #if defined(__MANUVR_EVENT_PROFILER)
      /**
      * Asks if this schedule is being profiled...
      *  Returns true if so, and false if not.
      */
      inline bool profilingEnabled() {       return (prof_data != nullptr); };

      void printProfilerData(StringBuilder*);
      void profilingEnabled(bool enabled);
      void clearProfilingData();           // Clears profiling data associated with the given schedule.
    #endif



  private:
    const MessageTypeDef*  message_def;  // The definition for the message (once it is associated).
    EventReceiver*  originator;        // This is an optional ref to the class that raised this runnable.
    Argument* arg       = nullptr;       // The optional list of arguments associated with this event.
    uint16_t event_code = MANUVR_MSG_UNDEFINED; // The identity of the event (or command).
    uint16_t  _flags;             // Optional flags that might be important for a runnable.

    int16_t  _sched_recurs;            // See Note 2.
    uint32_t _sched_period;            // How often does this schedule execute?
    uint32_t thread_time_to_wait;      // How much longer until the schedule fires?

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



    // Where runtime-loaded message defs go.
    static std::map<uint16_t, const MessageTypeDef*> message_defs_extended;
};

typedef ManuvrMsg ManuvrRunnable;  // TODO: Only here until class merger is finished.

#endif
