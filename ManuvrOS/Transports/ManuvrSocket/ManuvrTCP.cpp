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

#include <arpa/inet.h>



#define READ_PIPE  0
#define WRITE_PIPE 1


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
  __pipe_ids[READ_PIPE]  = -1;
  __pipe_ids[WRITE_PIPE] = -1;


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


// Original pejorated demo code. Is it useful to have a traffic mirror mode 
//   for other Manuvrables?
//
//void HandleClient(int sock) {
//  char buffer[255];
//  int received = -1;
//  /* Receive message */
//  if ((received = recv(sock, buffer, 255, 0)) < 0) {
//    Kernel::log("Failed to receive initial bytes from client");
//  }
//  /* Send bytes and check for more incoming data in loop */
//  while (received > 0) {
//    /* Send back received data */
//    if (send(sock, buffer, received, 0) != received) {
//      Kernel::log("Failed to send bytes to client");
//    }
//    /* Check for more data */
//    if ((received = recv(sock, buffer, 255, 0)) < 0) {
//      Kernel::log("Failed to receive additional bytes from client");
//    }
//  }
//  close(sock);
//}
           

volatile ManuvrTCP *active_socket = NULL;  // TODO: We need to be able to service many ports...


void HandleClient(int sock, XenoSession* session) {
  unsigned char *buf = (unsigned char *) alloca(512);  // TODO: Arbitrary. ---J. Ian Lindsay   Thu Dec 03 03:49:08 MST 2015

  int received = -1;
  /* Receive message */
  if ((received = recv(sock, buf, 255, 0)) < 0) {
    Kernel::log("Failed to receive initial bytes from client");
  }
  
  StringBuilder output;
  
  /* Send bytes and check for more incoming data in loop */
  while (received > 0) {
    output.concatf("Sending %d bytes into session.\n", received);
    
    // Do stuff regarding the data we just read...
    if (NULL != session) {
      session->bin_stream_rx(buf, received);
    }
    else {
      ManuvrEvent *event = Kernel::returnEvent(MANUVR_MSG_XPORT_RECEIVE);
      event->addArg(sock);
      StringBuilder *nu_data = new StringBuilder(buf, received);
      event->markArgForReap(event->addArg(nu_data), true);
      Kernel::staticRaiseEvent(event);
    }
    Kernel::getInstance()->printDebug(&output);
    Kernel::log(&output);

    /* Check for more data */
    if ((received = recv(sock, buf, 255, 0)) < 0) {
      Kernel::log("Failed to receive additional bytes from client");
    }
  }
  close(sock);
}
           


int8_t ManuvrTCP::connect() {
  // We're being told to act as a client.
  return 0;
}


int8_t ManuvrTCP::listen() {
  // We're being told to start listening on whatever address was provided to the constructor.
  // That means we are a server.
  if (_sock) {
    Kernel::log("A TCP socket was told to listen when it already was. Doing nothing.");
    return -1;
  }
  
  in_addr_t temp_addr = inet_network(_addr);
  
  serv_addr.sin_family      = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  //serv_addr.sin_addr.s_addr = temp_addr;
  serv_addr.sin_port        = htons(_port_number);
  
  _sock = socket(AF_INET, SOCK_STREAM, 0);        // Open the socket...

  /* Bind the server socket */
  if (bind(_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) {
    Kernel::log("Failed to bind the server socket.\n");
    return -1;
  }
  /* Listen on the server socket */
  if (::listen(_sock, 4) < 0) {
    Kernel::log("Failed to listen on server socket\n");
    return -1;
  }
  
  listening(true);

  int child_pid = fork();
  
  if (0 == child_pid) {
    // We are the child. We are listening for connections, and it is expected that we will block
    //   while we wait for connections. We can't allow the main thread to bind to SIGIO.
    StringBuilder output("Forked into PID ");
    output.concat(child_pid);
    output.concat("\n");
    Kernel::log(&output);
    
    while (1) {
      unsigned int clientlen = sizeof(cli_addr);

      /* Wait for client connection */
      if ((cli_sock = accept(_sock, (struct sockaddr *) &cli_addr, &clientlen)) < 0) {
        output.concat("Failed to accept client connection.\n");
      }
      else {
        output.concat("Client connected: ");
        output.concat((char*) inet_ntoa(cli_addr.sin_addr));
        output.concat("\n");
      }
      Kernel::log(&output);
      
      if (!session) {
        session = new XenoSession(this);
        __kernel->subscribe(session);
      }
      
      HandleClient(cli_sock, session);
      
      for (uint16_t i = 0; i < sizeof(cli_addr);  i++) {
        *((uint8_t *) &cli_addr  + i) = 0;
      }
    }
  }
  else {
    Kernel::log("TCP Now listening.\n");
  }
  return 0;
}


int8_t ManuvrTCP::reset() {
  listen();
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
  
  if (connected() | listening()) {
    int bytes_written = (out_len == (int) send(_sock, out, out_len, 0));
    
    if (bytes_written != out_len) {
      Kernel::log("Failed to send bytes to client");
    }
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
  
  if (connected() | listening()) {
    int bytes_written = (out_len == (int) send(cli_sock, out, out_len, 0));
    Kernel::log("Send bytes back\n");
    if (bytes_written != out_len) {
      Kernel::log("Failed to send bytes to client");
    }
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

