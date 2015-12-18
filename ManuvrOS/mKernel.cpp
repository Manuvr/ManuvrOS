#include "FirmwareDefs.h"
#include <ManuvrOS/mKernel.h>
#include <ManuvrOS/Platform/Platform.h>


/****************************************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \    
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |   
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/    
*
* Static members and initializers should be located here. Initializers first, functions second.
****************************************************************************************************/
mKernel* mKernel::INSTANCE = NULL;
PriorityQueue<ManuvrRunnable*> mKernel::isr_event_queue;

const unsigned char MSG_ARGS_EVENTRECEIVER[] = {SYS_EVENTRECEIVER_FM, 0, 0}; 
const unsigned char MSG_ARGS_NO_ARGS[] = {0}; 

const MessageTypeDef message_defs[] = {
  {  MANUVR_MSG_SYS_BOOT_COMPLETED   , 0x0000,               "BOOT_COMPLETED"   , MSG_ARGS_NO_ARGS }, // Raised when bootstrap is finished.

  {  MANUVR_MSG_SYS_ADVERTISE_SRVC   , 0x0000,               "ADVERTISE_SRVC"       , MSG_ARGS_EVENTRECEIVER }, // A system service might feel the need to advertise it's arrival.
  {  MANUVR_MSG_SYS_RETRACT_SRVC     , 0x0000,               "RETRACT_SRVC"         , MSG_ARGS_EVENTRECEIVER }, // A system service sends this to tell others to stop using it.

  {  MANUVR_MSG_SYS_BOOTLOADER       , MSG_FLAG_EXPORTABLE,  "SYS_BOOTLOADER"       , MSG_ARGS_NO_ARGS }, // Reboots into the STM32F4 bootloader.
  {  MANUVR_MSG_SYS_REBOOT           , MSG_FLAG_EXPORTABLE,  "SYS_REBOOT"           , MSG_ARGS_NO_ARGS }, // Reboots into THIS program.
  {  MANUVR_MSG_SYS_SHUTDOWN         , MSG_FLAG_EXPORTABLE,  "SYS_SHUTDOWN"         , MSG_ARGS_NO_ARGS }, // Raised when the system is pending complete shutdown.

  {  MANUVR_MSG_SCHED_ENABLE_BY_PID  , 0x0000,               "SCHED_ENABLE_BY_PID"  , MSG_ARGS_NO_ARGS }, // The given PID is being enabled.
  {  MANUVR_MSG_SCHED_DISABLE_BY_PID , 0x0000,               "SCHED_DISABLE_BY_PID" , MSG_ARGS_NO_ARGS }, // The given PID is being disabled.
  {  MANUVR_MSG_SCHED_PROFILER_START , 0x0000,               "SCHED_PROFILER_START" , MSG_ARGS_NO_ARGS }, // We want to profile the given PID.
  {  MANUVR_MSG_SCHED_PROFILER_STOP  , 0x0000,               "SCHED_PROFILER_STOP"  , MSG_ARGS_NO_ARGS }, // We want to stop profiling the given PID.
  {  MANUVR_MSG_SCHED_PROFILER_DUMP  , 0x0000,               "SCHED_PROFILER_DUMP"  , MSG_ARGS_NO_ARGS }, // Dump the profiler data for all PIDs (no args) or given PIDs.
  {  MANUVR_MSG_SCHED_DUMP_META      , 0x0000,               "SCHED_DUMP_META"      , MSG_ARGS_NO_ARGS }, // Tell the mKernel to dump its meta metrics.
  {  MANUVR_MSG_SCHED_DUMP_SCHEDULES , 0x0000,               "SCHED_DUMP_SCHEDULES" , MSG_ARGS_NO_ARGS }, // Tell the mKernel to dump schedules.
  {  MANUVR_MSG_SCHED_WIPE_PROFILER  , 0x0000,               "SCHED_WIPE_PROFILER"  , MSG_ARGS_NO_ARGS }, // Tell the mKernel to wipe its profiler data. Pass PIDs to be selective.
  {  MANUVR_MSG_SCHED_DEFERRED_EVENT , 0x0000,               "SCHED_DEFERRED_EVENT" , MSG_ARGS_NO_ARGS }, // Tell the mKernel to broadcast the attached Event so many ms into the future.

  #if defined (__MANUVR_FREERTOS) | defined (__MANUVR_LINUX)
    // These are messages that are only present under some sort of threading model. They are meant
    //   to faciliate task hand-off, IPC, and concurrency protection.
    {  MANUVR_MSG_CREATED_THREAD_ID,    0x0000,              "CREATED_THREAD_ID",     ManuvrMsg::MSG_ARGS_U32 }, 
    {  MANUVR_MSG_DESTROYED_THREAD_ID,  0x0000,              "DESTROYED_THREAD_ID",   ManuvrMsg::MSG_ARGS_U32 }, 
    {  MANUVR_MSG_UNBLOCK_THREAD,       0x0000,              "UNBLOCK_THREAD",        ManuvrMsg::MSG_ARGS_U32 }, 
    #if defined (__MANUVR_FREERTOS)
    #endif

    #if defined (__MANUVR_LINUX)
    #endif
  #endif

  /* 
    For messages that have arguments, we have the option of defining inline lables for each parameter.
    This is advantageous for debugging and writing front-ends. We case-off here to make this choice at
    compile time.
  */
  #if defined (__ENABLE_MSG_SEMANTICS)
  #else
  #endif
};


/*
* All external access to mKernel's non-static members should obtain it's reference via this fxn...
*   Note that services that are dependant on us during the bootstrap phase should have a reference
*   passed into their constructors, rather than forcing them to call this and risking an infinite 
*   recursion.
*/
mKernel* mKernel::getInstance() {
  if (INSTANCE == NULL) {
    // This is a valid means of instantiating the kernel. Typically, user code
    //   would have the mKernel on the stack, but if they want to live in the heap, 
    //   that's fine by us. Oblige...
    mKernel::INSTANCE = new mKernel();
  }
  // And that is how the singleton do...
  return (mKernel*) mKernel::INSTANCE;
}



/****************************************************************************************************
*   ___ _              ___      _ _              _      _       
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___ 
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
****************************************************************************************************/

/**
* Vanilla constructor.
*/
mKernel::mKernel() {
  __class_initializer();
  INSTANCE           = this;  // For singleton reference. TODO: Will not parallelize.
  __kernel           = this;  // We extend EventReceiver. So we populate this.

  max_queue_depth     = 0;
  total_loops         = 0;
  total_events        = 0;
  total_events_dead   = 0;
  micros_occupied     = 0;
  max_events_per_loop = 2;

  current_event      = NULL;

  // TODO: Pulled in from Scheduler.
  next_pid             = 1;      // Next PID to assign.
  currently_executing  = 0;      // Hold PID of currently-executing Schedule. 0 if none.
  productive_loops     = 0;      // Number of calls to serviceScheduledEvents() that actually called a schedule.
  total_loops          = 0;      // Number of calls to serviceScheduledEvents().
  overhead             = 0;      // The time in microseconds required to service the last empty schedule loop.
  scheduler_ready      = false;  // TODO: Convert to a uint8 and track states.
  skipped_loops        = 0;
  total_skipped_loops  = 0;
  lagged_schedules     = 0;
  bistable_skip_detect = false;  // Set in advanceScheduler(), cleared in serviceScheduledEvents().
  clicks_in_isr        = 0;
  _ms_elapsed          = 0;
  // TODO: Pulled in from Scheduler.
  
  
  
  
  for (int i = 0; i < EVENT_MANAGER_PREALLOC_COUNT; i++) {
    /* We carved out a space in our allocation for a pool of events. Ideally, this would be enough
        for most of the load, most of the time. If the preallocation ends up being insufficient to
        meet demand, new Events will be created on the heap. But the events that we are dealing
        with here are special. They should remain circulating (or in standby) for the lifetime of 
        this class. */
    _preallocation_pool[i].returnToPrealloc(true);
    preallocated.insert(&_preallocation_pool[i]);
  }

  subscribe(this);           // We subscribe ourselves to events.
  setVerbosity((int8_t) 1);  // TODO: Do this to move verbosity from 0 to some default level.
  profiler(false);           // Turn off the profiler.

  ManuvrMsg::registerMessages(message_defs, sizeof(message_defs) / sizeof(MessageTypeDef));

  platformPreInit();    // Start the pre-bootstrap platform-specific machinery.
}

/**
* Destructor. Should probably never be called.
*/
mKernel::~mKernel() {
  destroyAllScheduleItems();
}



/****************************************************************************************************
* Logging members...                                                                                *
****************************************************************************************************/
StringBuilder mKernel::log_buffer;

#if defined (__MANUVR_FREERTOS)
  uint32_t mKernel::logger_pid = 0;
  uint32_t mKernel::kernel_pid = 0;
#endif

/*
* Logger pass-through functions. Please mind the variadics...
*/
volatile void mKernel::log(int severity, const char *str) {
  if (!INSTANCE->verbosity) return;
  log_buffer.concat(str);
  #if defined (__MANUVR_FREERTOS)
    unblockThread(logger_pid);
  #endif
}

