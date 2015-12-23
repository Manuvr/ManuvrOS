#ifndef __MANUVR_RUNNABLE_H__
  #define __MANUVR_RUNNABLE_H__
  
  #include "ManuvrMsg/ManuvrMsg.h"
  #include <ManuvrOS/MsgProfiler.h>
  
  class EventReceiver;


  /*
  * This class defines an event that gets passed around between classes.
  */
  class ManuvrRunnable : public ManuvrMsg {
    public:
      EventReceiver*  originator;        // This is an optional ref to the class that raised this event.
      EventReceiver*  specific_target;   // If the event is meant for a single class, put a pointer to it here.
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

      bool eventManagerShouldReap();

      int8_t execute();
      
      bool returnToPrealloc();
      bool returnToPrealloc(bool);
      bool isScheduled(bool);
      bool isManaged(bool);

      void printDebug();
      void printDebug(StringBuilder *);
      void printProfilerData(StringBuilder *);

        /* If some other class is managing this memory, this should return 'true'. */
      inline bool isManaged() {         return mem_managed;   }
      
      /* If the scheduler has a lock on this event, this should return 'true'. */
      inline bool isScheduled() {       return scheduled;     }

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
      
      inline bool shouldFire() {            return thread_fire;      }
      inline void autoClear(bool nu) {      autoclear = nu;          }
      inline bool autoClear() {             return autoclear;        }
      inline void threadEnabled(bool nu) {  thread_enabled = nu;     }
      inline bool threadEnabled() {         return thread_enabled;   }

      /**
      * Asks if this schedule is being profiled...
      *  Returns true if so, and false if not.
      */
      inline bool isProfiling() {       return (prof_data != NULL); };

      /* If this event is scheduled, aborts it. Returns true if the schedule was aborted. */
      bool abort();

      void enableProfiling(bool enabled);
      void clearProfilingData();           // Clears profiling data associated with the given schedule.


    private:
      TaskProfilerData* prof_data;  // If this schedule is being profiled, the ref will be here.
      uint8_t  flags;               // Optional flags that might be important for an event.
      bool     mem_managed;         // Set to true to cause the Kernel to not free().
      bool     scheduled;           // Set to true to cause the Kernel to not free().
      bool     preallocated;        // Set to true to cause the Kernel to return this event to its prealloc.

      bool     thread_enabled;      // Is the schedule running?
      bool     thread_fire;         // Is the schedule to be executed?
      bool     autoclear;           // If true, this schedule will be removed after its last execution.

      void __class_initializer();
  };

#endif  //__MANUVR_RUNNABLE_H__
