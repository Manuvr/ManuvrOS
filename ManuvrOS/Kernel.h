#ifndef __MANUVR_KERNEL_H__
  #define __MANUVR_KERNEL_H__

  class Kernel;
    
  #include <inttypes.h>
  #include "EnumeratedTypeCodes.h"
  #include <ManuvrOS/CommonConstants.h>
  
  #include "FirmwareDefs.h"
  #include "ManuvrRunnable.h"
  #include "EventReceiver.h"
  #include "DataStructures/PriorityQueue.h"
  #include "DataStructures/LightLinkedList.h"
  #include <DataStructures/StringBuilder.h>
  #include <map>

  #include <ManuvrOS/MsgProfiler.h>
  
  #if defined (__MANUVR_FREERTOS)
  #include <FreeRTOS_ARM.h>
  #endif

  #define EVENT_PRIORITY_HIGHEST            100
  #define EVENT_PRIORITY_DEFAULT              2
  #define EVENT_PRIORITY_LOWEST               0

  #define SCHEDULER_MAX_SKIP_BEFORE_RESET  10   // Skipping this many loops will cause us to reboot.


  #if defined(__MANUVR_CONSOLE_SUPPORT) || defined(__MANUVR_DEBUG)
    #ifdef __MANUVR_DEBUG
      #define DEFAULT_CLASS_VERBOSITY    7
    #else
      #define DEFAULT_CLASS_VERBOSITY    4
    #endif
  #else
    #define DEFAULT_CLASS_VERBOSITY      0
  #endif


  #ifdef __cplusplus
   extern "C" {
  #endif

  typedef int (*listenerFxnPtr) (ManuvrRunnable*);

  extern uint32_t micros();  // TODO: Only needed for a single inline fxn. Retain?


  
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
  class Kernel : public EventReceiver {
    public:
      
      Kernel(void);
      ~Kernel(void);
      
      /*
      * Functions for adding to, removing from, and retrieving modules from the kernel. 
      */
      EventReceiver* getSubscriberByName(const char*);
      int8_t subscribe(EventReceiver *client);                    // A class calls this to subscribe to events.
      int8_t subscribe(EventReceiver *client, uint8_t priority);  // A class calls this to subscribe to events.
      int8_t unsubscribe(EventReceiver *client);                  // A class calls this to unsubscribe.
      
      /*
      * Calling this will register a message in the Kernel, along with a call-ahead,
      *   a callback, and options. Only the first parameter is strictly required, but
      *   at least one of the function parameters should be non-null.
      */
      int8_t registerCallbacks(uint16_t msgCode, listenerFxnPtr ca, listenerFxnPtr cb, uint32_t options);
      inline int8_t before(uint16_t msgCode, listenerFxnPtr ca, uint32_t options) {
        return registerCallbacks(msgCode, ca, NULL, options);
      };
      inline int8_t on(uint16_t msgCode, listenerFxnPtr cb, uint32_t options) {
        return registerCallbacks(msgCode, NULL, cb, options);
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
      ManuvrRunnable* createSchedule(uint32_t period, int16_t recurrence, bool auto_clear, FunctionPointer sch_callback);
      ManuvrRunnable* createSchedule(uint32_t period, int16_t recurrence, bool auto_clear, EventReceiver*  sch_callback);
      bool removeSchedule(ManuvrRunnable*);  // Clears all data relating to the given schedule.
      bool addSchedule(ManuvrRunnable*);


      /*
      * These are the core functions of the kernel that must be called from outside.
      */
      int8_t bootstrap(void);                  // Bootstrap the kernel.
      int8_t procIdleFlags(void);              // Execute pending Runnables.
      void advanceScheduler(unsigned int);     // Push all scheduled Runnables forward by one tick.
      inline void advanceScheduler() {   advanceScheduler(MANUVR_PLATFORM_TIMER_PERIOD_MS);  };

      // Logging messages, as well as an override to log locally.
      void printDebug(StringBuilder*);
      inline void printDebug() {        printDebug(&local_log);      };
      #if defined(__MANUVR_DEBUG)
        void print_type_sizes();      // Prints the memory-costs of various classes.
      #endif

      /* Takes user input in the form of direct strings. */
      void accumulateConsoleInput(uint8_t *buf, int len, bool terminal);

      /* Profiling support.. */
      float cpu_usage();
      void profiler(bool enabled);
      void printProfiler(StringBuilder*);
      
      inline void maxEventsPerLoop(int8_t nu) { max_events_per_loop = (nu > 0) ? nu : 1; }
      inline int8_t maxEventsPerLoop() {        return max_events_per_loop; }
      inline int queueSize() {                  return INSTANCE->event_queue.size();     }
      inline bool containsPreformedEvent(ManuvrRunnable* event) {   return event_queue.contains(event);  };
      

      /* Overrides from EventReceiver
         Just gracefully fall into those when needed. */
      const char* getReceiverName();
      int8_t notify(ManuvrRunnable*);
      int8_t callback_proc(ManuvrRunnable *);
      void procDirectDebugInstruction(StringBuilder *);



      static StringBuilder log_buffer;

      /* These functions deal with logging.*/
      volatile static void log(const char *fxn_name, int severity, const char *str, ...);  // Pass-through to the logger class, whatever that happens to be.
      volatile static void log(int severity, const char *str);                             // Pass-through to the logger class, whatever that happens to be.
      volatile static void log(const char *str);                                           // Pass-through to the logger class, whatever that happens to be.
      volatile static void log(char *str);                                           // Pass-through to the logger class, whatever that happens to be.
      volatile static void log(StringBuilder *str);

      static Kernel* getInstance(void);
      static int8_t  raiseEvent(uint16_t event_code, EventReceiver* data);
      static int8_t  staticRaiseEvent(ManuvrRunnable* event);
      static bool    abortEvent(ManuvrRunnable* event);
      static int8_t  isrRaiseEvent(ManuvrRunnable* event);
      
      #if defined (__MANUVR_FREERTOS)
        static uint32_t logger_pid;
        static uint32_t kernel_pid;
        
        void provideKernelPID(TaskHandle_t pid){   kernel_pid = (uint32_t) pid; }
        void provideLoggerPID(TaskHandle_t pid){   logger_pid = (uint32_t) pid; }
        static void unblockThread(uint32_t pid){   vTaskResume((TaskHandle_t) pid); }
      #endif

      /* Factory method. Returns a preallocated Event. */
      static ManuvrRunnable* returnEvent(uint16_t event_code);


    protected:
      int8_t bootComplete();

  
    private:
      ManuvrRunnable* current_event;
      PriorityQueue<ManuvrRunnable*>   event_queue;   // Runnables that are pending execution.
      PriorityQueue<ManuvrRunnable*>   schedules;     // These are Runnables scheduled to be run.
      PriorityQueue<ManuvrRunnable*>   preallocated;  // This is the listing of pre-allocated Runnables.
      PriorityQueue<TaskProfilerData*> event_costs;   // Message code is the priority. Calculates average cost in uS.

      PriorityQueue<EventReceiver*>    subscribers;   // Our manifest of EventReceivers we service.
      std::map<uint16_t, PriorityQueue<listenerFxnPtr>*> ca_listeners;  // Call-ahead listeners.
      std::map<uint16_t, PriorityQueue<listenerFxnPtr>*> cb_listeners;  // Call-back listeners.

      StringBuilder last_user_input;

      uint32_t _ms_elapsed;           // How much time has passed since we serviced our schedules?
      
      // Profiling and logging variables...
      uint32_t lagged_schedules;      // How many schedules were skipped? Ideally this is zero.
      uint32_t micros_occupied;       // How many micros have we spent procing Runnables?
      uint32_t max_idle_loop_time;    // How many uS does it take to run an idle loop?
      uint32_t total_loops;           // How many times have we looped?
      uint32_t total_events;          // How many events have we proc'd?
      uint32_t total_events_dead;     // How many events have we proc'd that went unacknowledged?
      
      uint32_t events_destroyed;      // How many events have we destroyed?
      uint32_t prealloc_starved;      // How many times did we starve the prealloc queue?
      uint32_t burden_of_specific;    // How many events have we reaped?
      uint32_t idempotent_blocks;     // How many times has the idempotent flag prevented a raiseEvent()?
      uint32_t insertion_denials;     // How many times have we rejected events?
      uint32_t max_queue_depth;       // What is the deepest point the queue has reached?
      
      ManuvrRunnable _preallocation_pool[EVENT_MANAGER_PREALLOC_COUNT];

      uint8_t  max_events_p_loop;     // What is the most events we've handled in a single loop?
      int8_t   max_events_per_loop;
      bool     profiler_enabled;      // Should we spend time profiling this component?
      bool     bistable_skip_detect;  // Set in advanceScheduler(), cleared in serviceScheduledEvents().


      int8_t procCallAheads(ManuvrRunnable *active_event);
      int8_t procCallBacks(ManuvrRunnable *active_event);

      unsigned int countActiveSchedules(void);  // How many active schedules are present?
      int serviceScheduledEvents(void);         // Prep any schedules that have come due for exec.

      int8_t validate_insertion(ManuvrRunnable*);
      void reclaim_event(ManuvrRunnable*);
      inline void update_maximum_queue_depth() {   max_queue_depth = (event_queue.size() > (int) max_queue_depth) ? event_queue.size() : max_queue_depth;   };
      
      /*  */
      inline bool should_run_another_event(int8_t loops, uint32_t begin) {     return (max_events_per_loop ? ((int8_t) max_events_per_loop > loops) : ((micros() - begin) < 1200));   };
      
      static Kernel* INSTANCE;
      static PriorityQueue<ManuvrRunnable*> isr_event_queue;   // Events that have been raised from ISRs.
  };

  
  
  #ifdef __cplusplus
  }
  #endif
  
#endif  // __MANUVR_KERNEL_H__