volatile void mKernel::log(char *str) {
  if (!INSTANCE->verbosity) return;
  log_buffer.concat(str);
  #if defined (__MANUVR_FREERTOS)
    unblockThread(logger_pid);
  #endif
}

volatile void mKernel::log(const char *str) {
  if (!INSTANCE->verbosity) return;
  log_buffer.concat(str);
  #if defined (__MANUVR_FREERTOS)
    unblockThread(logger_pid);
  #endif
}

volatile void mKernel::log(const char *fxn_name, int severity, const char *str, ...) {
  if (!INSTANCE->verbosity) return;
  log_buffer.concatf("%d  %s:\t", severity, fxn_name);
  va_list marker;
  
  va_start(marker, str);
  log_buffer.concatf(str, marker);
  va_end(marker);
  #if defined (__MANUVR_FREERTOS)
    unblockThread(logger_pid);
  #endif
}

volatile void mKernel::log(StringBuilder *str) {
  if (!INSTANCE->verbosity) return;
  log_buffer.concatHandoff(str);
  #if defined (__MANUVR_FREERTOS)
    unblockThread(logger_pid);
  #endif
}




/****************************************************************************************************
* mKernel operation...                                                                               *
****************************************************************************************************/
int8_t mKernel::bootstrap() {
  platformInit();    // Start the platform-specific machinery.

  ManuvrRunnable *boot_completed_ev = mKernel::returnEvent(MANUVR_MSG_SYS_BOOT_COMPLETED);
  boot_completed_ev->priority = EVENT_PRIORITY_HIGHEST;
  raiseEvent(MANUVR_MSG_SYS_BOOT_COMPLETED, NULL);
  return 0;
}


/*
* This is the means by which we feed raw string input from a console into the 
*   kernel's user input slot.
*/
void mKernel::accumulateConsoleInput(uint8_t *buf, int len, bool terminal) {
  last_user_input.concat(buf, len);

  if (terminal) {
    // If the ISR saw a CR or LF on the wire, we tell the parser it is ok to
    // run in idle time.
    ManuvrRunnable* event = returnEvent(MANUVR_MSG_USER_DEBUG_INPUT);
    event->specific_target = (EventReceiver*) this;
    mKernel::staticRaiseEvent(event);
  }
}



/****************************************************************************************************
* Subscriptions and client management fxns.                                                         *
****************************************************************************************************/

/**
* A class calls this to subscribe to events. After calling this, the class will have its notify()
*   member called for each event. In this manner, we give instantiated EventReceivers the ability
*   to listen to events.
*
* @param  client  The class that will be listening for Events.
* @return 0 on success and -1 on failure.
*/
int8_t mKernel::subscribe(EventReceiver *client) {
  if (NULL == client) return -1;

  client->setVerbosity(DEFAULT_CLASS_VERBOSITY);
  int8_t return_value = subscribers.insert(client);
  if (boot_completed) {
    // This subscriber is joining us after bootup. Call its bootComplete() fxn to cause it to init.
    client->notify(returnEvent(MANUVR_MSG_SYS_BOOT_COMPLETED));
  }
  return ((return_value >= 0) ? 0 : -1);
}


/**
* A class calls this to subscribe to events with a given absolute priority.
* A class might want to specify a priority such that it can monitor broadcasts
*   before the intended class receives them.
* A class might also want high priority because it knows that it will be receiving
*   many events, and having it near the head of the list minimizes waste.
*
* @param  client    The class that will be listening for Events.
* @param  priority  The priority of the client in the Event queue.
* @return 0 on success and -1 on failure.
*/
int8_t mKernel::subscribe(EventReceiver *client, uint8_t priority) {
  if (NULL == client) return -1;

  client->setVerbosity(DEFAULT_CLASS_VERBOSITY);
  int8_t return_value = subscribers.insert(client, priority);
  if (boot_completed) {
    // This subscriber is joining us after bootup. Call its bootComplete() fxn to cause it to init.
    //client.bootComplete();
  }
  return ((return_value >= 0) ? 0 : -1);
}


/**
* A class calls this to unsubscribe.
*
* @param  client    The class that will no longer be listening for Events.
* @return 0 on success and -1 on failure.
*/
int8_t mKernel::unsubscribe(EventReceiver *client) {
  if (NULL == client) return -1;
  return (subscribers.remove(client) ? 0 : -1);
}


EventReceiver* mKernel::getSubscriberByName(const char* search_str) {
  EventReceiver* working;
  for (int i = 0; i < subscribers.size(); i++) {
    working = subscribers.get(i);
    if (!strcasecmp(working->getReceiverName(), search_str)) {
      return working;
    }
  }
  return NULL;
}


/****************************************************************************************************
* Static member functions for posting events.                                                       *
****************************************************************************************************/


/**
* Used to add an event to the idle queue. Use this for simple events that don't need args.
*
* @param  code  The identity code of the event to be raised.
* @param  cb    An optional callback pointer to be called when this event is finished.
* @return -1 on failure, and 0 on success.
*/
int8_t mKernel::raiseEvent(uint16_t code, EventReceiver* cb) {
  int8_t return_value = 0;
  
  // We are creating a new Event. Try to snatch a prealloc'd one and fall back to malloc if needed.
  ManuvrRunnable* nu = INSTANCE->preallocated.dequeue();
  if (nu == NULL) {
    INSTANCE->prealloc_starved++;
    nu = new ManuvrRunnable(code, cb);
  }
  else {
    nu->repurpose(code);
    nu->callback = cb;
  }

  if (0 == INSTANCE->validate_insertion(nu)) {
    INSTANCE->event_queue.insert(nu);
    INSTANCE->update_maximum_queue_depth();   // Check the queue depth
    #if defined (__MANUVR_FREERTOS)
      unblockThread(kernel_pid);
    #endif
  }
  else {
    if (INSTANCE->verbosity > 4) {
      #ifdef __MANUVR_DEBUG
      StringBuilder output("raiseEvent():\tvalidate_insertion() failed:\n");
      output.concat(ManuvrMsg::getMsgTypeString(code));
      mKernel::log(&output);
      #endif
      INSTANCE->insertion_denials++;
    }
    INSTANCE->reclaim_event(nu);
    return_value = -1;
  }
  return return_value;
}



/**
* Used to add a pre-formed event to the idle queue. Use this when a sophisticated event
*   needs to be formed elsewhere and passed in. mKernel will only insert it into the
*   queue in this case.
*
* @param   event  The event to be inserted into the idle queue.
* @return  -1 on failure, and 0 on success.
*/
int8_t mKernel::staticRaiseEvent(ManuvrRunnable* event) {
  int8_t return_value = 0;
  if (0 == INSTANCE->validate_insertion(event)) {
    INSTANCE->event_queue.insert(event, event->priority);
    INSTANCE->update_maximum_queue_depth();   // Check the queue depth
    #if defined (__MANUVR_FREERTOS)
      unblockThread(kernel_pid);
    #endif
  }
  else {
    if (INSTANCE->verbosity > 4) {
      #ifdef __MANUVR_DEBUG
      StringBuilder output("staticRaiseEvent():\tvalidate_insertion() failed:\n");
      event->printDebug(&output);;
      mKernel::log(&output);
      #endif
      INSTANCE->insertion_denials++;
    }
    INSTANCE->reclaim_event(event);
    return_value = -1;
  }
  return return_value;
}


/**
* Used to add a pre-formed event to the idle queue. Use this when a sophisticated event
*   needs to be formed elsewhere and passed in. mKernel will only insert it into the
*   queue in this case.
*
* @param   event  The event to be removed from the idle queue.
* @return  true if the given event was aborted, false otherwise.
*/
bool mKernel::abortEvent(ManuvrRunnable* event) {
  if (!INSTANCE->event_queue.remove(event)) {
    // Didn't find it? Check  the isr_queue...
    if (!INSTANCE->isr_event_queue.remove(event)) {
      return false;
    }
  }
  return true;
}


// TODO: It would be bettter to put a semaphore on the event_queue and set it in the idle loop.
//       That way, we could check for it here, and have the (probable) possibility of not incurring
//       the cost for merging these two queues if we don't have to.
//             ---J. Ian Lindsay   Fri Jul 03 16:54:14 MST 2015
int8_t mKernel::isrRaiseEvent(ManuvrRunnable* event) {
  int return_value = -1;
  maskableInterrupts(false);
  return_value = isr_event_queue.insertIfAbsent(event, event->priority);
  maskableInterrupts(true);
  #if defined (__MANUVR_FREERTOS)
    unblockThread(kernel_pid);
  #endif
  return return_value;
}




/**
* Factory method. Returns a preallocated Event.
* After we return the event, we lose track of it. So if the caller doesn't ever
*   call raiseEvent(), the Event we return will become a memory leak.
* The event we retun will have a callback field populated with a ref to mKernel.
*   So if a caller needs their own reference in that slot, caller will need to do it.
*
* @param  code  The desired identity code of the event.
* @return A pointer to the prepared event. Will not return NULL unless we are out of memory.
*/
ManuvrRunnable* mKernel::returnEvent(uint16_t code) {
  // We are creating a new Event. Try to snatch a prealloc'd one and fall back to malloc if needed.
  ManuvrRunnable* return_value = INSTANCE->preallocated.dequeue();
  if (return_value == NULL) {
    INSTANCE->prealloc_starved++;
    return_value = new ManuvrRunnable(code, (EventReceiver*) INSTANCE);
  }
  else {
    return_value->repurpose(code);
    return_value->callback = (EventReceiver*) INSTANCE;
  }
  return return_value;
}




