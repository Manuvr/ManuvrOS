#ifndef __MANUVR_MSG_MANAGER_H__
  #define __MANUVR_MSG_MANAGER_H__
  
  #include <inttypes.h>
  #include <map>
  #include "EnumeratedTypeCodes.h"
  
  #include "ManuvrMsg/ManuvrMsg.h"
  #include "DataStructures/PriorityQueue.h"
  #include "DataStructures/LightLinkedList.h"
  
  //#ifdef ARDUINO
  //  #include "Arduino.h"
  //#endif

  
  #define EVENT_MANAGER_PREALLOC_COUNT      8   // How large a preallocation buffer should we keep?

  
  #define EVENT_CALLBACK_RETURN_ERROR       -1 // Horrible things happened in the originating class. This should never happen.
  #define EVENT_CALLBACK_RETURN_UNDEFINED   0  // Illegal return code. EventManager will reap events whose callbacks return this.
  #define EVENT_CALLBACK_RETURN_REAP        1  // The callback fxn has specifically told us to reap this event.
  #define EVENT_CALLBACK_RETURN_RECYCLE     2  // The callback class is asking that this event be recycled into the event queue.
  #define EVENT_CALLBACK_RETURN_DROP        3  // The callback class is telling EventManager that it should dequeue
  
  
  #define EVENT_PRIORITY_HIGHEST            100
  #define EVENT_PRIORITY_DEFAULT              2
  #define EVENT_PRIORITY_LOWEST               0
  
  
  #ifdef TEST_BENCH
    #define DEFAULT_CLASS_VERBOSITY    7
  #else
    #define DEFAULT_CLASS_VERBOSITY    3
  #endif
  

  
  #ifdef __cplusplus
   extern "C" {
  #endif
  
  typedef int (*listenerFxnPtr) (ManuvrEvent*);
  
  class ManuvrEvent;
  class Scheduler;
  class EventReceiver;

  
  extern unsigned long micros();
  
  
  class TaskProfilerData {
    public:
      TaskProfilerData();
      ~TaskProfilerData();
      
      uint32_t msg_code;
      uint32_t run_time_last;
      uint32_t run_time_best;
      uint32_t run_time_worst;
      uint32_t run_time_average;
      uint32_t run_time_total;
      uint32_t executions;       // How many times has this task been used?
      bool     profiling_active;
      
      void printDebug(StringBuilder *);
      static void printDebugHeader(StringBuilder *);
  };
  
  
  
  /*
  * This class defines an event that gets passed around between classes.
  */
  class ManuvrEvent : public ManuvrMsg {
    public:
      EventReceiver*   callback;         // This is an optional ref to the class that raised this event.
      EventReceiver*   specific_target;  // If the event is meant for a single class, put a pointer to it here.
  
      int32_t          priority;
  
      ManuvrEvent(uint16_t msg_code, EventReceiver* cb);
      ManuvrEvent(uint16_t msg_code);
      ManuvrEvent();
      ~ManuvrEvent();
      
      int8_t repurpose(uint16_t code);
  
      bool eventManagerShouldReap();
  
      bool returnToPrealloc();
      bool returnToPrealloc(bool);
      bool isScheduled(bool);
      bool isManaged(bool);
  
      void printDebug();
      void printDebug(StringBuilder *);
  
        /* If some other class is managing this memory, this should return 'true'. */
      inline bool isManaged() {         return mem_managed;   }
      
      /* If the scheduler has a lock on this event, this should return 'true'. */
      inline bool isScheduled() {       return scheduled;     }
      
      /* If this event is scheduled, aborts it. Returns true if the schedule was aborted. */
      bool abort();


    protected:
      uint8_t          flags;         // Optional flags that might be important for an event.
      bool             mem_managed;      // Set to true to cause the EventManager to not free().
      bool             scheduled;        // Set to true to cause the EventManager to not free().
      bool             preallocated;  // Set to true to cause the EventManager to return this event to its prealloc.

      void __class_initializer();

      
    private:
  };
  

  
  
  
  
  /**
  * This is an 'interface' class that will allow other classes to receive notice of Events.
  */
  class EventReceiver {
    public:
      virtual ~EventReceiver() {};
      
      /*
      * This is the fxn by which subscribers are notified of events. This fxn should return
      *   zero for no action taken, and non-zero if the event needs to be re-evaluated
      *   before being passed on to the next subscriber.
      */
      virtual int8_t notify(ManuvrEvent*);
      
      /*
      * These have no reason to be here other than to enforce some discipline while
      *   writing things. Eventually, they ought to be cased off by the preprocessor
      *   so that production builds don't get cluttered by debugging junk that won't
      *   see use.
      */
      void                printDebug();
      virtual void        printDebug(StringBuilder*);
      virtual void        procDirectDebugInstruction(StringBuilder *input);
      virtual const char* getReceiverName() = 0;
      
      /* These are intended to be overridden. */
      virtual int8_t callback_proc(ManuvrEvent *);
      int8_t raiseEvent(ManuvrEvent* event);
  
      int8_t setVerbosity(int8_t);
      int8_t getVerbosity();
      int purgeLogs();
      
      
    protected:
      Scheduler* scheduler;
      StringBuilder local_log;
      uint16_t class_state;         // TODO: Roll verbosity and flags into this. 
      int8_t verbosity;                   // How chatty is this class in the log?
      bool boot_completed;

      virtual int8_t bootComplete();        // This is called from the base notify().
      virtual void   __class_initializer();
  
        
    private:
      int8_t setVerbosity(ManuvrEvent*);  // Private because it should be set with an Event.
  };
  
  
  
  /*
  * This class is the machinery that handles Events. It should probably only be instantiated once.
  */
  class EventManager : public EventReceiver {
    public:
      uint32_t micros_occupied;       // How many micros have we spent procing events?
      ManuvrEvent* current_event;
      
      EventManager(void);
      ~EventManager(void);
      
      int8_t subscribe(EventReceiver *client);                    // A class calls this to subscribe to events.
      int8_t subscribe(EventReceiver *client, uint8_t priority);  // A class calls this to subscribe to events.
      int8_t unsubscribe(EventReceiver *client);                  // A class calls this to unsubscribe.
      
      EventReceiver* getSubscriberByName(const char*);
      
      /*
      * Calling this will register a message in the EventManager, along with a call-ahead,
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
      
      void printProfiler(StringBuilder *);
      const char* getReceiverName();
      void printDebug(StringBuilder *);
      
      void clean_first_discard();
      float cpu_usage();
      
      bool containsPreformedEvent(ManuvrEvent*);
      
      
      inline void maxEventsPerLoop(int8_t nu) { max_events_per_loop = nu;   }
      inline int8_t maxEventsPerLoop() {        return max_events_per_loop; }
      inline int queueSize() {                  return INSTANCE->event_queue.size();   }
      

      /* Overrides from EventReceiver
         EventManager is special, and it will naturally have both methods from EventReceiver.
         Just gracefully fall into those when needed. */

  
      static int8_t raiseEvent(uint16_t event_code, EventReceiver* data);
      static int8_t staticRaiseEvent(ManuvrEvent* event);
      static bool   abortEvent(ManuvrEvent* event);
      static int8_t isrRaiseEvent(ManuvrEvent* event);
      
      /* Factory method. Returns a preallocated Event. */
      static ManuvrEvent* returnEvent(uint16_t event_code);


    protected:
      int8_t bootComplete();

  
    private:
      PriorityQueue<ManuvrEvent*> preallocated;
      PriorityQueue<ManuvrEvent*> discarded;
      PriorityQueue<ManuvrEvent*> event_queue;   // Events that have been raised.
      PriorityQueue<EventReceiver*>    subscribers;   // Our subscription manifest.
      PriorityQueue<TaskProfilerData*> event_costs;     // Message code is the priority. Calculates average cost in uS.
      
      map<uint16_t, PriorityQueue<listenerFxnPtr>*> ca_listeners;
      map<uint16_t, PriorityQueue<listenerFxnPtr>*> cb_listeners;
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
      
      
      static EventManager* INSTANCE;
      static PriorityQueue<ManuvrEvent*> isr_event_queue;   // Events that have been raised from ISRs.
  };
  
  
  #ifdef __cplusplus
  }
  #endif 
    
  
#endif  // __MANUVR_MSG_MANAGER_H__

