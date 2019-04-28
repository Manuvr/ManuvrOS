/*
File:   EventReceiver.cpp
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


#include <Platform/Platform.h>


EventReceiver::EventReceiver(const char* nom) : _receiver_name(nom) {
}

EventReceiver::~EventReceiver() {
  platform.kernel()->unsubscribe(this);
  #if defined(__BUILD_HAS_THREADS)
    if (_thread_id > 0) {
      deleteThread(&_thread_id);
    }
  #endif
  Kernel::log(&local_log);  // Clears the local_log.
}


/**
* Call to set the log verbosity for this class. 7 is most verbose. -1 will disable logging altogether.
*
* @param   nu_verbosity  The desired verbosity of this class.
* @return  -1 on failure, and 0 on no change, and 1 on success.
*/
int8_t EventReceiver::setVerbosity(int8_t nu_verbosity) {
  if (getVerbosity() == nu_verbosity) return 0;
  switch (nu_verbosity) {
    case LOG_DEBUG:   /* 7 - debug-level messages */
    case LOG_INFO:    /* 6 - informational */
      #ifdef MANUVR_DEBUG
      local_log.concatf("%s:\tVerbosity set to %d\n", getReceiverName(), nu_verbosity);
      Kernel::log(&local_log);
      #endif
    case LOG_NOTICE:  /* 5 - normal but significant condition */
    case LOG_WARNING: /* 4 - warning conditions */
    case LOG_ERR:     /* 3 - error conditions */
    case LOG_CRIT:    /* 2 - critical conditions */
    case LOG_ALERT:   /* 1 - action must be taken immediately */
    case LOG_EMERG:   /* 0 - system is unusable */
      _class_state = (nu_verbosity & MANUVR_ER_FLAG_VERBOSITY_MASK) | (_class_state & ~MANUVR_ER_FLAG_VERBOSITY_MASK);
      return 1;
    default:
      #ifdef MANUVR_DEBUG
      if (getVerbosity() > 4) {
        local_log.concatf("Illegal verbosity value.\n", getReceiverName(), nu_verbosity);
        Kernel::log(&local_log);
      }
      #endif
      return -1;
  }
}


/**
* If the local_log is not empty, forward the logs to the Kernel.
* This alieviates us of the responsibility of freeing the log.
*/
void EventReceiver::flushLocalLog() {
  if (local_log.length() > 0) Kernel::log(&local_log);
}


/**
* This is a convenience method for posting an event when we want a callback. If there is not
*   already an originator specified, add ourselves as the originator.
*/
int8_t EventReceiver::raiseEvent(ManuvrMsg* event) {
  if (nullptr != event) {
    event->setOriginator(this);
    return Kernel::staticRaiseEvent(event);
  }
  return -1;
}



/* Override for lazy programmers. */
void EventReceiver::printDebug() {
  printDebug(&local_log);
  flushLocalLog();
}


/**
* There is a NULL-check performed upstream for the StringBuilder member. So no need
*   to do it again here.
*/
void EventReceiver::printDebug(StringBuilder *output) {
  #if defined(__BUILD_HAS_THREADS)
  output->concatf("\n==< %s %s  Thread %u >=======\n", getReceiverName(), (erAttached() ? "" : "   (UNATTACHED) "), _thread_id);
  #else
  output->concatf("\n==< %s %s>======================\n", getReceiverName(), (erAttached() ? "" : "   (UNATTACHED) "));
  #endif
}


/*******************************************************************************
* ######## ##     ## ######## ##    ## ########  ######
* ##       ##     ## ##       ###   ##    ##    ##    ##
* ##       ##     ## ##       ####  ##    ##    ##
* ######   ##     ## ######   ## ## ##    ##     ######
* ##        ##   ##  ##       ##  ####    ##          ##
* ##         ## ##   ##       ##   ###    ##    ##    ##
* ########    ###    ######## ##    ##    ##     ######
*
* These are overrides from EventReceiver interface...
*******************************************************************************/

/**
* This is called when the kernel attaches the module.
* This is the first time the class can be expected to have kernel access.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t EventReceiver::attached() {
  if (!erAttached()) {
    _mark_attached();
    return 1;
  }
  return 0;
}


/**
* Events that have a calllback value that is not null will have this fxn called
*   immediately following Event completion.
* Your shadow can bite.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t EventReceiver::erConfigure(Argument*) {
  return 0;
}


/**
* If we find ourselves in this fxn, it means an event that this class built (the argument)
*   has been serviced and we are now getting the chance to see the results. The argument
*   to this fxn will never be NULL.
*
* Depending on class implementations, we might choose to handle the completed Event differently. We
*   might add values to event's Argument chain and return RECYCLE. We may also free() the event
*   ourselves and return DROP. By default, we will return REAP to instruct the Kernel
*   to either free() the event or return it to it's preallocate queue, as appropriate. If the event
*   was crafted to not be in the heap in its own allocation, we will return DROP instead.
*
* @param  event  The event for which service has been completed.
* @return A callback return code.
*/
int8_t EventReceiver::callback_proc(ManuvrMsg* event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = (0 == event->refCount()) ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->eventCode()) {
    default:
      break;
  }

  return return_value;
}


/**
* This is the function that is called to notify this class of an event.
* This particular function is the base notify() method for all downstream
*   inherritance of EventReceiver. Any events that ought to be responded to
*   by all EventReceivers should be cased out here. If an extending class
*   needs to override any of these events, all it needs to do is case off
*   the event in its own notify() method.
*
* @param active_event is the event being broadcast.
* @return the number of actions taken on this event, or -1 on failure.
*/
int8_t EventReceiver::notify(ManuvrMsg* active_event) {
  if (active_event) {
    switch (active_event->eventCode()) {
      case MANUVR_MSG_SYS_BOOT_COMPLETED:
        return attached();
      case MANUVR_MSG_SYS_CONF_LOAD:
        if (active_event->getArgs()) {
          return erConfigure(active_event->getArgs());
        }
        break;
      default:
        break;
    }
  }
  flushLocalLog();
  return 0;
}
