#ifndef __MANUVR_MSG_MANAGER_H__
  #define __MANUVR_MSG_MANAGER_H__
  
  #include <inttypes.h>
  #include "EnumeratedTypeCodes.h"
  #include <ManuvrOS/CommonConstants.h>
  
  #include "DataStructures/PriorityQueue.h"
  #include "DataStructures/LightLinkedList.h"
  #include <StringBuilder/StringBuilder.h>

  #define EVENT_CALLBACK_RETURN_ERROR       -1 // Horrible things happened in the originating class. This should never happen.
  #define EVENT_CALLBACK_RETURN_UNDEFINED   0  // Illegal return code. Kernel will reap events whose callbacks return this.
  #define EVENT_CALLBACK_RETURN_REAP        1  // The callback fxn has specifically told us to reap this event.
  #define EVENT_CALLBACK_RETURN_RECYCLE     2  // The callback class is asking that this event be recycled into the event queue.
  #define EVENT_CALLBACK_RETURN_DROP        3  // The callback class is telling Kernel that it should dequeue
  
  
  #ifdef __cplusplus
   extern "C" {
  #endif
  
 
  class Kernel;
  class ManuvrEvent;

  
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
      
      // TODO: Why in God's name did I do it this way vs a static CONST? Was it because
      //   I didn't know how to link such that it never took up RAM? If so, I can fix that now...
      //        ---J. Ian Lindsay   Thu Dec 03 03:20:26 MST 2015
      virtual const char* getReceiverName() = 0;
      
      /* These are intended to be overridden. */
      virtual int8_t callback_proc(ManuvrEvent *);
      int8_t raiseEvent(ManuvrEvent* event);
  
      int8_t setVerbosity(int8_t);
      int8_t getVerbosity();
      int purgeLogs();
      
      
    protected:
      Kernel* __kernel;
      StringBuilder local_log;
      uint16_t class_state;         // TODO: Roll verbosity and flags into this. 
      int8_t verbosity;                   // How chatty is this class in the log?
      bool boot_completed;

      virtual int8_t bootComplete();        // This is called from the base notify().
      virtual void   __class_initializer();
  
        
    private:
      int8_t setVerbosity(ManuvrEvent*);  // Private because it should be set with an Event.
  };
  
  
  #ifdef __cplusplus
  }
  #endif 

  #include "Kernel.h"
  
#endif  // __MANUVR_MSG_MANAGER_H__

