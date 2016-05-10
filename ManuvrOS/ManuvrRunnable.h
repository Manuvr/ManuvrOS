/*
File:   ManuvrRunnable.h
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


#ifndef __MANUVR_RUNNABLE_H__
  #define __MANUVR_RUNNABLE_H__

  #include "ManuvrMsg/ManuvrMsg.h"
  #include <MsgProfiler.h>

  #define MANUVR_RUNNABLE_FLAG_MEM_MANAGED     0x01  // Set to true to cause the Kernel to not free().
  #define MANUVR_RUNNABLE_FLAG_PREALLOCD       0x02  // Set to true to cause the Kernel to return this runnable to its prealloc.
  #define MANUVR_RUNNABLE_FLAG_AUTOCLEAR       0x04  // If true, this schedule will be removed after its last execution.
  #define MANUVR_RUNNABLE_FLAG_THREAD_ENABLED  0x08  // Is the schedule running?
  #define MANUVR_RUNNABLE_FLAG_SCHEDULED       0x10  // Set to true to cause the Kernel to not free().
  #define MANUVR_RUNNABLE_FLAG_PENDING_EXEC    0x20  // This schedule is pending execution.


  class EventReceiver;


  /*
  * This class defines a Runnable that gets passed around between classes.
  */
  class ManuvrRunnable : public ManuvrMsg {
    public:
      EventReceiver*  originator;        // This is an optional ref to the class that raised this runnable.
      EventReceiver*  specific_target;   // If the runnable is meant for a single class, put a pointer to it here.
      FunctionPointer schedule_callback; // Pointers to the schedule service function.

      int32_t         priority;          // Set the default priority for this Runnable

      // The things below were pulled in from ScheduleItem.
      uint32_t thread_time_to_wait;      // How much longer until the schedule fires?
      uint32_t thread_period;            // How often does this schedule execute?
      int16_t  thread_recurs;            // See Note 2.
      // End ScheduleItem

      ManuvrRunnable(int16_t recurrence, uint32_t sch_period, bool ac, FunctionPointer sch_callback);
      ManuvrRunnable(int16_t recurrence, uint32_t sch_period, bool ac, EventReceiver*  originator);
      ManuvrRunnable(uint16_t msg_code, EventReceiver* originator);
      ManuvrRunnable(uint16_t msg_code);
      ManuvrRunnable();
      ~ManuvrRunnable();

      int8_t repurpose(uint16_t code);

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

      void printDebug();
      void printDebug(StringBuilder *);
      void printProfilerData(StringBuilder *);

      /* Functions for pinging the profiler data. */
      void noteExecutionTime(uint32_t start, uint32_t stop);


      //TODO: /* These are accessors to concurrency-sensitive members. */
      /* These are accessors to formerly-public members of ScheduleItem. */
      bool alterScheduleRecurrence(int16_t recurrence);
      bool alterSchedulePeriod(uint32_t nu_period);
      bool alterSchedule(FunctionPointer sch_callback);
      bool alterSchedule(uint32_t sch_period, int16_t recurrence, bool auto_clear, FunctionPointer sch_callback);
      bool enableSchedule(bool enable);      // Re-enable a previously-disabled schedule.
      bool removeSchedule();                 // Clears all data relating to the given schedule.
      bool willRunAgain();                   // Returns true if the indicated schedule will fire again.
      void fireNow(bool nu);
      inline void fireNow() {               fireNow(true);           }
      bool delaySchedule(uint32_t by_ms);    // Set the schedule's TTW to the given value this execution only.
      inline bool delaySchedule() {         return delaySchedule(thread_period);  }  // Reset the given schedule to its period and enable it.

      /* Any required setup finished without problems? */
      inline bool shouldFire() { return (_flags & MANUVR_RUNNABLE_FLAG_PENDING_EXEC); };
      inline void shouldFire(bool en) {
        _flags = (en) ? (_flags | MANUVR_RUNNABLE_FLAG_PENDING_EXEC) : (_flags & ~(MANUVR_RUNNABLE_FLAG_PENDING_EXEC));
      };

      /* Any required setup finished without problems? */
      inline bool threadEnabled() { return (_flags & MANUVR_RUNNABLE_FLAG_THREAD_ENABLED); };
      inline void threadEnabled(bool en) {
        _flags = (en) ? (_flags | MANUVR_RUNNABLE_FLAG_THREAD_ENABLED) : (_flags & ~(MANUVR_RUNNABLE_FLAG_THREAD_ENABLED));
      };

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

      /**
      * Asks if this schedule is being profiled...
      *  Returns true if so, and false if not.
      */
      inline bool profilingEnabled() {       return (prof_data != NULL); };

      /* If this runnable is scheduled, aborts it. Returns true if the schedule was aborted. */
      bool abort();

      void profilingEnabled(bool enabled);
      void clearProfilingData();           // Clears profiling data associated with the given schedule.


    private:
      TaskProfilerData* prof_data;  // If this schedule is being profiled, the ref will be here.
      uint8_t  _flags;              // Optional flags that might be important for a runnable.

      void __class_initializer();
  };

#endif  //__MANUVR_RUNNABLE_H__
