#ifndef __MANUVR_MSG_MANAGER_H__
  #define __MANUVR_MSG_MANAGER_H__
  
  #include <inttypes.h>
  #include "EnumeratedTypeCodes.h"
  
  #include "ManuvrMsg/ManuvrMsg.h"
  #include "DataStructures/PriorityQueue.h"
  #include "DataStructures/LightLinkedList.h"
  
  #ifdef ARDUINO
    #include "Arduino.h"
  #endif

  
  #define EVENT_MANAGER_PREALLOC_COUNT        20   // How large a preallocation buffer should we keep?
  #define EVENT_MANAGER_MAX_EVENTS_PER_LOOP    3   // How many Events we proc before we allow the loop to end.
  
  
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
  
  class ManuvrEvent;
  class EventReceiver;

  
  
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
  
      int8_t           priority;
  
      ManuvrEvent(uint16_t msg_code, EventReceiver* cb);
      ManuvrEvent(uint16_t msg_code);
      ManuvrEvent();
      ~ManuvrEvent();
      
      int8_t repurpose(uint16_t code);
  
      bool isManaged(bool);
      bool isScheduled(bool);
      bool returnToPrealloc(bool);


      /**
      * If the memory isn't managed explicitly by some other class, this will tell the EventManager to delete
      *   the completed event.
      * Preallocation implies no reap.
      *
      * @return true if the EventManager ought to free() this Event. False otherwise.
      */
      inline bool eventManagerShouldReap() { 
        return !(mem_managed | preallocated | scheduled); 
      }
      
      /* If some other class is managing this memory, this should return 'true'. */
      inline bool isManaged() {         return mem_managed;   }
      
      /* If the scheduler has a lock on this event, this should return 'true'. */
      inline bool isScheduled() {       return scheduled;     }

      /**
      * Was this event preallocated?
      * Preallocation implies no reap.
      *
      * @return true if the EventManager ought to return this event to its preallocation queue.
      */
      inline bool returnToPrealloc() {  return preallocated;  }
      
      

      void printDebug();
      void printDebug(StringBuilder *);
  
  
    protected:
      uint8_t          flags;         // Optional flags that might be important for an event.
      bool             preallocated;  // Set to true to cause the EventManager to return this event to its prealloc.
      bool             mem_managed;      // Set to true to cause the EventManager to not free().
      bool             scheduled;        // Set to true to cause the EventManager to not free().

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
      StringBuilder local_log;
      Scheduler* scheduler;
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
      ManuvrEvent* current_event;
      
      EventManager(void);
      ~EventManager(void);
      
      int8_t subscribe(EventReceiver *client);                    // A class calls this to subscribe to events.
      int8_t subscribe(EventReceiver *client, uint8_t priority);  // A class calls this to subscribe to events.
      int8_t unsubscribe(EventReceiver *client);                  // A class calls this to unsubscribe.
      
      EventReceiver* getSubscriberByName(const char*);
      
      int8_t procIdleFlags(void);
      
      void profiler(bool enabled);
      
      void printProfiler(StringBuilder *);
      const char* getReceiverName();
      void printDebug(StringBuilder *);
      
      void clean_first_discard();
      float cpu_usage();
      
      bool containsPreformedEvent(ManuvrEvent*);

      /* Overrides from EventReceiver
         EventManager is special, and it will naturally have both methods from EventReceiver.
         Just gracefully fall into those when needed. */
  
  
      static int8_t raiseEvent(uint16_t event_code, EventReceiver* data);
      static int8_t staticRaiseEvent(ManuvrEvent* event);
      
      /* Factory method. Returns a preallocated Event for a one-off use. */
      static ManuvrEvent* returnEvent(uint16_t event_code);
      
      /* Factory method. Returns a preallocated Event for a class to build a preform. */
      static ManuvrEvent* returnPreformEvent(uint16_t event_code);


    protected:
      int8_t bootComplete();

  
    private:
      ManuvrEvent _preallocation_pool[EVENT_MANAGER_PREALLOC_COUNT];
      PriorityQueue<ManuvrEvent*> preallocated;
      PriorityQueue<ManuvrEvent*> discarded;
      PriorityQueue<ManuvrEvent*> event_queue;   // Events that have been raised.
      PriorityQueue<EventReceiver*>    subscribers;   // Our subscription manifest.
      
      // Profiling and logging variables...
      uint32_t micros_occupied;       // How many micros have we spent procing events?
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
      uint32_t requested_preforms;    // How many preformed events have other classes asked for?

      PriorityQueue<TaskProfilerData*> event_costs;     // Message code is the priority. Calculates average cost in uS.

      uint8_t  max_events_p_loop;     // What is the most events we've handled in a single loop?
      bool profiler_enabled;          // Should we spend time profiling this component?


      int8_t validate_insertion(ManuvrEvent*);
      void reclaim_event(ManuvrEvent*);
      inline void update_maximum_queue_depth() {   max_queue_depth = (event_queue.size() > (int) max_queue_depth) ? event_queue.size() : max_queue_depth;   };
      
      
      static EventManager* INSTANCE;
  };
  
  
  #ifdef __cplusplus
  }
  #endif 
    
  
#endif  // __MANUVR_MSG_MANAGER_H__