/**
* This is the code that checks an incoming event for validity prior to inserting it
*   into the event_queue. Checks for grammatical validity and idempotency should go
*   here.
*
* @param event The inbound event that we need to evaluate.
* @return 0 if the event is good-to-go. Otherwise, an appropriate failure code.
*/
int8_t mKernel::validate_insertion(ManuvrRunnable* event) {
  if (NULL == event) return -1;                                   // No NULL events.
  if (MANUVR_MSG_UNDEFINED == event->event_code) {
    return -2;  // No undefined events.
  }

  if (event_queue.contains(event)) {
    // Bail out with error, because this event (which is status-bearing) cannot be in the
    //   queue more than once.
    return -3;
  }
  
  // Those are the basic checks. Now for the advanced functionality...
  if (event->isIdempotent()) {
    for (int i = 0; i < event_queue.size(); i++) {
      // No duplicate idempotent events allowed...
      if (event_queue.get(i)->event_code == event->event_code) {
        idempotent_blocks++;
        return -3;
      }
    }
  }
  return 0;
}




/**
* This is where events go to die. This function should inspect the Event and send it
*   to the appropriate place.
*
* @param active_event The event that has reached the end of its life-cycle.
*/
void mKernel::reclaim_event(ManuvrRunnable* active_event) {
  if (NULL == active_event) {
    return;
  }
  bool reap_current_event = active_event->eventManagerShouldReap();
  //if (verbosity > 5) {
  //  local_log.concatf("We will%s be reaping %s.\n", (reap_current_event ? "":" not"), active_event->getMsgTypeString());
  //  mKernel::log(&local_log);
  //}

  if (reap_current_event) {                   // If we are to reap this event...
    delete active_event;                      // ...free() it...
    events_destroyed++;
    burden_of_specific++;                     // ...and note the incident.
  }
  else {                                      // If we are NOT to reap this event...
    if (active_event->isManaged()) {
    }
    else if (active_event->returnToPrealloc()) {   // ...is it because we preallocated it?
      active_event->clearArgs();              // If so, wipe the Event...
      preallocated.insert(active_event);      // ...and return it to the preallocate queue.
    }                                         // Otherwise, we let it drop and trust some other class is managing it.
    //else {
    //  if (verbosity > 6) local_log.concat("mKernel::reclaim_event(): Doing nothing. Hope its managed elsewhere.\n");
    //}
  }
  
  if (local_log.length() > 0) {    mKernel::log(&local_log);  }
}




// This is the splice into v2's style of event handling (callaheads).
int8_t mKernel::procCallAheads(ManuvrRunnable *active_event) {
  int8_t return_value = 0;
  PriorityQueue<listenerFxnPtr> *ca_queue = ca_listeners[active_event->event_code];
  if (NULL != ca_queue) {
    listenerFxnPtr current_fxn;
    for (int i = 0; i < ca_queue->size(); i++) {
      current_fxn = ca_queue->recycle();  // TODO: This is ugly for many reasons.
      if (current_fxn(active_event)) {
        return_value++;
      }
    }
  }
  return return_value;
}

// This is the splice into v2's style of event handling (callbacks).
int8_t mKernel::procCallBacks(ManuvrRunnable *active_event) {
  int8_t return_value = 0;
  PriorityQueue<listenerFxnPtr> *cb_queue = cb_listeners[active_event->event_code];
  if (NULL != cb_queue) {
    listenerFxnPtr current_fxn;
    for (int i = 0; i < cb_queue->size(); i++) {
      current_fxn = cb_queue->recycle();  // TODO: This is ugly for many reasons.
      if (current_fxn(active_event)) {
        return_value++;
      }
    }
  }
  return return_value;
}




/**
* Process any open events.
*
* Definition of "well-defined behavior" for this fxn....
* - This fxn is essentially the notify() fxn that is ALWAYS first to proc.
* - This fxn should somehow self-limit the number of Events that it procs for any single
*     call. We don't want to cause bad re-entrancy problem in the mKernel by spending
*     all of our time here (although we might re-work the mKernel to make this acceptable).
*
* @return the number of events processed, or a negative value on some failure.
*/
int8_t mKernel::procIdleFlags() {
  uint32_t profiler_mark   = micros();
  uint32_t profiler_mark_0 = 0;   // Profiling requests...
  uint32_t profiler_mark_1 = 0;   // Profiling requests...
  uint32_t profiler_mark_2 = 0;   // Profiling requests...
  uint32_t profiler_mark_3 = 0;   // Profiling requests...
  int8_t return_value      = 0;   // Number of Events we've processed this call.
  uint16_t msg_code_local  = 0;   

  ManuvrRunnable *active_event = NULL;  // Our short-term focus.
  uint8_t activity_count    = 0;     // Incremented whenever a subscriber reacts to an event.

  globalIRQDisable();
  while (isr_event_queue.size() > 0) {
    active_event = isr_event_queue.dequeue();

    if (0 == validate_insertion(active_event)) {
      event_queue.insertIfAbsent(active_event, active_event->priority);
    }
    else reclaim_event(active_event);
  }
  globalIRQEnable();
    
  active_event = NULL;   // Pedantic...
  
  /* As long as we have an open event and we aren't yet at our proc ceiling... */
  while (event_queue.hasNext() && should_run_another_event(return_value, profiler_mark)) {
    active_event = event_queue.dequeue();       // Grab the Event and remove it in the same call.
    msg_code_local = active_event->event_code;  // This gets used after the life of the event.
    
    current_event = active_event;
    
    // Chat and measure.
    #ifdef __MANUVR_DEBUG
    if (verbosity >= 5) local_log.concatf("Servicing: %s\n", active_event->getMsgTypeString());
    #endif
    if (profiler_enabled) profiler_mark_0 = micros();

    procCallAheads(active_event);
    
    // Now we start notify()'ing subscribers.
    EventReceiver *subscriber;   // No need to assign.
    if (profiler_enabled) profiler_mark_1 = micros();
    
    if (NULL != active_event->specific_target) {
      subscriber = active_event->specific_target;
      switch (subscriber->notify(active_event)) {
        case 0:   // The nominal case. No response.
          break;
        case -1:  // The subscriber choked. Figure out why. Technically, this is action. Case fall-through...
          subscriber->printDebug(&local_log);
        default:   // The subscriber acted.
          activity_count++;
          if (profiler_enabled) profiler_mark_2 = micros();
          break;
      }
    }
    else {
      for (int i = 0; i < subscribers.size(); i++) {
        subscriber = subscribers.get(i);
        
        switch (subscriber->notify(active_event)) {
          case 0:   // The nominal case. No response.
            break;
          case -1:  // The subscriber choked. Figure out why. Technically, this is action. Case fall-through...
            subscriber->printDebug(&local_log);
          default:   // The subscriber acted.
            activity_count++;
            if (profiler_enabled) profiler_mark_2 = micros();
            break;
        }
      }
    }
    
    procCallBacks(active_event);
    
    if (0 == activity_count) {
      #ifdef __MANUVR_DEBUG
      if (verbosity >= 3) local_log.concatf("\tDead event: %s\n", active_event->getMsgTypeString());
      #endif
      total_events_dead++;
    }

    /* Should we clean up the Event? */
    bool clean_up_active_event = true;  // Defaults to 'yes'.
    if (NULL != active_event->callback) {
      if ((EventReceiver*) this != active_event->callback) {  // We don't want to invoke our own callback.
        /* If the event has a valid callback, do the callback dance and take instruction
           from the return value. */
        //   if (verbosity >=7) output.concatf("specific_event_callback returns %d\n", active_event->callback->callback_proc(active_event));
        switch (active_event->callback->callback_proc(active_event)) {
          case EVENT_CALLBACK_RETURN_RECYCLE:     // The originating class wants us to re-insert the event.
            #ifdef __MANUVR_DEBUG
            if (verbosity > 5) local_log.concatf("Recycling %s.\n", active_event->getMsgTypeString());
            #endif
            if (0 == validate_insertion(active_event)) {
              event_queue.insert(active_event, active_event->priority);
              // This is the one case where we do NOT want the event reclaimed.
              clean_up_active_event = false;
            }
            break;
          case EVENT_CALLBACK_RETURN_ERROR:       // Something went wrong. Should never occur.
          case EVENT_CALLBACK_RETURN_UNDEFINED:   // The originating class doesn't care what we do with the event.
            //if (verbosity > 1) local_log.concatf("mKernel found a possible mistake. Unexpected return case from callback_proc.\n");
            // NOTE: No break;
          case EVENT_CALLBACK_RETURN_DROP:        // The originating class expects us to drop the event.
            #ifdef __MANUVR_DEBUG
            if (verbosity > 5) local_log.concatf("Dropping %s after running.\n", active_event->getMsgTypeString());
            #endif
            // NOTE: No break;
          case EVENT_CALLBACK_RETURN_REAP:        // The originating class is explicitly telling us to reap the event.
            // NOTE: No break;
          default:
            //if (verbosity > 0) local_log.concatf("Event %s has no cleanup case.\n", active_event->getMsgTypeString());
            break;
        }
      }
      else {
        //reclaim_event(active_event);
      }
    }
    else {
      /* If there is no callback specified for the Event, we rely on the flags in the Event itself to
         decide if it should be reaped. If its memory is being managed by some other class, the reclaim_event()
         fxn will simply remove it from the event_queue and consider the matter closed. */
    }
    
    // All of the logic above ultimately informs this choice.
    if (clean_up_active_event) {
      reclaim_event(active_event);
    }

    total_events++;
    
    // This is a stat-gathering block.
    if (profiler_enabled) {
      
      profiler_mark_3 = micros();

      MessageTypeDef* tmp_msg_def = (MessageTypeDef*) ManuvrMsg::lookupMsgDefByCode(msg_code_local);
      
      TaskProfilerData* profiler_item = NULL;
      int cost_size = event_costs.size();
      int i = 0;
      while ((NULL == profiler_item) && (i < cost_size)) {
        if (event_costs.get(i)->msg_code == msg_code_local) {
          profiler_item = event_costs.get(i);
        }
        i++;                       
      }
      if (NULL == profiler_item) {
        // If we don't yet have a profiler item for this message type...
        profiler_item = new TaskProfilerData();    // ...create one...
        profiler_item->msg_code = msg_code_local;   // ...assign the code...
        event_costs.insert(profiler_item, 1);       // ...and insert it for the future.
      }
      else {
        // If we already have a profiler item for this code...
        event_costs.incrementPriority(profiler_item);
      }
      profiler_item->executions++;
      profiler_item->run_time_last    = max((unsigned long) profiler_mark_2, (unsigned long) profiler_mark_1) - min((unsigned long) profiler_mark_2, (unsigned long) profiler_mark_1);
      profiler_item->run_time_best    = min((unsigned long) profiler_item->run_time_best, (unsigned long) profiler_item->run_time_last);
      profiler_item->run_time_worst   = max((unsigned long) profiler_item->run_time_worst, (unsigned long) profiler_item->run_time_last);
      profiler_item->run_time_total  += profiler_item->run_time_last;
      profiler_item->run_time_average = profiler_item->run_time_total / ((profiler_item->executions) ? profiler_item->executions : 1);

      #ifdef __MANUVR_DEBUG
      if (verbosity >= 6) local_log.concatf("%s finished.\n\tTotal time: %ld uS\n", tmp_msg_def->debug_label, (profiler_mark_3 - profiler_mark_0));
      if (profiler_mark_2) {
        if (verbosity >= 6) local_log.concatf("\tTook %ld uS to notify.\n", profiler_item->run_time_last);
      }
      #endif
      profiler_mark_2 = 0;  // Reset for next iteration.
    }

    if (event_queue.size() > 30) {
      #ifdef __MANUVR_DEBUG
      local_log.concatf("Depth %10d \t %s\n", event_queue.size(), ManuvrMsg::getMsgTypeString(msg_code_local));
      #endif
    }

    return_value++;   // We just serviced an Event.
  }
  
  total_loops++;
  profiler_mark_3 = micros();
  uint32_t runtime_this_loop = max((uint32_t) profiler_mark_3, (uint32_t) profiler_mark) - min((uint32_t) profiler_mark_3, (uint32_t) profiler_mark);
  if (return_value > 0) {
    micros_occupied += runtime_this_loop;
    max_events_p_loop = max((uint32_t) max_events_p_loop, 0x0000007F & (uint32_t) return_value);
  }
  else if (0 == return_value) {
    max_idle_loop_time = max((uint32_t) max_idle_loop_time, (max((uint32_t) profiler_mark, (uint32_t) profiler_mark_3) - min((uint32_t) profiler_mark, (uint32_t) profiler_mark_3)));
  }
  else {
    // there was a problem. Do nothing.
  }

  if (local_log.length() > 0) mKernel::log(&local_log);
  current_event = NULL;
  return return_value;
}


