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


#include <CommonConstants.h>
#include <Kernel.h>
#include <Platform/Platform.h>

#include <MsgProfiler.h>

// Conditional inclusion for different threading models...
#if defined(__MANUVR_LINUX)
#elif defined(__MANUVR_FREERTOS)
  #include <FreeRTOS_ARM.h>
#endif


/*******************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here.
*******************************************************************************/
Kernel*     Kernel::INSTANCE = nullptr;
BufferPipe* Kernel::_logger  = nullptr;  // The logger slot.
PriorityQueue<ManuvrRunnable*> Kernel::isr_exec_queue;


/*
* These are the hard-coded message types that the program knows about.
* This is where we decide what kind of arguments each message is capable of carrying.
*
* Until someone does something smarter, there is no enforcemnt of types in this definition. Only
*   cardinality. Trying to get the type as something incompatible should return an error, but this
*   leaves type definition as a negotiable-matter between the code that wrote the message, and the
*   code reading it. We can also add a type string, as I had in BridgeBox, but this can also be
*   dynamically built.
*/
const MessageTypeDef ManuvrMsg::message_defs[] = {
  /* Reserved codes */
  {  MANUVR_MSG_UNDEFINED            , 0x0000,               "<UNDEF>"          , ManuvrMsg::MSG_ARGS_NONE }, // This should be the first entry for failure cases.

  /* Protocol basics. */
  #if defined(MANUVR_OVER_THE_WIRE)
  {  MANUVR_MSG_REPLY_FAIL           , MSG_FLAG_EXPORTABLE,               "REPLY_FAIL"           , ManuvrMsg::MSG_ARGS_NONE }, //  This reply denotes that the packet failed to parse (despite passing checksum).
  {  MANUVR_MSG_REPLY_RETRY          , MSG_FLAG_EXPORTABLE,               "REPLY_RETRY"          , ManuvrMsg::MSG_ARGS_NONE }, //  This reply asks for a reply of the given Unique ID.
  {  MANUVR_MSG_REPLY                , MSG_FLAG_EXPORTABLE,               "REPLY"                , ManuvrMsg::MSG_ARGS_NONE }, //  This reply is for success-case.
  {  MANUVR_MSG_SELF_DESCRIBE        , MSG_FLAG_DEMAND_ACK | MSG_FLAG_EXPORTABLE,  "SELF_DESCRIBE"        , ManuvrMsg::MSG_ARGS_NONE }, // Starting an application on the receiver. Needs a string.
  {  MANUVR_MSG_SYNC_KEEPALIVE       , MSG_FLAG_DEMAND_ACK | MSG_FLAG_EXPORTABLE,  "KA"                   , ManuvrMsg::MSG_ARGS_NONE }, //  A keep-alive message to be ack'd.
  {  MANUVR_MSG_LEGEND_TYPES         , MSG_FLAG_DEMAND_ACK | MSG_FLAG_EXPORTABLE,  "LEGEND_TYPES"         , ManuvrMsg::MSG_ARGS_NONE }, // No args? Asking for this legend. One arg: Legend provided.
  {  MANUVR_MSG_LEGEND_MESSAGES      , MSG_FLAG_DEMAND_ACK | MSG_FLAG_EXPORTABLE,  "LEGEND_MESSAGES"      , ManuvrMsg::MSG_ARGS_NONE }, // No args? Asking for this legend. One arg: Legend provided.
  {  MANUVR_MSG_LEGEND_SEMANTIC      , MSG_FLAG_DEMAND_ACK | MSG_FLAG_EXPORTABLE,  "LEGEND_SEMANTIC"      , ManuvrMsg::MSG_ARGS_NONE }, // No args? Asking for this legend. One arg: Legend provided.
  #endif

  #if defined(MANUVR_STORAGE)
  #endif

  {  MANUVR_MSG_SESS_ESTABLISHED     , MSG_FLAG_DEMAND_ACK | MSG_FLAG_EXPORTABLE,  "SESS_ESTABLISHED"     , ManuvrMsg::MSG_ARGS_NONE }, // Session established.
  {  MANUVR_MSG_SESS_HANGUP          , MSG_FLAG_EXPORTABLE,                        "SESS_HANGUP"          , ManuvrMsg::MSG_ARGS_NONE }, // Session hangup.
  {  MANUVR_MSG_SESS_AUTH_CHALLENGE  , MSG_FLAG_DEMAND_ACK | MSG_FLAG_EXPORTABLE,  "SESS_AUTH_CHALLENGE"  , ManuvrMsg::MSG_ARGS_NONE }, // A code for challenge-response authentication.

  {  MANUVR_MSG_MSG_FORWARD          , MSG_FLAG_DEMAND_ACK | MSG_FLAG_EXPORTABLE,  "MSG_FORWARD"          , ManuvrMsg::MSG_ARGS_NONE }, // No args? Asking for this legend. One arg: Legend provided.


  {  MANUVR_MSG_SYS_BOOT_COMPLETED   , 0x0000,               "BOOT_COMPLETED"   , ManuvrMsg::MSG_ARGS_NONE }, // Raised when bootstrap is finished.
  {  MANUVR_MSG_SYS_CONF_LOAD        , 0x0000,               "CONF_LOAD"        , ManuvrMsg::MSG_ARGS_NONE }, // Recipients will comb arguments for config and apply it.
  {  MANUVR_MSG_SYS_CONF_SAVE        , 0x0000,               "CONF_SAVE"        , ManuvrMsg::MSG_ARGS_NONE }, // Recipients will attach their persistable data.

  {  MANUVR_MSG_SYS_ADVERTISE_SRVC   , 0x0000,               "ADVERTISE_SRVC"       , ManuvrMsg::MSG_ARGS_NONE }, // A system service might feel the need to advertise it's arrival.
  {  MANUVR_MSG_SYS_RETRACT_SRVC     , 0x0000,               "RETRACT_SRVC"         , ManuvrMsg::MSG_ARGS_NONE }, // A system service sends this to tell others to stop using it.

  {  MANUVR_MSG_SYS_BOOTLOADER       , MSG_FLAG_EXPORTABLE,  "SYS_BOOTLOADER"       , ManuvrMsg::MSG_ARGS_NONE }, // Reboots into the STM32F4 bootloader.
  {  MANUVR_MSG_SYS_REBOOT           , MSG_FLAG_EXPORTABLE,  "SYS_REBOOT"           , ManuvrMsg::MSG_ARGS_NONE }, // Reboots into THIS program.
  {  MANUVR_MSG_SYS_SHUTDOWN         , MSG_FLAG_EXPORTABLE,  "SYS_SHUTDOWN"         , ManuvrMsg::MSG_ARGS_NONE }, // Raised when the system is pending complete shutdown.
  {  MANUVR_MSG_SYS_EXIT             , MSG_FLAG_EXPORTABLE,  "SYS_EXIT"             , ManuvrMsg::MSG_ARGS_NONE }, // Raised when the process is to exit without shutdown.

  {  MANUVR_MSG_DEFERRED_FXN         , 0x0000,               "DEFERRED_FXN",          ManuvrMsg::MSG_ARGS_NONE }, // Message to allow for deferred fxn calls without an EventReceiver.

  {  MANUVR_MSG_SYS_FAULT_REPORT     , 0x0000,               "SYS_FAULT"            , ManuvrMsg::MSG_ARGS_NONE }, //

  {  MANUVR_MSG_SYS_DATETIME_CHANGED , MSG_FLAG_EXPORTABLE,  "SYS_DATETIME_CHANGED" , ManuvrMsg::MSG_ARGS_NONE }, // Raised when the system time changes.
  {  MANUVR_MSG_SYS_SET_DATETIME     , MSG_FLAG_EXPORTABLE,  "SYS_SET_DATETIME"     , ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_SYS_REPORT_DATETIME  , MSG_FLAG_EXPORTABLE,  "SYS_REPORT_DATETIME"  , ManuvrMsg::MSG_ARGS_NONE }, //

  {  MANUVR_MSG_INTERRUPTS_MASKED    , 0x0000,               "INTERRUPTS_MASKED"    , ManuvrMsg::MSG_ARGS_NONE }, // Anything that depends on interrupts is now broken.

  {  MANUVR_MSG_SESS_SUBCRIBE        , MSG_FLAG_EXPORTABLE,  "SESS_SUBCRIBE"        , ManuvrMsg::MSG_ARGS_NONE }, // Used to subscribe this session to other events.
  {  MANUVR_MSG_SESS_UNSUBCRIBE      , MSG_FLAG_EXPORTABLE,  "SESS_UNSUBCRIBE"      , ManuvrMsg::MSG_ARGS_NONE }, // Used to unsubscribe this session from other events.
  {  MANUVR_MSG_SESS_ORIGINATE_MSG   , 0x0000,               "SESS_ORIGINATE_MSG"   , ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_SESS_SERVICE         , 0x0000,               "SESS_SERVICE"         , ManuvrMsg::MSG_ARGS_NONE }, //

  {  MANUVR_MSG_XPORT_SEND,         0x0000,  "XPORT_TX"           , ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_XPORT_RECEIVE,      0x0000,  "XPORT_RX"           , ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_XPORT_QUEUE_RDY,    0x0000,  "XPORT_Q"            , ManuvrMsg::MSG_ARGS_NONE }, //
  {  MANUVR_MSG_XPORT_CB_QUEUE_RDY, 0x0000,  "XPORT_CB_Q"         , ManuvrMsg::MSG_ARGS_NONE }, //

  #if defined (__MANUVR_FREERTOS) | defined (__MANUVR_LINUX)
  {  MANUVR_MSG_OIC_READY            , 0x0000,  "OIC_READY"        , ManuvrMsg::MSG_ARGS_NONE },  //
  {  MANUVR_MSG_OIC_REG_RESOURCES    , 0x0000,  "OIC_REG_RESRCS"   , ManuvrMsg::MSG_ARGS_NONE },  //
  {  MANUVR_MSG_OIC_DISCOVERY        , 0x0000,  "OIC_DISCOVERY"    , ManuvrMsg::MSG_ARGS_NONE },  //
  {  MANUVR_MSG_OIC_DISCOVER_OFF     , 0x0000,  "OIC_DISCOVER_OFF" , ManuvrMsg::MSG_ARGS_NONE },  //
  {  MANUVR_MSG_OIC_DISCOVER_PING    , 0x0000,  "OIC_PING"         , ManuvrMsg::MSG_ARGS_NONE },  //
  #endif

  #if defined (__MANUVR_FREERTOS) | defined (__MANUVR_LINUX)
    // These are messages that are only present under some sort of threading model. They are meant
    //   to faciliate task hand-off, IPC, and concurrency protection.
    {  MANUVR_MSG_CREATED_THREAD_ID,    0x0000,              "CREATED_THREAD_ID",     ManuvrMsg::MSG_ARGS_NONE },
    {  MANUVR_MSG_DESTROYED_THREAD_ID,  0x0000,              "DESTROYED_THREAD_ID",   ManuvrMsg::MSG_ARGS_NONE },
    {  MANUVR_MSG_UNBLOCK_THREAD,       0x0000,              "UNBLOCK_THREAD",        ManuvrMsg::MSG_ARGS_NONE },
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
  {  MANUVR_MSG_USER_DEBUG_INPUT     , MSG_FLAG_EXPORTABLE,               "USER_DEBUG_INPUT"     , ManuvrMsg::MSG_ARGS_NONE, "Command\0\0" }, //
  {  MANUVR_MSG_SYS_ISSUE_LOG_ITEM   , MSG_FLAG_EXPORTABLE,               "SYS_ISSUE_LOG_ITEM"   , ManuvrMsg::MSG_ARGS_NONE, "Body\0\0" }, // Classes emit this to get their log data saved/sent.
  {  MANUVR_MSG_SYS_POWER_MODE       , MSG_FLAG_EXPORTABLE,               "SYS_POWER_MODE"       , ManuvrMsg::MSG_ARGS_NONE, "Power Mode\0\0" }, //
  {  MANUVR_MSG_SYS_LOG_VERBOSITY    , MSG_FLAG_EXPORTABLE,               "SYS_LOG_VERBOSITY"    , ManuvrMsg::MSG_ARGS_NONE, "Level\0\0"      },   // This tells client classes to adjust their log verbosity.
  #else
  {  MANUVR_MSG_USER_DEBUG_INPUT     , MSG_FLAG_EXPORTABLE,               "USER_DEBUG_INPUT"     , ManuvrMsg::MSG_ARGS_NONE, nullptr }, //
  {  MANUVR_MSG_SYS_ISSUE_LOG_ITEM   , MSG_FLAG_EXPORTABLE,               "SYS_ISSUE_LOG_ITEM"   , ManuvrMsg::MSG_ARGS_NONE, nullptr }, // Classes emit this to get their log data saved/sent.
  {  MANUVR_MSG_SYS_POWER_MODE       , MSG_FLAG_EXPORTABLE,               "SYS_POWER_MODE"       , ManuvrMsg::MSG_ARGS_NONE, nullptr }, //
  {  MANUVR_MSG_SYS_LOG_VERBOSITY    , MSG_FLAG_EXPORTABLE,               "SYS_LOG_VERBOSITY"    , ManuvrMsg::MSG_ARGS_NONE, nullptr },   // This tells client classes to adjust their log verbosity.
  #endif
};


