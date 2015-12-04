/*
File:   ManuvrSerial.cpp
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


#include "ManuvrSerial.h"
#include "FirmwareDefs.h"
#include "ManuvrOS/XenoSession/XenoSession.h"

#include <ManuvrOS/Kernel.h>


#if defined (STM32F4XX)        // STM32F4

  
#elif defined (__MK20DX128__)  // Teensy3


#elif defined (__MK20DX256__)  // Teensy3.1


#elif defined (ARDUINO)        // Fall-through case for basic Arduino support.

  
#else
  //Assuming a linux environment. Cross your fingers....
  // TODO: Need a #define for LINUX32/64 that is set upstream so we don't have to cross our fingers.
  //       ---J. Ian Lindsay   Thu Dec 03 03:33:35 MST 2015
  #include <cstdio>
  #include <stdlib.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <sys/signal.h>
  #include <fstream>
  #include <iostream>


  // TODO: This limits us to supporting a single instance. Same issue as in i2cAdapter.
  //   A strategy for this needs to be formed.
  //       ---J. Ian Lindsay   Thu Dec 03 03:31:04 MST 2015
  struct termios termAttr;
  struct sigaction serial_handler;
  volatile ManuvrSerial *active_tty = NULL;  // TODO: We need to be able to service many ports...
  
  /*
  * In a linux environment, we need a function outside of this class to catch signals.
  * This is called when the serial port has something to say. 
  */
  void tty_signal_handler(int status) {
    if (NULL != active_tty) ((ManuvrSerial*) active_tty)->read_port();
  }

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
ManuvrSerial::ManuvrSerial(char* tty_path, int b_rate) {
  __class_initializer();
  _addr     = tty_path;
  _baud_rate = b_rate;
  _options  = 0;
}


/**
* Constructor.
*/
ManuvrSerial::ManuvrSerial(char* tty_path, int b_rate, uint32_t opts) {
  __class_initializer();
  _addr     = tty_path;
  _baud_rate = b_rate;
  _options  = opts;
}



/**
* Destructor
*/
ManuvrSerial::~ManuvrSerial() {
}



/**
* This is here for compatibility with C++ standards that do not allow for definition and declaration
*   in the header file. Takes no parameters, and returns nothing.
*/
void ManuvrSerial::__class_initializer() {
  pid_read_abort     = 0;
  _options           = 0;
  _sock              = 0;
  _pid               = 0;
    
  // Build some pre-formed Events.
  read_abort_event.repurpose(MANUVR_MSG_XPORT_QUEUE_RDY);
  read_abort_event.isManaged(true);
  read_abort_event.specific_target = (EventReceiver*) this;
  read_abort_event.callback        = (EventReceiver*) this;
  read_abort_event.priority        = 5;
  read_abort_event.addArg(xport_id);  // Add our assigned transport ID to our pre-baked argument.
}





/****************************************************************************************************
* Port I/O fxns                                                                                     *
****************************************************************************************************/

int8_t ManuvrSerial::init() {
  uint8_t xport_state_modifier = MANUVR_XPORT_STATE_CONNECTED | MANUVR_XPORT_STATE_LISTENING | MANUVR_XPORT_STATE_INITIALIZED;
  #ifdef __MANUVR_DEBUG
  if (verbosity > 4) local_log.concatf("Resetting port %s...\n", _addr);
  #endif
  bytes_sent         = 0;
  bytes_received     = 0;

  #if defined (STM32F4XX)        // STM32F4

  #elif defined (__MK20DX128__)  // Teensy3
  
  #elif defined (__MK20DX256__)  // Teensy3.1
  
  #elif defined (ARDUINO)        // Fall-through case for basic Arduino support.
    
  #else   //Assuming a linux environment. Cross your fingers....
  if (_sock) {
    close(_sock);
  }
  
  _sock = open(_addr, _options);
  if (_sock == -1) {
    #ifdef __MANUVR_DEBUG
      if (verbosity > 1) local_log.concatf("Unable to open port: (%s)\n", _addr);
      Kernel::log(&local_log);
    #endif
    unset_xport_state(xport_state_modifier);
    return -1;
  }
    #ifdef __MANUVR_DEBUG
  if (verbosity > 4) local_log.concatf("Opened port (%s) at %d\n", _addr, _baud_rate);
    #endif
  set_xport_state(MANUVR_XPORT_STATE_INITIALIZED);

  serial_handler.sa_handler = tty_signal_handler;
  serial_handler.sa_flags = 0;
  serial_handler.sa_restorer = NULL; 
  sigaction(SIGIO, &serial_handler, NULL);

  _pid  = getpid();
  fcntl(_sock, F_SETOWN, _pid);               // This process owns the port.
  fcntl(_sock, F_SETFL, O_NDELAY | O_ASYNC);  // Read returns immediately.

  tcgetattr(_sock, &termAttr);
  cfsetspeed(&termAttr, _baud_rate);
  // TODO: These choices should come from _options. Find a good API to emulate.
  //    ---J. Ian Lindsay   Thu Dec 03 03:43:12 MST 2015
  termAttr.c_cflag &= ~PARENB;          // No parity
  termAttr.c_cflag &= ~CSTOPB;          // 1 stop bit
  termAttr.c_cflag &= ~CSIZE;           // Enable char size mask
  termAttr.c_cflag |= CS8;              // 8-bit characters
  termAttr.c_cflag |= (CLOCAL | CREAD);
  termAttr.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  termAttr.c_iflag &= ~(IXON | IXOFF | IXANY);
  termAttr.c_oflag &= ~OPOST;
  
  if (tcsetattr(_sock, TCSANOW, &termAttr) == 0) {
    set_xport_state(xport_state_modifier);
    initialized(false);
    #ifdef __MANUVR_DEBUG
      if (verbosity > 6) local_log.concatf("Port opened, and handler bound.\n");
    #endif //__MANUVR_DEBUG
  }
  else {
    unset_xport_state(xport_state_modifier);
    initialized(false);
    #ifdef __MANUVR_DEBUG
      if (verbosity > 1) local_log.concatf("Failed to tcsetattr...\n");
    #endif
  }
  #endif //LINUX
  
  if (local_log.length() > 0) Kernel::log(&local_log);
  return 0;
}



