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
This is basically only for linux for now.
  
*/


#include "ManuvrSocket.h"
#include "FirmwareDefs.h"
#include <ManuvrOS/XenoSession/XenoSession.h>

#include <ManuvrOS/Kernel.h>

#if defined(__MANUVR_FREERTOS) || defined(__MANUVR_LINUX)
  #include <arpa/inet.h>

  // Threaded platforms will need this to compensate for a loss of ISR.
  extern void* xport_read_handler(void* active_xport);
  
  
  /*
  * Since listening for connections on this transport involves blocking, we have a
  *   thread dedicated to the task...
  */
  void* socket_listener_loop(void* active_xport) {
    if (NULL != active_xport) {
      ManuvrTCP* listening_inst = (ManuvrTCP*) active_xport;
      StringBuilder output;
      int      cli_sock;
      struct sockaddr_in cli_addr;
      while (listening_inst->listening()) {
        unsigned int clientlen = sizeof(cli_addr);
  
        /* Wait for client connection */
        if ((cli_sock = accept(listening_inst->getSockID(), (struct sockaddr *) &cli_addr, &clientlen)) < 0) {
          output.concat("Failed to accept client connection.\n");
        }
        else {
          ManuvrTCP* nu_connection = new ManuvrTCP(listening_inst, cli_sock, &cli_addr);
          
          ManuvrRunnable* event = Kernel::returnEvent(MANUVR_MSG_SYS_ADVERTISE_SRVC);
          event->addArg((EventReceiver*) nu_connection);
          Kernel::staticRaiseEvent(event);

          output.concat("Client connected: ");
          output.concat((char*) inet_ntoa(cli_addr.sin_addr));
          output.concat("\n");
        }
        Kernel::log(&output);
        
        for (uint16_t i = 0; i < sizeof(cli_addr);  i++) {
          // Zero the sockaddr structure for next use. The new transport 
          //   instance should have copied it by now.
          *((uint8_t *) &cli_addr  + i) = 0;
        }
      }
      // Close the listener...
      // TODO: Is this all we need to do?   ---J. Ian Lindsay   Fri Jan 15 11:32:12 PST 2016
      close(listening_inst->getSockID());
    }
    else {
      Kernel::log("Tried to listen with a NULL transport.");
    }
    
    return NULL;
  }
  
#else
  // No special globals needed for this platform.
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
ManuvrTCP::ManuvrTCP(const char* addr, int port) : ManuvrXport() {
  __class_initializer();
  _port_number = port;
  _addr        = addr;

  // These will vary across UDP/WS/TCP.
  _options     = 0;
}


ManuvrTCP::ManuvrTCP(const char* addr, int port, uint32_t opts) : ManuvrXport() {
  __class_initializer();
  _port_number = port;
  _addr        = addr;

  // These will vary across UDP/WS/TCP.
  _options     = opts;
}


/**
* This constructor is called by a listening instance of ManuvrTCP.
*/
ManuvrTCP::ManuvrTCP(ManuvrTCP* listening_instance, int sock, struct sockaddr_in* nu_sockaddr) : ManuvrXport() {
  __class_initializer();
  _sock          = sock;
  _addr          = listening_instance->_addr;
  _port_number   = listening_instance->_port_number;
  _options       = listening_instance->_options;

  listening_instance->_connections.insert(this);  // TODO: This is starting to itch...

  for (uint16_t i = 0; i < sizeof(_sockaddr);  i++) {
    // Copy the sockaddr struct into this instance.
    *((uint8_t *) &_sockaddr + i) = *(((uint8_t*)nu_sockaddr) + i);
  }
  
  // Inherrit the listener's configuration...
  nonSessionUsage(listening_instance->nonSessionUsage());
  
  connected(true);  // TODO: Possibly not true....
}


/**
* Destructor
*/
ManuvrTCP::~ManuvrTCP() {
  __kernel->unsubscribe(this);
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
  read_abort_event.originator      = (EventReceiver*) this;
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
  for (uint16_t i = 0; i < sizeof(_sockaddr);  i++) {
    *((uint8_t *) &_sockaddr + i) = 0;
  }
}





/****************************************************************************************************
* Port I/O fxns                                                                                     *
****************************************************************************************************/

