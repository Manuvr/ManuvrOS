#ifndef __MANUVR_KERNEL_H__
  #define __MANUVR_KERNEL_H__

  class Kernel;
    
  #include <inttypes.h>
  #include "EnumeratedTypeCodes.h"
  #include <ManuvrOS/CommonConstants.h>
  
  #include "FirmwareDefs.h"
  #include "ManuvrEvent.h"
  #include "EventManager.h"
  #include "DataStructures/PriorityQueue.h"
  #include "DataStructures/LightLinkedList.h"
  #include <StringBuilder/StringBuilder.h>
  #include <map>

  #include <ManuvrOS/MsgProfiler.h>

  #define EVENT_PRIORITY_HIGHEST            100
  #define EVENT_PRIORITY_DEFAULT              2
  #define EVENT_PRIORITY_LOWEST               0


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

  typedef int (*listenerFxnPtr) (ManuvrEvent*);

  extern uint32_t micros();  // TODO: Only needed for a single inline fxn. Retain?

/* Type for schedule items... */
class ScheduleItem {
  public:
    uint32_t pid;                      // The process ID of this item. Zero is invalid.
    uint32_t thread_time_to_wait;      // How much longer until the schedule fires?
    uint32_t thread_period;            // How often does this schedule execute?
    FunctionPointer schedule_callback; // Pointers to the schedule service function.
    ManuvrEvent* event;                // If we have an event to fire, point to it here.

    TaskProfilerData* prof_data;       // If this schedule is being profiled, the ref will be here.
    EventReceiver*  callback_to_er;    // Optional callback to an EventReceiver.
    int16_t  thread_recurs;            // See Note 2.

    bool     thread_enabled;           // Is the schedule running?
    bool     thread_fire;              // Is the schedule to be executed?
    bool     autoclear;                // If true, this schedule will be removed after its last execution.


    ScheduleItem(uint32_t nu_pid, int16_t recurrence, uint32_t sch_period, bool ac, FunctionPointer sch_callback);
    ScheduleItem(uint32_t nu_pid, int16_t recurrence, uint32_t sch_period, bool ac, EventReceiver*  sch_callback, ManuvrEvent*);
    ~ScheduleItem();
    
    void enableProfiling(bool enabled);
    bool isProfiling();
    void clearProfilingData();           // Clears profiling data associated with the given schedule.
    
    void printDebug(StringBuilder*);


  private:
};



/**  Note 2:
* If this value is equal to -1, then the schedule recurs for as long as it remains enabled.
*  If the value is zero, the schedule is disabled upon execution.
*  If the value is anything else, the schedule remains enabled and this value is decremented.
*/




#define SCHEDULER_MAX_SKIP_BEFORE_RESET  10   // Skipping this many loops will cause us to reboot.

// This is the only version I've tested...
class Scheduler : public EventReceiver {
  public:
    Scheduler();   // Constructor
    ~Scheduler();  // Destructor

    /* Despite being public members, these values should not be written from outside the class.
       They are only public for the sake of convenience of reading them. Since they are profiling-related,
       no major class functionality (other than the profiler output) relies on them. */
    uint32_t next_pid;            // Next PID to assign.
    uint32_t currently_executing; // Hold PID of currently-executing Schedule. 0 if none.
    uint32_t productive_loops;    // Number of calls to serviceScheduledEvents() that actually called a schedule.
    uint32_t total_loops;         // Number of calls to serviceScheduledEvents().
    uint32_t overhead;            // The time in microseconds required to service the last empty schedule loop.
    bool     scheduler_ready;     // TODO: Convert to a uint8 and track states.

    int getTotalSchedules(void);   // How many total schedules are present?
    unsigned int getActiveSchedules(void);  // How many active schedules are present?
    uint32_t peekNextPID(void);         // Discover the next PID without actually incrementing it.
    
    bool scheduleBeingProfiled(uint32_t g_pid);
    void beginProfiling(uint32_t g_pid);
    void stopProfiling(uint32_t g_pid);
    void clearProfilingData(uint32_t g_pid);        // Clears profiling data associated with the given schedule.
    
    // Alters an existing schedule (if PID is found),
    bool alterSchedule(uint32_t schedule_index, uint32_t sch_period, int16_t recurrence, bool auto_clear, FunctionPointer sch_callback);
    bool alterSchedule(uint32_t schedule_index, bool auto_clear);
    bool alterSchedule(uint32_t schedule_index, FunctionPointer sch_callback);
    bool alterScheduleRecurrence(uint32_t schedule_index, int16_t recurrence);
    bool alterSchedulePeriod(uint32_t schedule_index, uint32_t sch_period);
    
