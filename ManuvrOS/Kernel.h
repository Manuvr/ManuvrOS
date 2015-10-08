#ifndef __MANUVR_KERNEL_H__
  #define __MANUVR_KERNEL_H__
  
  #include <inttypes.h>
  #include "EnumeratedTypeCodes.h"
  
  #include "ManuvrMsg/ManuvrMsg.h"
  #include "DataStructures/PriorityQueue.h"
  #include "DataStructures/LightLinkedList.h"
  
  
  /*
  * This class is the machinery that handles Events. It should probably only be instantiated once.
  */
  class ManuvrKernel : public EventReceiver {
    public:
      uint32_t micros_occupied;       // How many micros have we spent procing events?
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
#endif  // __MANUVR_KERNEL_H__

