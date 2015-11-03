#include "Kernel.h"

bool continue_running = true;


ManuvrKernel::ManuvrKernel() {
}

ManuvrKernel::~ManuvrKernel() {
}




///**
//* Every so many milliseconds, pitch this event to the given Receiver.
//*
//*/
//uint32_t ManuvrKernel::every(uint32_t sch_period, FunctionPointer fxn, EverntReceiver* er) {
//  uint32_t return_value  = 0;
//  if (sch_period > 1) {
//    if (fxn != NULL) {
//      ScheduleItem *nu_sched = new ScheduleItem(get_valid_new_pid(), -1, sch_period, false, fxn);
//      if (nu_sched != NULL) {  // Did we actually malloc() successfully?
//        return_value  = nu_sched->pid;
//        schedules.insert(nu_sched);
//        nu_sched->enableProfiling(return_value);
//      }
//    }
//  }
//  return return_value;
//}




int8_t ManuvrKernel::run(void) {
  int x;
  int y;
  while (continue_running) {
    x = __e_man.procIdleFlags();
    y = __sched.serviceScheduledEvents();
  }
  return 0;
}

int8_t ManuvrKernel::step(void) {
  int x = __e_man.procIdleFlags();
  int y = __sched.serviceScheduledEvents();
  return x+y;
}


/****************************************************************************************************
* Resource fetch functions...                                                                       *
****************************************************************************************************/

Scheduler*    ManuvrKernel::fetchScheduler(void) {     return &__sched;     }
EventManager* ManuvrKernel::fetchEventManager(void) {  return &__e_man;   }


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

void ManuvrKernel::printDebug(StringBuilder* output) {
  if (NULL == output) return;
  EventReceiver::printDebug(output);
}


/**
* Debug support function.
*
* @return a pointer to a string constant.
*/
const char* ManuvrKernel::getReceiverName() {  return "ManuvrKernel";  }

/**
* There is a NULL-check performed upstream for the scheduler member. So no need 
*   to do it again here.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t ManuvrKernel::bootComplete() {
  return EventReceiver::bootComplete();   // Call up to get scheduler ref and class init.
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
int8_t ManuvrKernel::callback_proc(ManuvrEvent *event) {
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



int8_t ManuvrKernel::notify(ManuvrEvent *active_event) {
  int8_t return_value = 0;
  
  switch (active_event->event_code) {
    case MANUVR_MSG_SYS_POWER_MODE:
      break;

    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }
      
  //if (local_log.length() > 0) {    StaticHub::log(&local_log);  }
  return return_value;
}



void ManuvrKernel::procDirectDebugInstruction(StringBuilder *input) {
#ifdef __MANUVR_CONSOLE_SUPPORT
  char* str = input->position(0);
  ManuvrEvent *event = NULL;  // Pitching events is a common thing in this fxn...

  uint8_t temp_byte = 0;
  if (*(str) != 0) {
    temp_byte = atoi((char*) str+1);
  }

  /* These are debug case-offs that are typically used to test functionality, and are then
     struck from the build. */
  switch (*(str)) {
    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }
  
#endif
  //if (local_log.length() > 0) {    StaticHub::log(&local_log);  }
}