int8_t ManuvrTCP::connect() {
  // We're being told to act as a client.
  if (listening()) {
    Kernel::log("A TCP socket was told to connect while it is listening. Doing nothing.");
    return -1;
  }

  if (connected()) {
    Kernel::log("A TCP socket was told to connect while it already was. Doing nothing.");
    return -1;
  }

  //in_addr_t temp_addr = inet_network(_addr);
  
  _sockaddr.sin_family      = AF_INET;
  _sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  //_sockaddr.sin_addr.s_addr = temp_addr;
  _sockaddr.sin_port        = htons(_port_number);

  _sock = socket(AF_INET, SOCK_STREAM, 0);        // Open the socket...
  
  /* Bind the server socket */
  if (::connect(_sock, (struct sockaddr *) &_sockaddr, sizeof(_sockaddr))) {
    StringBuilder log;
    log.concatf("Failed to connect to %s:%d.\n", _addr, _port_number);
    Kernel::log(&log);
    return -1;
  }

  initialized(true);
  connected(true);
  return 0;
}


int8_t ManuvrTCP::listen() {
  // We're being told to start listening on whatever address was provided to the constructor.
  // That means we are a server.
  if (_sock) {
    Kernel::log("A TCP socket was told to listen when it already was. Doing nothing.");
    return -1;
  }
  
  //in_addr_t temp_addr = inet_network(_addr);
  
  _sockaddr.sin_family      = AF_INET;
  _sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  //_sockaddr.sin_addr.s_addr = temp_addr;
  _sockaddr.sin_port        = htons(_port_number);
  
  _sock = socket(AF_INET, SOCK_STREAM, 0);        // Open the socket...

  /* Bind the server socket */
  if (bind(_sock, (struct sockaddr *) &_sockaddr, sizeof(_sockaddr))) {
    Kernel::log("Failed to bind the server socket.\n");
    return -1;
  }
  /* Listen on the server socket */
  if (::listen(_sock, 4) < 0) {
    Kernel::log("Failed to listen on server socket\n");
    return -1;
  }
  
  initialized(true);
  createThread(&_thread_id, NULL, socket_listener_loop, (void*) this);
  
  listening(true);
  local_log.concatf("TCP Now listening at %s:%d.\n", _addr, _port_number);

  if (local_log.length() > 0) Kernel::log(&local_log);
  return 0;
}


int8_t ManuvrTCP::reset() {
  return 0;
}



int8_t ManuvrTCP::read_port() {
  if (connected()) {
    unsigned char *buf = (unsigned char *) alloca(256);
    int n;
    ManuvrRunnable *event    = NULL;
    StringBuilder  *nu_data  = NULL;

    while (connected()) {
      printf("Buffer is 0x%08x from socket %d\n", (unsigned long) buf, _sock);
      n = read(_sock, buf, 255);
      if (n > 0) {
        bytes_received += n;

        // Do stuff regarding the data we just read...
        if (NULL != session) {
          session->bin_stream_rx(buf, n);
        }
        else {
          event = Kernel::returnEvent(MANUVR_MSG_XPORT_RECEIVE);
          //event->addArg(_sock);
          nu_data = new StringBuilder(buf, n);
          event->markArgForReap(event->addArg(nu_data), true);
          raiseEvent(event);
          Kernel::log(nu_data);
        }
      }
      else {
        // Don't thrash the CPU for no reason...
        sleep_millis(20);
      }
    }
  }
  else if (verbosity > 1) {
    local_log.concat("Somehow we are trying to read a port that is not marked as open.\n");
  }
  
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
    int bytes_written = (int) write(_sock, out, out_len);
    bytes_sent += bytes_written;
    if (bytes_written == out_len) {
      return true;
    }
    Kernel::log("Failed to send bytes to client");
  }
  return false;
}


/**
* Does what it claims to do on linux.
* Returns false on error and true on success.
*/
bool ManuvrTCP::write_port(int sock, unsigned char* out, int out_len) {
  if (sock == -1) {
    if (verbosity > 2) Kernel::log(__PRETTY_FUNCTION__, LOG_ERR, "Unable to write to socket at: (%s:%d)\n", _addr, _port_number);
    return false;
  }
  
  if (connected()) {
    int bytes_written = (int) write(_sock, out, out_len);
    bytes_sent += bytes_written;
    if (bytes_written == out_len) {
      return true;
    }
    Kernel::log("Failed to send bytes to client");
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
  read_abort_event.alterScheduleRecurrence(0);
  read_abort_event.alterSchedulePeriod(300);
  read_abort_event.autoClear(false);
  read_abort_event.enableSchedule(false);
  read_abort_event.enableSchedule(false);
  __kernel->addSchedule(&read_abort_event);

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
int8_t ManuvrTCP::callback_proc(ManuvrRunnable *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */ 
  int8_t return_value = event->kernelShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;
  
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



int8_t ManuvrTCP::notify(ManuvrRunnable *active_event) {
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