int8_t mKernel::registerCallbacks(uint16_t msgCode, listenerFxnPtr ca, listenerFxnPtr cb, uint32_t options) {
  if (ca != NULL) {
    PriorityQueue<listenerFxnPtr> *ca_queue = ca_listeners[msgCode];
    if (NULL == ca_queue) {
      ca_queue = new PriorityQueue<listenerFxnPtr>();
      ca_listeners[msgCode] = ca_queue;
    }
    ca_queue->insert(ca);
  }
  
  if (cb != NULL) {
    PriorityQueue<listenerFxnPtr> *cb_queue = cb_listeners[msgCode];
    if (NULL == cb_queue) {
      cb_queue = new PriorityQueue<listenerFxnPtr>();
      cb_listeners[msgCode] = cb_queue;
    }
    cb_queue->insert(cb);
  }
  return options%255;
}


/**
* Turns the profiler on or off. Clears its collected stats regardless.
*
* @param   enabled  If true, enables the profiler. If false, disables it.
*/
void mKernel::profiler(bool enabled) {
  profiler_enabled   = enabled;
  max_idle_loop_time = 0;
  max_events_p_loop  = 0;

  max_idle_loop_time = 0;
  events_destroyed   = 0;  
  prealloc_starved   = 0;
  burden_of_specific = 0;
  idempotent_blocks  = 0; 
  insertion_denials  = 0;   

  while (event_costs.hasNext()) delete event_costs.dequeue();
}


float mKernel::cpu_usage() {
  return (micros_occupied / (float)(millis()*10));
}



/****************************************************************************************************
*  ▄▄▄▄▄▄▄▄▄▄   ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄   ▄         ▄  ▄▄▄▄▄▄▄▄▄▄▄ 
* ▐░░░░░░░░░░▌ ▐░░░░░░░░░░░▌▐░░░░░░░░░░▌ ▐░▌       ▐░▌▐░░░░░░░░░░░▌
* ▐░█▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀█░▌▐░▌       ▐░▌▐░█▀▀▀▀▀▀▀▀▀ 
* ▐░▌       ▐░▌▐░▌          ▐░▌       ▐░▌▐░▌       ▐░▌▐░▌          
* ▐░▌       ▐░▌▐░█▄▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄█░▌▐░▌       ▐░▌▐░▌ ▄▄▄▄▄▄▄▄ 
* ▐░▌       ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░▌ ▐░▌       ▐░▌▐░▌▐░░░░░░░░▌
* ▐░▌       ▐░▌▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀█░▌▐░▌       ▐░▌▐░▌ ▀▀▀▀▀▀█░▌
* ▐░▌       ▐░▌▐░▌          ▐░▌       ▐░▌▐░▌       ▐░▌▐░▌       ▐░▌
* ▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄█░▌
* ▐░░░░░░░░░░▌ ▐░░░░░░░░░░░▌▐░░░░░░░░░░▌ ▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
*  ▀▀▀▀▀▀▀▀▀▀   ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀   ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀▀ 
****************************************************************************************************/

#if defined(__MANUVR_DEBUG)
/**
* Print the type sizes to the kernel log.
*
* @param   StringBuilder*  The buffer that this fxn will write output into.
*/
void mKernel::print_type_sizes() {
  StringBuilder temp("---< Type sizes >-----------------------------\n");
  temp.concatf("Elemental data structures:\n");
  temp.concatf("\t StringBuilder         %d\n", sizeof(StringBuilder));
  temp.concatf("\t LinkedList<void*>     %d\n", sizeof(LinkedList<void*>));
  temp.concatf("\t PriorityQueue<void*>  %d\n", sizeof(PriorityQueue<void*>));

  temp.concatf(" Core singletons:\n");
  temp.concatf("\t mKernel                %d\n", sizeof(mKernel));
  temp.concatf("\t   mKernel           %d\n", sizeof(mKernel));

  temp.concatf(" Messaging components:\n");
  temp.concatf("\t ManuvrRunnable           %d\n", sizeof(ManuvrRunnable));
  temp.concatf("\t ManuvrMsg             %d\n", sizeof(ManuvrMsg));
  temp.concatf("\t Argument              %d\n", sizeof(Argument));
  temp.concatf("\t mKernelItem         %d\n", sizeof(ManuvrRunnable));
  temp.concatf("\t TaskProfilerData      %d\n", sizeof(TaskProfilerData));

  mKernel::log(&temp);
}
#endif


