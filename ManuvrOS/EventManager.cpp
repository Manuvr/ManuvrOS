#include "FirmwareDefs.h"
#include <ManuvrOS/Kernel.h>
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
Kernel* Kernel::INSTANCE = NULL;
PriorityQueue<ManuvrEvent*> Kernel::isr_event_queue;

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
  {  MANUVR_MSG_SCHED_DUMP_META      , 0x0000,               "SCHED_DUMP_META"      , MSG_ARGS_NO_ARGS }, // Tell the Scheduler to dump its meta metrics.
  {  MANUVR_MSG_SCHED_DUMP_SCHEDULES , 0x0000,               "SCHED_DUMP_SCHEDULES" , MSG_ARGS_NO_ARGS }, // Tell the Scheduler to dump schedules.
  {  MANUVR_MSG_SCHED_WIPE_PROFILER  , 0x0000,               "SCHED_WIPE_PROFILER"  , MSG_ARGS_NO_ARGS }, // Tell the Scheduler to wipe its profiler data. Pass PIDs to be selective.
  {  MANUVR_MSG_SCHED_DEFERRED_EVENT , 0x0000,               "SCHED_DEFERRED_EVENT" , MSG_ARGS_NO_ARGS }, // Tell the Scheduler to broadcast the attached Event so many ms into the future.
                                   
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
  INSTANCE           = this;
  __kernel           = this;
  current_event      = NULL;
  subscribe(this);   // We subscribe ourselves to events.
  setVerbosity((int8_t) 1);  // TODO: Why does this crash ViamSonus?
  profiler(false);

  max_queue_depth     = 0;
  total_loops         = 0;
  total_events        = 0;
  total_events_dead   = 0;
  micros_occupied     = 0;
  max_events_per_loop = 2;

  for (int i = 0; i < EVENT_MANAGER_PREALLOC_COUNT; i++) {
    /* We carved out a space in our allocation for a pool of events. Ideally, this would be enough
        for most of the load, most of the time. If the preallocation ends up being insufficient to
        meet demand, new Events will be created on the heap. But the events that we are dealing
        with here are special. They should remain circulating (or in standby) for the lifetime of 
        this class. */
    _preallocation_pool[i].returnToPrealloc(true);
    preallocated.insert(&_preallocation_pool[i]);
  }

  ManuvrMsg::registerMessages(message_defs, sizeof(message_defs) / sizeof(MessageTypeDef));
}

/**
* Destructor. Should probably never be called.
*/
Kernel::~Kernel() {
}



/****************************************************************************************************
* Logging members...                                                                                *
****************************************************************************************************/
StringBuilder Kernel::log_buffer;

/*
* Logger pass-through functions. Please mind the variadics...
*/
volatile void Kernel::log(int severity, const char *str) {
  if (!INSTANCE->verbosity) return;
  log_buffer.concat(str);
}

volatile void Kernel::log(char *str) {
  if (!INSTANCE->verbosity) return;
  log_buffer.concat(str);
}

volatile void Kernel::log(const char *str) {
  if (!INSTANCE->verbosity) return;
  log_buffer.concat(str);
}

volatile void Kernel::log(const char *fxn_name, int severity, const char *str, ...) {
  if (!INSTANCE->verbosity) return;
  log_buffer.concatf("%d  %s:\t", severity, fxn_name);
  va_list marker;
  
  va_start(marker, str);
  log_buffer.concatf(str, marker);
  va_end(marker);
}

volatile void Kernel::log(StringBuilder *str) {
  if (!INSTANCE->verbosity) return;
  log_buffer.concatHandoff(str);
}

/****************************************************************************************************
* Big pile of ugly..........                                                                        *
* Big pile of ugly..........                                                                        *
* Big pile of ugly..........                                                                        *
* Big pile of ugly..........                                                                        *
* Big pile of ugly..........                                                                        *
* Big pile of ugly..........                                                                        *
* Big pile of ugly..........                    These are the things left over                      *
* Big pile of ugly..........                    from StaticHub. They need to                        *
* Big pile of ugly..........                    justify their existance or DIAF.                    *
* Big pile of ugly..........                                                                        *
* Big pile of ugly..........                       ---J. Ian Lindsay   Tue Dec 01 01:28:09 MST 2015 *
* Big pile of ugly..........                                                                        *
* Big pile of ugly..........                                                                        *
* Big pile of ugly..........                                                                        *
* Big pile of ugly..........                                                                        *
* Big pile of ugly..........                                                                        *
* Big pile of ugly..........                                                                        *
****************************************************************************************************/


