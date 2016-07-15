/*
File:   Kernel.cpp
Author: J. Ian Lindsay
Date:   2013.07.10

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


#include "FirmwareDefs.h"
#include <Kernel.h>
#include <Platform/Platform.h>


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
Kernel* Kernel::INSTANCE = NULL;
PriorityQueue<ManuvrRunnable*> Kernel::isr_exec_queue;

const unsigned char MSG_ARGS_EVENTRECEIVER[] = {SYS_EVENTRECEIVER_FM, 0, 0};
const unsigned char MSG_ARGS_NO_ARGS[] = {0};

const MessageTypeDef message_defs[] = {
  {  MANUVR_MSG_SYS_BOOT_COMPLETED   , 0x0000,               "BOOT_COMPLETED"   , MSG_ARGS_NO_ARGS }, // Raised when bootstrap is finished.

  {  MANUVR_MSG_SYS_ADVERTISE_SRVC   , 0x0000,               "ADVERTISE_SRVC"       , MSG_ARGS_EVENTRECEIVER }, // A system service might feel the need to advertise it's arrival.
  {  MANUVR_MSG_SYS_RETRACT_SRVC     , 0x0000,               "RETRACT_SRVC"         , MSG_ARGS_EVENTRECEIVER }, // A system service sends this to tell others to stop using it.

  {  MANUVR_MSG_SYS_BOOTLOADER       , MSG_FLAG_EXPORTABLE,  "SYS_BOOTLOADER"       , MSG_ARGS_NO_ARGS }, // Reboots into the STM32F4 bootloader.
  {  MANUVR_MSG_SYS_REBOOT           , MSG_FLAG_EXPORTABLE,  "SYS_REBOOT"           , MSG_ARGS_NO_ARGS }, // Reboots into THIS program.
  {  MANUVR_MSG_SYS_SHUTDOWN         , MSG_FLAG_EXPORTABLE,  "SYS_SHUTDOWN"         , MSG_ARGS_NO_ARGS }, // Raised when the system is pending complete shutdown.

  {  MANUVR_MSG_DEFERRED_FXN         , 0x0000,               "DEFERRED_FXN",          MSG_ARGS_NO_ARGS }, // Message to allow for deferred fxn calls without an EventReceiver.

  {  MANUVR_MSG_XPORT_SEND           , MSG_FLAG_IDEMPOTENT,  "XPORT_SEND"           , ManuvrMsg::MSG_ARGS_STR_BUILDER }, //

  {  MANUVR_MSG_SYS_FAULT_REPORT     , 0x0000,               "SYS_FAULT"            , ManuvrMsg::MSG_ARGS_U32 }, //

  {  MANUVR_MSG_SYS_DATETIME_CHANGED , MSG_FLAG_EXPORTABLE,  "SYS_DATETIME_CHANGED" , ManuvrMsg::MSG_ARGS_NONE }, // Raised when the system time changes.
  {  MANUVR_MSG_SYS_SET_DATETIME     , MSG_FLAG_EXPORTABLE,  "SYS_SET_DATETIME"     , ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_SYS_REPORT_DATETIME  , MSG_FLAG_EXPORTABLE,  "SYS_REPORT_DATETIME"  , ManuvrMsg::MSG_ARGS_NONE }, //

  {  MANUVR_MSG_INTERRUPTS_MASKED    , 0x0000,               "INTERRUPTS_MASKED"    , ManuvrMsg::MSG_ARGS_NONE }, // Anything that depends on interrupts is now broken.

  {  MANUVR_MSG_SESS_SUBCRIBE        , MSG_FLAG_EXPORTABLE,  "SESS_SUBCRIBE"        , ManuvrMsg::MSG_ARGS_NONE }, // Used to subscribe this session to other events.
  {  MANUVR_MSG_SESS_UNSUBCRIBE      , MSG_FLAG_EXPORTABLE,  "SESS_UNSUBCRIBE"      , ManuvrMsg::MSG_ARGS_NONE }, // Used to unsubscribe this session from other events.
  {  MANUVR_MSG_SESS_ORIGINATE_MSG   , MSG_FLAG_IDEMPOTENT,  "SESS_ORIGINATE_MSG"   , ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_SESS_SERVICE         , 0x0000             ,  "SESS_SERVICE"         , ManuvrMsg::MSG_ARGS_NONE }, //

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
  {  MANUVR_MSG_USER_DEBUG_INPUT     , MSG_FLAG_EXPORTABLE,               "USER_DEBUG_INPUT"     , ManuvrMsg::MSG_ARGS_STR_BUILDER, "Command\0\0" }, //
  {  MANUVR_MSG_SYS_ISSUE_LOG_ITEM   , MSG_FLAG_EXPORTABLE,               "SYS_ISSUE_LOG_ITEM"   , ManuvrMsg::MSG_ARGS_STR_BUILDER, "Body\0\0" }, // Classes emit this to get their log data saved/sent.
  {  MANUVR_MSG_SYS_POWER_MODE       , MSG_FLAG_EXPORTABLE,               "SYS_POWER_MODE"       , ManuvrMsg::MSG_ARGS_U8, "Power Mode\0\0" }, //
  {  MANUVR_MSG_SYS_LOG_VERBOSITY    , MSG_FLAG_EXPORTABLE,               "SYS_LOG_VERBOSITY"    , ManuvrMsg::MSG_ARGS_U8, "Level\0\0"      },   // This tells client classes to adjust their log verbosity.
  #else
  {  MANUVR_MSG_USER_DEBUG_INPUT     , MSG_FLAG_EXPORTABLE,               "USER_DEBUG_INPUT"     , ManuvrMsg::MSG_ARGS_STR_BUILDER, NULL }, //
  {  MANUVR_MSG_SYS_ISSUE_LOG_ITEM   , MSG_FLAG_EXPORTABLE,               "SYS_ISSUE_LOG_ITEM"   , ManuvrMsg::MSG_ARGS_STR_BUILDER, NULL }, // Classes emit this to get their log data saved/sent.
  {  MANUVR_MSG_SYS_POWER_MODE       , MSG_FLAG_EXPORTABLE,               "SYS_POWER_MODE"       , ManuvrMsg::MSG_ARGS_U8, NULL }, //
  {  MANUVR_MSG_SYS_LOG_VERBOSITY    , MSG_FLAG_EXPORTABLE,               "SYS_LOG_VERBOSITY"    , ManuvrMsg::MSG_ARGS_U8, NULL },   // This tells client classes to adjust their log verbosity.
  #endif
};


/*
* All external access to Kernel's non-static members should obtain it's reference via this fxn...
*   Note that services that are dependant on us during the bootstrap phase should have a reference
*   passed into their constructors, rather than forcing them to call this and risking an infinite
*   recursion.
*/
Kernel* Kernel::getInstance() {
  if (INSTANCE == NULL) {
    // This is a valid means of instantiating the kernel. Typically, user code
    //   would have the Kernel on the stack, but if they want to live in the heap,
    //   that's fine by us. Oblige...
    Kernel::INSTANCE = new Kernel();
  }
  // And that is how the singleton do...
  return (Kernel*) Kernel::INSTANCE;
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
Kernel::Kernel() {
  __class_initializer();
  INSTANCE           = this;  // For singleton reference. TODO: Will not parallelize.
  __kernel           = this;  // We extend EventReceiver. So we populate this.

  current_event        = NULL;
  max_queue_depth      = 0;
  total_events         = 0;
  total_events_dead    = 0;
  micros_occupied      = 0;
  max_events_per_loop  = 2;
  lagged_schedules     = 0;
  _ms_elapsed          = 0;

  _skips_observed      = 0;

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
  profiler(false);           // Turn off the profiler.
  setVerbosity((int8_t) DEFAULT_CLASS_VERBOSITY);

  ManuvrMsg::registerMessages(message_defs, sizeof(message_defs) / sizeof(MessageTypeDef));

  platformPreInit();    // Start the pre-bootstrap platform-specific machinery.
}

/**
* Destructor. Should probably never be called.
*/
Kernel::~Kernel() {
  while (schedules.hasNext()) {  delete schedules.dequeue();   }
}



/****************************************************************************************************
* Logging members...                                                                                *
****************************************************************************************************/
StringBuilder Kernel::log_buffer;

#if defined (__MANUVR_FREERTOS)
  uint32_t Kernel::logger_pid = 0;
  uint32_t Kernel::kernel_pid = 0;
#endif

/*
* Logger pass-through functions. Please mind the variadics...
*/
volatile void Kernel::log(int severity, const char *str) {
  if (!INSTANCE->getVerbosity()) return;
  log_buffer.concat(str);
  #if defined (__MANUVR_FREERTOS)
    if (logger_pid) vTaskResume((TaskHandle_t) logger_pid);
  #endif
}

volatile void Kernel::log(char *str) {
  if (!INSTANCE->getVerbosity()) return;
  log_buffer.concat(str);
  #if defined (__MANUVR_FREERTOS)
    if (logger_pid) vTaskResume((TaskHandle_t) logger_pid);
  #endif
}

volatile void Kernel::log(const char *str) {
  if (!INSTANCE->getVerbosity()) return;
  log_buffer.concat(str);
  #if defined (__MANUVR_FREERTOS)
    if (logger_pid) vTaskResume((TaskHandle_t) logger_pid);
  #endif
}

volatile void Kernel::log(const char *fxn_name, int severity, const char *str, ...) {
  if (!INSTANCE->getVerbosity()) return;
  log_buffer.concatf("%d  %s:\t", severity, fxn_name);
  va_list marker;

  va_start(marker, str);
  log_buffer.concatf(str, marker);
  va_end(marker);
  #if defined (__MANUVR_FREERTOS)
    if (logger_pid) vTaskResume((TaskHandle_t) logger_pid);
  #endif
}

volatile void Kernel::log(StringBuilder *str) {
  if (!INSTANCE->getVerbosity()) return;
  log_buffer.concatHandoff(str);
  #if defined (__MANUVR_FREERTOS)
    if (logger_pid) vTaskResume((TaskHandle_t) logger_pid);
  #endif
}




/****************************************************************************************************
* Kernel operation...                                                                               *
****************************************************************************************************/
int8_t Kernel::bootstrap() {
  platformInit();    // Start the platform-specific machinery.

  ManuvrRunnable *boot_completed_ev = Kernel::returnEvent(MANUVR_MSG_SYS_BOOT_COMPLETED);
  boot_completed_ev->priority = EVENT_PRIORITY_HIGHEST;
  Kernel::staticRaiseEvent(boot_completed_ev);

  #if defined (__MANUVR_FREERTOS)
    //vTaskStartScheduler();
  #endif

  return 0;
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
int8_t Kernel::subscribe(EventReceiver *client) {
  if (NULL == client) return -1;

  client->setVerbosity(DEFAULT_CLASS_VERBOSITY);
  int8_t return_value = subscribers.insert(client);
  if (booted()) {
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
int8_t Kernel::subscribe(EventReceiver *client, uint8_t priority) {
  if (NULL == client) return -1;

  client->setVerbosity(DEFAULT_CLASS_VERBOSITY);
  int8_t return_value = subscribers.insert(client, priority);
  if (booted()) {
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
int8_t Kernel::unsubscribe(EventReceiver *client) {
  if (NULL == client) return -1;
  return (subscribers.remove(client) ? 0 : -1);
}


EventReceiver* Kernel::getSubscriberByName(const char* search_str) {
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
* Code related to event definition and management...                                                *
****************************************************************************************************/

int8_t Kernel::registerCallbacks(uint16_t msgCode, listenerFxnPtr ca, listenerFxnPtr cb, uint32_t options) {
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




/****************************************************************************************************
* Static member functions for posting events.                                                       *
****************************************************************************************************/

/**
* Used to add an event to the idle queue. Use this for simple events that don't need args.
*
* @param  code  The identity code of the event to be raised.
* @param  ori   An optional originator pointer to be called when this event is finished.
* @return -1 on failure, and 0 on success.
*/
int8_t Kernel::raiseEvent(uint16_t code, EventReceiver* ori) {
  int8_t return_value = 0;

  // We are creating a new Event. Try to snatch a prealloc'd one and fall back to malloc if needed.
  ManuvrRunnable* nu = INSTANCE->preallocated.dequeue();
  if (nu == NULL) {
    INSTANCE->prealloc_starved++;
    nu = new ManuvrRunnable(code, ori);
  }
  else {
    nu->repurpose(code);
    nu->originator = ori;
  }

  if (0 == INSTANCE->validate_insertion(nu)) {
    INSTANCE->exec_queue.insert(nu);
    INSTANCE->update_maximum_queue_depth();   // Check the queue depth
    #if defined (__MANUVR_FREERTOS)
      //if (kernel_pid) unblockThread(kernel_pid);
    #endif
  }
  else {
    if (INSTANCE->getVerbosity() > 4) {
      #ifdef __MANUVR_DEBUG
      StringBuilder output("raiseEvent():\tvalidate_insertion() failed:\n");
      output.concat(ManuvrMsg::getMsgTypeString(code));
      Kernel::log(&output);
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
*   needs to be formed elsewhere and passed in. Kernel will only insert it into the
*   queue in this case.
*
* @param   event  The event to be inserted into the idle queue.
* @return  -1 on failure, and 0 on success.
*/
int8_t Kernel::staticRaiseEvent(ManuvrRunnable* event) {
  int8_t return_value = 0;
  if (0 == INSTANCE->validate_insertion(event)) {
    INSTANCE->exec_queue.insert(event, event->priority);
    //INSTANCE->update_maximum_queue_depth();   // Check the queue depth
    #if defined (__MANUVR_FREERTOS)
      //if (kernel_pid) unblockThread(kernel_pid);
    #endif
  }
  else {
    if (INSTANCE->getVerbosity() > 4) {
      #ifdef __MANUVR_DEBUG
      StringBuilder output("staticRaiseEvent():\tvalidate_insertion() failed:\n");
      event->printDebug(&output);;
      Kernel::log(&output);
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
*   needs to be formed elsewhere and passed in. Kernel will only insert it into the
*   queue in this case.
*
* @param   event  The event to be removed from the idle queue.
* @return  true if the given event was aborted, false otherwise.
*/
bool Kernel::abortEvent(ManuvrRunnable* event) {
  if (!INSTANCE->exec_queue.remove(event)) {
    // Didn't find it? Check  the isr_queue...
    if (!INSTANCE->isr_exec_queue.remove(event)) {
      return false;
    }
  }
  return true;
}


// TODO: It would be bettter to put a semaphore on the exec_queue and set it in the idle loop.
//       That way, we could check for it here, and have the (probable) possibility of not incurring
//       the cost for merging these two queues if we don't have to.
//             ---J. Ian Lindsay   Fri Jul 03 16:54:14 MST 2015
int8_t Kernel::isrRaiseEvent(ManuvrRunnable* event) {
  int return_value = -1;
  maskableInterrupts(false);
  return_value = isr_exec_queue.insertIfAbsent(event, event->priority);
  maskableInterrupts(true);
  #if defined (__MANUVR_FREERTOS)
    //if (kernel_pid) unblockThread(kernel_pid);
  #endif
  return return_value;
}



/**
* Factory method. Returns a preallocated Event.
* After we return the event, we lose track of it. So if the caller doesn't ever
*   call raiseEvent(), the Event we return will become a memory leak.
* The event we retun will have an originator field populated with a ref to Kernel.
*   So if a caller needs their own reference in that slot, caller will need to do it.
*
* @param  code  The desired identity code of the event.
* @return A pointer to the prepared event. Will not return NULL unless we are out of memory.
*/
ManuvrRunnable* Kernel::returnEvent(uint16_t code) {
  // We are creating a new Event. Try to snatch a prealloc'd one and fall back to malloc if needed.
  ManuvrRunnable* return_value = INSTANCE->preallocated.dequeue();
  if (return_value == NULL) {
    INSTANCE->prealloc_starved++;
    return_value = new ManuvrRunnable(code, (EventReceiver*) INSTANCE);
  }
  else {
    return_value->repurpose(code);
    return_value->originator = (EventReceiver*) INSTANCE;
  }
  return return_value;
}



/**
* This is the code that checks an incoming event for validity prior to inserting it
*   into the exec_queue. Checks for grammatical validity and idempotency should go
*   here.
*
* @param event The inbound event that we need to evaluate.
* @return 0 if the event is good-to-go. Otherwise, an appropriate failure code.
*/
int8_t Kernel::validate_insertion(ManuvrRunnable* event) {
  if (NULL == event) return -1;                                // No NULL events.
  if (MANUVR_MSG_UNDEFINED == event->event_code) {
    return -2;  // No undefined events.
  }

  if (exec_queue.contains(event)) {
    // Bail out with error, because this event (which is status-bearing) cannot be in the
    //   queue more than once.
    return -3;
  }

  // Those are the basic checks. Now for the advanced functionality...
  if (event->isIdempotent()) {
    ManuvrRunnable* working;
    for (int i = 0; i < exec_queue.size(); i++) {
      // No duplicate idempotent events allowed...
      working = exec_queue.get(i);
      if ((working) && (working->event_code == event->event_code)) {
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
* @param active_runnable The event that has reached the end of its life-cycle.
*/
void Kernel::reclaim_event(ManuvrRunnable* active_runnable) {
  if (NULL == active_runnable) {
    return;
  }

  if (schedules.contains(active_runnable)) {
    // This Runnable is still in the scheduler queue. Do not reclaim it.
    return;
  }

  bool reap_current_event = active_runnable->kernelShouldReap();

  #ifdef __MANUVR_DEBUG
  //if (getVerbosity() > 5) {
  //  local_log.concatf("We will%s be reaping %s.\n", (reap_current_event ? "":" not"), active_runnable->getMsgTypeString());
  //  Kernel::log(&local_log);
  //}
  #endif // __MANUVR_DEBUG

  if (reap_current_event) {                   // If we are to reap this event...
    delete active_runnable;                      // ...free() it...
    events_destroyed++;
    burden_of_specific++;                     // ...and note the incident.
  }
  else {                                      // If we are NOT to reap this event...
    if (active_runnable->isManaged()) {
    }
    else if (active_runnable->returnToPrealloc()) {   // ...is it because we preallocated it?
      active_runnable->clearArgs();              // If so, wipe the Event...
      preallocated.insert(active_runnable);      // ...and return it to the preallocate queue.
    }                                         // Otherwise, we let it drop and trust some other class is managing it.
    #ifdef __MANUVR_DEBUG
    else {
      if (getVerbosity() > 6) {
        local_log.concat("Kernel::reclaim_event(): Doing nothing. Hope its managed elsewhere.\n");
        Kernel::log(&local_log);
      }
    }
    #endif // __MANUVR_DEBUG
  }
}



// This is the splice into v2's style of event handling (callaheads).
int8_t Kernel::procCallAheads(ManuvrRunnable *active_runnable) {
  int8_t return_value = 0;
  PriorityQueue<listenerFxnPtr> *ca_queue = ca_listeners[active_runnable->event_code];
  if (NULL != ca_queue) {
    listenerFxnPtr current_fxn;
    for (int i = 0; i < ca_queue->size(); i++) {
      current_fxn = ca_queue->recycle();  // TODO: This is ugly for many reasons.
      if (current_fxn(active_runnable)) {
        return_value++;
      }
    }
  }
  return return_value;
}

// This is the splice into v2's style of event handling (callbacks).
int8_t Kernel::procCallBacks(ManuvrRunnable *active_runnable) {
  int8_t return_value = 0;
  PriorityQueue<listenerFxnPtr> *cb_queue = cb_listeners[active_runnable->event_code];
  if (NULL != cb_queue) {
    listenerFxnPtr current_fxn;
    for (int i = 0; i < cb_queue->size(); i++) {
      current_fxn = cb_queue->recycle();  // TODO: This is ugly for many reasons.
      if (current_fxn(active_runnable)) {
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
*     call. We don't want to cause bad re-entrancy problem in the Kernel by spending
*     all of our time here (although we might re-work the Kernel to make this acceptable).
*
* @return the number of events processed, or a negative value on some failure.
*/
int8_t Kernel::procIdleFlags() {
  uint32_t profiler_mark   = micros();
  uint32_t profiler_mark_0 = 0;   // Profiling requests...
  uint32_t profiler_mark_1 = 0;   // Profiling requests...
  uint32_t profiler_mark_2 = 0;   // Profiling requests...
  uint32_t profiler_mark_3 = 0;   // Profiling requests...
  int8_t return_value      = 0;   // Number of Events we've processed this call.
  uint16_t msg_code_local  = 0;

  serviceSchedules();

  ManuvrRunnable *active_runnable = NULL;  // Our short-term focus.
  uint8_t activity_count    = 0;     // Incremented whenever a subscriber reacts to an event.

  globalIRQDisable();
  while (isr_exec_queue.size() > 0) {
    active_runnable = isr_exec_queue.dequeue();

    if (0 == validate_insertion(active_runnable)) {
      exec_queue.insertIfAbsent(active_runnable, active_runnable->priority);
    }
    else reclaim_event(active_runnable);
  }
  globalIRQEnable();

  active_runnable = NULL;   // Pedantic...

  /* As long as we have an open event and we aren't yet at our proc ceiling... */
  while (exec_queue.hasNext() && should_run_another_event(return_value, profiler_mark)) {
    active_runnable = exec_queue.dequeue();       // Grab the Event and remove it in the same call.
    msg_code_local = active_runnable->event_code;  // This gets used after the life of the event.

    current_event = active_runnable;

    // Chat and measure.
    #ifdef __MANUVR_DEBUG
    //if (getVerbosity() > 6) local_log.concatf("Servicing: %s\n", active_runnable->getMsgTypeString());
    #endif

    profiler_mark_0 = micros();

    procCallAheads(active_runnable);

    // Now we start notify()'ing subscribers.
    EventReceiver *subscriber;   // No need to assign.
    if (_profiler_enabled()) profiler_mark_1 = micros();

    if (NULL != active_runnable->schedule_callback) {
      // TODO: This is hold-over from the scheduler. Need to modernize it.
      StringBuilder temp_log;
      active_runnable->printDebug(&temp_log);
      printf("\n\n%s\n", temp_log.string());
      ((FunctionPointer) active_runnable->schedule_callback)();   // Call the schedule's service function.
      if (_profiler_enabled()) profiler_mark_2 = micros();
      activity_count++;
    }
    else if (NULL != active_runnable->specific_target) {
      subscriber = active_runnable->specific_target;
      switch (subscriber->notify(active_runnable)) {
        case 0:   // The nominal case. No response.
          break;
        case -1:  // The subscriber choked. Figure out why. Technically, this is action. Case fall-through...
          subscriber->printDebug(&local_log);
        default:   // The subscriber acted.
          activity_count++;
          if (_profiler_enabled()) profiler_mark_2 = micros();
          break;
      }
    }
    else {
      for (int i = 0; i < subscribers.size(); i++) {
        subscriber = subscribers.get(i);

        switch (subscriber->notify(active_runnable)) {
          case 0:   // The nominal case. No response.
            break;
          case -1:  // The subscriber choked. Figure out why. Technically, this is action. Case fall-through...
            subscriber->printDebug(&local_log);
          default:   // The subscriber acted.
            activity_count++;
            if (_profiler_enabled()) profiler_mark_2 = micros();
            break;
        }
      }
    }

    procCallBacks(active_runnable);

    active_runnable->noteExecutionTime(profiler_mark_0, micros());

    if (0 == activity_count) {
      #ifdef __MANUVR_DEBUG
      if (getVerbosity() >= 3) local_log.concatf("\tDead event: %s\n", active_runnable->getMsgTypeString());
      #endif
      total_events_dead++;
    }

    /* Should we clean up the Event? */
    bool clean_up_active_runnable = true;  // Defaults to 'yes'.
    if (NULL != active_runnable->originator) {
      if ((EventReceiver*) this != active_runnable->originator) {  // We don't want to invoke our own originator callback.
        /* If the event has a valid originator, do the callback dance and take instruction
           from the return value. */
        //   if (verbosity >=7) output.concatf("specific_event_callback returns %d\n", active_runnable->originator->callback_proc(active_runnable));
        switch (active_runnable->originator->callback_proc(active_runnable)) {
          case EVENT_CALLBACK_RETURN_RECYCLE:     // The originating class wants us to re-insert the event.
            #ifdef __MANUVR_DEBUG
            if (getVerbosity() > 6) local_log.concatf("Recycling %s.\n", active_runnable->getMsgTypeString());
            #endif
            if (0 == validate_insertion(active_runnable)) {
              exec_queue.insert(active_runnable, active_runnable->priority);
              // This is the one case where we do NOT want the event reclaimed.
              clean_up_active_runnable = false;
            }
            break;
          case EVENT_CALLBACK_RETURN_ERROR:       // Something went wrong. Should never occur.
          case EVENT_CALLBACK_RETURN_UNDEFINED:   // The originating class doesn't care what we do with the event.
            //if (verbosity > 1) local_log.concatf("Kernel found a possible mistake. Unexpected return case from callback_proc.\n");
            // NOTE: No break;
          case EVENT_CALLBACK_RETURN_DROP:        // The originating class expects us to drop the event.
            #ifdef __MANUVR_DEBUG
            //if (getVerbosity() > 6) local_log.concatf("Dropping %s after running.\n", active_runnable->getMsgTypeString());
            #endif
            // NOTE: No break;
          case EVENT_CALLBACK_RETURN_REAP:        // The originating class is explicitly telling us to reap the event.
            // NOTE: No break;
          default:
            //if (verbosity > 0) local_log.concatf("Event %s has no cleanup case.\n", active_runnable->getMsgTypeString());
            break;
        }
      }
      else {
        //reclaim_event(active_runnable);
      }
    }
    else {
      /* If there is no originator specified for the Event, we rely on the flags in the Event itself to
         decide if it should be reaped. If its memory is being managed by some other class, the reclaim_event()
         fxn will simply remove it from the exec_queue and consider the matter closed. */
    }

    // All of the logic above ultimately informs this choice.
    if (clean_up_active_runnable) {
      reclaim_event(active_runnable);
    }

    total_events++;

    // This is a stat-gathering block.
    if (_profiler_enabled()) {
      profiler_mark_3 = micros();

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
        MessageTypeDef* tmp_msg_def = (MessageTypeDef*) ManuvrMsg::lookupMsgDefByCode(msg_code_local);
        if (getVerbosity() >= 6) {
          if (profiler_mark_2) local_log.concatf("%s finished.\tTotal time: %5ld uS\tTook %5ld uS to notify.\n", tmp_msg_def->debug_label, (profiler_mark_3 - profiler_mark_0), profiler_item->run_time_last);
          else                 local_log.concatf("%s finished.\tTotal time: %5ld uS\n", tmp_msg_def->debug_label, (profiler_mark_3 - profiler_mark_0));
        }
      #endif
      profiler_mark_2 = 0;  // Reset for next iteration.
    }

    if (exec_queue.size() > 30) {
      #ifdef __MANUVR_DEBUG
      local_log.concatf("Depth %10d \t %s\n", exec_queue.size(), ManuvrMsg::getMsgTypeString(msg_code_local));
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

  if (local_log.length() > 0) Kernel::log(&local_log);
  current_event = NULL;
  return return_value;
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

/**
* Turns the profiler on or off. Clears its collected stats regardless.
*
* @param   enabled  If true, enables the profiler. If false, disables it.
*/
void Kernel::profiler(bool enabled) {
  _profiler_enabled(enabled);
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


// TODO: This never worked terribly well. Need to tap the timer
//   and profile to do it correctly. Still better than nothing.
float Kernel::cpu_usage() {
  return (micros_occupied / (float)(millis()*10));
}


#if defined(__MANUVR_DEBUG)
/**
* Print the type sizes to the kernel log.
*
* @param   StringBuilder*  The buffer that this fxn will write output into.
*/
void Kernel::print_type_sizes() {
  StringBuilder temp("---< Type sizes >-----------------------------\n");
  temp.concatf("Elemental data structures:\n");
  temp.concatf("\t StringBuilder         %zu\n", sizeof(StringBuilder));
  temp.concatf("\t LinkedList<void*>     %zu\n", sizeof(LinkedList<void*>));
  temp.concatf("\t PriorityQueue<void*>  %zu\n", sizeof(PriorityQueue<void*>));

  temp.concatf(" Core singletons:\n");
  temp.concatf("\t Kernel                %zu\n", sizeof(Kernel));

  temp.concatf(" Messaging components:\n");
  temp.concatf("\t ManuvrRunnable        %zu\n", sizeof(ManuvrRunnable));
  temp.concatf("\t ManuvrMsg             %zu\n", sizeof(ManuvrMsg));
  temp.concatf("\t Argument              %zu\n", sizeof(Argument));
  temp.concatf("\t TaskProfilerData      %zu\n", sizeof(TaskProfilerData));

  Kernel::log(&temp);
}
#endif


/**
* Print the profiler to the provided buffer.
*
* @param   StringBuilder*  The buffer that this fxn will write output into.
*/
void Kernel::printProfiler(StringBuilder* output) {
  if (NULL == output) return;
  output->concatf("-- total_events       \t%u\n", (unsigned long) total_events);
  output->concatf("-- total_events_dead  \t%u\n", (unsigned long) total_events_dead);
  output->concatf("-- max_queue_depth    \t%u\n", (unsigned long) max_queue_depth);
  output->concatf("-- total_loops        \t%u\n", (unsigned long) total_loops);
  output->concatf("-- max_idle_loop_time \t%u\n", (unsigned long) max_idle_loop_time);
  output->concatf("-- max_events_p_loop  \t%u\n", (unsigned long) max_events_p_loop);

  if (_profiler_enabled()) {
    output->concat("-- Profiler:\n");
    if (total_events) {
      output->concatf("   prealloc hit fraction: \t%f\%\n", (double)(1-((burden_of_specific - prealloc_starved) / total_events)) * 100);
    }
    output->concatf("   CPU use by clock: %f\n", (double)cpu_usage());

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
  }
  else {
    output->concat("-- Kernel profiler disabled.\n\n");
  }


  if (schedules.size() > 0) {
    ManuvrRunnable *current;
    output->concat("\n\t PID         Execd      total us   average    worst      best       last\n\t -----------------------------------------------------------------------------\n");

    for (int i = 0; i < schedules.size(); i++) {
      current = schedules.get(i);
      if (current->profilingEnabled()) {
        current->printProfilerData(output);
      }
    }
  }
}


/**
* Debug support function.
*
* @return a pointer to a string constant.
*/
const char* Kernel::getReceiverName() {  return "Kernel";  }


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void Kernel::printDebug(StringBuilder* output) {
  if (NULL == output) return;
  uint32_t initial_sp = getStackPointer();
  uint32_t final_sp = getStackPointer();

  EventReceiver::printDebug(output);

  output->concatf("-- %s v%s    Build date: %s %s\n--\n", IDENTITY_STRING, VERSION_STRING, __DATE__, __TIME__);
  //output->concatf("-- our_mem_addr:             0x%08x\n", (uint32_t) this);
  if (getVerbosity() > 5) {
    output->concat("-- Current datetime          ");
    currentDateTime(output);
    output->concatf("\n-- millis()                  0x%08x\n", millis());
    output->concatf("-- micros()                  0x%08x\n", micros());
    output->concatf("-- _ms_elapsed               %u\n", (unsigned long) _ms_elapsed);
  }

  if (getVerbosity() > 6) {
    output->concatf("-- getStackPointer()         0x%08x\n", getStackPointer());
    output->concatf("-- stack grows %s\n--\n", (final_sp > initial_sp) ? "up" : "down");
  }

  if (getVerbosity() > 4) {
    output->concatf("-- Queue depth:              %d\n", exec_queue.size());
    output->concatf("-- Preallocation depth:      %d\n", preallocated.size());
    output->concatf("-- Prealloc starves:         %u\n", (unsigned long) prealloc_starved);
    output->concatf("-- events_destroyed:         %u\n", (unsigned long) events_destroyed);
    output->concatf("-- burden_of_being_specific  %u\n", (unsigned long) burden_of_specific);
    output->concatf("-- idempotent_blocks         %u\n", (unsigned long) idempotent_blocks);
    output->concatf("-- Scheduler skips           %u\n", (unsigned long) _skips_observed);
  }

  if (_er_flag(MKERNEL_FLAG_SKIP_FAILSAFE)) {
    output->concatf("-- %u skips before fail-to-bootloader.\n", (unsigned long) MAXIMUM_SEQUENTIAL_SKIPS);
  }

  output->concatf("-- Lagged schedules  %u\n", (unsigned long) lagged_schedules);
  output->concatf("-- Total schedules:  %d\n-- Active schedules: %d\n\n", schedules.size(), countActiveSchedules());

  if (subscribers.size() > 0) {
    output->concatf("-- Subscribers: (%d total):\n", subscribers.size());
    for (int i = 0; i < subscribers.size(); i++) {
      output->concatf("\t %d: %s\n", i, subscribers.get(i)->getReceiverName());
    }
    output->concat("\n");
  }

  if (NULL != current_event) {
    output->concat("-- Current Runnable:\n");
    current_event->printDebug(output);
  }

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
int8_t Kernel::bootComplete() {
  EventReceiver::bootComplete();
  _mark_boot_complete();
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
*   ourselves and return DROP. By default, we will return REAP to instruct the Kernel
*   to either free() the event or return it to it's preallocate queue, as appropriate. If the event
*   was crafted to not be in the heap in its own allocation, we will return DROP instead.
*
* @param  event  The event for which service has been completed.
* @return A callback return code.
*/
int8_t Kernel::callback_proc(ManuvrRunnable *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = event->kernelShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->event_code) {
    case MANUVR_MSG_SYS_BOOT_COMPLETED:
      if (getVerbosity() > 4) Kernel::log("Boot complete.\n");
      _mark_boot_complete();
      break;
    default:
      break;
  }
  return return_value;
}



int8_t Kernel::notify(ManuvrRunnable *active_runnable) {
  int8_t return_value = 0;

  switch (active_runnable->event_code) {
    #if defined(__MANUVR_CONSOLE_SUPPORT)
      case MANUVR_MSG_USER_DEBUG_INPUT:
        if (active_runnable->argCount()) {
          // If the event came with a StringBuilder, concat it onto the last_user_input.
          StringBuilder* _tmp = NULL;
          if (0 == active_runnable->consumeArgAs(&_tmp)) {
            last_user_input.concatHandoff(_tmp);
          }
        }
        _route_console_input();
        return_value++;
        break;
    #endif  // __MANUVR_CONSOLE_SUPPORT

    case MANUVR_MSG_SELF_DESCRIBE:
      // Field order: 1 uint32, 4 required null-terminated strings, 1 optional.
      // uint32:     MTU                (in terms of bytes)
      // String:     Protocol version   (IE: "0.0.1")
      // String:     Identity           (IE: "Digitabulum") Generally the name of the Manuvrable.
      // String:     Firmware version   (IE: "1.5.4")
      // String:     Hardware version   (IE: "4")
      // String:     Extended detail    (User-defined)
      if (0 == active_runnable->args.size()) {
        // We are being asked to self-describe.
        active_runnable->addArg((uint32_t)    PROTOCOL_MTU);
        active_runnable->addArg((uint32_t)    0);                  // Device flags.
        active_runnable->addArg((const char*) PROTOCOL_VERSION);
        active_runnable->addArg((const char*) IDENTITY_STRING);
        active_runnable->addArg((const char*) VERSION_STRING);
        active_runnable->addArg((const char*) HW_VERSION_STRING);
        active_runnable->addArg((uint32_t)    0);                  // Optional serial number.
        #ifdef EXTENDED_DETAIL_STRING
          active_runnable->addArg((const char*) EXTENDED_DETAIL_STRING);
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
      if (0 < active_runnable->argCount()) {
        EventReceiver* er_ptr;
        if (0 == active_runnable->getArgAs(&er_ptr)) {
          if (MANUVR_MSG_SYS_ADVERTISE_SRVC == active_runnable->event_code) {
            subscribe((EventReceiver*) er_ptr);
          }
          else {
            unsubscribe((EventReceiver*) er_ptr);
          }
        }
      }
      break;

    case MANUVR_MSG_LEGEND_MESSAGES:     // Dump the message definitions.
      if (0 == active_runnable->argCount()) {   // Only if we are seeing a request.
        StringBuilder *tmp_sb = new StringBuilder();
        if (ManuvrMsg::getMsgLegend(tmp_sb) ) {
          active_runnable->addArg(tmp_sb);
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
        if (0 == active_runnable->getArgAs(&log_item)) {
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

    default:
      return_value += EventReceiver::notify(active_runnable);
      break;
  }
  return return_value;
}




/****************************************************************************************************
* These are functions that help us manage scheduled Runnables...                                    *
****************************************************************************************************/

/**
* Returns the number of schedules presently active.
*/
unsigned int Kernel::countActiveSchedules() {
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
*  Call this function to create a new schedule with the given period, a given number of repititions, and with a given function call.
*
*  Will automatically set the schedule active, provided the input conditions are met.
*  Returns the newly-created PID on success, or 0 on failure.
*/
ManuvrRunnable* Kernel::createSchedule(uint32_t sch_period, int16_t recurrence, bool ac, FunctionPointer sch_callback) {
  ManuvrRunnable* return_value = NULL;
  if (sch_period > 1) {
    if (sch_callback != NULL) {
      return_value = new ManuvrRunnable(recurrence, sch_period, ac, sch_callback);
      if (return_value != NULL) {  // Did we actually malloc() successfully?
        return_value->isScheduled(true);
        schedules.insert(return_value);
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
ManuvrRunnable* Kernel::createSchedule(uint32_t sch_period, int16_t recurrence, bool ac, EventReceiver* ori) {
  ManuvrRunnable* return_value = NULL;
  if (sch_period > 1) {
    return_value = new ManuvrRunnable(recurrence, sch_period, ac, ori);
    if (return_value != NULL) {  // Did we actually malloc() successfully?
      return_value->isScheduled(true);
      schedules.insert(return_value);
    }
  }
  return return_value;
}



/**
* Call this function to push the schedules forward by a given number of ms.
* We can be assured of exclusive access to the scheules queue as long as we
*   are in an ISR.
*/
void Kernel::advanceScheduler(unsigned int ms_elapsed) {
  _ms_elapsed += (uint32_t) ms_elapsed;

  if (_skip_detected()) {
    // Failsafe block
    _skips_observed++;
    //if (getVerbosity() > 3) Kernel::log("Scheduler skip\n");
    if (_er_flag(MKERNEL_FLAG_SKIP_FAILSAFE)) {
      if (_skips_observed >= MAXIMUM_SEQUENTIAL_SKIPS) {
    //    jumpToBootloader();
      }
    }
  }
  else {
    // The nominal case.
    _skip_detected(true);
  }
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
bool Kernel::removeSchedule(ManuvrRunnable *obj) {
  if (obj != NULL) {
    if (obj != current_event) {
      obj->isScheduled(false);
      schedules.remove(obj);
    }
    else {
      obj->autoClear(true);
      obj->thread_recurs = 0;
    }
    return true;
  }
  return false;
}

bool Kernel::addSchedule(ManuvrRunnable *obj) {
  if (obj != NULL) {
    if (!schedules.contains(obj)) {
      obj->isScheduled(true);
      schedules.insert(obj);
    }
    return true;
  }
  return false;
}



/**
* This is the function that is called from the main loop to offload big
*  tasks into idle CPU time. If many scheduled items have fired, function
*  will churn through all of them. The presumption is that they are
*  latency-sensitive.
*/
int Kernel::serviceSchedules() {
  if (!booted()) return -1;
  int return_value = 0;

  int x = schedules.size();
  ManuvrRunnable *current;

  for (int i = 0; i < x; i++) {
    current = schedules.recycle();
    if (current->threadEnabled()) {
      if ((current->thread_time_to_wait > _ms_elapsed) && (!current->shouldFire())){
        current->thread_time_to_wait -= _ms_elapsed;
      }
      else {
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
        current->fireNow(false);          // ...mark it as serviced.
        Kernel::staticRaiseEvent(current);
      }
    }
  }

  // We just ran a loop. Punch the bistable swtich.
  _skip_detected(false);
  _skips_observed = 0;

  _ms_elapsed = 0;
  return return_value;
}


/****************************************************************************************************
* The code below is related to accepting and parsing user input. It is only relevant if console     *
*   support is enabled.                                                                             *
****************************************************************************************************/
#if defined(__MANUVR_CONSOLE_SUPPORT)

/*
* This is the means by which we feed raw string input from a console into the
*   kernel's user input slot.
*/
void Kernel::accumulateConsoleInput(uint8_t *buf, int len, bool terminal) {
  if (len > 0) {
    last_user_input.concat(buf, len);

    if (terminal) {
      // If the ISR saw a CR or LF on the wire, we tell the parser it is ok to
      // run in idle time.
      ManuvrRunnable* event = returnEvent(MANUVR_MSG_USER_DEBUG_INPUT);
      event->specific_target = (EventReceiver*) this;
      Kernel::staticRaiseEvent(event);
    }
  }
}


/**
* Responsible for taking any accumulated console input, doing some basic
*   error-checking, and routing it to its intended target.
*/
int8_t Kernel::_route_console_input() {
  last_user_input.string();
  StringBuilder _raw_from_console;
  // Now we take the data from the buffer so that further input isn't lost. JIC.
  _raw_from_console.concatHandoff(&last_user_input);

  _raw_from_console.split(" ");
  if (_raw_from_console.count() > 0) {
    const char* str = (const char *) _raw_from_console.position(0);
    int subscriber_idx = atoi(str);
    switch (*str) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        // If the first position is a number, we drop the first position.
        _raw_from_console.drop_position(0);
        break;
    }

    if (_raw_from_console.count() > 0) {
      // If there are still positions, lookup the subscriber and send it the input.
      EventReceiver* subscriber = subscribers.get(subscriber_idx);
      if (NULL != subscriber) {
        subscriber->procDirectDebugInstruction(&_raw_from_console);
      }
      else if (getVerbosity() > 2) {
        local_log.concatf("No such subscriber: %d\n", subscriber_idx);
      }
    }
  }

  if (local_log.length() > 0) Kernel::log(&local_log);
  return 0;
}


/**
* Console commands are space-delimited and arrive here already NULL-checked and
*   tokenized. We can be assured that there is at least one token.
*/
void Kernel::procDirectDebugInstruction(StringBuilder* input) {
  const char* str = (char *) input->position(0);
  char c    = *str;
  int temp_int = 0;

  if (input->count() > 1) {
    // If there is a second token, we proceed on good-faith that it's an int.
    temp_int = input->position_as_int(1);
  }
  else if (strlen(str) > 1) {
    // We allow a short-hand for the sake of short commands that involve a single int.
    temp_int = atoi(str + 1);
  }

  switch (c) {
    case 'B':
      if (temp_int == 128) {
        Kernel::raiseEvent(MANUVR_MSG_SYS_BOOTLOADER, NULL);
        break;
      }
      local_log.concatf("Will only jump to bootloader if the number '128' follows the command.\n");
      break;
    case 'b':
      if (temp_int == 128) {
        Kernel::raiseEvent(MANUVR_MSG_SYS_REBOOT, NULL);
        break;
      }
      local_log.concatf("Will only reboot if the number '128' follows the command.\n");
      break;
    case 'f':  // FPU benchmark
      {
        float a = 1.001;
        long time_var2 = millis();
        for (int i = 0;i < 1000000;i++) {
          a += 0.01 * sqrtf(a);
        }
        local_log.concatf("Running floating-point test...\nTime:      %ul ms\n", (unsigned long) millis() - time_var2);
        local_log.concatf("Value:     %.5f\nFPU test concluded.\n", (double) a);
      }
      break;
    case 'r':        // Read so many random integers...
      { // TODO: I don't think the RNG is ever being turned off. Save some power....
        temp_int = (temp_int <= 0) ? PLATFORM_RNG_CARRY_CAPACITY : temp_int;
        for (uint8_t i = 0; i < temp_int; i++) {
          uint32_t x = randomInt();
          if (x) {
            local_log.concatf("Random number: 0x%08x\n", x);
          }
          else {
            local_log.concatf("RNG underflow. Reinit()\n");
            init_RNG();
          }
        }
      }
      break;

    #if defined(__MANUVR_DEBUG)
    case 'i':   // Debug prints.
      if (1 == temp_int) {
        local_log.concat("Kernel profiling enabled.\n");
        profiler(true);
      }
      else if (2 == temp_int) {
        printDebug(&local_log);
      }
      else if (3 == temp_int) {
        local_log.concat("Kernel profiling disabled.\n");
        profiler(false);
      }
      else {
        printDebug(&local_log);
      }
      break;
    #endif //__MANUVR_DEBUG
    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }

  if (local_log.length() > 0) Kernel::log(&local_log);
  last_user_input.clear();
}
#endif  //__MANUVR_CONSOLE_SUPPORT
