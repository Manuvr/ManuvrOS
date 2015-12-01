/*
File:   ManuvrSocket.cpp
Author: J. Ian Lindsay
Date:   2015.09.17

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


#include "ManuvrSocket.h"
#include "FirmwareDefs.h"
#include "ManuvrOS/XenoSession/XenoSession.h"

#include "ManuvrOS/Kernel.h"


#if defined (STM32F4XX)        // STM32F4

#else   //Assuming a linux environment. Cross your fingers....
  #include <cstdio>
  #include <stdlib.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <sys/signal.h>
  #include <fstream>
  #include <iostream>
  #include <sys/socket.h>

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
ManuvrSocket::ManuvrSocket() {
  __class_initializer();
  options    = 0;
}


/**
* Destructor
*/
ManuvrSocket::~ManuvrSocket() {
}



/**
* This is here for compatibility with C++ standards that do not allow for definition and declaration
*   in the header file. Takes no parameters, and returns nothing.
*/
void ManuvrSocket::__class_initializer() {
  __class_initializer();
  xport_id           = ManuvrXport::TRANSPORT_ID_POOL++;
  xport_state        = MANUVR_XPORT_STATE_UNINITIALIZED;
  pid_read_abort     = 0;
  options            = 0;
  port_number        = 0;
  bytes_sent         = 0;
  bytes_received     = 0;
  read_timeout_defer = false;
  session            = NULL;
    
  // Build some pre-formed Events.
  read_abort_event.repurpose(MANUVR_MSG_XPORT_QUEUE_RDY);
  read_abort_event.isManaged(true);
  read_abort_event.specific_target = (EventReceiver*) this;
  read_abort_event.callback        = (EventReceiver*) this;
  read_abort_event.priority        = 5;
  //read_abort_event.addArg();  // Add our assigned transport ID to our pre-baked argument.

  __kernel = Kernel::getInstance();
  __kernel->subscribe((EventReceiver*) this);  // Subscribe to the Kernel.
  
  pid_read_abort = __kernel->createSchedule(30, 0, false, this, &read_abort_event);
  __kernel->disableSchedule(pid_read_abort);
}





int8_t ManuvrSocket::provide_session(XenoSession* ses) {
  if ((NULL != session) && (ses != session)) {
    // If we are about to clobber an existing session, we need to free it
    // first.
    __kernel->unsubscribe(session);
    delete session;
    session = NULL;
  }
  session = ses;
  //session->setVerbosity(verbosity);
  set_xport_state(MANUVR_XPORT_STATE_HAS_SESSION);
  return 0;
}





XenoSession* ManuvrSocket::getSession() {
  return session;
}




/****************************************************************************************************
* Port I/O fxns                                                                                     *
****************************************************************************************************/

int8_t ManuvrSocket::read_port() {
  if (connected()) {
    unsigned char *buf = (unsigned char *) alloca(512);
    #if defined (STM32F4XX)        // STM32F4

    #else   //Assuming a linux environment. Cross your fingers....
      int n = read(port_number, buf, 255);
      int total_read = n;
      while (n > 0) {
        n = read(port_number, buf, 255);
        total_read += n;
      }
  
      if (total_read > 0) {
        // Do stuff regarding the data we just read...
        if (NULL != session) {
          session->bin_stream_rx(buf, total_read);
        }
        else {
          ManuvrEvent *event = Kernel::returnEvent(MANUVR_MSG_XPORT_RECEIVE);
          event->addArg(port_number);
          StringBuilder *nu_data = new StringBuilder(buf, total_read);
          event->markArgForReap(event->addArg(nu_data), true);
          Kernel::staticRaiseEvent(event);
        }
      }
    #endif
  }
  else if (verbosity > 1) local_log.concat("Somehow we are trying to read a port that is not marked as open.\n");
  
  if (local_log.length() > 0) Kernel::log(&local_log);
  return 0;
}


/**
* Does what it claims to do on linux.
* Returns false on error and true on success.
*/
bool ManuvrSocket::write_port(unsigned char* out, int out_len) {
  if (port_number == -1) {
    if (verbosity > 2) Kernel::log(__PRETTY_FUNCTION__, LOG_ERR, "Unable to write to port: (%s)\n", tty_name);
    return false;
  }
  
  if (connected()) {
    #if defined (STM32F4XX)        // STM32F4
  
    #else   //Assuming a linux environment. Cross your fingers....
      return (out_len == (int) write(port_number, out, out_len));
    #endif
  }
  return false;
}