/**
* Print the profiler to the provided buffer.
*
* @param   StringBuilder*  The buffer that this fxn will write output into.
*/
void mKernel::printProfiler(StringBuilder* output) {
  if (NULL == output) return;
  output->concatf("\t total_events       \t%u\n",   (unsigned long) total_events);
  output->concatf("\t total_events_dead  \t%u\n\n", (unsigned long) total_events_dead);
  output->concatf("\t max_queue_depth    \t%u\n",   (unsigned long) max_queue_depth);
  output->concatf("\t total_loops        \t%u\n",   (unsigned long) total_loops);
  output->concatf("\t max_idle_loop_time \t%u\n", (unsigned long) max_idle_loop_time);
  output->concatf("\t max_events_p_loop  \t%u\n", (unsigned long) max_events_p_loop);
    
  if (profiler_enabled) {
    output->concat("-- Profiler:\n");
    if (total_events) {
      output->concatf("\tprealloc hit fraction: %f\%\n\n", (1-((burden_of_specific - prealloc_starved) / total_events)) * 100);
    }

    TaskProfilerData *profiler_item;
    int stat_mode = event_costs.getPriority(0);
    int x = event_costs.size();

    TaskProfilerData::printDebugHeader(output);
    for (int i = 0; i < x; i++) {
      profiler_item = event_costs.get(i);
      stat_mode     = event_costs.getPriority(i);
      output->concatf("\t (%10d)\t", stat_mode);
      profiler_item->printDebug(output);
    }
    output->concatf("\n\t CPU use by clock: %f\n\n", (double)cpu_usage());
  }
  else {
    output->concat("-- mKernel profiler disabled.\n\n");
  }


  bool header_unprinted = true;
  if (schedules.size() > 0) {
    ManuvrRunnable *current;

    StringBuilder active_str;
    StringBuilder secondary_str;
    for (int i = 0; i < schedules.size(); i++) {
      current = schedules.get(i);
      if (current->isProfiling()) {
        TaskProfilerData *prof_data = current->prof_data;
        if (header_unprinted) {
          header_unprinted = false;
          output->concat("\t PID         Execd      total us   average    worst      best       last\n\t -----------------------------------------------------------------------------\n");
        }
        if (current->threadEnabled()) {
          active_str.concatf("\t %10u  %9d  %9d  %9d  %9d  %9d  %9d\n", current->pid, prof_data->executions, prof_data->run_time_total, prof_data->run_time_average, prof_data->run_time_worst, prof_data->run_time_best, prof_data->run_time_last);
        }
        else {
          secondary_str.concatf("\t %10u  %9d  %9d  %9d  %9d  %9d  %9d  (INACTIVE)\n", current->pid, prof_data->executions, prof_data->run_time_total, prof_data->run_time_average, prof_data->run_time_worst, prof_data->run_time_best, prof_data->run_time_last);
        }
        
        output->concatHandoff(&active_str);
        output->concatHandoff(&secondary_str);
      }
    }
  }
  
  if (header_unprinted) {
    output->concat("Nothing being profiled.\n");
  }
}


/**
* Debug support function.
*
* @return a pointer to a string constant.
*/
const char* mKernel::getReceiverName() {  return "mKernel";  }


/**
* Debug support method. This fxn is only present in debug builds. 
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void mKernel::printDebug(StringBuilder* output) {
  if (NULL == output) return;
  uint32_t initial_sp = getStackPointer();
  uint32_t final_sp = getStackPointer();

  EventReceiver::printDebug(output);
  
  currentDateTime(output);
  output->concatf("\n-- %s v%s    Build date: %s %s\n--\n", IDENTITY_STRING, VERSION_STRING, __DATE__, __TIME__);
  output->concatf("-- our_mem_addr:             0x%08x\n", (uint32_t) this);
  if (verbosity > 5) output->concatf("-- boot_completed:           %s\n", (boot_completed) ? "yes" : "no");
  if (verbosity > 6) output->concatf("-- getStackPointer()         0x%08x\n", getStackPointer());
  if (verbosity > 6) output->concatf("-- stack grows %s\n--\n", (final_sp > initial_sp) ? "up" : "down");
  if (verbosity > 6) output->concatf("-- millis()                  0x%08x\n", millis());
  if (verbosity > 6) output->concatf("-- micros()                  0x%08x\n", micros());

  output->concatf("-- Queue depth:              %d\n", event_queue.size());
  output->concatf("-- Preallocation depth:      %d\n", preallocated.size());
  output->concatf("-- Total subscriber count:   %d\n", subscribers.size());
  output->concatf("-- Prealloc starves:         %u\n",   (unsigned long) prealloc_starved);
  //output->concatf("-- events_destroyed:         %u\n",   (unsigned long) events_destroyed);
  output->concatf("-- burden_of_being_specific  %u\n",   (unsigned long) burden_of_specific);
  output->concatf("-- idempotent_blocks         %u\n\n", (unsigned long) idempotent_blocks);
  
  if (subscribers.size() > 0) {
    output->concatf("-- Subscribers: (%d total):\n", subscribers.size());
    for (int i = 0; i < subscribers.size(); i++) {
      output->concatf("\t %d: %s\n", i, subscribers.get(i)->getReceiverName());
    }
    output->concat("\n");
  }
  
  if (NULL != current_event) {
    current_event->printDebug(output);
  }

  output->concatf("--- Schedules location:  0x%08x\n", &schedules);
  output->concatf("--- Total loops:      %u\n--- Productive loops: %u\n---Skipped loops: %u\n---Lagged schedules %u\n", (unsigned long) total_loops, (unsigned long) productive_loops, (unsigned long) total_skipped_loops, (unsigned long) lagged_schedules);
  if (total_loops) output->concatf("--- Duty cycle:       %2.4f%\n--- Overhead:         %d microseconds\n", ((double)((double) productive_loops / (double) total_loops) * 100), overhead);
  output->concatf("--- Next PID:         %u\n--- Total schedules:  %d\n--- Active schedules: %d\n\n", peekNextPID(), getTotalSchedules(), getActiveSchedules());

  printProfiler(output);
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
* Boot done finished-up. 
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t mKernel::bootComplete() {
  EventReceiver::bootComplete();
  boot_completed = true;
  scheduler_ready = true;
  maskableInterrupts(true);  // Now configure interrupts, lift interrupt masks, and let the madness begin.
  return 1;
}


/**
* If we find ourselves in this fxn, it means an event that this class built (the argument)
*   has been serviced and we are now getting the chance to see the results. The argument 
*   to this fxn will never be NULL.
*
* Depending on class implementations, we might choose to handle the completed Event differently. We 
*   might add values to event's Argument chain and return RECYCLE. We may also free() the event
*   ourselves and return DROP. By default, we will return REAP to instruct the mKernel
*   to either free() the event or return it to it's preallocate queue, as appropriate. If the event
*   was crafted to not be in the heap in its own allocation, we will return DROP instead.
*
* @param  event  The event for which service has been completed.
* @return A callback return code.
*/
int8_t mKernel::callback_proc(ManuvrRunnable *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */ 
  int8_t return_value = event->eventManagerShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;
  
  /* Some class-specific set of conditionals below this line. */
  switch (event->event_code) {
    case MANUVR_MSG_SYS_BOOT_COMPLETED:
      if (verbosity > 4) mKernel::log("Boot complete.\n");
      boot_completed = true;
      break;
    default:
      break;
  }
  return return_value;
}