const uint16_t ManuvrMsg::TOTAL_MSG_DEFS = sizeof(ManuvrMsg::message_defs) / sizeof(MessageTypeDef);


void Kernel::nextTick(BufferPipe* p) {
  INSTANCE->_pipe_io_pend.insert(p);
  INSTANCE->_pending_pipes(true);
}

//void Kernel::nextTick(FxnPointer* p) {
//  INSTANCE->_pipe_io_pend.insert(p);
//  INSTANCE->_pending_pipes(true);
//}



/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

/**
* Vanilla constructor.
*/
Kernel::Kernel() : EventReceiver() {
  setReceiverName("Kernel");
  INSTANCE             = this;  // For singleton reference. TODO: Will not parallelize.
  __kernel             = this;  // We extend EventReceiver. So we populate this.

  current_event        = nullptr;
  max_queue_depth      = 0;
  total_events         = 0;
  total_events_dead    = 0;
  micros_occupied      = 0;
  max_events_per_loop  = 2;
  lagged_schedules     = 0;
  _ms_elapsed          = 0;

  _skips_observed      = 0;
  max_idle_count       = 100;
  consequtive_idles    = max_idle_count;

  for (int i = 0; i < EVENT_MANAGER_PREALLOC_COUNT; i++) {
    /* We carved out a space in our allocation for a pool of events. Ideally, this would be enough
        for most of the load, most of the time. If the preallocation ends up being insufficient to
        meet demand, new Events will be created on the heap. But the events that we are dealing
        with here are special. They should remain circulating (or in standby) for the lifetime of
        this class. */
    _preallocation_pool[i].returnToPrealloc(true);
    preallocated.insert(&_preallocation_pool[i]);
  }

  #if defined(__MANUVR_DEBUG)
    profiler(true);          // spend time and memory measuring performance.
  #else
    profiler(false);         // Turn off the profiler.
  #endif
  subscribe(this, 200);      // We subscribe ourselves to events.
  setVerbosity((int8_t) DEFAULT_CLASS_VERBOSITY);
}