    /* Add a new schedule. Returns the PID. If zero is returned, function failed.
     * 
     * Parameters:
     * sch_period      The period of the schedule service routine.
     * recurrence      How many times should this schedule run?
     * auto_clear      When recurrence reaches 0, should the schedule be reaped?
     * sch_callback    The service function. Must be a pointer to a (void fxn(void)).
     */    
    uint32_t createSchedule(uint32_t sch_period, int16_t recurrence, bool auto_clear, FunctionPointer sch_callback);
    uint32_t createSchedule(uint32_t sch_period, int16_t recurrence, bool auto_clear, EventReceiver*  sch_callback, ManuvrEvent*);
    
    bool scheduleEnabled(uint32_t g_pid);   // Is the given schedule presently enabled?

    bool enableSchedule(uint32_t g_pid);   // Re-enable a previously-disabled schedule.
    bool disableSchedule(uint32_t g_pid);  // Turn a schedule off without removing it.
    bool removeSchedule(uint32_t g_pid);   // Clears all data relating to the given schedule.
    bool delaySchedule(uint32_t g_pid, uint32_t by_ms);  // Set the schedule's TTW to the given value this execution only.
    bool delaySchedule(uint32_t g_pid);                  // Reset the given schedule to its period and enable it.
    
    bool fireSchedule(uint32_t g_pid);                  // Fire the given schedule on the next idle loop.
    
    bool willRunAgain(uint32_t g_pid);                  // Returns true if the indicated schedule will fire again.

    int serviceScheduledEvents(void);        // Execute any schedules that have come due.
    void advanceScheduler(void);             // Push all enabled schedules forward by one tick.
    void advanceScheduler(unsigned int);     // Push all enabled schedules forward by one tick.
    
    const char* getReceiverName();

    void printDebug(StringBuilder*);
    void printProfiler(StringBuilder*);
    void printSchedule(uint32_t g_pid, StringBuilder*);
    
    /* Overrides from EventReceiver */
    int8_t notify(ManuvrEvent*);
    int8_t callback_proc(ManuvrEvent *);

    // DEBUG FXNS
    void procDirectDebugInstruction(StringBuilder *);
    // DEBUG FXNS

  protected:
    int8_t bootComplete();

    
  private:
    PriorityQueue<ScheduleItem*> schedules;
    PriorityQueue<ScheduleItem*> execution_queue;
    uint32_t clicks_in_isr;
    uint32_t total_skipped_loops;
    uint32_t lagged_schedules;

    /* These members are concerned with reliability. */
    uint16_t skipped_loops;
    bool     bistable_skip_detect;  // Set in advanceScheduler(), cleared in serviceScheduledEvents().
    uint32_t _ms_elapsed;
    
    bool alterSchedule(ScheduleItem *obj, uint32_t sch_period, int16_t recurrence, bool auto_clear, FunctionPointer sch_callback);

    void destroyAllScheduleItems(void);
    
    bool removeSchedule(ScheduleItem *obj);
    

    uint32_t get_valid_new_pid(void);    
    ScheduleItem* findNodeByPID(uint32_t g_pid);
    void destroyScheduleItem(ScheduleItem *r_node);