int8_t mKernel::notify(ManuvrRunnable *active_event) {
  uint32_t temp_uint32 = 0;
  int8_t return_value = 0;
  
  switch (active_event->event_code) {
    case MANUVR_MSG_USER_DEBUG_INPUT:
      procDirectDebugInstruction(&last_user_input);
      return_value++;
      break;

    case MANUVR_MSG_SELF_DESCRIBE:
      // Field order: 1 uint32, 4 required null-terminated strings, 1 optional.
      // uint32:     MTU                (in terms of bytes)
      // String:     Protocol version   (IE: "0.0.1")
      // String:     Identity           (IE: "Digitabulum") Generally the name of the Manuvrable.
      // String:     Firmware version   (IE: "1.5.4")
      // String:     Hardware version   (IE: "4")
      // String:     Extended detail    (User-defined)
      if (0 == active_event->args.size()) {
        // We are being asked to self-describe.
        active_event->addArg((uint32_t)    PROTOCOL_MTU);
        active_event->addArg((const char*) PROTOCOL_VERSION);
        active_event->addArg((const char*) IDENTITY_STRING);
        active_event->addArg((const char*) VERSION_STRING);
        active_event->addArg((const char*) HW_VERSION_STRING);
        #ifdef EXTENDED_DETAIL_STRING
          active_event->addArg((const char*) EXTENDED_DETAIL_STRING);
        #endif
        return_value++;
      }
      break;
      
    case MANUVR_MSG_SYS_REBOOT:
      reboot();
      break;
    case MANUVR_MSG_SYS_SHUTDOWN:
      seppuku();  // TODO: We need to distinguish between this and SYSTEM shutdown for linux.
      break;
    case MANUVR_MSG_SYS_BOOTLOADER:
      jumpToBootloader();
      break;
      
    case MANUVR_MSG_SYS_ADVERTISE_SRVC:  // Some service is annoucing its arrival.
    case MANUVR_MSG_SYS_RETRACT_SRVC:    // Some service is annoucing its departure.
      if (0 < active_event->argCount()) {
        EventReceiver* er_ptr; 
        if (0 == active_event->getArgAs(&er_ptr)) {
          if (MANUVR_MSG_SYS_ADVERTISE_SRVC == active_event->event_code) {
            subscribe((EventReceiver*) er_ptr);
          }
          else {
            unsubscribe((EventReceiver*) er_ptr);
          }
        }
      }
      break;
      
    case MANUVR_MSG_LEGEND_MESSAGES:     // Dump the message definitions.
      if (0 == active_event->argCount()) {   // Only if we are seeing a request.
        StringBuilder *tmp_sb = new StringBuilder();
        if (ManuvrMsg::getMsgLegend(tmp_sb) ) {
          active_event->addArg(tmp_sb);
          return_value++;
        }
        else {
          //if (verbosity > 1) local_log.concatf("There was a problem writing the message legend. This is bad. Size %d.\n", tmp_sb->length());
        }
      }
      else {
        // We may be receiving a message definition message from another party.
        // For now, we've decided to handle this in XenoSession.
      }
      break;

    case MANUVR_MSG_SYS_ISSUE_LOG_ITEM:
      {
        StringBuilder *log_item;
        if (0 == active_event->getArgAs(&log_item)) {
          log_buffer.concatHandoff(log_item);
        }
      }
      break;

    #if defined (__MANUVR_FREERTOS)
    case MANUVR_MSG_CREATED_THREAD_ID:
      break;

    case MANUVR_MSG_DESTROYED_THREAD_ID:
      break;

    case MANUVR_MSG_UNBLOCK_THREAD:
      break;
    #endif
    
    case MANUVR_MSG_SCHED_ENABLE_BY_PID:
      while (0 == active_event->consumeArgAs(&temp_uint32)) {
        if (enableSchedule(temp_uint32)) {
          return_value++;
        }
      }
      break;
    case MANUVR_MSG_SCHED_DISABLE_BY_PID:
      while (0 == active_event->consumeArgAs(&temp_uint32)) {
        if (disableSchedule(temp_uint32)) {
          return_value++;
        }
      }
      break;
    case MANUVR_MSG_SCHED_PROFILER_START:
      while (0 == active_event->consumeArgAs(&temp_uint32)) {
        beginProfiling(temp_uint32);
        return_value++;
      }
      break;
    case MANUVR_MSG_SCHED_PROFILER_STOP:
      while (0 == active_event->consumeArgAs(&temp_uint32)) {
        stopProfiling(temp_uint32);
        return_value++;
      }
      break;
    case MANUVR_MSG_SCHED_PROFILER_DUMP:
      if (active_event->args.size() > 0) {
        while (0 == active_event->consumeArgAs(&temp_uint32)) {
          printProfiler(&local_log);
          return_value++;
        }
      }
      else {
        printProfiler(&local_log);
        return_value++;
      }
      break;
    case MANUVR_MSG_SCHED_DUMP_META:
      printDebug(&local_log);
      return_value++;
      break;
    case MANUVR_MSG_SCHED_DUMP_SCHEDULES:
      if (active_event->args.size() > 0) {
        ManuvrRunnable *nu_sched;
        while (0 == active_event->consumeArgAs(&temp_uint32)) {
          nu_sched  = findNodeByPID(temp_uint32);
          if (NULL != nu_sched) nu_sched->printDebug(&local_log);
        }
      }
      else {
        ManuvrRunnable *nu_sched;
        for (int i = 0; i < schedules.size(); i++) {
          nu_sched  = schedules.get(i);
          if (NULL != nu_sched) nu_sched->printDebug(&local_log);
          return_value++;
        }
      }
      return_value++;
      break;
    case MANUVR_MSG_SCHED_DEFERRED_EVENT:
      {
        //uint32_t period = 1000;
        //int16_t recurrence = 1;
        ////sch_callback
        //if (createSchedule(period, recurrence, true, FunctionPointer sch_callback)) {
        //  return_value++;
        //}
      }
      break;

    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }
  return return_value;
}                             


/**
* Because this is the root of all console commands into the system, we treat
*   things a bit differently here.
* Console commands start with the index of the subscriber they are addressed
*   to. The following string will be passed into the EventReceiver.
*/
void mKernel::procDirectDebugInstruction(StringBuilder* input) {
  #ifdef __MANUVR_CONSOLE_SUPPORT
  char *str = (char *) input->string();
  char c = *(str);
  uint8_t subscriber_idx = 0;
  uint8_t temp_byte = 0;

  if (*(str) != 0) {
    // This is always safe because the null terminator shows up as zero, and
    //   we already know that the string is not zero-length.
    subscriber_idx = atoi((char*) str);
    temp_byte = atoi((char*) str+1);
  }
  ManuvrRunnable *event = NULL;  // Pitching events is a common thing in this fxn...
  
  StringBuilder parse_mule;
  
  EventReceiver* subscriber = subscribers.get(subscriber_idx);
  if ((NULL == subscriber) || ((EventReceiver*)this == subscriber)) {
    // If there was no subscriber specified, or WE were specified 
    //   (for some reason), we process the command ourselves.
  
    switch (c) {
      case 'B':
        if (temp_byte == 128) {
          mKernel::raiseEvent(MANUVR_MSG_SYS_BOOTLOADER, NULL);
          break;
        }
        local_log.concatf("Will only jump to bootloader if the number '128' follows the command.\n");
        break;
      case 'b':
        if (temp_byte == 128) {
          mKernel::raiseEvent(MANUVR_MSG_SYS_REBOOT, NULL);
          break;
        }
        local_log.concatf("Will only reboot if the number '128' follows the command.\n");
        break;
  
      case 'r':        // Read so many random integers...
        { // TODO: I don't think the RNG is ever being turned off. Save some power....
          temp_byte = (temp_byte == 0) ? PLATFORM_RNG_CARRY_CAPACITY : temp_byte;
          for (uint8_t i = 0; i < temp_byte; i++) {
            uint32_t x = randomInt();
            if (x) {
              local_log.concatf("Random number: 0x%08x\n", x);
            }
            else {
              local_log.concatf("Restarting RNG\n");
              init_RNG();
            }
          }
        }
        break;
  
      case 'u':
        switch (temp_byte) {
          case 1:
            mKernel::raiseEvent(MANUVR_MSG_SELF_DESCRIBE, NULL);
            break;
          case 3:
            mKernel::raiseEvent(MANUVR_MSG_LEGEND_MESSAGES, NULL);
            break;
          default:
            break;
        }
        break;
  
      case 'y':    // Power mode.
        if (255 != temp_byte) {
          event = mKernel::returnEvent(MANUVR_MSG_SYS_POWER_MODE);
          event->addArg((uint8_t) temp_byte);
          EventReceiver::raiseEvent(event);
          local_log.concatf("Power mode is now %d.\n", temp_byte);
        }
        else {
        }
        break;
  
      #if defined(__MANUVR_DEBUG)
      case 'i':   // Debug prints.
        if (1 == temp_byte) {
          local_log.concat("mKernel profiling enabled.\n");
          profiler(true);
        }
        else if (2 == temp_byte) {
          printDebug(&local_log);
        }
        else if (3 == temp_byte) {
          local_log.concat("mKernel profiling disabled.\n");
          profiler(false);
        }
        else {
          printDebug(&local_log);
        }
        break;
      #endif //__MANUVR_DEBUG
  
      case 'v':           // Set log verbosity.
        parse_mule.concat(str);
        parse_mule.drop_position(0);

        event = new ManuvrRunnable(MANUVR_MSG_SYS_LOG_VERBOSITY);
        switch (parse_mule.count()) {
          case 2:
            event->specific_target = getSubscriberByName((const char*) (parse_mule.position_trimmed(1)));
            local_log.concatf("Directing verbosity change to %s.\n", (NULL == event->specific_target) ? "NULL" : event->specific_target->getReceiverName());
          case 1:
            event->addArg((uint8_t) parse_mule.position_as_int(0));
            break;
          default:
            break;
        }
        EventReceiver::raiseEvent(event);
        break;
  
      case 'S':
        if (temp_byte) {
          ManuvrRunnable *nu_sched;
          nu_sched  = findNodeByPID(temp_byte);
          if (NULL != nu_sched) nu_sched->printDebug(&local_log);
        }
        else {
          ManuvrRunnable *nu_sched;
          for (int i = 0; i < schedules.size(); i++) {
            nu_sched  = schedules.get(i);
            if (NULL != nu_sched) nu_sched->printDebug(&local_log);
          }
        }
        break;

      default:
        EventReceiver::procDirectDebugInstruction(input);
        break;
    }   
  }
  else {
    input->cull(1); // Shuck the identifier byte.
    subscriber->procDirectDebugInstruction(input);
  }
  #endif  //__MANUVR_CONSOLE_SUPPORT
  
  if (local_log.length() > 0) mKernel::log(&local_log);
  last_user_input.clear();
}














/*
File:   mKernel.cpp
Author: J. Ian Lindsay
Date:   2013.07.10

This class is meant to be a real-time task scheduler for small microcontrollers. It
should be driven by a periodic interrupt of some sort, but it may also be effectively
used with a reliable polling scheme (at the possible cost of timing accuracy).

A simple profiler is included which will allow the user of this class to determine
run-times and possibly even adjust task duty cycles accordingly.

Copyright (C) 2013 J. Ian Lindsay
All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/


/****************************************************************************************************
* Class-management functions...                                                                     *
****************************************************************************************************/

/**
*  Given the schedule PID, begin profiling.
*/
void mKernel::beginProfiling(uint32_t g_pid) {
  ManuvrRunnable *current = findNodeByPID(g_pid);
  current->enableProfiling(true);
}