int8_t ManuvrSocket::sendBuffer(StringBuilder* buf) {
  write_port(buf->string(), buf->length());
  return 0;
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
const char* ManuvrSocket::getReceiverName() {  return "ManuvrSocket";  }


/**
* Debug support method. This fxn is only present in debug builds. 
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void ManuvrSocket::printDebug(StringBuilder *temp) {
  if (temp == NULL) return;
  
  EventReceiver::printDebug(temp);
  temp->concatf("--- xport_state    \t 0x%02x\n", xport_state);
  temp->concatf("--- xport_id       \t 0x%04x\n", xport_id);
  temp->concatf("--- bytes sent     \t %u\n", bytes_sent);
  temp->concatf("--- bytes received \t %u\n\n", bytes_received);
  temp->concatf("--- tty_name       \t %s\n", tty_name);
  temp->concatf("--- connected      \t %s\n", (connected() ? "yes" : "no"));
  temp->concatf("--- has session    \t %s\n\n", (hasSession() ? "yes" : "no"));

}


/**
* There is a NULL-check performed upstream for the scheduler member. So no need 
*   to do it again here.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t ManuvrSocket::bootComplete() {
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
*   ourselves and return DROP. By default, we will return REAP to instruct the Kernel
*   to either free() the event or return it to it's preallocate queue, as appropriate. If the event
*   was crafted to not be in the heap in its own allocation, we will return DROP instead.
*
* @param  event  The event for which service has been completed.
* @return A callback return code.
*/
int8_t ManuvrSocket::callback_proc(ManuvrEvent *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */ 
  int8_t return_value = event->eventManagerShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;
  
  /* Some class-specific set of conditionals below this line. */
  switch (event->event_code) {
    case MANUVR_MSG_XPORT_SEND:
      event->clearArgs();
      break;
    default:
      break;
  }
  
  return return_value;
}



int8_t ManuvrSocket::notify(ManuvrEvent *active_event) {
  int8_t return_value = 0;
  
  switch (active_event->event_code) {
    case MANUVR_MSG_SESS_ORIGINATE_MSG:
      break;

    case MANUVR_MSG_XPORT_INIT:
    case MANUVR_MSG_XPORT_RESET:
    case MANUVR_MSG_XPORT_CONNECT:
    case MANUVR_MSG_XPORT_DISCONNECT:
    case MANUVR_MSG_XPORT_ERROR:
    case MANUVR_MSG_XPORT_SESSION:
    case MANUVR_MSG_XPORT_QUEUE_RDY:
    case MANUVR_MSG_XPORT_CB_QUEUE_RDY:
      break;

    case MANUVR_MSG_XPORT_SEND:
      if (NULL != session) {
        if (connected()) {
          StringBuilder* temp_sb;
          if (0 == active_event->getArgAs(&temp_sb)) {
            if (verbosity > 3) local_log.concatf("We about to print %d bytes to the com port.\n", temp_sb->length());
            write_port(temp_sb->string(), temp_sb->length());
          }
          
          //uint16_t xenomsg_id = session->nextMessage(&outbound_msg);
          //if (xenomsg_id) {
          //  if (write_port(outbound_msg.string(), outbound_msg.length()) ) {
          //    if (verbosity > 2) local_log.concatf("There was a problem writing to %s.\n", tty_name);
          //  }
          //  return_value++;
          //}
          else if (verbosity > 6) local_log.concat("Ignoring a broadcast that wasn't meant for us.\n");
        }
        else if (verbosity > 3) local_log.concat("Session is chatting, but we don't appear to have a connection.\n");
      }
      return_value++;
      break;
      
    case MANUVR_MSG_XPORT_RECEIVE:
    case MANUVR_MSG_XPORT_RESERVED_0:
    case MANUVR_MSG_XPORT_RESERVED_1:
    case MANUVR_MSG_XPORT_SET_PARAM:
    case MANUVR_MSG_XPORT_GET_PARAM:
    
    case MANUVR_MSG_XPORT_IDENTITY:
      if (event_addresses_us(active_event) ) {
        if (verbosity > 3) local_log.concat("The com port class received an event that was addressed to it, that is not handled yet.\n");
        active_event->printDebug(&local_log);
        return_value++;
      }
      break;

    case MANUVR_MSG_XPORT_DEBUG:
      if (event_addresses_us(active_event) ) {
        printDebug(&local_log);
        return_value++;
      }
      break;

    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }
  
  if (local_log.length() > 0) Kernel::log(&local_log);
  return return_value;
}




