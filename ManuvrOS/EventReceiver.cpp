#include "EventManager.h"
#ifndef TEST_BENCH
  #include "StaticHub/StaticHub.h"
#else
  #include "demo/StaticHub.h"
#endif



/**
* Call to set the log verbosity for this class. 7 is most verbose. -1 will disable logging altogether.
*
* @param   nu_verbosity  The desired verbosity of this class.
* @return  -1 on failure, and 0 on no change, and 1 on success.
*/
int8_t EventReceiver::getVerbosity() {
  return verbosity;
}


/**
* Call to set the log verbosity for this class. 7 is most verbose. -1 will disable logging altogether.
*
* @param   nu_verbosity  The desired verbosity of this class.
* @return  -1 on failure, and 0 on no change, and 1 on success.
*/
int8_t EventReceiver::setVerbosity(int8_t nu_verbosity) {
  if (verbosity == nu_verbosity) return 0;
	switch (nu_verbosity) { 
    case LOG_DEBUG:   /* 7 - debug-level messages */
    case LOG_INFO:    /* 6 - informational */
      local_log.concatf("%s:\tVerbosity set to %d\n", getReceiverName(), nu_verbosity);
      StaticHub::log(&local_log);
    case LOG_NOTICE:  /* 5 - normal but significant condition */
    case LOG_WARNING: /* 4 - warning conditions */
    case LOG_ERR:     /* 3 - error conditions */
    case LOG_CRIT:    /* 2 - critical conditions */
    case LOG_ALERT:   /* 1 - action must be taken immediately */
    case LOG_EMERG:   /* 0 - system is unusable */
      verbosity = nu_verbosity;
      return 1;
    default:
      if (verbosity > 4) {
        local_log.concatf("Illegal verbosity value.\n", getReceiverName(), nu_verbosity);
        StaticHub::log(&local_log);
      }
      return -1;
  }
}


/**
* Call to set the log verbosity for this class. 7 is most verbose. -1 will disable logging altogether.
* This is an override to make code briefer in the notify() methods of classes that extend EventReceiver.
* Warning: Lots of possible return paths.
*
* @param   active_event  An event bearing the code for "set verbosity".
* @return  -1 on failure, and 0 on no change, and 1 on success.
*/
int8_t EventReceiver::setVerbosity(ManuvrEvent* active_event) {
  if (NULL == active_event) return -1;
  if (MANUVR_MSG_SYS_LOG_VERBOSITY != active_event->event_code) return -1;
  switch (active_event->argCount()) {
    case 0:
      local_log.concatf("%s:\tVerbosity is %d\n", getReceiverName(), verbosity);
      StaticHub::log(&local_log);
      return 1;
    case 1:
      {
        int8_t temp_int_8 = 0;
        if (DIG_MSG_ERROR_NO_ERROR != active_event->getArgAs(&temp_int_8)) return -1;
        return setVerbosity(temp_int_8);
      }
  }
  return 0;
}


/**
* This is a base-level debug function that takes direct input from a user.
*
* @param   input  A buffer containing the user's direct input.
*/
void EventReceiver::procDirectDebugInstruction(StringBuilder *input) {
  StaticHub::log("EventReceiver::procDirectDebugInstruction():\t default handler.\n");
}


/**
* This is a convenience method for posting an event when we want a callback. If there is not
*   already a callback specified, add ourselves as the callback.
*/
int8_t EventReceiver::raiseEvent(ManuvrEvent* event) {
  if (event != NULL) {
    event->callback = (EventCallback*) this;
    return EventManager::staticRaiseEvent(event);
  }
  else {
    return -1;
  }
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
int8_t EventReceiver::notify(ManuvrEvent *active_event) {
  switch (active_event->event_code) {
    case MANUVR_MSG_SYS_LOG_VERBOSITY:
      return setVerbosity(active_event);
    case MANUVR_MSG_SYS_BOOT_COMPLETED:
      scheduler = StaticHub::getInstance()->fetchScheduler();
      if (NULL != scheduler) {
        return bootComplete();
      }
  }
  return 0;
}


/* Override for lazy programmers. */
void EventReceiver::printDebug() {
  printDebug(&local_log);
  if (local_log.length() > 0) {    StaticHub::log(&local_log);  }
}


/**
* There is a NULL-check performed upstream for the StringBuilder member. So no need 
*   to do it again here.
*/
void EventReceiver::printDebug(StringBuilder *output) {
  output->concatf("\n==< %s >===================================\n", getReceiverName()); 
}


/**
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t EventReceiver::bootComplete() {
  return 0;
}