int8_t ManuvrSerial::connect() {
  // We're a serial port. If we are initialized, we are always connected.
  return 0;
}


int8_t ManuvrSerial::listen() {
  // We're a serial port. If we are initialized, we are always listening.
  return 0;
}


int8_t ManuvrSerial::reset() {
  // TODO:  Differentiate.   ---J. Ian Lindsay   Thu Dec 03 03:48:26 MST 2015
  init();
  return 0;
}



int8_t ManuvrSerial::read_port() {
  if (connected()) {
    unsigned char *buf = (unsigned char *) alloca(512);  // TODO: Arbitrary. ---J. Ian Lindsay   Thu Dec 03 03:49:08 MST 2015
    #if defined (STM32F4XX)        // STM32F4
  
    #elif defined (__MK20DX128__)  // Teensy3
    
    #elif defined (__MK20DX256__)  // Teensy3.1
    
    #elif defined (ARDUINO)        // Fall-through case for basic Arduino support.
      
    #else   //Assuming a linux environment. Cross your fingers....
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
    #endif
  }
  else {
    #ifdef __MANUVR_DEBUG
    if (verbosity > 1) local_log.concat("Somehow we are trying to read a port that is not marked as open.\n");
    #endif
  }
  
  if (local_log.length() > 0) Kernel::log(&local_log);
  return 0;
}


/**
* Does what it claims to do on linux.
* Returns false on error and true on success.
*/
bool ManuvrSerial::write_port(unsigned char* out, int out_len) {
  if (_sock == -1) {
    #ifdef __MANUVR_DEBUG
    if (verbosity > 2) Kernel::log(__PRETTY_FUNCTION__, LOG_ERR, "Unable to write to port: (%s)\n", _addr);
    #endif
    return false;
  }
  
  if (connected()) {
    #if defined (STM32F4XX)        // STM32F4
  
    #elif defined (__MK20DX128__)  // Teensy3
    
    #elif defined (__MK20DX256__)  // Teensy3.1
    
    #elif defined (ARDUINO)        // Fall-through case for basic Arduino support.
      
    #else   //Assuming a linux environment. Cross your fingers....
      return (out_len == (int) write(_sock, out, out_len));
    #endif
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
const char* ManuvrSerial::getReceiverName() {  return "ManuvrSerial";  }


/**
* Debug support method. This fxn is only present in debug builds. 
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void ManuvrSerial::printDebug(StringBuilder *temp) {
  if (temp == NULL) return;

  ManuvrXport::printDebug(temp);
  temp->concatf("-- _addr           %s\n",     _addr);
  temp->concatf("-- _options        0x%08x\n", _options);
  temp->concatf("-- _sock           0x%08x\n", _sock);
  temp->concatf("-- Baud            %d\n",     _baud_rate);
}


/**
* There is a NULL-check performed upstream for the scheduler member. So no need 
*   to do it again here.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t ManuvrSerial::bootComplete() {
  EventReceiver::bootComplete();
  
  // Tolerate 30ms of latency on the line before flushing the buffer.
  pid_read_abort = __kernel->createSchedule(30, 0, false, this, &read_abort_event);
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
int8_t ManuvrSerial::callback_proc(ManuvrEvent *event) {
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



int8_t ManuvrSerial::notify(ManuvrEvent *active_event) {
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
            #ifdef __MANUVR_DEBUG
            if (verbosity > 3) local_log.concatf("We about to print %d bytes to the com port.\n", temp_sb->length());
            #endif
            write_port(temp_sb->string(), temp_sb->length());
          }
          
          //uint16_t xenomsg_id = session->nextMessage(&outbound_msg);
          //if (xenomsg_id) {
          //  if (write_port(outbound_msg.string(), outbound_msg.length()) ) {
          //    if (verbosity > 2) local_log.concatf("There was a problem writing to %s.\n", _addr);
          //  }
          //  return_value++;
          //}
          #ifdef __MANUVR_DEBUG
          else if (verbosity > 6) local_log.concat("Ignoring a broadcast that wasn't meant for us.\n");
          #endif
        }
        else {
          #ifdef __MANUVR_DEBUG
          if (verbosity > 3) local_log.concat("Session is chatting, but we don't appear to have a connection.\n");
          #endif
        }
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
        #ifdef __MANUVR_DEBUG
        if (verbosity > 3) local_log.concat("The com port class received an event that was addressed to it, that is not handled yet.\n");
        #endif
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