/**
*  Given the schedule PID, stop profiling.
* Stops profiling without destroying the collected data.
* Note: If profiling is ever re-started on this schedule, the profiling data
*  that this function preserves will be wiped.
*/
void mKernel::stopProfiling(uint32_t g_pid) {
  ManuvrRunnable *current = findNodeByPID(g_pid);
  current->enableProfiling(false);
}


/**
*  Given the schedule PID, reset the profiling data.
*  Typically, we'd do this when the profiler is being turned on.
*/
void mKernel::clearProfilingData(uint32_t g_pid) {
  ManuvrRunnable *current = findNodeByPID(g_pid);
  if (NULL != current) current->clearProfilingData();
}




/****************************************************************************************************
* Linked-list helper functions...                                                                   *
****************************************************************************************************/

// Fire the given schedule on the next idle loop.
bool mKernel::fireSchedule(uint32_t g_pid) {
  ManuvrRunnable *current = this->findNodeByPID(g_pid);
  if (NULL != current) {
    current->fireNow(true);
    current->thread_time_to_wait = current->thread_period;
    execution_queue.insert(current);
    return true;
  }
  return false;
}


/**
* Returns the number of schedules presently defined.
*/
int mKernel::getTotalSchedules() {
  return schedules.size();
}


/**
* Returns the number of schedules presently active.
*/
unsigned int mKernel::getActiveSchedules() {
  unsigned int return_value = 0;
  ManuvrRunnable *current;
  for (int i = 0; i < schedules.size(); i++) {
    current = schedules.get(i);
    if (current->threadEnabled()) {
      return_value++;
    }
  }
  return return_value;
}



/**
* Destroy everything in the list. Should only be called by the destructor, but no harm
*  in calling it for other reasons. Will stop and wipe all schedules. 
*/
void mKernel::destroyAllScheduleItems() {
  execution_queue.clear();
  while (schedules.hasNext()) {  delete schedules.dequeue();   }
}


/**
* Traverses the linked list and returns a pointer to the node that has the given PID.
* Returns NULL if a node is not found that meets this criteria.
*/
ManuvrRunnable* mKernel::findNodeByPID(uint32_t g_pid) {
  ManuvrRunnable *current = NULL;
  for (int i = 0; i < schedules.size(); i++) {
    current = schedules.get(i);
    if (current->pid == g_pid) {
      return current;
    }
  }
  return NULL;
}


/**
*  When we assign a new PID, call this function to get one. Since we don't want
*    to collide with one that already exists, or get the zero value. 
*/
uint32_t mKernel::get_valid_new_pid() {
    uint32_t return_value = next_pid++;
    if (return_value == 0) {
        return_value = get_valid_new_pid();  // Recurse...
    }
    // Takes too long, but represents a potential bug.
    //else if (this->findNodeByPID(return_value) == NULL) {
    //	return_value = this->get_valid_new_pid();  // Recurse...
    //}

	return return_value;
}


uint32_t mKernel::peekNextPID() {  return next_pid;  }


/**
*  Call this function to create a new schedule with the given period, a given number of repititions, and with a given function call.
*
*  Will automatically set the schedule active, provided the input conditions are met.
*  Returns the newly-created PID on success, or 0 on failure.
*/
uint32_t mKernel::createSchedule(uint32_t sch_period, int16_t recurrence, bool ac, FunctionPointer sch_callback) {
  uint32_t return_value  = 0;
  if (sch_period > 1) {
    if (sch_callback != NULL) {
      ManuvrRunnable *nu_sched = new ManuvrRunnable(get_valid_new_pid(), recurrence, sch_period, ac, sch_callback);
      if (nu_sched != NULL) {  // Did we actually malloc() successfully?
        return_value  = nu_sched->pid;
        schedules.insert(nu_sched);
        nu_sched->enableProfiling(return_value);
      }
    }
  }
  return return_value;
}


/**
*  Call this function to create a new schedule with the given period, a given number of repititions, and with a given function call.
*
*  Will automatically set the schedule active, provided the input conditions are met.
*  Returns the newly-created PID on success, or 0 on failure.
*/
uint32_t mKernel::createSchedule(uint32_t sch_period, int16_t recurrence, bool ac, EventReceiver* sch_callback, ManuvrRunnable* event) {
  uint32_t return_value  = 0;
  if (sch_period > 1) {
    ManuvrRunnable *nu_sched = new ManuvrRunnable(get_valid_new_pid(), recurrence, sch_period, ac, sch_callback, event);
    if (nu_sched != NULL) {  // Did we actually malloc() successfully?
      return_value  = nu_sched->pid;
      schedules.insert(nu_sched);
      nu_sched->enableProfiling(return_value);
    }
  }
  return return_value;
}


/**
* Call this function to alter a given schedule. Set with the given period, a given number of times, with a given function call.
*  Returns true on success or false if the given PID is not found, or there is a problem with the parameters.
*
* Will not set the schedule active, but will clear any pending executions for this schedule, as well as reset the timer for it.
*/
bool mKernel::alterSchedule(ManuvrRunnable *obj, uint32_t sch_period, int16_t recurrence, bool ac, FunctionPointer sch_callback) {
  bool return_value  = false;
  if (sch_period > 1) {
    if (sch_callback != NULL) {
      if (obj != NULL) {
        obj->fireNow(false);
        obj->autoClear(ac);
        obj->thread_recurs       = recurrence;
        obj->thread_period       = sch_period;
        obj->thread_time_to_wait = sch_period;
        obj->schedule_callback   = sch_callback;
        return_value  = true;
      }
    }
  }
  return return_value;
}

bool mKernel::alterSchedule(uint32_t g_pid, uint32_t sch_period, int16_t recurrence, bool ac, FunctionPointer sch_callback) {
  return alterSchedule(findNodeByPID(g_pid), sch_period, recurrence, ac, sch_callback);
}

bool mKernel::alterSchedule(uint32_t schedule_index, bool ac) {
  bool return_value  = false;
  ManuvrRunnable *nu_sched  = findNodeByPID(schedule_index);
  if (nu_sched != NULL) {
    nu_sched->autoClear(ac);
    return_value  = true;
  }
  return return_value;
}

bool mKernel::alterSchedule(uint32_t schedule_index, FunctionPointer sch_callback) {
  bool return_value  = false;
  if (sch_callback != NULL) {
    ManuvrRunnable *nu_sched  = findNodeByPID(schedule_index);
    if (nu_sched != NULL) {
      nu_sched->schedule_callback   = sch_callback;
      return_value  = true;
    }
  }
  return return_value;
}

bool mKernel::alterSchedulePeriod(uint32_t schedule_index, uint32_t sch_period) {
  bool return_value  = false;
  if (sch_period > 1) {
    ManuvrRunnable *nu_sched  = findNodeByPID(schedule_index);
    if (nu_sched != NULL) {
      nu_sched->fireNow(false);
      nu_sched->thread_period       = sch_period;
      nu_sched->thread_time_to_wait = sch_period;
      return_value  = true;
    }
  }
  return return_value;
}

bool mKernel::alterScheduleRecurrence(uint32_t schedule_index, int16_t recurrence) {
  bool return_value  = false;
  ManuvrRunnable *nu_sched  = findNodeByPID(schedule_index);
  if (nu_sched != NULL) {
    nu_sched->fireNow(false);
    nu_sched->thread_recurs       = recurrence;
    return_value  = true;
  }
  return return_value;
}


/**
* Returns true if...
* A) The schedule exists
*    AND
* B) The schedule is enabled, and has at least one more runtime before it *might* be auto-reaped.
*/
bool mKernel::willRunAgain(uint32_t g_pid) {
  ManuvrRunnable *nu_sched  = findNodeByPID(g_pid);
  if (nu_sched != NULL) {
    if (nu_sched->threadEnabled()) {
      if ((nu_sched->thread_recurs == -1) || (nu_sched->thread_recurs > 0)) {
        return true;
      }
    }
  }
  return false;
}


bool mKernel::scheduleEnabled(uint32_t g_pid) {
  ManuvrRunnable *nu_sched  = findNodeByPID(g_pid);
  if (nu_sched != NULL) {
    return nu_sched->threadEnabled();
  }
  return false;
}


/**
* Enable a previously disabled schedule.
*  Returns true on success and false on failure.
*/
bool mKernel::enableSchedule(uint32_t g_pid) {
  ManuvrRunnable *nu_sched  = findNodeByPID(g_pid);
  if (nu_sched != NULL) {
    nu_sched->threadEnabled(true);
    return true;
  }
  return false;
}


bool mKernel::delaySchedule(ManuvrRunnable *obj, uint32_t by_ms) {
  if (obj != NULL) {
    obj->thread_time_to_wait = by_ms;
    obj->threadEnabled(true);
    return true;
  }
  return false;
}

/**
* Causes a given schedule's TTW (time-to-wait) to be set to the value we provide (this time only).
* If the schedule wasn't enabled before, it will be when we return.
*/
bool mKernel::delaySchedule(uint32_t g_pid, uint32_t by_ms) {
  ManuvrRunnable *nu_sched  = findNodeByPID(g_pid);
  return delaySchedule(nu_sched, by_ms);
}

