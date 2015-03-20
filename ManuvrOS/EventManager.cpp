#include "EventManager.h"
#ifndef TEST_BENCH
  #include "StaticHub/StaticHub.h"
#else
  #include "demo/StaticHub.h"
#endif



/****************************************************************************************************
* Static initializers                                                                               *
****************************************************************************************************/
EventManager* EventManager::INSTANCE = NULL;


/****************************************************************************************************
* Class management                                                                                  *
****************************************************************************************************/

/**
* Vanilla constructor.
*/
EventManager::EventManager() {
  EventReceiver::__class_initializer();
  INSTANCE       = this;
  current_event = NULL;
  setVerbosity((int8_t) 0);  // TODO: Why does this crash ViamSonus?
  boot_completed = false;
  profiler(true);
  
  for (int i = 0; i < EVENT_MANAGER_PREALLOC_COUNT; i++) {
    /* We carved out a space in our allocation for a pool of events. Ideally, this would be enough
        for most of the load, most of the time. If the preallocation ends up being insufficient to
        meet demand, new Events will be created on the heap. But the events that we are dealing
        with here are special. They should remain circulating (or in standby) for the lifetime of 
        this class. */
    _preallocation_pool[i].returnToPrealloc(true);
    preallocated.insert(&_preallocation_pool[i]);
  }
}