    bool delaySchedule(ScheduleItem *obj, uint32_t by_ms);
};

  
  
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
      uint32_t micros_occupied;       // How many micros have we spent procing events?
      ManuvrEvent* current_event;
      
      Kernel(void);
      ~Kernel(void);
      
      int8_t subscribe(EventReceiver *client);                    // A class calls this to subscribe to events.
      int8_t subscribe(EventReceiver *client, uint8_t priority);  // A class calls this to subscribe to events.
      int8_t unsubscribe(EventReceiver *client);                  // A class calls this to unsubscribe.
      
      EventReceiver* getSubscriberByName(const char*);

      /* These functions deal with logging.*/
      volatile static void log(const char *fxn_name, int severity, const char *str, ...);  // Pass-through to the logger class, whatever that happens to be.
      volatile static void log(int severity, const char *str);                             // Pass-through to the logger class, whatever that happens to be.
      volatile static void log(const char *str);                                           // Pass-through to the logger class, whatever that happens to be.
      volatile static void log(char *str);                                           // Pass-through to the logger class, whatever that happens to be.
      volatile static void log(StringBuilder *str);

      int8_t bootstrap(void);

      /*
      TODO: These functions were the last things removed from StaticHub. 
      */
      void feedUSBBuffer(uint8_t *buf, int len, bool terminal);



      /*
      TODO: This particular pile of garbage is temporary pass-through for the Scheduler.
        It should be removed once the Kernel has completed its metamorphosis.
      */
      inline uint32_t createSchedule(uint32_t sch_period, int16_t recurrence, bool auto_clear, FunctionPointer sch_callback) {
        return __scheduler.createSchedule(sch_period, recurrence, auto_clear, sch_callback);
      };
      inline uint32_t createSchedule(uint32_t sch_period, int16_t recurrence, bool auto_clear, EventReceiver*  sch_callback, ManuvrEvent* ev) {
        return __scheduler.createSchedule(sch_period, recurrence, auto_clear, sch_callback, ev);
      };
      inline int serviceScheduledEvents() {
        return __scheduler.serviceScheduledEvents();
      };
      inline bool disableSchedule(uint32_t g_pid) {
        return __scheduler.disableSchedule(g_pid);
      };
      inline bool enableSchedule(uint32_t g_pid) {
        return __scheduler.enableSchedule(g_pid);
      };
      inline bool removeSchedule(uint32_t g_pid) {
        return __scheduler.removeSchedule(g_pid);
      };
      inline bool fireSchedule(uint32_t g_pid) {
        return __scheduler.fireSchedule(g_pid);
      };
      inline bool alterScheduleRecurrence(uint32_t schedule_index, int16_t recurrence) {
        return __scheduler.alterScheduleRecurrence(schedule_index, recurrence);
      };
      inline void advanceScheduler() { 
        __scheduler.advanceScheduler();
      }
      inline void advanceScheduler(unsigned int ms_elapsed) { 
        __scheduler.advanceScheduler(ms_elapsed);
      }

      
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
      

      int8_t procIdleFlags(void);
      
      void profiler(bool enabled);
      
      #if defined(__MANUVR_DEBUG)
        void print_type_sizes();      // Prints the memory-costs of various classes.
      #endif
      void printProfiler(StringBuilder *);
      const char* getReceiverName();
      
      // Logging messages, as well as an override to log locally.
      void printDebug(StringBuilder*);
      inline void printDebug() {
        printDebug(&local_log);
      };
      
      float cpu_usage();
      
      
      inline void maxEventsPerLoop(int8_t nu) { max_events_per_loop = nu;   }
      inline int8_t maxEventsPerLoop() {        return max_events_per_loop; }
      inline int queueSize() {                  return INSTANCE->event_queue.size();   }
      inline bool containsPreformedEvent(ManuvrEvent* event) {   return event_queue.contains(event);  };
      

      /* Overrides from EventReceiver
         Kernel is special, and it will naturally have both methods from EventReceiver.
         Just gracefully fall into those when needed. */
      int8_t notify(ManuvrEvent*);
      int8_t callback_proc(ManuvrEvent *);
      void procDirectDebugInstruction(StringBuilder *);


      static StringBuilder log_buffer;

      static Kernel* getInstance(void);
      static int8_t  raiseEvent(uint16_t event_code, EventReceiver* data);
      static int8_t  staticRaiseEvent(ManuvrEvent* event);
      static bool    abortEvent(ManuvrEvent* event);
      static int8_t  isrRaiseEvent(ManuvrEvent* event);
      
      /* Factory method. Returns a preallocated Event. */
      static ManuvrEvent* returnEvent(uint16_t event_code);


    protected:
      int8_t bootComplete();

  
    private:
      Scheduler __scheduler;
      PriorityQueue<ManuvrEvent*> preallocated;
      PriorityQueue<ManuvrEvent*> event_queue;   // Events that have been raised.
      PriorityQueue<EventReceiver*>    subscribers;   // Our subscription manifest.
      PriorityQueue<TaskProfilerData*> event_costs;     // Message code is the priority. Calculates average cost in uS.
      
      std::map<uint16_t, PriorityQueue<listenerFxnPtr>*> ca_listeners;
      std::map<uint16_t, PriorityQueue<listenerFxnPtr>*> cb_listeners;
      int8_t procCallAheads(ManuvrEvent *active_event);
      int8_t procCallBacks(ManuvrEvent *active_event);
      
      // Profiling and logging variables...
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
      
      ManuvrEvent _preallocation_pool[EVENT_MANAGER_PREALLOC_COUNT];

      uint8_t  max_events_p_loop;     // What is the most events we've handled in a single loop?
      int8_t max_events_per_loop;
      bool profiler_enabled;          // Should we spend time profiling this component?


      int8_t validate_insertion(ManuvrEvent*);
      void reclaim_event(ManuvrEvent*);
      inline void update_maximum_queue_depth() {   max_queue_depth = (event_queue.size() > (int) max_queue_depth) ? event_queue.size() : max_queue_depth;   };
      
      /*  */
      inline bool should_run_another_event(int8_t loops, uint32_t begin) {     return (max_events_per_loop ? ((int8_t) max_events_per_loop > loops) : ((micros() - begin) < 1200));   };
      
      static Kernel* INSTANCE;
      static PriorityQueue<ManuvrEvent*> isr_event_queue;   // Events that have been raised from ISRs.
  };

  
  
  #ifdef __cplusplus
  }
  #endif
  
#endif  // __MANUVR_KERNEL_H__

