/*
File:   Kernel.h
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
* Thanks for the nifty tip, Olaf Bergmann. :-)
*/
#ifdef __GNUC__
  #define UNUSED_PARAM __attribute__ ((unused))
#else /* not a GCC */
  #define UNUSED_PARAM
#endif /* GCC */


#ifndef __MANUVR_KERNEL_H__
  #define __MANUVR_KERNEL_H__

  #include <map>
  #include "CommonConstants.h"
  #include "Utilities.h"
  #include "EnumeratedTypeCodes.h"
  #include "PriorityQueue.h"
  #include "ElementPool.h"
  #include "StringBuilder.h"
  #include "AbstractPlatform.h"
  #include "StopWatch.h"

  #include <EventReceiver.h>
  #ifdef MANUVR_CONSOLE_SUPPORT
    #include <XenoSession/Console/ConsoleInterface.h>
  #endif
  class StopWatch;

  /*
  * These state flags are hosted by the EventReceiver. This may change in the future.
  * Might be too much convention surrounding their assignment across inherritence.
  */
  #define MKERNEL_FLAG_PROFILING     0x01    // Should we spend time profiling this component?
  #define MKERNEL_FLAG_SKIP_DETECT   0x02    // Set in advanceScheduler(), cleared in serviceSchedules().
  #define MKERNEL_FLAG_SKIP_FAILSAFE 0x04    // Too many skips will send us to the bootloader.
  #define MKERNEL_FLAG_PENDING_PIPE  0x08    // There is Pipe I/O pending.
  #define MKERNEL_FLAG_IDLE          0x10    // The kernel is idle.


  #ifdef __cplusplus
  extern "C" {
  #endif




  /****************************************************************************************************
  *  ___ ___   ____  ____   __ __  __ __  ____       __  _    ___  ____   ____     ___  _
  * |   T   T /    T|    \ |  T  T|  T  ||    \     |  l/ ]  /  _]|    \ |    \   /  _]| T
  * | _   _ |Y  o  ||  _  Y|  |  ||  |  ||  D  )    |  ' /  /  [_ |  D  )|  _  Y /  [_ | |
  * |  \_/  ||     ||  |  ||  |  ||  |  ||    /     |    \ Y    _]|    / |  |  |Y    _]| l___
  * |   |   ||  _  ||  |  ||  :  |l  :  !|    \     |     Y|   [_ |    \ |  |  ||   [_ |     T
  * |   |   ||  |  ||  |  |l     | \   / |  .  Y    |  .  ||     T|  .  Y|  |  ||     T|     |
  * l___j___jl__j__jl__j__j \__,_j  \_/  l__j\_j    l__j\_jl_____jl__j\_jl__j__jl_____jl_____j
  *
  ****************************************************************************************************/

  /*
  * This class is the machinery that handles Events. It should probably only be instantiated once.
  */
  class Kernel : public EventReceiver
    #ifdef MANUVR_CONSOLE_SUPPORT
      , public ConsoleInterface
    #endif
    {
    public:
      Kernel();
      ~Kernel();

      #ifdef MANUVR_CONSOLE_SUPPORT
        /* Overrides from ConsoleInterface */
        uint consoleGetCmds(ConsoleCommand**);
        inline const char* consoleName() { return getReceiverName();  };
        void consoleCmdProc(StringBuilder* input);
      #endif

      /*
      * Functions for adding to, removing from, and retrieving modules from the kernel.
      */
      EventReceiver* getSubscriberByName(const char*);
      int8_t subscribe(EventReceiver *client);                    // A class calls this to subscribe to events.
      int8_t subscribe(EventReceiver *client, uint8_t priority);  // A class calls this to subscribe to events.
      int8_t unsubscribe(EventReceiver *client);                  // A class calls this to unsubscribe.
      inline EventReceiver* getSubscriber(int i) {                // Fetch a subscriber by index.
        return subscribers.get(i);
      };

      /*
      * Calling this will register a message in the Kernel, along with a call-ahead,
      *   a callback, and options. Only the first parameter is strictly required, but
      *   at least one of the function parameters should be non-null.
      */
      int8_t registerCallbacks(uint16_t msgCode, listenerFxnPtr ca, listenerFxnPtr cb, uint32_t options);
      inline int8_t before(uint16_t msgCode, listenerFxnPtr ca, uint32_t options) {
        return registerCallbacks(msgCode, ca, nullptr, options);
      };
      inline int8_t on(uint16_t msgCode, listenerFxnPtr cb, uint32_t options) {
        return registerCallbacks(msgCode, nullptr, cb, options);
      };


      // TODO: These members were ingested from the Scheduler.
      /* Add a new schedule. Returns the PID. If zero is returned, function failed.
       *
       * Parameters:
       * sch_period      The period of the schedule service routine.
       * recurrence      How many times should this schedule run?
       * auto_clear      When recurrence reaches 0, should the schedule be reaped?
       * sch_callback    The service function. Must be a pointer to a (void fxn(void)).
       */
      ManuvrMsg* createSchedule(uint32_t period, int16_t recurrence, bool auto_clear, FxnPointer sch_callback);
      ManuvrMsg* createSchedule(uint32_t period, int16_t recurrence, bool auto_clear, EventReceiver*  sch_callback);
      bool removeSchedule(ManuvrMsg*);  // Clears all data relating to the given schedule.
      bool addSchedule(ManuvrMsg*);
      void printScheduler(StringBuilder*);

      /*
      * These are the core functions of the kernel that must be called from outside.
      */
      int8_t procIdleFlags();                  // Execute pending Msgs.
      void advanceScheduler(unsigned int);     // Push all scheduled Msgs forward by one tick.
      inline void advanceScheduler() {   advanceScheduler(MANUVR_PLATFORM_TIMER_PERIOD_MS);  };

      /* Profiling support.. */
      float cpu_usage();
      inline double dutyCycle() {
        return (_millis_working / (double)(_millis_working + _millis_idle));
      };

      void profiler(bool enabled);
      void printProfiler(StringBuilder*);

      inline void maxEventsPerLoop(int8_t nu) { max_events_per_loop = (nu > 0) ? nu : 1; }
      inline int8_t maxEventsPerLoop() {        return max_events_per_loop; }
      inline int queueSize() {                  return INSTANCE->exec_queue.size();     }
      inline bool containsPreformedEvent(ManuvrMsg* event) {   return exec_queue.contains(event);  };
      inline bool idle() {                     return (_er_flag(MKERNEL_FLAG_IDLE));              };

      /* Overrides from EventReceiver
         Just gracefully fall into those when needed. */
      int8_t notify(ManuvrMsg*);
      int8_t callback_proc(ManuvrMsg*);
      void printDebug(StringBuilder*);


      static BufferPipe* _logger;        // The log pipe.
      static uint32_t lagged_schedules;  // How many schedules were skipped? Ideally this is zero.

      /* These functions deal with logging.*/
      static void log(int severity, const char *str);  // Pass-through to the logger class, whatever that happens to be.
      static void log(const char *str);                // Pass-through to the logger class, whatever that happens to be.
      static void log(char *str);                      // Pass-through to the logger class, whatever that happens to be.
      static void log(StringBuilder *str);
      static int8_t attachToLogger(BufferPipe*);
      static int8_t detachFromLogger(BufferPipe*);

      static int8_t raiseEvent(uint16_t event_code, EventReceiver* data);
      static int8_t staticRaiseEvent(ManuvrMsg* event);
      static bool   abortEvent(ManuvrMsg* event);
      static int8_t isrRaiseEvent(ManuvrMsg* event);
      static void   nextTick(BufferPipe*);

      /* Returns a preallocated ManuvrMsg. */
      static ManuvrMsg* returnEvent(uint16_t event_code);
      static ManuvrMsg* returnEvent(uint16_t event_code, EventReceiver*);



    private:
      ManuvrMsg _preallocation_pool[EVENT_MANAGER_PREALLOC_COUNT];
      ManuvrMsg* current_event = nullptr;  // The presently-executing event.
      ElementPool<ManuvrMsg>           _msg_prealloc; // This is the listing of pre-allocated Msgs.
      PriorityQueue<ManuvrMsg*>        exec_queue;    // Msgs that are pending execution.
      PriorityQueue<ManuvrMsg*>        schedules;     // These are Msgs scheduled to be run.

      PriorityQueue<BufferPipe*>       _pipe_io_pend; // Pending BufferPipe transfers that wish to be async.
      PriorityQueue<StopWatch*>        event_costs;   // Message code is the priority. Calculates average cost in uS.
      PriorityQueue<EventReceiver*>    subscribers;   // Our manifest of EventReceivers we service.
      std::map<uint16_t, PriorityQueue<listenerFxnPtr>*> ca_listeners;  // Call-ahead listeners.
      std::map<uint16_t, PriorityQueue<listenerFxnPtr>*> cb_listeners;  // Call-back listeners.

      uint32_t _ms_elapsed        = 0; // How much time has passed since we serviced our schedules?
      uint32_t _skips_observed    = 0; // How many sequential scheduler skips have we noticed?
      /* Profiling and logging variables... */
      uint32_t micros_occupied    = 0; // How many micros have we spent procing Msgs?
      uint32_t total_loops        = 0; // How many times have we looped?
      uint32_t total_events       = 0; // How many events have we proc'd?
      uint32_t total_events_dead  = 0; // How many events have we proc'd that went unacknowledged?
      uint32_t max_queue_depth    = 0; // What is the deepest point the queue has reached?
      uint32_t max_idle_loop_time;     // How many uS does it take to run an idle loop?
      uint32_t idle_loops;             // How many idle loops have we run?
      uint16_t consequtive_idles;      // How many consecutive idle loops?
      uint16_t max_idle_count;         // How many consecutive idle loops before we act?
      uint32_t insertion_denials;      // How many times have we rejected events?


      uint8_t  max_events_p_loop;     // What is the most events we've handled in a single loop?
      int8_t   max_events_per_loop;

      int8_t procCallAheads(ManuvrMsg* active_event);
      int8_t procCallBacks(ManuvrMsg* active_event);

      unsigned int countActiveSchedules();  // How many active schedules are present?
      int serviceSchedules();         // Prep any schedules that have come due for exec.

      int8_t validate_insertion(ManuvrMsg*);
      void reclaim_event(ManuvrMsg*);
      inline void update_maximum_queue_depth() {   max_queue_depth = (exec_queue.size() > (int) max_queue_depth) ? exec_queue.size() : max_queue_depth;   };


      /*  */
      inline bool should_run_another_event(int8_t loops, uint32_t begin) {     return (max_events_per_loop ? ((int8_t) max_events_per_loop > loops) : ((micros() - begin) < 1200));   };

      inline bool _profiler_enabled() {         return (_er_flag(MKERNEL_FLAG_PROFILING));            };
      inline void _profiler_enabled(bool nu) {  return (_er_set_flag(MKERNEL_FLAG_PROFILING, nu));    };
      inline bool _skip_detected() {            return (_er_flag(MKERNEL_FLAG_SKIP_DETECT));          };
      inline void _skip_detected(bool nu) {     return (_er_set_flag(MKERNEL_FLAG_SKIP_DETECT, nu));  };
      inline bool _pending_pipes() {            return (_er_flag(MKERNEL_FLAG_PENDING_PIPE));         };
      inline void _pending_pipes(bool nu) {     return (_er_set_flag(MKERNEL_FLAG_PENDING_PIPE, nu)); };
      void _idle(bool nu);

      static Kernel*     INSTANCE;
      static PriorityQueue<ManuvrMsg*> isr_exec_queue;   // Events that have been raised from ISRs.

      static unsigned long _millis_idle;
      static unsigned long _millis_working;
      static unsigned long _idle_trans_point;
  };



  #ifdef __cplusplus
  }
  #endif


/* Tail inclusion... */
#include <DataStructures/BufferPipe.h>

#endif  // __MANUVR_KERNEL_H__
