/*
File:   ManuvrTCP.cpp
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


This driver is designed to give Manuvr platform-abstracted TCP socket connection.
This is basically only for linux.
  
*/


#include "ManuvrSocket.h"
#include "FirmwareDefs.h"
#include <ManuvrOS/XenoSession/XenoSession.h>

#include <ManuvrOS/Kernel.h>




/****************************************************************************************************
* Static initializers                                                                               *
****************************************************************************************************/


/****************************************************************************************************
* Class management                                                                                  *
****************************************************************************************************/

/**
* Constructor.
*/
ManuvrTCP::ManuvrTCP(char* addr, int port) : ManuvrXport() {
  __class_initializer();
  _port_number = port;
  _addr        = addr;

  // These will vary across UDP/WS/TCP.
  _options     = 0;
}


ManuvrTCP::ManuvrTCP(char* addr, int port, uint32_t opts) : ManuvrXport() {
  __class_initializer();
  _port_number = port;
  _addr        = addr;

  // These will vary across UDP/WS/TCP.
  _options     = opts;
}



/**
* Destructor
*/
ManuvrTCP::~ManuvrTCP() {
}



/**
* This is here for compatibility with C++ standards that do not allow for definition and declaration
*   in the header file. Takes no parameters, and returns nothing.
*/
void ManuvrTCP::__class_initializer() {
  _options           = 0;
  _port_number       = 0;
  _sock              = 0;


  // Build some pre-formed Events.
  read_abort_event.repurpose(MANUVR_MSG_XPORT_QUEUE_RDY);
  read_abort_event.isManaged(true);
  read_abort_event.specific_target = (EventReceiver*) this;
  read_abort_event.callback        = (EventReceiver*) this;
  read_abort_event.priority        = 5;
  read_abort_event.addArg(xport_id);  // Add our assigned transport ID to our pre-baked argument.

  /*
  TODO: Wrap this up into the efficiency blog...
    Note: this post assumes that either...
        (your style/C++-standard does not allow declaration and definition in your header)
        AND
        (you have an initialization sequence you manually code in either your constructor,
          or (as I've done in this fxn) an off-board fxn.)
      If the above evaluates to "true" in your case, the post below applies to you.

    // Suppose I have these members...
    struct sockaddr_in serv_addr = {0};
    struct sockaddr_in cli_addr  = {0};
    
    // Tells the compiler to do this on every instantiation:
    for (int i = 0; i < sizeof(serv_addr); i++) {  (void*)(&serv_addr + i) = 0; } // Zero the struct.
    for (int i = 0; i < sizeof(cli_addr);  i++) {  (void*)(&cli_addr  + i) = 0; } // Zero the struct.
    
    // But consider the benefit if I....
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;

    for (int i = 0; i < sizeof(cli_addr);  i++) {
      // TODO: Cast to an aligned-type for more gains.
      *((uint8_t *) &serv_addr + i) = 0;
      *((uint8_t *) &cli_addr  + i) = 0;
    }
        
    If your compiler is smart, it will optimize this for you.
    Do you trust your compiler to be smart all the time?
  */
  
  // Zero the socket parameter structures. 
  for (uint16_t i = 0; i < sizeof(cli_addr);  i++) {
    *((uint8_t *) &serv_addr + i) = 0;
    *((uint8_t *) &cli_addr  + i) = 0;
  }
}





/****************************************************************************************************
* Port I/O fxns                                                                                     *
****************************************************************************************************/

int8_t ManuvrTCP::connect() {
  // We're a serial port. If we are initialized, we are always connected.
  return 0;
}


int8_t ManuvrTCP::listen() {
  // We're being told to start listening on whatever address was provided to the constructor.
  // 
  if (_sock) {
  }
  serv_addr.sin_family      = AF_INET;
  serv_addr.sin_addr.s_addr = 0;//conf.getConfigIpAddress();    // This should be read from the config.
  serv_addr.sin_port        = htons(_port_number);
  
  _sock = socket(AF_INET, SOCK_STREAM, 0);        // Open the socket...
  
  
  return 0;
}


int8_t ManuvrTCP::reset() {
  return 0;
}



int8_t ManuvrTCP::read_port() {
  if (connected()) {
    unsigned char *buf = (unsigned char *) alloca(512);
    
      int n = read(_sock, buf, 255);
      int total_read = n;
      while (n > 0) {
        n = read(_sock, buf, 255);
        total_read += n;
      }
  
      if (total_read > 0) {
        // Do stuff regarding the data we just read...
        if (NULL != session) {
          session->bin_stream_rx(buf, total_read);
        }
        else {
          ManuvrEvent *event = Kernel::returnEvent(MANUVR_MSG_XPORT_RECEIVE);
          event->addArg(_sock);
          StringBuilder *nu_data = new StringBuilder(buf, total_read);
          event->markArgForReap(event->addArg(nu_data), true);
          Kernel::staticRaiseEvent(event);
        }
      }
    
  }
  else if (verbosity > 1) local_log.concat("Somehow we are trying to read a port that is not marked as open.\n");
  
  if (local_log.length() > 0) Kernel::log(&local_log);
  return 0;
}


/**
* Does what it claims to do on linux.
* Returns false on error and true on success.
*/
bool ManuvrTCP::write_port(unsigned char* out, int out_len) {
  if (_sock == -1) {
    if (verbosity > 2) Kernel::log(__PRETTY_FUNCTION__, LOG_ERR, "Unable to write to socket at: (%s:%d)\n", _addr, _port_number);
    return false;
  }
  
  if (connected()) {
    return (out_len == (int) write(_sock, out, out_len));
  }
  return false;
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
const char* ManuvrTCP::getReceiverName() {  return "ManuvrTCP";  }


/**
* Debug support method. This fxn is only present in debug builds. 
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void ManuvrTCP::printDebug(StringBuilder *temp) {
  if (temp == NULL) return;

  ManuvrXport::printDebug(temp);
  temp->concatf("-- _addr           %s:%d\n",  _addr, _port_number);
  temp->concatf("-- _options        0x%08x\n", _options);
  temp->concatf("-- _sock           0x%08x\n", _sock);
}


/**
* TODO: Until I do something smarter...
* We are obliged to call the ManuvrXport's version of bootComplete(), which in turn
*   will call the EventReceiver version of that fxn.
* ---J. Ian Lindsay   Thu Dec 03 03:25:48 MST 2015
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t ManuvrTCP::bootComplete() {   // ?? TODO ??
  EventReceiver::bootComplete();
  
  // We will suffer a 300ms latency if the platform's networking stack doesn't flush
  //   its buffer in time.
  pid_read_abort = __kernel->createSchedule(300, 0, false, this, &read_abort_event);
  __kernel->disableSchedule(pid_read_abort);

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
int8_t ManuvrTCP::callback_proc(ManuvrEvent *event) {
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



int8_t ManuvrTCP::notify(ManuvrEvent *active_event) {
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
            if (verbosity > 3) local_log.concatf("We about to print %d bytes to the socket.\n", temp_sb->length());
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
        if (verbosity > 3) local_log.concat("The TCP class received an event that was addressed to it, that is not handled yet.\n");
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

