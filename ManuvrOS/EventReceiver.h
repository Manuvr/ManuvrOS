/*
File:   EventReceiver.h
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


#ifndef __MANUVR_MSG_MANAGER_H__
  #define __MANUVR_MSG_MANAGER_H__

  #include <inttypes.h>
  #include "EnumeratedTypeCodes.h"
  #include <CommonConstants.h>

  #include "DataStructures/PriorityQueue.h"
  #include "DataStructures/LightLinkedList.h"
  #include <DataStructures/StringBuilder.h>

  #define EVENT_CALLBACK_RETURN_ERROR       -1 // Horrible things happened in the originating class. This should never happen.
  #define EVENT_CALLBACK_RETURN_UNDEFINED   0  // Illegal return code. Kernel will reap events whose callbacks return this.
  #define EVENT_CALLBACK_RETURN_REAP        1  // The callback fxn has specifically told us to reap this event.
  #define EVENT_CALLBACK_RETURN_RECYCLE     2  // The callback class is asking that this event be recycled into the event queue.
  #define EVENT_CALLBACK_RETURN_DROP        3  // The callback class is telling Kernel that it should dequeue

  #define MANUVR_ER_FLAG_VERBOSITY_MASK     0x0007  // The bottom 3 bits are for the verbosity value.
  #define MANUVR_ER_FLAG_BOOT_COMPLETE      0x0008  // Has the class been bootstrapped?
  #define MANUVR_ER_FLAG_EVENT_PENDING      0x0010  // Set when the ER has an event waiting for service.
  #define MANUVR_ER_FLAG_HIDDEN_FROM_DISCOV 0x0020  // Setting this indicates that the ER should not expose itself to discovery.
  #define MANUVR_ER_FLAG_RESERVED_8         0x0040  // Reserved
  #define MANUVR_ER_FLAG_RESERVED_7         0x0080  // Reserved
  #define MANUVR_ER_FLAG_RESERVED_6         0x0100  // Reserved
  #define MANUVR_ER_FLAG_RESERVED_5         0x0200  // Reserved
  #define MANUVR_ER_FLAG_RESERVED_4         0x0400  // Reserved
  #define MANUVR_ER_FLAG_RESERVED_3         0x0800  // Reserved
  #define MANUVR_ER_FLAG_RESERVED_2         0x1000  // Reserved
  #define MANUVR_ER_FLAG_RESERVED_1         0x2000  // Reserved
  #define MANUVR_ER_FLAG_RESERVED_0         0x4000  // Reserved
  #define MANUVR_ER_FLAG_THREADED           0x8000  // This ER is maintaining its own thread.

  #ifdef __cplusplus
   extern "C" {
  #endif


  class Kernel;
  class ManuvrRunnable;


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
      virtual int8_t notify(ManuvrRunnable*);

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
      virtual int8_t callback_proc(ManuvrRunnable *);
      int8_t raiseEvent(ManuvrRunnable* event);

      int purgeLogs();

      /**
      * Call to set the log verbosity for this class. 7 is most verbose. -1 will disable logging altogether.
      *
      * @param   nu_verbosity  The desired verbosity of this class.
      * @return  -1 on failure, and 0 on no change, and 1 on success.
      */
      inline int8_t getVerbosity() {   return (_class_state & MANUVR_ER_FLAG_VERBOSITY_MASK);       };
      inline bool   booted() {         return (0 != (_class_state & MANUVR_ER_FLAG_BOOT_COMPLETE)); };
      int8_t setVerbosity(int8_t);


    protected:
      Kernel* __kernel;
      StringBuilder local_log;

      virtual int8_t bootComplete();        // This is called from the base notify().
      virtual void   __class_initializer();

      inline void _mark_boot_complete() {   _class_state |= MANUVR_ER_FLAG_BOOT_COMPLETE;  };


    private:
      #if defined(__MANUVR_LINUX) | defined(__MANUVR_FREERTOS)
        // In threaded environments, we allow resources to enable their own threading
        //   if needed.
        int _thread_id;
      #endif

      uint16_t _class_state;

      int8_t setVerbosity(ManuvrRunnable*);  // Private because it should be set with an Event.

  };


  #ifdef __cplusplus
  }
  #endif

  #include "Kernel.h"

#endif  // __MANUVR_MSG_MANAGER_H__