/**
* Destructor. Should probably never be called.
*/
Kernel::~Kernel() {
  while (schedules.hasNext()) {
    reclaim_event(schedules.dequeue());
  }
}



/****************************************************************************************************
* Logging members...                                                                                *
****************************************************************************************************/

#if defined (__MANUVR_FREERTOS)
  uint32_t Kernel::kernel_pid = 0;
#endif

/*
* Logger pass-through functions. Please mind the variadics...
*/
volatile void Kernel::log(int severity, const char *str) {
  if (nullptr != _logger) {
    StringBuilder log_buffer(str);
    _logger->toCounterparty(&log_buffer, MEM_MGMT_RESPONSIBLE_BEARER);
  }
}

volatile void Kernel::log(char *str) {
  if (nullptr != _logger) {
    StringBuilder log_buffer(str);
    _logger->toCounterparty(&log_buffer, MEM_MGMT_RESPONSIBLE_BEARER);
  }
}

volatile void Kernel::log(const char *str) {
  if (nullptr != _logger) {
    StringBuilder log_buffer(str);
    _logger->toCounterparty(&log_buffer, MEM_MGMT_RESPONSIBLE_BEARER);
  }
}

volatile void Kernel::log(StringBuilder *str) {
  if (nullptr != _logger) {
    _logger->toCounterparty(str, MEM_MGMT_RESPONSIBLE_BEARER);
  }
  str->clear();
}

