#include "EventManager.h"

#include "StaticHub/StaticHub.h"



/**
* This is here for compatibility with C++ standards that do not allow for definition and declaration
*   in the header file. Takes no parameters, and returns nothing.
*/
void EventReceiver::__class_initializer() {
  scheduler = NULL;
  verbosity = DEFAULT_CLASS_VERBOSITY;
  boot_completed = false;
}



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
      #ifdef __MANUVR_DEBUG
      local_log.concatf("%s:\tVerbosity set to %d\n", getReceiverName(), nu_verbosity);
      StaticHub::log(&local_log);
      #endif
    case LOG_NOTICE:  /* 5 - normal but significant condition */
    case LOG_WARNING: /* 4 - warning conditions */
    case LOG_ERR:     /* 3 - error conditions */
    case LOG_CRIT:    /* 2 - critical conditions */
    case LOG_ALERT:   /* 1 - action must be taken immediately */
    case LOG_EMERG:   /* 0 - system is unusable */
      verbosity = nu_verbosity;
      return 1;
    default:
      #ifdef __MANUVR_DEBUG
      if (verbosity > 4) {
        local_log.concatf("Illegal verbosity value.\n", getReceiverName(), nu_verbosity);
        StaticHub::log(&local_log);
      }
      #endif
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
      #ifdef __MANUVR_DEBUG
      local_log.concatf("%s:\tVerbosity is %d\n", getReceiverName(), verbosity);
      StaticHub::log(&local_log);
      #endif
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


/*
* Returns the number of bytes freed.
*/
int EventReceiver::purgeLogs() {
  int return_value = 0;
  int lll = local_log.length();
  if (lll > 0) {
    if (verbosity > 4) {
      StaticHub::log(&local_log);
    }
    local_log.clear();
    #ifdef __MANUVR_DEBUG
    local_log.concatf("%s GCd %d bytes.\n", getReceiverName(), lll);  // TODO: This never happens.
    StaticHub::log(&local_log);
    #endif
  }
  return return_value;
}



/**
* This is a base-level debug function that takes direct input from a user.
*
* @param   input  A buffer containing the user's direct input.
*/
void EventReceiver::procDirectDebugInstruction(StringBuilder *input) {
#ifdef __MANUVR_CONSOLE_SUPPORT
  char* str = input->position(0);
  
  switch (*(str)) {
    case 'i':    // Print debug
      printDebug(&local_log);
      break;
    default:
      #ifdef __MANUVR_DEBUG
      local_log.concatf("%s: No case in procDirectDebugInstruction().\n", getReceiverName());
      #endif
      break;
  }
  
  if (local_log.length() > 0) {    StaticHub::log(&local_log);  }
#endif
}


/**
* This is a convenience method for posting an event when we want a callback. If there is not
*   already a callback specified, add ourselves as the callback.
*/
int8_t EventReceiver::raiseEvent(ManuvrEvent* event) {
  if (event != NULL) {
    event->callback = (EventReceiver*) this;
    return EventManager::staticRaiseEvent(event);
  }
  else {
    return -1;
  }
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
  output->concatf("--- bootstrap_completed \t %s\n--- scheduler present?  \t %s\n", (boot_completed) ? "yes" : "no", (NULL != scheduler) ? "yes" : "no");
}



/****************************************************************************************************
*  ▄▄▄▄▄▄▄▄▄▄▄  ▄               ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄        ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄ 
* ▐░░░░░░░░░░░▌▐░▌             ▐░▌▐░░░░░░░░░░░▌▐░░▌      ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
* ▐░█▀▀▀▀▀▀▀▀▀  ▐░▌           ▐░▌ ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌░▌     ▐░▌ ▀▀▀▀█░█▀▀▀▀ ▐░█▀▀▀▀▀▀▀▀▀ 
* ▐░▌            ▐░▌         ▐░▌  ▐░▌          ▐░▌▐░▌    ▐░▌     ▐░▌     ▐░▌          
* ▐░█▄▄▄▄▄▄▄▄▄    ▐░▌       ▐░▌   ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌ ▐░▌   ▐░▌     ▐░▌     ▐░█▄▄▄▄▄▄▄▄▄ 
* ▐░░░░░░░░░░░▌    ▐░▌     ▐░▌    ▐░░░░░░░░░░░▌▐░▌  ▐░▌  ▐░▌     ▐░▌     ▐░░░░░░░░░░░▌
* ▐░█▀▀▀▀▀▀▀▀▀      ▐░▌   ▐░▌     ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌   ▐░▌ ▐░▌     ▐░▌      ▀▀▀▀▀▀▀▀▀█░▌
* ▐░▌                ▐░▌ ▐░▌      ▐░▌          ▐░▌    ▐░▌▐░▌     ▐░▌               ▐░▌
* ▐░█▄▄▄▄▄▄▄▄▄        ▐░▐░▌       ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌     ▐░▐░▌     ▐░▌      ▄▄▄▄▄▄▄▄▄█░▌
* ▐░░░░░░░░░░░▌        ▐░▌        ▐░░░░░░░░░░░▌▐░▌      ▐░░▌     ▐░▌     ▐░░░░░░░░░░░▌
*  ▀▀▀▀▀▀▀▀▀▀▀          ▀          ▀▀▀▀▀▀▀▀▀▀▀  ▀        ▀▀       ▀       ▀▀▀▀▀▀▀▀▀▀▀ 
* 
* These are overrides from EventReceiver interface...
****************************************************************************************************/


/**
* Events that have a calllback value that is not null will have this fxn called
*   immediately following Event completion.
* Your shadow can bite.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t EventReceiver::bootComplete() {
  StaticHub *sh = StaticHub::getInstance();
  if (NULL != sh) {
    scheduler = sh->fetchScheduler();
  }
  boot_completed = true;
  return 1;
}


/**
* If we find ourselves in this fxn, it means an event that this class built (the argument)
*   has been serviced and we are now getting the chance to see the results. The argument 
*   to this fxn will never be NULL.
*
* Depending on class implementations, we might choose to handle the completed Event differently. We 
*   might add values to event's Argument chain and return RECYCLE. We may also free() the event
*   ourselves and return DROP. By default, we will return REAP to instruct the EventManager
*   to either free() the event or return it to it's preallocate queue, as appropriate. If the event
*   was crafted to not be in the heap in its own allocation, we will return DROP instead.
*
* @param  event  The event for which service has been completed.
* @return A callback return code.
*/
int8_t EventReceiver::callback_proc(ManuvrEvent *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */ 
  int8_t return_value = event->eventManagerShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;
  
  /* Some class-specific set of conditionals below this line. */
  switch (event->event_code) {
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
int8_t EventReceiver::notify(ManuvrEvent *active_event) {
  switch (active_event->event_code) {
    case MANUVR_MSG_SYS_RELEASE_CRUFT:   // System is telling us to GC if we can.
      return purgeLogs();
    case MANUVR_MSG_SYS_LOG_VERBOSITY:
      return setVerbosity(active_event);
    case MANUVR_MSG_SYS_BOOT_COMPLETED:
      return bootComplete();
  }
  return 0;
}


