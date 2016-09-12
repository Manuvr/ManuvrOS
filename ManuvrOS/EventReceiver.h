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


Lifecycle:
  0) Instantiation  // Constructor
  1) Subscription   // Subscription to Kernel.
  2) Startup        // Kernel activates class when boot completes.
  2) Teardown       // Either by event or direct call.
  3) Unsubscription
  4) Destruction
*/


#ifndef __MANUVR_MSG_MANAGER_H__
  #define __MANUVR_MSG_MANAGER_H__

  #include <inttypes.h>
  #include <EnumeratedTypeCodes.h>
  #include <CommonConstants.h>

  #include <DataStructures/PriorityQueue.h>
  #include <DataStructures/StringBuilder.h>

  #define EVENT_CALLBACK_RETURN_ERROR       -1 // Horrible things happened in the originating class. This should never happen.
  #define EVENT_CALLBACK_RETURN_UNDEFINED   0  // Illegal return code. Kernel will reap events whose callbacks return this.
  #define EVENT_CALLBACK_RETURN_REAP        1  // The callback fxn has specifically told us to reap this event.
  #define EVENT_CALLBACK_RETURN_RECYCLE     2  // The callback class is asking that this event be recycled into the event queue.
  #define EVENT_CALLBACK_RETURN_DROP        3  // The callback class is telling Kernel that it should dequeue

  #define MANUVR_ER_FLAG_VERBOSITY_MASK     0x07  // The bottom 3 bits are for the verbosity value.
  #define MANUVR_ER_FLAG_BOOT_COMPLETE      0x08  // Has the class been bootstrapped?
  #define MANUVR_ER_FLAG_EVENT_PENDING      0x10  // Set when the ER has an event waiting for service.
  #define MANUVR_ER_FLAG_HIDDEN_FROM_DISCOV 0x20  // Setting this indicates that the ER should not expose itself to discovery.
  #define MANUVR_ER_FLAG_THREADED           0x40  // This ER is maintaining its own thread.
  #define MANUVR_ER_FLAG_CONF_DIRTY         0x80  // Our configuration should be persisted.

  class Kernel;
  class ManuvrRunnable;


  extern "C" {
    /**
    * This is an 'interface' class that will allow other classes to receive notice of Events.
    */
    class EventReceiver {
      public:
        virtual ~EventReceiver();

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
        void         printDebug();
        virtual void printDebug(StringBuilder*);
        #ifdef __MANUVR_CONSOLE_SUPPORT
          virtual void procDirectDebugInstruction(StringBuilder *input);
        #endif

        /* These are intended to be overridden. */
        virtual int8_t callback_proc(ManuvrRunnable *);
        int8_t raiseEvent(ManuvrRunnable* event);

        inline const char* getReceiverName() {   return _receiver_name;  }

        /**
        * Call to set the log verbosity for this class. 7 is most verbose. 0 will disable logging altogether.
        *
        * @param   nu_verbosity  The desired verbosity of this class.
        * @return  -1 on failure, and 0 on no change, and 1 on success.
        */
        int8_t setVerbosity(int8_t);

        /**
        * What is this ER's present verbosity level?
        *
        * @return  An integer [0-7]. Larger values reflect chattier classes.
        */
        inline int8_t getVerbosity() {   return (_class_state & MANUVR_ER_FLAG_VERBOSITY_MASK);       };

        /**
        *
        * @return  1 if action was taken, 0 if not, -1 on error.
        */
        virtual int8_t bootComplete();        // This is called from the base notify().

        /**
        * Called for runtime configuration changes mid-lifecycle.
        *
        * @return  1 if action was taken, 0 if not, -1 on error.
        */
        virtual int8_t erConfigure(Argument*); // This is called from the base notify().

        /**
        * Has the class been boot-strapped?
        *
        * @return  true if the class has been booted.
        */
        inline bool   booted() {       return (0 != (_class_state & MANUVR_ER_FLAG_BOOT_COMPLETE)); };

        /**
        * Does this ER's configuration need to be persisted?
        *
        * @return  true if the class has been booted.
        */
        inline bool   dirtyConf() {    return (0 != (_class_state & MANUVR_ER_FLAG_CONF_DIRTY));    };


      protected:
        Kernel* __kernel;
        StringBuilder local_log;

        EventReceiver();

        inline void _mark_boot_complete() {   _class_state |= MANUVR_ER_FLAG_BOOT_COMPLETE;  };

        inline void setReceiverName(const char* nom) {  _receiver_name = nom;  }
        void flushLocalLog();

        // These inlines are for convenience of extending classes.
        inline uint8_t _er_flags() {                 return _extnd_state;            };
        inline bool _er_flag(uint8_t _flag) {        return (_extnd_state & _flag);  };
        inline void _er_flip_flag(uint8_t _flag) {   _extnd_state ^= _flag;          };
        inline void _er_clear_flag(uint8_t _flag) {  _extnd_state &= ~_flag;         };
        inline void _er_set_flag(uint8_t _flag) {    _extnd_state |= _flag;          };
        inline void _er_set_flag(uint8_t _flag, bool nu) {
          if (nu) _extnd_state |= _flag;
          else    _extnd_state &= ~_flag;
        };


      private:
        uint8_t _class_state;
        uint8_t _extnd_state;  // This is here for use by the extending class.
        const char* _receiver_name;

        #if defined(__MANUVR_LINUX) | defined(__MANUVR_FREERTOS)
          // In threaded environments, we allow resources to enable their own threading
          //   if needed.
          int _thread_id;
        #endif

        int8_t setVerbosity(ManuvrRunnable*);  // Private because it should be set with an Event.
    };
  }

  #include "Kernel.h"

#endif  // __MANUVR_MSG_MANAGER_H__