// TODO: Only one pipe can move log data at this moment.
int8_t Kernel::attachToLogger(BufferPipe* _pipe) {
  if (nullptr == _logger) {
    _logger = _pipe;
    return 0;
  }
  return -1;
}

// TODO: Only one pipe can move log data at this moment.
int8_t Kernel::detachFromLogger(BufferPipe* _pipe) {
  if (_pipe == _logger) {
    _logger = nullptr;
    return 0;
  }
  return -1;
}



/*******************************************************************************
* Subscriptions and client management fxns.                                    *
*******************************************************************************/

/**
* A class calls this to subscribe to events. After calling this, the class will have its notify()
*   member called for each event. In this manner, we give instantiated EventReceivers the ability
*   to listen to events.
*
* @param  client  The class that will be listening for Events.
* @return 0 on success and -1 on failure.
*/
int8_t Kernel::subscribe(EventReceiver *client) {
  if (nullptr == client) return -1;

  client->setVerbosity(DEFAULT_CLASS_VERBOSITY);
  int8_t return_value = subscribers.insert(client);
  if (erAttached()) {
    // This subscriber is joining us after bootup. Call its attached() fxn to cause it to init.
    client->attached();
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
  if (nullptr == client) return -1;

  client->setVerbosity(DEFAULT_CLASS_VERBOSITY);
  int8_t return_value = subscribers.insert(client, priority);
  if (erAttached()) {
    // This subscriber is joining us after bootup. Call its attached() fxn to cause it to init.
    //client.attached();
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
  if (nullptr == client) return -1;
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
  return nullptr;
}


/****************************************************************************************************
* Code related to event definition and management...                                                *
****************************************************************************************************/

int8_t Kernel::registerCallbacks(uint16_t msgCode, listenerFxnPtr ca, listenerFxnPtr cb, uint32_t options) {
  if (ca != nullptr) {
    PriorityQueue<listenerFxnPtr> *ca_queue = ca_listeners[msgCode];
    if (nullptr == ca_queue) {
      ca_queue = new PriorityQueue<listenerFxnPtr>();
      ca_listeners[msgCode] = ca_queue;
    }
    ca_queue->insert(ca);
  }

  if (cb != nullptr) {
    PriorityQueue<listenerFxnPtr> *cb_queue = cb_listeners[msgCode];
    if (nullptr == cb_queue) {
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
* @param  ori   An optional  pointer to be called when this event is finished.
* @return -1 on failure, and 0 on success.
*/
int8_t Kernel::raiseEvent(uint16_t code, EventReceiver* ori) {
  // We are creating a new Event. Try to snatch a prealloc'd one and fall back to malloc if needed.
  ManuvrRunnable* nu = INSTANCE->preallocated.dequeue();
  if (nu == nullptr) {
    INSTANCE->prealloc_starved++;
    nu = new ManuvrRunnable(code, ori);
  }
  else {
    nu->repurpose(code);
    nu->setOriginator(ori);
  }

  return staticRaiseEvent(nu);
}


/**
* Used to add a pre-formed event to the idle queue. Use this when a sophisticated event
*   needs to be formed elsewhere and passed in. Kernel will only insert it into the
*   queue in this case.
*
* @param   event  The event to be inserted into the idle queue.
* @return  -1 on failure, and 0 on success.
*/
int8_t Kernel::staticRaiseEvent(ManuvrRunnable* active_runnable) {
  int8_t return_value = INSTANCE->validate_insertion(active_runnable);
  if (0 == return_value) {
    INSTANCE->update_maximum_queue_depth();   // Check the queue depth
    #if defined (__MANUVR_FREERTOS)
      //if (kernel_pid) unblockThread(kernel_pid);
    #endif
    return return_value;
  }
  INSTANCE->insertion_denials++;

  if (-1 == return_value) {
    // We can't discover anything about a NULL event. Serious problems upstream.
    return return_value;
  }

  #if defined(__MANUVR_DEBUG)
    if (INSTANCE->getVerbosity() > 4) {
      StringBuilder output;
      output.concatf(
        "Kernel::validate_insertion() failed (%d) for MSG code %s\n",
        return_value,
        ManuvrMsg::getMsgTypeString(active_runnable->eventCode())
      );

      if (INSTANCE->getVerbosity() > 6) {
        active_runnable->printDebug(&output);
      }
      Kernel::log(&output);
    }
  #endif

  switch (return_value) {
    case 0:    // Clear for insertion.
      break;
    case -1:   // NULL runnable! How?!?!
    case -3:   // Pointer idempotency. THIS EXACT runnable is already enqueue.
      // So don't reclaim it.
      break;
    case -2:   // UNDEFINED event. This shall not stand, man....
    case -4:   // DEPRECATED: Message-level idempotency.
    default:   // Should never occur.
      INSTANCE->reclaim_event(active_runnable);
      break;
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
  if (return_value == nullptr) {
    INSTANCE->prealloc_starved++;
    return_value = new ManuvrRunnable(code, (EventReceiver*) INSTANCE);
  }
  else {
    return_value->repurpose(code, (EventReceiver*) INSTANCE);
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
  if (nullptr == event) return -1;                                // No NULL events.
  if (MANUVR_MSG_UNDEFINED == event->eventCode()) {
    return -2;  // No undefined events.
  }

  if (exec_queue.contains(event)) {
    // Bail out with error, because this event (which is status-bearing) cannot be in the
    //   queue more than once.
    return -3;
  }

  // Go ahead and insert.
  INSTANCE->exec_queue.insert(event, event->priority);
  return 0;
}



/**
* This is where events go to die. This function should inspect the Event and send it
*   to the appropriate place.
*
* @param active_runnable The event that has reached the end of its life-cycle.
*/
void Kernel::reclaim_event(ManuvrRunnable* active_runnable) {
  if (nullptr == active_runnable) {
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


/*******************************************************************************
* Kernel operation...                                                          *
*******************************************************************************/

// This is the splice into v2's style of event handling (callaheads).
int8_t Kernel::procCallAheads(ManuvrRunnable *active_runnable) {
  int8_t return_value = 0;
  PriorityQueue<listenerFxnPtr> *ca_queue = ca_listeners[active_runnable->eventCode()];
  if (nullptr != ca_queue) {
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
  PriorityQueue<listenerFxnPtr> *cb_queue = cb_listeners[active_runnable->eventCode()];
  if (nullptr != cb_queue) {
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
  int8_t   return_value    = 0;   // Number of Events we've processed this call.
  uint16_t msg_code_local  = 0;

  serviceSchedules();

  ManuvrRunnable *active_runnable = nullptr;  // Our short-term focus.
  uint8_t activity_count    = 0;     // Incremented whenever a subscriber reacts to an event.

  globalIRQDisable();
  while (isr_exec_queue.size() > 0) {
    active_runnable = isr_exec_queue.dequeue();

    switch (validate_insertion(active_runnable)) {
      case 0:    // Clear for insertion.
        break;
      case -1:   // NULL runnable! How?!?!
        break;
      case -2:   // UNDEFINED event. This shall not stand, man....
        break;
      case -3:   // Pointer idempotency. THIS EXACT runnable is already enqueue.
        break;
      default:   // Should never occur.
        break;
    }
  }
  globalIRQEnable();

  active_runnable = nullptr;   // Pedantic...

  /* As long as we have an open event and we aren't yet at our proc ceiling... */
  while (exec_queue.hasNext() && should_run_another_event(return_value, profiler_mark)) {
    active_runnable = exec_queue.dequeue();       // Grab the Event and remove it in the same call.
    msg_code_local = active_runnable->eventCode();  // This gets used after the life of the event.

    current_event = active_runnable;

    // Chat and measure.
    profiler_mark_0 = micros();

    procCallAheads(active_runnable);

    // Now we start notify()'ing subscribers.
    EventReceiver *subscriber;   // No need to assign.
    if (_profiler_enabled()) profiler_mark_1 = micros();

    if (active_runnable->singleTarget()) {
      activity_count += active_runnable->execute();
    }
    else {
      for (int i = 0; i < subscribers.size(); i++) {
        subscriber = subscribers.get(i);

        switch (subscriber->notify(active_runnable)) {
          case -1:  // The subscriber choked. Figure out why. Technically, this is action. Case fall-through...
            subscriber->printDebug(&local_log);
          default:   // The subscriber acted.
            activity_count++;
          case 0:   // The nominal case. No response.
            break;
        }
      }
    }
    if (_profiler_enabled()) profiler_mark_2 = micros();

    procCallBacks(active_runnable);

    #if defined(__MANUVR_EVENT_PROFILER)
      active_runnable->noteExecutionTime(profiler_mark_0, micros());
    #endif  //__MANUVR_EVENT_PROFILER

    if (0 == activity_count) {
      #ifdef __MANUVR_DEBUG
      if (getVerbosity() >= 3) local_log.concatf("\tDead event: %s\n", active_runnable->getMsgTypeString());
      #endif
      total_events_dead++;
    }

    /* Should we clean up the Event? */
    bool clean_up_active_runnable = true;  // Defaults to 'yes'.

    switch (active_runnable->callbackOriginator()) {
      case EVENT_CALLBACK_RETURN_RECYCLE:     // The originating class wants us to re-insert the event.
        #ifdef __MANUVR_DEBUG
        if (getVerbosity() > 6) local_log.concatf("Recycling %s.\n", active_runnable->getMsgTypeString());
        #endif
        switch (validate_insertion(active_runnable)) {
          case 0:    // Clear for insertion.
            exec_queue.insert(active_runnable, active_runnable->priority);
            clean_up_active_runnable = false;
            break;
          case -1:   // NULL runnable! How?!?!
            break;
          case -2:   // UNDEFINED event. This shall not stand, man....
            break;
          case -3:   // Pointer idempotency. THIS EXACT runnable is already enqueue.
            break;
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


    // All of the logic above ultimately informs this choice.
    if (clean_up_active_runnable) {
      reclaim_event(active_runnable);
    }

    total_events++;

    #if defined(__MANUVR_EVENT_PROFILER)
      // This is a stat-gathering block.
      if (_profiler_enabled()) {
        profiler_mark_3 = micros();

        TaskProfilerData* profiler_item = nullptr;
        int cost_size = event_costs.size();
        int i = 0;
        while ((nullptr == profiler_item) && (i < cost_size)) {
          if (event_costs.get(i)->msg_code == msg_code_local) {
            profiler_item = event_costs.get(i);
          }
          i++;
        }
        if (nullptr == profiler_item) {
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
        profiler_item->run_time_last    = wrap_accounted_delta(profiler_mark_2, profiler_mark_1);
        profiler_item->run_time_best    = strict_min(profiler_item->run_time_last, profiler_item->run_time_best);
        profiler_item->run_time_worst   = strict_max(profiler_item->run_time_last, profiler_item->run_time_worst);
        profiler_item->run_time_total  += profiler_item->run_time_last;
        profiler_item->run_time_average = profiler_item->run_time_total / ((profiler_item->executions) ? profiler_item->executions : 1);

        profiler_mark_2 = 0;  // Reset for next iteration.
      }
    #endif  //__MANUVR_EVENT_PROFILER

    if (exec_queue.size() > 30) {
      #ifdef __MANUVR_DEBUG
      local_log.concatf("Depth %10d \t %s\n", exec_queue.size(), ManuvrMsg::getMsgTypeString(msg_code_local));
      #endif
    }

    return_value++;   // We just serviced an Event.
  }

  if (_pending_pipes()) {
    BufferPipe* _temp_io = _pipe_io_pend.dequeue();
    while (nullptr != _temp_io) {
      _temp_io->asyncCallback();
      _temp_io = _pipe_io_pend.dequeue();
    }
    _pending_pipes(false);
  }

  total_loops++;
  current_event = nullptr;
  profiler_mark_3 = micros();
  flushLocalLog();

  uint32_t runtime_this_loop = wrap_accounted_delta(profiler_mark, profiler_mark_3);
  if (return_value > 0) {
    // We ran at-least one Runnable.
    micros_occupied += runtime_this_loop;
    consequtive_idles = max_idle_count;  // Reset the idle loop down-counter.
    if (max_events_p_loop < (uint8_t) return_value) {
      max_events_p_loop = (uint8_t) return_value;
    }
  }
  else if (0 == return_value) {
    // We did nothing this time.
    uint32_t this_idle_time = wrap_accounted_delta(profiler_mark, profiler_mark_3);
    if (this_idle_time > max_idle_loop_time) {
      max_idle_loop_time = this_idle_time;
    }

    switch (consequtive_idles) {
      case 0:
        // If we have reached our threshold for idleness, we invoke the plaform
        //   idle hook. Note that this will be invoked on EVERY LOOP until a new
        //   Runnable causes action.
        platform.idleHook();
        break;
      case 1:
        #ifdef __MANUVR_DEBUG
          if (getVerbosity() > 6) Kernel::log("Kernel idle.\n");
        #endif
        // TODO: This would be the place to implement a CPU freq scaler.
        // NOTE: No break. Still need to decrement the idle counter.
      default:
        consequtive_idles--;
        break;
    }
  }
  else {
    // there was a problem. Do nothing.
  }
  return return_value;
}



/*******************************************************************************
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
*******************************************************************************/

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
  insertion_denials  = 0;

  #if defined(__MANUVR_EVENT_PROFILER)
    while (event_costs.hasNext()) delete event_costs.dequeue();
  #endif   // __MANUVR_EVENT_PROFILER
}


// TODO: This never worked terribly well. Need to tap the timer
//   and profile to do it correctly. Still better than nothing.
float Kernel::cpu_usage() {
  return (micros_occupied / (float)(millis()*10));
}


/**
* Print the profiler to the provided buffer.
*
* @param   StringBuilder*  The buffer that this fxn will write output into.
*/
void Kernel::printProfiler(StringBuilder* output) {
  if (nullptr == output) return;
  if (getVerbosity() > 4) {
    output->concatf("-- Queue depth        \t%d\n", exec_queue.size());
    output->concatf("-- Preallocation depth\t%d\n", preallocated.size());
    output->concatf("-- Prealloc starves   \t%u\n", (unsigned long) prealloc_starved);
    output->concatf("-- events_destroyed   \t%u\n", (unsigned long) events_destroyed);
    output->concatf("-- specificity burden \t%u\n", (unsigned long) burden_of_specific);
  }

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

    #if defined(__MANUVR_EVENT_PROFILER)
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
    #endif   // __MANUVR_EVENT_PROFILER
  }
  else {
    output->concat("-- Kernel profiler disabled.\n\n");
  }

  #if defined(__MANUVR_EVENT_PROFILER)
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
  #endif   // __MANUVR_EVENT_PROFILER
}


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void Kernel::printScheduler(StringBuilder* output) {
  output->concatf("-- Total schedules:    %d\n-- Active schedules:   %d\n\n", schedules.size(), countActiveSchedules());
  output->concatf("-- _ms_elapsed         %u\n", (unsigned long) _ms_elapsed);
  if (lagged_schedules)    output->concatf("-- Lagged schedules:   %u\n", (unsigned long) lagged_schedules);
  if (_skips_observed)     output->concatf("-- Scheduler skips:    %u\n", (unsigned long) _skips_observed);
  if (_er_flag(MKERNEL_FLAG_SKIP_FAILSAFE)) {
    output->concatf("-- %u skips before fail-to-bootloader.\n", (unsigned long) MAXIMUM_SEQUENTIAL_SKIPS);
  }

  #if defined(__MANUVR_DEBUG)
  for (int i = 0; i < schedules.size(); i++) {
    schedules.recycle()->printDebug(output);
  }
  #endif
}


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void Kernel::printDebug(StringBuilder* output) {
  if (nullptr == output) return;
  EventReceiver::printDebug(output);

  //output->concatf("-- our_mem_addr:             %p\n", this);
  if (subscribers.size() > 0) {
    output->concatf("-- Subscribers: (%d total):\n", subscribers.size());
    for (int i = 0; i < subscribers.size(); i++) {
      output->concatf("\t %d: %s\n", i, subscribers.get(i)->getReceiverName());
    }
    output->concat("\n");
  }
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
* Boot done finished-up.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t Kernel::attached() {
  EventReceiver::attached();
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
  switch (event->eventCode()) {
    case MANUVR_MSG_SYS_BOOT_COMPLETED:
      // TODO: We should probably recycle the BOOT_COMPLETE until nothing responds to it.
      maskableInterrupts(true);  // Now configure interrupts, lift interrupt masks, and let the madness begin.
      if (getVerbosity() > 4) Kernel::log("Boot complete.\n");
      if (_logger) _logger->toCounterparty(ManuvrPipeSignal::FLUSH, nullptr);
      break;

    case MANUVR_MSG_SYS_CONF_SAVE:
      platform.storeConf(event->getArgs());
      break;
    case MANUVR_MSG_SYS_REBOOT:
      if (_logger) _logger->toCounterparty(ManuvrPipeSignal::FLUSH, nullptr);
      platform.reboot();
      break;
    case MANUVR_MSG_SYS_SHUTDOWN:
      if (_logger) _logger->toCounterparty(ManuvrPipeSignal::FLUSH, nullptr);
      platform.seppuku();  // TODO: We need to distinguish between this and SYSTEM shutdown for linux.
      break;
    case MANUVR_MSG_SYS_BOOTLOADER:
      //if (_logger) _logger->toCounterparty(ManuvrPipeSignal::FLUSH, nullptr);
      platform.jumpToBootloader();
      break;

    default:
      break;
  }
  return return_value;
}



int8_t Kernel::notify(ManuvrRunnable *active_runnable) {
  int8_t return_value = 0;

  switch (active_runnable->eventCode()) {
    case MANUVR_MSG_SYS_REBOOT:
    case MANUVR_MSG_SYS_SHUTDOWN:
    case MANUVR_MSG_SYS_BOOTLOADER:
      // These messages codes, we will capture the callback.
      active_runnable->setOriginator(this);
      return_value++;
      break;

    #if defined(MANUVR_CONSOLE_SUPPORT)
      case MANUVR_MSG_USER_DEBUG_INPUT:
        if (active_runnable->argCount()) {
          // If the event came with a StringBuilder, concat it onto the last_user_input.
          StringBuilder* _tmp = nullptr;
          if (0 == active_runnable->getArgAs(&_tmp)) {
            _route_console_input(_tmp);
          }
        }
        return_value++;
        break;
    #endif  // MANUVR_CONSOLE_SUPPORT

    case MANUVR_MSG_SYS_ADVERTISE_SRVC:  // Some service is annoucing its arrival.
    case MANUVR_MSG_SYS_RETRACT_SRVC:    // Some service is annoucing its departure.
      if (0 < active_runnable->argCount()) {
        EventReceiver* er_ptr;
        if (0 == active_runnable->getArgAs(&er_ptr)) {
          if (MANUVR_MSG_SYS_ADVERTISE_SRVC == active_runnable->eventCode()) {
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

    case MANUVR_MSG_SYS_CONF_LOAD:
      break;
    case MANUVR_MSG_SYS_CONF_SAVE:
      break;

    case MANUVR_MSG_SYS_ISSUE_LOG_ITEM:
      if (nullptr != _logger) {
        StringBuilder *log_item;
        if (0 == active_runnable->getArgAs(&log_item)) {
          _logger->toCounterparty(log_item, MEM_MGMT_RESPONSIBLE_BEARER);
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
    if (current->scheduleEnabled()) {
      return_value++;
    }
  }
  return return_value;
}



/**
*  Call this function to create a new schedule with the given period, a given number of repititions, and with a given function call.
*
*  Will automatically set the schedule active, provided the input conditions are met.
*  Returns the newly-created Runnable on success, or 0 on failure.
*/
ManuvrRunnable* Kernel::createSchedule(uint32_t sch_period, int16_t recurrence, bool ac, FxnPointer sch_callback) {
  ManuvrRunnable* return_value = nullptr;
  if (sch_period > 1) {
    if (sch_callback) {
      if (ac) {
        // If the schedule is supposed to auto-clear, we will pull it from our
        //   preallocation pool, since we know that it will eventually expire.
        // TODO: Make it happen.
        return_value = new ManuvrRunnable(recurrence, sch_period, ac, sch_callback);
      }
      else {
        // Without that assurance, we heap it. It may never be returned.
        return_value = new ManuvrRunnable(recurrence, sch_period, ac, sch_callback);
      }

      if (nullptr != return_value) {  // Did we actually malloc() successfully?
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
  ManuvrRunnable* return_value = nullptr;
  if (sch_period > 1) {
    return_value = new ManuvrRunnable(recurrence, sch_period, ac, ori);
    if (return_value) {  // Did we actually malloc() successfully?
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
  if (obj) {
    if (obj != current_event) {
      obj->isScheduled(false);
      schedules.remove(obj);
    }
    else {
      obj->autoClear(true);
      obj->alterScheduleRecurrence(0);
    }
    return true;
  }
  return false;
}

bool Kernel::addSchedule(ManuvrRunnable *obj) {
  if (obj) {
    if (!schedules.contains(obj)) {
      obj->isScheduled(true);
      schedules.insert(obj);
    }
    return true;
  }
  return false;
}

uint32_t Kernel::lagged_schedules = 0;

/**
* This is the function that is called from the main loop to offload big
*  tasks into idle CPU time. If many scheduled items have fired, function
*  will churn through all of them. The presumption is that they are
*  latency-sensitive.
*/
int Kernel::serviceSchedules() {
  if (!platform.booted() || (0 == _ms_elapsed)) return -1;
  int return_value = 0;
  uint32_t mse = _ms_elapsed;  // Concurrency....
  _ms_elapsed = 0;

  int x = schedules.size();
  ManuvrRunnable *current;

  for (int i = 0; i < x; i++) {
    current = schedules.recycle();
    if (current->scheduleEnabled()) {
      switch (current->applyTime(mse)) {
        case 1:   // Schedule should be exec'd and retained.
          Kernel::staticRaiseEvent(current);
          break;
        case -1:  // Schedule should be dropped and executed.
          Kernel::staticRaiseEvent(current);
        case -2:  // Schedule should be dropped without execution.
          removeSchedule(current);
          x--;
          break;
        case 0:   // Nominal outcome. No action.
        default:  // Nonsense.
          break;
      }
    }
  }

  // We just ran a loop. Punch the bistable swtich.
  _skip_detected(false);
  _skips_observed = 0;

  return return_value;
}


/****************************************************************************************************
* The code below is related to accepting and parsing user input. It is only relevant if console     *
*   support is enabled.                                                                             *
****************************************************************************************************/
#if defined(MANUVR_CONSOLE_SUPPORT)

/**
* Responsible for taking any accumulated console input, doing some basic
*   error-checking, and routing it to its intended target.
*/
int8_t Kernel::_route_console_input(StringBuilder* last_user_input) {
  StringBuilder _raw_from_console;  // We do this to avoid leaks.
  // Now we take the data from the buffer so that further input isn't lost. JIC.
  _raw_from_console.concatHandoff(last_user_input);

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
      if (nullptr != subscriber) {
        subscriber->procDirectDebugInstruction(&_raw_from_console);
      }
      else if (getVerbosity() > 2) {
        local_log.concatf("No such subscriber: %d\n", subscriber_idx);
      }
    }
  }

  flushLocalLog();
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
    case 'b':
      if (128 == temp_int) {
        Kernel::raiseEvent(('B' == c ? MANUVR_MSG_SYS_BOOTLOADER : MANUVR_MSG_SYS_REBOOT), this);
      }
      else {
        local_log.concatf("Will only %s if the number '128' follows the command.\n", ('B' == c) ? "jump to bootloader" : "reboot");
      }
      break;
    case 'P':
    case 'p':
      local_log.concatf("Kernel profiling %sabled.\n", ('P' == c) ? "en" : "dis");
      profiler('P' == c);
      break;

    case 'y':    // Power mode.
      {
        ManuvrRunnable* event = returnEvent(MANUVR_MSG_SYS_POWER_MODE);
        event->addArg((uint8_t) temp_int);
        EventReceiver::raiseEvent(event);
        local_log.concatf("Power mode is now %d.\n", temp_int);
      }
      break;

    #if defined(MANUVR_STORAGE)
      case 'S':
        local_log.concat("Attempting configuration save...\n");
        Kernel::raiseEvent(MANUVR_MSG_SYS_CONF_SAVE, this);
        break;
    #endif // MANUVR_STORAGE

    #if defined(__MANUVR_DEBUG)
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
      temp_int = (temp_int <= 0) ? PLATFORM_RNG_CARRY_CAPACITY : temp_int;
      for (uint8_t i = 0; i < temp_int; i++) {
        local_log.concatf("Random number: 0x%08x\n", randomInt());
      }
      break;

    case 'I':
      {
        Identity* ident = nullptr;
        switch (temp_int) {
          case 1:
            break;
          case 0:
            break;
          default:
            break;
        }
      }
      break;

    case 'i':   // Debug prints.
      switch (temp_int) {
        case 2:
          printProfiler(&local_log);
          break;

        case 3:
          platform.printDebug(&local_log);
          break;

        #if defined(__HAS_CRYPT_WRAPPER)
          case 4:
            platform.printCryptoOverview(&local_log);
            break;
        #endif

        case 5:
          printScheduler(&local_log);
          break;

        case 6:
          {
            const IdentFormat* ident_types = Identity::supportedNotions();
            local_log.concat("-- Supported notions of identity: \n");
            while (IdentFormat::UNDETERMINED != *ident_types) {
              local_log.concatf("\t %s\n", Identity::identityTypeString(*ident_types++));
            }
          }
          break;
        case 7:
          Identity::staticToString(platform.selfIdentity(), &local_log);
          break;

        default:
          printDebug(&local_log);
          break;
      }
      break;
    #endif //__MANUVR_DEBUG
    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }

  input->clear();
  flushLocalLog();
}
#endif  //MANUVR_CONSOLE_SUPPORT