/**
* Causes a given schedule's TTW (time-to-wait) to be reset to its period.
* If the schedule wasn't enabled before, it will be when we return.
*/
bool mKernel::delaySchedule(uint32_t g_pid) {
  ManuvrRunnable *nu_sched = findNodeByPID(g_pid);
  return delaySchedule(nu_sched, nu_sched->thread_period);
}



/**
* Call this function to push the schedules forward by a given number of ms.
* We can be assured of exclusive access to the scheules queue as long as we
*   are in an ISR.
*/
void mKernel::advanceScheduler(unsigned int ms_elapsed) {
  clicks_in_isr++;
  _ms_elapsed += (uint32_t) ms_elapsed;
  
  
  if (bistable_skip_detect) {
    // Failsafe block
#ifdef STM32F4XX
    if (skipped_loops > 5000) {
      // TODO: We are hung in a way that we probably cannot recover from. Reboot...
      jumpToBootloader();
    }
    else if (skipped_loops == 2000) {
      printf("Hung scheduler...\n");
    }
    else if (skipped_loops == 2040) {
      /* Doing all this String manipulation in an ISR would normally be an awful idea.
         But we don't care here because we're hung anyhow, and we need to know why. */
      StringBuilder output;
      mKernel::getInstance()->fetchmKernel()->printDebug(&output);
      printf("%s\n", (char*) output.string());
    }
    else if (skipped_loops == 2200) {
      StringBuilder output;
      printDebug(&output);
      printf("%s\n", (char*) output.string());
    }
    else if (skipped_loops == 3500) {
      StringBuilder output;
      mKernel::getInstance()->printDebug(&output);
      printf("%s\n", (char*) output.string());
    }
    skipped_loops++;
#endif
  }
  else {
    bistable_skip_detect = true;
  }
}


/**
* Call this function to push the schedules forward.
* We can be assured of exclusive access to the scheules queue as long as we
*   are in an ISR.
*/
void mKernel::advanceScheduler() {
  clicks_in_isr++;
  _ms_elapsed++;
  
  //int x = schedules.size();
  //ManuvrRunnable *current;
  //
  //for (int i = 0; i < x; i++) {
  //  current = schedules.recycle();
  //  if (current->threadEnabled()) {
  //    if (current->thread_time_to_wait > 0) {
  //      current->thread_time_to_wait--;
  //    }
  //    else {
  //      current->fireNow(true);
  //      current->thread_time_to_wait = current->thread_period;
  //      execution_queue.insert(current);
  //    }
  //  }
  //}
  if (bistable_skip_detect) {
    // Failsafe block
#ifdef STM32F4XX
    if (skipped_loops > 5000) {
      // TODO: We are hung in a way that we probably cannot recover from. Reboot...
      jumpToBootloader();
    }
    else if (skipped_loops == 2000) {
      printf("Hung scheduler...\n");
    }
    else if (skipped_loops == 2040) {
      /* Doing all this String manipulation in an ISR would normally be an awful idea.
         But we don't care here because we're hung anyhow, and we need to know why. */
      StringBuilder output;
      mKernel::getInstance()->fetchmKernel()->printDebug(&output);
      printf("%s\n", (char*) output.string());
    }
    else if (skipped_loops == 2200) {
      StringBuilder output;
      printDebug(&output);
      printf("%s\n", (char*) output.string());
    }
    else if (skipped_loops == 3500) {
      StringBuilder output;
      mKernel::getInstance()->printDebug(&output);
      printf("%s\n", (char*) output.string());
    }
    skipped_loops++;
#endif
  }
  else {
    bistable_skip_detect = true;
  }
}


/**
* Call to disable a given schedule.
*  Will reset the time_to_wait so that if the schedule is re-enabled, it doesn't fire sooner than expected.
*  Returns true on success and false on failure.
*/
bool mKernel::disableSchedule(uint32_t g_pid) {
  ManuvrRunnable *nu_sched  = findNodeByPID(g_pid);
  if (nu_sched != NULL) {
      nu_sched->threadEnabled(false);
      nu_sched->fireNow(false);
      nu_sched->thread_time_to_wait = nu_sched->thread_period;
      return true;
  }
  return false;
}


/**
* Will remove the indicated schedule and wipe its profiling data.
* In case this gets called from the schedule's service function (IE,
*   if the schedule tries to delete itself), let it expire this run
*   rather than ripping the rug out from under ourselves.
*
* @param  A pointer to the schedule item to be removed. 
* @return true on success and false on failure.
*/
bool mKernel::removeSchedule(ManuvrRunnable *obj) {
  if (obj != NULL) {
    if (obj->pid != this->currently_executing) {
      schedules.remove(obj);
      execution_queue.remove(obj);
      delete obj;
    }
    else { 
      obj->autoClear(true);
      obj->thread_recurs = 0;
    }
    return true;
  }
  return false;
}

bool mKernel::removeSchedule(uint32_t g_pid) {
  ManuvrRunnable *obj  = findNodeByPID(g_pid);
  return removeSchedule(obj);
}


/**
* This is the function that is called from the main loop to offload big
*  tasks into idle CPU time. If many scheduled items have fired, function
*  will churn through all of them. The presumption is that they are 
*  latency-sensitive.
*/
int mKernel::serviceScheduledEvents() {
  if (!scheduler_ready) return -1;
  int return_value = 0;
  uint32_t origin_time = micros();


  uint32_t temp_clicks = clicks_in_isr;
  clicks_in_isr = 0;
  if (0 == temp_clicks) {
    return return_value;
  }
  
  int x = schedules.size();
  ManuvrRunnable *current;
  
  for (int i = 0; i < x; i++) {
    current = schedules.recycle();
    if (current->threadEnabled()) {
      if (current->thread_time_to_wait > _ms_elapsed) {
        current->thread_time_to_wait -= _ms_elapsed;
      }
      else {
        current->fireNow(true);
        uint32_t adjusted_ttw = (_ms_elapsed - current->thread_time_to_wait);
        if (adjusted_ttw <= current->thread_period) {
          current->thread_time_to_wait = current->thread_period - adjusted_ttw;
        }
        else {
          // TODO: Possible error-case? Too many clicks passed. We have schedule jitter...
          // For now, we'll just throw away the difference.
          current->thread_time_to_wait = current->thread_period;
          lagged_schedules++;
        }
        execution_queue.insert(current);
      }
    }
  }
  
  uint32_t profile_start_time = 0;
  uint32_t profile_stop_time  = 0;

  current = execution_queue.dequeue();
  while (NULL != current) {
    if (current->shouldFire()) {
      currently_executing = current->pid;
      TaskProfilerData *prof_data = NULL;

      if (current->isProfiling()) {
        prof_data = current->prof_data;
        profile_start_time = micros();
      }

      if (NULL != current) {  // This is an event-based schedule.
        //if (NULL != current->event->specific_target) {
        //  // TODO: Note: If we do this, we are outside of the profiler's scope.
        //  current->event->specific_target->notify(current->event);   // Pitch the schedule's event.
        //  
        //  if (NULL != current->event->callback) {
        //    current->event->callback->callback_proc(current->event);
        //  }
        //}
        //else {
          mKernel::staticRaiseEvent(current);
        //}
      }
      else if (NULL != current->schedule_callback) {
        ((void (*)(void)) current->schedule_callback)();   // Call the schedule's service function.
      }
      
      if (NULL != prof_data) {
        profile_stop_time = micros();
        prof_data->run_time_last    = max(profile_start_time, profile_stop_time) - min(profile_start_time, profile_stop_time);  // Rollover invarient.
        prof_data->run_time_best    = min(prof_data->run_time_best,  prof_data->run_time_last);
        prof_data->run_time_worst   = max(prof_data->run_time_worst, prof_data->run_time_last);
        prof_data->run_time_total  += prof_data->run_time_last;
        prof_data->run_time_average = prof_data->run_time_total / ((prof_data->executions) ? prof_data->executions : 1);
        prof_data->executions++;
      }            
      current->fireNow(false);
      currently_executing = 0;
         
      switch (current->thread_recurs) {
        case -1:           // Do nothing. Schedule runs indefinitely.
          break;
        case 0:            // Disable (and remove?) the schedule.
          if (current->autoClear()) {
            removeSchedule(current);
          }
          else {
            current->threadEnabled(false);  // Disable the schedule...
            current->fireNow(false);          // ...mark it as serviced.
            current->thread_time_to_wait = current->thread_period;  // ...and reset the timer.
          }
          break;
        default:           // Decrement the run count.
          current->thread_recurs--;
          break;
      }
      return_value++;
    }
    current = execution_queue.dequeue();
  }
  overhead = micros() - origin_time;
  total_loops++;
  if (return_value > 0) productive_loops++;
  
  // We just ran a loop. Punch the bistable swtich and clear the skip count.
  bistable_skip_detect = false;
  total_skipped_loops += skipped_loops;
  skipped_loops        = 0;
  
  _ms_elapsed = 0;
  return return_value;
}


void mKernel::printSchedule(uint32_t g_pid, StringBuilder* output) {
  if (NULL == output) return;

  ManuvrRunnable *nu_sched;
  nu_sched  = findNodeByPID(g_pid);
  if (NULL != nu_sched) nu_sched->printDebug(output);
}



