#ifndef __MANUVR_EVENT_H__
  #define __MANUVR_EVENT_H__
  
  #include "ManuvrMsg/ManuvrMsg.h"
  
  class EventReceiver;

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
      bool             mem_managed;      // Set to true to cause the Kernel to not free().
      bool             scheduled;        // Set to true to cause the Kernel to not free().
      bool             preallocated;  // Set to true to cause the Kernel to return this event to its prealloc.

      void __class_initializer();

      
    private:
  };
  
#endif