int8_t Kernel::bootstrap() {
  subscribe((EventReceiver*) &__scheduler);    // Subscribe the Scheduler.
  
//  mcp73833 = new MCP73833(9, 10);
  
  // Setup the first i2c adapter and Subscribe it to Kernel.
//  i2c     = new I2CAdapter(0);
//  mgc3130 = new MGC3130(16, 17);

//  ina219      = new INA219(0x4A);
//  adp8866     = new ADP8866(7, 8, 0x27);

//  event_manager.subscribe((EventReceiver*) i2c);
//  event_manager.subscribe((EventReceiver*) adp8866);

//  ((I2CAdapter*) i2c)->addSlaveDevice(ina219);
//  ((I2CAdapter*) i2c)->addSlaveDevice(adp8866);
  
  platformInit();    // Start the platform-specific machinery.
  
//  mgc3130->init();

  //ManuvrEvent *boot_completed_ev = Kernel::returnEvent(MANUVR_MSG_SYS_BOOT_COMPLETED);
  raiseEvent(MANUVR_MSG_SYS_BOOT_COMPLETED, NULL);
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
int8_t Kernel::callback_proc(ManuvrEvent *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */ 
  int8_t return_value = event->eventManagerShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;
  
  /* Some class-specific set of conditionals below this line. */
  switch (event->event_code) {
    case MANUVR_MSG_SYS_BOOT_COMPLETED:
      Kernel::log("Boot complete.\n");
      boot_completed = true;
      break;
    default:
      break;
  }
  return return_value;
}



int8_t Kernel::notify(ManuvrEvent *active_event) {
  int8_t return_value = 0;
  
  switch (active_event->event_code) {
    case MANUVR_MSG_USER_DEBUG_INPUT:
      last_user_input.concatHandoff(&usb_rx_buffer);
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
      
    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }
  return return_value;
}                             




/*
* This is called from the USB peripheral. It is called when the short static
* character array that forms the USB rx buffer is either filled up, or we see
* a new-line character on the wire.
*/
void Kernel::feedUSBBuffer(uint8_t *buf, int len, bool terminal) {
  usb_rx_buffer.concat(buf, len);

  if (terminal) {
    // If the ISR saw a CR or LF on the wire, we tell the parser it is ok to
    // run in idle time.
    ManuvrEvent* event = returnEvent(MANUVR_MSG_USER_DEBUG_INPUT);
    //TODO: Crashes after eating StaticHub. event->specific_target = (EventReceiver*) this;
    Kernel::staticRaiseEvent(event);
  }
}



void Kernel::procDirectDebugInstruction(StringBuilder* input) {
  char *str = (char *) input->string();
  char c = *(str);
  uint8_t temp_byte = 0;        // Many commands here take a single integer argument.
  if (*(str) != 0) {
    temp_byte = atoi((char*) str+1);
  }
  ManuvrEvent *event = NULL;  // Pitching events is a common thing in this fxn...
  
  StringBuilder parse_mule;
  
  switch (c) {
    case 'B':
      if (temp_byte == 128) {
        Kernel::raiseEvent(MANUVR_MSG_SYS_BOOTLOADER, NULL);
        break;
      }
      local_log.concatf("Will only jump to bootloader if the number '128' follows the command.\n");
      break;
    case 'b':
      if (temp_byte == 128) {
        Kernel::raiseEvent(MANUVR_MSG_SYS_REBOOT, NULL);
        break;
      }
      local_log.concatf("Will only reboot if the number '128' follows the command.\n");
      break;

    case '6':        // Read so many random integers...
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

#ifdef __MANUVR_CONSOLE_SUPPORT

    case 'u':
      switch (temp_byte) {
        case 1:
          Kernel::raiseEvent(MANUVR_MSG_SELF_DESCRIBE, NULL);
          break;
        case 3:
          Kernel::raiseEvent(MANUVR_MSG_LEGEND_MESSAGES, NULL);
          break;
        default:
          break;
      }
      break;

    case 'y':    // Power mode.
      switch (temp_byte) {
        case 255:
          break;
        default:
          event = Kernel::returnEvent(MANUVR_MSG_SYS_POWER_MODE);
          event->addArg((uint8_t) temp_byte);
          EventReceiver::raiseEvent(event);
          local_log.concatf("Power mode is now %d.\n", temp_byte);
          break;
      }
      break;

    case 'i':   // Debug prints.
      if (1 == temp_byte) {
        local_log.concat("Kernel profiling enabled.\n");
        profiler(true);
      }
      else if (2 == temp_byte) {
        printDebug(&local_log);
      }
      else if (6 == temp_byte) {
        local_log.concat("Kernel profiling disabled.\n");
        profiler(false);
      }
      else {
        printDebug(&local_log);
      }
      break;


    case 'v':           // Set log verbosity.
      parse_mule.concat(str);
      parse_mule.drop_position(0);
      
      event = new ManuvrEvent(MANUVR_MSG_SYS_LOG_VERBOSITY);
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
    #endif

    default:
      // TODO: Cycle through the subscribers and check their names against the input.
      //   If a match is found, pass the command into that class for handling.
      break;
  }
  if (local_log.length() > 0) Kernel::log(&local_log);
  last_user_input.clear();
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
int8_t Kernel::subscribe(EventReceiver *client, uint8_t priority) {
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
* Static member functions for posting events.                                                       *
****************************************************************************************************/


/**
* Used to add an event to the idle queue. Use this for simple events that don't need args.
*
* @param  code  The identity code of the event to be raised.
* @param  cb    An optional callback pointer to be called when this event is finished.
* @return -1 on failure, and 0 on success.
*/
int8_t Kernel::raiseEvent(uint16_t code, EventReceiver* cb) {
  int8_t return_value = 0;
  
  // We are creating a new Event. Try to snatch a prealloc'd one and fall back to malloc if needed.
  ManuvrEvent* nu = INSTANCE->preallocated.dequeue();
  if (nu == NULL) {
    INSTANCE->prealloc_starved++;
    nu = new ManuvrEvent(code, cb);
  }
  else {
    nu->repurpose(code);
    nu->callback = cb;
  }

  int ret_val = INSTANCE->validate_insertion(nu);
  if (0 == ret_val) {
    INSTANCE->event_queue.insert(nu);
    INSTANCE->update_maximum_queue_depth();   // Check the queue depth 
  }
  else {
    if (INSTANCE->verbosity > 4) {
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
int8_t Kernel::staticRaiseEvent(ManuvrEvent* event) {
  int8_t return_value = 0;
  if (0 == INSTANCE->validate_insertion(event)) {
    INSTANCE->event_queue.insert(event, event->priority);

    // Check the queue depth.
    INSTANCE->update_maximum_queue_depth();
  }
  else {
    if (INSTANCE->verbosity > 4) {
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
bool Kernel::abortEvent(ManuvrEvent* event) {
  if (!INSTANCE->event_queue.remove(event)) {
    // Didn't find it? Check  the isr_queue...
    if (!INSTANCE->isr_event_queue.remove(event)) {
      return false;
    }
  }
  return true;
}


// TODO: It would be bettter to put a semaphore on the evvent_queue and set it in the idel loop.
//       That way, we could check for it here, and have the (probable) possibility of not incurring
//       the cost for merging these two queues if we don't have to.
//             ---J. Ian Lindsay   Fri Jul 03 16:54:14 MST 2015
int8_t Kernel::isrRaiseEvent(ManuvrEvent* event) {
  int return_value = -1;
#ifdef STM32F4XX
  asm volatile ("cpsie i");
#elif defined(ARDUINO)
  cli();
#endif
  return_value = isr_event_queue.insertIfAbsent(event, event->priority);
#ifdef STM32F4XX
  asm volatile ("cpsid i");
#elif defined(ARDUINO)
  sei();
#endif
  return return_value;
}




/**
* Factory method. Returns a preallocated Event.
* After we return the event, we lose track of it. So if the caller doesn't ever
*   call raiseEvent(), the Event we return will become a memory leak.
* The event we retun will have a callback field populated with a ref to Kernel.
*   So if a caller needs their own reference in that slot, caller will need to do it.
*
* @param  code  The desired identity code of the event.
* @return A pointer to the prepared event. Will not return NULL unless we are out of memory.
*/
ManuvrEvent* Kernel::returnEvent(uint16_t code) {
  // We are creating a new Event. Try to snatch a prealloc'd one and fall back to malloc if needed.
  ManuvrEvent* return_value = INSTANCE->preallocated.dequeue();
  if (return_value == NULL) {
    INSTANCE->prealloc_starved++;
    return_value = new ManuvrEvent(code, (EventReceiver*) INSTANCE);
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
int8_t Kernel::validate_insertion(ManuvrEvent* event) {
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


bool Kernel::containsPreformedEvent(ManuvrEvent* event) {
  return event_queue.contains(event);
}



/**
* This is where events go to die. This function should inspect the Event and send it
*   to the appropriate place.
*
* @param active_event The event that has reached the end of its life-cycle.
*/
void Kernel::reclaim_event(ManuvrEvent* active_event) {
  if (NULL == active_event) {
    return;
  }
  bool reap_current_event = active_event->eventManagerShouldReap();
  //if (verbosity > 5) {
  //  local_log.concatf("We will%s be reaping %s.\n", (reap_current_event ? "":" not"), active_event->getMsgTypeString());
  //  Kernel::log(&local_log);
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
    //  if (verbosity > 6) local_log.concat("Kernel::reclaim_event(): Doing nothing. Hope its managed elsewhere.\n");
    //}
  }
  
  if (local_log.length() > 0) {    Kernel::log(&local_log);  }
}




// This is the splice into v2's style of event handling (callaheads).
int8_t Kernel::procCallAheads(ManuvrEvent *active_event) {
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
int8_t Kernel::procCallBacks(ManuvrEvent *active_event) {
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
*     call. We don't want to cause bad re-entrancy problem in the Scheduler by spending
*     all of our time here (although we might re-work the Scheduler to make this acceptable).
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

  ManuvrEvent *active_event = NULL;  // Our short-term focus.
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
            //if (verbosity > 1) local_log.concatf("Kernel found a possible mistake. Unexpected return case from callback_proc.\n");
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

  if (local_log.length() > 0) Kernel::log(&local_log);
  current_event = NULL;
  return return_value;
}


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


/**
* Turns the profiler on or off. Clears its collected stats regardless.
*
* @param   enabled  If true, enables the profiler. If false, disables it.
*/
void Kernel::profiler(bool enabled) {
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


/**
* Print the profiler to the provided buffer.
*
* @param   StringBuilder*  The buffer that this fxn will write output into.
*/
void Kernel::printProfiler(StringBuilder* output) {
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

    output->concat("\n\t\t Execd \t\t Event \t\t total us   average     worst    best      last\n");
    for (int i = 0; i < x; i++) {
      profiler_item = event_costs.get(i);
      stat_mode     = event_costs.getPriority(i);
      
      output->concatf("\t (%10d)\t%18s  %9d %9d %9d %9d %9d\n", stat_mode, ManuvrMsg::getMsgTypeString(profiler_item->msg_code), (unsigned long) profiler_item->run_time_total, (unsigned long) profiler_item->run_time_average, (unsigned long) profiler_item->run_time_worst, (unsigned long) profiler_item->run_time_best, (unsigned long) profiler_item->run_time_last);
    }
    output->concatf("\n\t CPU use by clock: %f\n\n", (double)cpu_usage());
  }
  else {
    output->concat("-- Kernel profiler disabled.\n\n");
  }
}


float Kernel::cpu_usage() {
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

/**
* There is a NULL-check performed upstream for the scheduler member. So no need 
*   to do it again here.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t Kernel::bootComplete() {
  EventReceiver::bootComplete();
  boot_completed = true;
  maskableInterrupts(true);  // Now configure interrupts, lift interrupt masks, and let the madness begin.
  return 1;
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
  
  currentDateTime(output);
  output->concatf("\n-- %s v%s    Build date: %s %s\n--\n", IDENTITY_STRING, VERSION_STRING, __DATE__, __TIME__);
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

  printProfiler(output);
}