/**
* Destructor. Should probably never be called.
*/
EventManager::~EventManager() {
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
int8_t EventManager::subscribe(EventReceiver *client) {
  if (NULL == client) return -1;

  client->setVerbosity(DEFAULT_CLASS_VERBOSITY);
  int8_t return_value = subscribers.insert(client);
  if (boot_completed) {
    // This subscriber is joining us after bootup. Call its bootComplete() fxn to cause it to init.
    //client->notify(returnEvent(MANUVR_MSG_SYS_BOOT_COMPLETED));
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
int8_t EventManager::subscribe(EventReceiver *client, uint8_t priority) {
  if (NULL == client) return -1;

  client->setVerbosity(DEFAULT_CLASS_VERBOSITY);
  int8_t return_value = subscribers.insert(client, priority);
  if (verbosity > 4) {
    local_log.concat(client->getReceiverName());
    local_log.concat(" tried to add itself with a specd priority.\n");
    StaticHub::log(&local_log);
  }
  if (boot_completed) {
    // This subscriber is joining us after bootup. Call its bootComplete() fxn to cause it to init.
    //client->notify(returnEvent(MANUVR_MSG_SYS_BOOT_COMPLETED));
  }
  return ((return_value >= 0) ? 0 : -1);
}


/**
* A class calls this to unsubscribe.
*
* @param  client    The class that will no longer be listening for Events.
* @return 0 on success and -1 on failure.
*/
int8_t EventManager::unsubscribe(EventReceiver *client) {
  if (NULL == client) return -1;
  return (subscribers.remove(client) ? 0 : -1);
}


EventReceiver* EventManager::getSubscriberByName(const char* search_str) {
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

void EventManager::update_maximum_queue_depth() {
  max_queue_depth = max(event_queue.size(), (int) max_queue_depth);
}


/**
* Used to add an event to the idle queue. Use this for simple events that don't need args.
*
* @param  code  The identity code of the event to be raised.
* @param  cb    An optional callback pointer to be called when this event is finished.
* @return -1 on failure, and 0 on success.
*/
int8_t EventManager::raiseEvent(uint16_t code, EventReceiver* cb) {
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
    if (INSTANCE->profiler_enabled) {
    }
    INSTANCE->update_maximum_queue_depth();   // Check the queue depth 
  }
  else {
    if (INSTANCE->verbosity > 3) {
      StaticHub::log("An incoming event failed validate_insertion(). Trapping it...\n");
      INSTANCE->insertion_denials++;
    }
    INSTANCE->reclaim_event(nu);
    return_value = -1;
  }
  return return_value;
}



/**
* Used to add a pre-formed event to the idle queue. Use this when a sophisticated event
*   needs to be formed elsewhere and passed in. EventManager will only insert it into the
*   queue in this case.
*
* @param   event  The event to be inserted into the idle queue.
* @return  -1 on failure, and 0 on success.
*/
int8_t EventManager::staticRaiseEvent(ManuvrEvent* event) {
  int8_t return_value = 0;
  if (0 == INSTANCE->validate_insertion(event)) {
    //INSTANCE->event_queue.insert(event, event->priority);
    INSTANCE->event_queue.insert(event, 0);
    if (INSTANCE->profiler_enabled) {
    }
    // Check the queue depth.
    INSTANCE->update_maximum_queue_depth();
  }
  else {
    if (INSTANCE->verbosity > 3) {
      StaticHub::log("Static: An incoming event failed validate_insertion(). Trapping it...\n");
      INSTANCE->insertion_denials++;
    }
    INSTANCE->reclaim_event(event);
    return_value = -1;
  }
  return return_value;
}


/**
* Factory method. Returns a preallocated Event.
* After we return the event, we lose track of it. So if the caller doesn't ever
*   call raiseEvent(), the Event we return will become a memory leak.
* The event we retun will have a callback field populated with a ref to EventManager.
*   So if a caller needs their own reference in that slot, caller will need to do it.
*
* @param  code  The desired identity code of the event.
* @return A pointer to the prepared event. Will not return NULL unless we are out of memory.
*/
ManuvrEvent* EventManager::returnEvent(uint16_t code) {
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
int8_t EventManager::validate_insertion(ManuvrEvent* event) {
  if (NULL == event) return -1;                                   // No NULL events.
  if (MANUVR_MSG_UNDEFINED == event->event_code) {
    return -2;  // No undefined events.
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
void EventManager::reclaim_event(ManuvrEvent* active_event) {
  if (NULL == active_event) {
    if (verbosity > 3) StaticHub::log("EventManager::reclaim_event() was passed a NULL event. This would have crashed us.\n");
    return;
  }
  bool reap_current_event = active_event->eventManagerShouldReap();
  if (verbosity > 5) {
    local_log.concatf("We will%s be reaping %s.\n", (reap_current_event ? "":" not"), active_event->getMsgTypeString());
    StaticHub::log(&local_log);
  }

  if (reap_current_event) {                   // If we are to reap this event...
    if (verbosity > 5) local_log.concat("EventManager::reclaim_event(): About to reap,\n");
    delete active_event;                      // ...free() it...
    events_destroyed++;
    //discarded.insert(active_event);
    burden_of_specific++;                     // ...and note the incident.
  }
  else {                                      // If we are NOT to reap this event...
    if (active_event->returnToPrealloc()) {   // ...is it because we preallocated it?
      if (verbosity > 5) local_log.concat("EventManager::reclaim_event(): Returning the event to prealloc,\n");
      active_event->clearArgs();              // If so, wipe the Event...
      preallocated.insert(active_event);      // ...and return it to the preallocate queue.
    }                                         // Otherwise, we let it drop and trust some other class is managing it.
    else {
      if (verbosity > 5) local_log.concat("EventManager::reclaim_event(): Doing nothing. Hope its managed elsewhere.\n");
    }
  }
  
  if (local_log.length() > 0) {    StaticHub::log(&local_log);  }
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
int8_t EventManager::procIdleFlags() {
  uint32_t profiler_mark   = (profiler_enabled) ? micros() : 0;   // Profiling requests...
  uint32_t profiler_mark_0 = 0;   // Profiling requests...
  uint32_t profiler_mark_1 = 0;   // Profiling requests...
  uint32_t profiler_mark_2 = 0;   // Profiling requests...
  uint32_t profiler_mark_3 = 0;   // Profiling requests...
  int8_t return_value      = 0;   // Number of Events we've processed this call.
  uint16_t msg_code_local  = 0;   
//if (boot_completed) crash_log.concat("procIdleFlags()\n");

  ManuvrEvent *active_event = NULL;  // Our short-term focus.
  uint8_t activity_count    = 0;     // Incremented whenever a subscriber reacts to an event.

  /* As long as we have an open event and we aren't yet at our proc ceiling... */
  while (event_queue.hasNext() && (EVENT_MANAGER_MAX_EVENTS_PER_LOOP > return_value)) {
    active_event = event_queue.dequeue();       // Grab the Event and remove it in the same call.
    msg_code_local = active_event->event_code;  // This gets used after the life of the event.
    
    current_event = active_event;
    
    // Chat and measure.
    if (verbosity >= 5) local_log.concatf("Servicing: %s\n", active_event->getMsgTypeString());
    if (profiler_enabled) profiler_mark_0 = micros();
//if (boot_completed) crash_log.concat("Servicing: \n");
//if (boot_completed) active_event->printDebug(&crash_log);
    // LOUD REMINDER! The switch() block below is the EventManager reacting to Events. Not related to
    //   Event processing. Because this class extends EventReceiver, we should technically call its own
    //   notify(), but with no need to put itself in its own subscriber queue. That would be puritanical
    //   but silly.
    // Instead, we'll take advantage of the position by treating EventManager as if it were the head and
    //   tail of the subscriber queue.     ---J. Ian Lindsay 2014.11.05
    if (active_event->callback != (EventReceiver*) this) {    // Don't react to our own internally-generated events.
//if (boot_completed) crash_log.concat("Beginning notify (EventReceiver)...\n");
      switch (active_event->event_code) {
        case MANUVR_MSG_LEGEND_MESSAGES:     // Dump the message definitions.
          if (0 == active_event->args.size()) {   // Only if we are seeing a request.
            StringBuilder tmp_sb;
            if (ManuvrMsg::getMsgLegend(&tmp_sb) ) {
              unsigned char *tmp_ptr;
              int tmp_len = tmp_sb.str_heap_ref(&tmp_ptr);
              // Normally, I dislike this style, but is illustrative...
              active_event->markArgForReap(
                active_event->addArg(
                  (void*) tmp_ptr, tmp_len
                ), true
              );

              if (verbosity >= 7) local_log.concatf("EventManager\t Sent message definition map. Size %d.\n", tmp_sb.length());
              activity_count++;
            }
          }
          else {
            // We may be receiving a message definition message from another party.
            // For now, we've decided to handle this in XenoSession.
          }
          break;
        default:
          // See the notes above. We still have to behave like any other EventReceiver....
          return_value += EventReceiver::notify(active_event);
          break;
      }
    }
//if (boot_completed) crash_log.concat("Beginning notify...\n");
    
    // Now we start notify()'ing subscribers.
    EventReceiver *subscriber;   // No need to assign.
    if (profiler_enabled) profiler_mark_1 = micros();
    
    if (NULL != active_event->specific_target) {
//if (boot_completed) crash_log.concatf("Specificly targeting %s\n", active_event->specific_target->getReceiverName());
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
//if (boot_completed) crash_log.concatf("Notify %s\n", subscriber->getReceiverName());
        
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
    
    if (0 == activity_count) {
      if (verbosity >= 3) local_log.concatf("\tDead event... No subscriber acknowledges %s\n", active_event->getMsgTypeString());
      total_events_dead++;
    }

//if (boot_completed) crash_log.concat("Beginning callback...\n");
    /* Clean up the Event. */
    if (NULL != active_event->callback) {
      if ((EventReceiver*) this != active_event->callback) {  // We don't want to invoke our own callback.
        /* If the event has a valid callback, do the callback dance and take instruction
           from the return value. */
        //   if (verbosity >=7) output.concatf("specific_event_callback returns %d\n", active_event->callback->callback_proc(active_event));
        switch (active_event->callback->callback_proc(active_event)) {
          case EVENT_CALLBACK_RETURN_ERROR:       // Something went wrong. Should never occur.
          case EVENT_CALLBACK_RETURN_UNDEFINED:   // The originating class doesn't care what we do with the event.
            if (verbosity > 5) local_log.concatf("EventManager found a possible mistake. Unexpected return case from callback_proc.\n");
          case EVENT_CALLBACK_RETURN_REAP:        // The originating class is explicitly telling us to reap the event.
            reclaim_event(active_event);
            break;
          case EVENT_CALLBACK_RETURN_RECYCLE:     // The originating class wants us to re-insert the event.
            if (verbosity > 5) local_log.concatf("EventManager is recycling event %s.\n", active_event->getMsgTypeString());
            event_queue.insert(active_event);
            break;
          case EVENT_CALLBACK_RETURN_DROP:        // The originating class expects us to drop the event.
            if (active_event->returnToPrealloc()) {
              if (verbosity > 5) local_log.concat("EventManager::reclaim_event(): Returning the event to prealloc,\n");
              active_event->clearArgs();              // If so, wipe the Event...
              preallocated.insert(active_event);      // ...and return it to the preallocate queue.
            }                                         // Otherwise, we let it drop and trust some other class is managing it.
            else {
              if (verbosity > 5) local_log.concatf("Dropping event %s after running.\n", active_event->getMsgTypeString());
            }
            break;
          default:
            if (verbosity > 5) local_log.concatf("Event %s has no cleanup case.\n", active_event->getMsgTypeString());
            break;
        }
      }
      else {
        reclaim_event(active_event);
      }
    }
    else {
      /* If there is no callback, ask the event if it should be reaped. If its memory is
         being managed by some other class, this call will return false, and we just remove
         if from the event_queue and consider the matter closed. */
      reclaim_event(active_event);
    }
//if (boot_completed) crash_log.concat("Beginning profiler cleanup...\n");

    // This is a stat-gathering block.
    if (profiler_enabled) {
      profiler_mark_3 = micros();
      total_events++;

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
      profiler_item->run_time_last    = max(profiler_mark_2, profiler_mark_1) - min(profiler_mark_2, profiler_mark_1);
      profiler_item->run_time_best    = min(profiler_item->run_time_best, profiler_item->run_time_last);
      profiler_item->run_time_worst   = max(profiler_item->run_time_worst, profiler_item->run_time_last);
      profiler_item->run_time_total  += profiler_item->run_time_last;
      profiler_item->run_time_average = profiler_item->run_time_total / ((profiler_item->executions) ? profiler_item->executions : 1);

      
      if (verbosity >= 6) local_log.concatf("%s finished.\n\tTotal time: %ld uS\n", tmp_msg_def->debug_label, (profiler_mark_3 - profiler_mark_0));
      if (profiler_mark_2) {
        if (verbosity >= 6) local_log.concatf("\tTook %ld uS to notify.\n", profiler_item->run_time_last);
      }
      profiler_mark_2 = 0;  // Reset for next iteration.
    }

    return_value++;   // We just serviced an Event.
  }
  
  if (profiler_enabled) {
    total_loops++;
    if (return_value) {
      max_events_p_loop = max(max_events_p_loop, (uint8_t) return_value);
    }
    else {
      max_idle_loop_time = max(max_idle_loop_time, (max(profiler_mark, profiler_mark_3) - min(profiler_mark, profiler_mark_3)));
    }
  }

  if (local_log.length() > 0) {
    if (boot_completed) StaticHub::log(&local_log);
    else {
      printf("%s", (char*) local_log.string());
      local_log.clear();
    }
  }
  
if (boot_completed) crash_log.clear();
  if (local_log.length() > 0) StaticHub::log(&local_log);
  current_event = NULL;
  return return_value;
}




/**
* Turns the profiler on or off. Clears its collected stats regardless.
*
* @param   enabled  If true, enables the profiler. If false, disables it.
*/
void EventManager::profiler(bool enabled) {
  profiler_enabled   = enabled;
  max_idle_loop_time = 0;
  max_events_p_loop  = 0;
  total_loops        = 0;
  total_events       = 0;
  total_events_dead  = 0;

  events_destroyed   = 0;  
  prealloc_starved   = 0;  
  burden_of_specific = 0;
  idempotent_blocks  = 0; 
  insertion_denials  = 0;   
  max_queue_depth    = 0;   

  while (event_costs.hasNext()) delete event_costs.dequeue();
}


/**
* Print the profiler to the provided buffer.
*
* @param   StringBuilder*  The buffer that this fxn will write output into.
*/
void EventManager::printProfiler(StringBuilder* output) {
  if (NULL == output) return;
  output->concatf("\t total_events               %u\n", total_events);
  output->concatf("\t total_events_dead          %u\n\n", total_events_dead);
  output->concatf("\t max_queue_depth            %u\n", max_queue_depth);
  output->concatf("\t total_loops                %u\n", total_loops);
    
  if (profiler_enabled) {
    output->concat("-- Profiler dump:\n");
    if (total_events) {
      output->concatf("\tFraction of prealloc hits: %f\n\n", ((burden_of_specific - prealloc_starved) / total_events));
    }

    output->concatf("\t max_idle_loop_time         %u\n", max_idle_loop_time);
    output->concatf("\t max_events_p_loop          %u\n", max_events_p_loop);

    TaskProfilerData *profiler_item;
    int stat_mode = event_costs.getPriority(0);
    float scalar = (stat_mode != 0) ? (20.0 / stat_mode) : 1.0;
    int x = event_costs.size();
    char* stat_line_buf = (char*) alloca(21);
    *(stat_line_buf + 20) = '\0';
    
    output->concat("\t                     Execd               Event         total us   average     worst    best    last\n");
    for (int i = 0; i < x; i++) {
      profiler_item = event_costs.get(i);
      stat_mode     = event_costs.getPriority(i);
      
      for (int n = 0; n < 20; n++) {
        *(stat_line_buf + n) = (n > (stat_mode * scalar * 20)) ? ' ' : '*';
      }
      
      output->concatf("\t%s (%d)\t%18s  %9d %9d %9d %9d %9d\n", stat_line_buf, stat_mode, ManuvrMsg::getMsgTypeString(profiler_item->msg_code), profiler_item->run_time_total, profiler_item->run_time_average, profiler_item->run_time_worst, profiler_item->run_time_best, profiler_item->run_time_last);
    }
    output->concat("\n");
  }
  else {
    output->concat("-- EventManager profiler is not enabled.\n\n");
  }
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

/* Debug */
void EventManager::clean_first_discard() {
  ManuvrEvent* event = discarded.dequeue();
  if (NULL == event) {
    delete event;
  }
}


/**
* There is a NULL-check performed upstream for the scheduler member. So no need 
*   to do it again here.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t EventManager::bootComplete() {
  EventReceiver::bootComplete();
  boot_completed = true;
  return 1;
}


/**
* Debug support function.
*
* @return a pointer to a string constant.
*/
const char* EventManager::getReceiverName() {  return "EventManager";  }


/**
* Debug support method. This fxn is only present in debug builds. 
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void EventManager::printDebug(StringBuilder* output) {
  if (NULL == output) return;
  EventReceiver::printDebug(output);

  output->concatf("-- Queue depth:               %d\n", event_queue.size());
  output->concatf("-- Preallocation depth:       %d\n", preallocated.size());
  output->concatf("-- Total subscriber count:    %d\n", subscribers.size());
  output->concatf("-- Prealloc starves:          %u\n", prealloc_starved);
  output->concatf("-- events_destroyed:          %u\n", events_destroyed);
  output->concatf("-- burden_of_being_specific   %u\n", burden_of_specific);
  output->concatf("-- idempotent_blocks          %u\n\n", idempotent_blocks);
  
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

  if (discarded.size() > 0) {
    output->concatf("\nDiscard queue Listing (%d total):\n", discarded.size());
    for (int i = 0; i < discarded.size(); i++) {
      discarded.get(i)->printDebug(output);
    }
    output->concat("\n");
  }

  printProfiler(output);
}



