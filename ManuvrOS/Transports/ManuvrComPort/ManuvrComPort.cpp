/*
File:   ManuvrComPort.cpp
Author: J. Ian Lindsay
Date:   2015.03.17

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


This driver is designed to give Manuvr platform-abstracted COM ports. By
  this is meant generic asynchronous serial ports. On Arduino, this means
  the Serial (or HardwareSerial) class. On linux, it means /dev/tty<x>.

Platforms that require it should be able to extend this driver for specific 
  kinds of hardware support. For an example of this, I would refer you to
  the STM32F4 case-offs I understand that this might seem "upside down"
  WRT how drivers are more typically implemented, and it may change later on.
  But for now, it seems like a good idea.  
*/


#include "ManuvrComPort.h"
#include "FirmwareDefs.h"
#include "ManuvrOS/XenoSession/XenoSession.h"

#ifndef TEST_BENCH
  #include "StaticHub/StaticHub.h"
#else
  #include "tests/StaticHub.h"
#endif


#if defined (STM32F4XX)        // STM32F4

  
#elif defined (__MK20DX128__)  // Teensy3


#elif defined (__MK20DX256__)  // Teensy3.1


#elif defined (ARDUINO)        // Fall-through case for basic Arduino support.

  
#else   //Assuming a linux environment. Cross your fingers....
  
#endif



/****************************************************************************************************
* Static initializers                                                                               *
****************************************************************************************************/


/****************************************************************************************************
* Class management                                                                                  *
****************************************************************************************************/

/**
* Constructor.
*/
ManuvrComPort::ManuvrComPort(const char* tty_nom, int b_rate) {
  __class_initializer();
  tty_name   = tty_nom;
  baud_rate  = b_rate;
}


/**
* Constructor.
*/
ManuvrComPort::ManuvrComPort(const char* tty_nom, int b_rate, uint32_t opts) {
  __class_initializer();
  tty_name   = tty_nom;
  baud_rate  = b_rate;
  options    = opts;
}



/**
* Destructor
*/
ManuvrComPort::~ManuvrComPort() {
}



/**
* This is here for compatibility with C++ standards that do not allow for definition and declaration
*   in the header file. Takes no parameters, and returns nothing.
*/
void ManuvrComPort::__class_initializer() {
  xport_id           = 0;
  xport_state        = MANUVR_XPORT_STATE_UNINITIALIZED;
  pid_read_abort     = 0;
  options            = 0;
  read_timeout_defer = false;
  session            = NULL;

  // Build some pre-formed Events.
  read_abort_event.repurpose(MANUVR_MSG_XPORT_QUEUE_RDY);
  read_abort_event.mem_managed     = true;
  read_abort_event.specific_target = (EventReceiver*) this;
  read_abort_event.callback        = (EventCallback*) this;
  read_abort_event.priority        = 5;
  //read_abort_event.addArg();  // Add our assigned transport ID to our pre-baked argument.

	StaticHub *sh = StaticHub::getInstance();
	scheduler = sh->fetchScheduler();
	sh->fetchEventManager()->subscribe((EventReceiver*) this);  // Subscribe to the EventManager.
	
	pid_read_abort = scheduler->createSchedule(30, 0, false, this, &read_abort_event);
	scheduler->disableSchedule(pid_read_abort);
}




int8_t ManuvrComPort::reset() {
  #if defined (STM32F4XX)        // STM32F4

  #elif defined (__MK20DX128__)  // Teensy3
  
  #elif defined (__MK20DX256__)  // Teensy3.1
  
  #elif defined (ARDUINO)        // Fall-through case for basic Arduino support.
    
  #else   //Assuming a linux environment. Cross your fingers....
    
  #endif
  return 0;
}


// Given a transport event, returns true if we need to act.
bool ManuvrComPort::event_addresses_us(ManuvrEvent *event) {
  uint16_t temp_uint16;
  
  if (event->argCount()) {
    if (0 == event->getArgAs(&temp_uint16)) {
      if (temp_uint16 == xport_id) {
        // The first argument is our ID.
        return true;
      }
    }
    // Either not the correct arg form, or not our ID.
    return false;
  }
  
  // No arguments implies no first argument.
  // No first argument implies event is addressed to 'all transports'.
  // 'all transports' implies 'true'. We need to care.
  return true;
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
* Debug support function.
*
* @return a pointer to a string constant.
*/
const char* ManuvrComPort::getReceiverName() {  return "ManuvrComPort";  }


/**
* Debug support method. This fxn is only present in debug builds. 
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void ManuvrComPort::printDebug(StringBuilder *temp) {
  if (temp == NULL) return;
  
  EventReceiver::printDebug(temp);
  
  temp->concatf("--- tty_name               %s\n", tty_name);
  temp->concatf("--- connected              %s\n", (connected() ? "yes" : "no"));
  temp->concatf("--- has session            %s\n\n", (hasSession() ? "yes" : "no"));
  
}



/**
* There is a NULL-check performed upstream for the scheduler member. So no need 
*   to do it again here.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t ManuvrComPort::bootComplete() {
  EventReceiver::bootComplete();
  
  reset();
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
int8_t ManuvrComPort::callback_proc(ManuvrEvent *event) {
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


int8_t ManuvrComPort::notify(ManuvrEvent *active_event) {
  int8_t return_value = 0;
  
  switch (active_event->event_code) {
    case MANUVR_MSG_XPORT_INIT:
    case MANUVR_MSG_XPORT_RESET:
    case MANUVR_MSG_XPORT_CONNECT:
    case MANUVR_MSG_XPORT_DISCONNECT:
    case MANUVR_MSG_TRANSPORT_ERROR:
    case MANUVR_MSG_TRANSPORT_SESSION:
    case MANUVR_MSG_XPORT_QUEUE_RDY:
    case MANUVR_MSG_XPORT_CB_QUEUE_RDY:
    case MANUVR_MSG_TRANSPORT_SEND:
    case MANUVR_MSG_TRANSPORT_RECEIVE:
    case MANUVR_MSG_TRANSPORT_RESERVED_0:
    case MANUVR_MSG_TRANSPORT_RESERVED_1:
    case MANUVR_MSG_TRANSPORT_SET_PARAM:
    case MANUVR_MSG_TRANSPORT_GET_PARAM:
    case MANUVR_MSG_TRANSPORT_IDENTITY:
      if (event_addresses_us(active_event) ) {
        local_log.concat("The com port class received an event that was addressed to it, that is not handled yet.\n");
        active_event->printDebug(&local_log);
        return_value++;
      }
      break;

    case MANUVR_MSG_TRANSPORT_DEBUG:
      if (event_addresses_us(active_event) ) {
        printDebug(&local_log);
        return_value++;
      }
      break;

    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }
  
  if (local_log.length() > 0) StaticHub::log(&local_log);
  return return_value;
}



