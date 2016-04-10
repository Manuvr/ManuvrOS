/*
File:   ManuvrSerial.cpp
Author: J. Ian Lindsay
Date:   2015.03.17

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
#include "XenoSession/XenoSession.h"

#include <Kernel.h>
#include <Platform/Platform.h>


#if defined (STM32F4XX)        // STM32F4


#elif defined(__MK20DX256__) | defined(__MK20DX128__)  // Teensy3.0/3.1

#elif defined (ARDUINO)        // Fall-through case for basic Arduino support.


#elif defined(__MANUVR_FREERTOS) || defined(__MANUVR_LINUX)
  #include <cstdio>
  #include <stdlib.h>
  #include <unistd.h>

  // Threaded platforms will need this to compensate for a loss of ISR.
  extern void* xport_read_handler(void* active_xport);
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
ManuvrSerial::ManuvrSerial(const char* tty_path, int b_rate) : ManuvrXport() {
  __class_initializer();
  _addr      = tty_path;
  _baud_rate = b_rate;
  _options   = 0;
}


/**
* Constructor.
*/
ManuvrSerial::ManuvrSerial(const char* tty_path, int b_rate, uint32_t opts) : ManuvrXport() {
  __class_initializer();
  _addr     = tty_path;
  _baud_rate = b_rate;
  _options  = opts;
}



/**
* Destructor
*/
ManuvrSerial::~ManuvrSerial() {
  __kernel->unsubscribe(this);
}



/**
* This is here for compatibility with C++ standards that do not allow for definition and declaration
*   in the header file. Takes no parameters, and returns nothing.
*/
void ManuvrSerial::__class_initializer() {
  _options           = 0;
  _sock              = 0;

  // Build some pre-formed Events.
  read_abort_event.repurpose(MANUVR_MSG_XPORT_QUEUE_RDY);
  read_abort_event.isManaged(true);
  read_abort_event.specific_target = (EventReceiver*) this;
  read_abort_event.originator      = (EventReceiver*) this;
  read_abort_event.priority        = 5;
  read_abort_event.addArg(xport_id);  // Add our assigned transport ID to our pre-baked argument.
}





/****************************************************************************************************
* Port I/O fxns                                                                                     *
****************************************************************************************************/



int8_t ManuvrSerial::init() {
  uint32_t xport_state_modifier = MANUVR_XPORT_FLAG_CONNECTED | MANUVR_XPORT_FLAG_LISTENING | MANUVR_XPORT_FLAG_INITIALIZED;
  #ifdef __MANUVR_DEBUG
  if (verbosity > 4) local_log.concatf("Resetting port %s...\n", _addr);
  #endif
  bytes_sent         = 0;
  bytes_received     = 0;

  #if defined (STM32F4XX)        // STM32F4

  #elif defined (__MK20DX128__)  // Teensy3

  #elif defined (__MK20DX256__)  // Teensy3.1

  #elif defined (ARDUINO)        // Fall-through case for basic Arduino support.

  #elif defined (__MANUVR_LINUX) // Linux environment
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
  set_xport_state(MANUVR_XPORT_FLAG_INITIALIZED);

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

    initialized(true);
    connected(true);
    listening(true);
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
    unsigned char *buf = (unsigned char *) alloca(256);
    int n;
    ManuvrRunnable *event    = NULL;
    StringBuilder  *nu_data  = NULL;

    #if defined (STM32F4XX)        // STM32F4

    #elif defined (__MK20DX128__)  // Teensy3

    #elif defined (__MK20DX256__)  // Teensy3.1

    #elif defined (ARDUINO)        // Fall-through case for basic Arduino support.

    #elif defined (__MANUVR_LINUX) // Linux with pthreads...
      while (connected()) {
        n = read(_sock, buf, 255);
        if (n > 0) {
          bytes_received += n;

          event = Kernel::returnEvent(MANUVR_MSG_XPORT_RECEIVE);
          nu_data = new StringBuilder(buf, n);
          event->markArgForReap(event->addArg(nu_data), true);

          //Do stuff regarding the data we just read...
          if (NULL != session) {
            session->notify(event);
          }
          else {
            raiseEvent(event);
          }
        }
        else {
          sleep_millis(50);
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
    int bytes_written = 0;
    #if defined (STM32F4XX)        // STM32F4

    #elif defined(STM32F7XX) | defined(STM32F746xx)
      bytes_written = out_len;
    #elif defined (__MK20DX128__)  // Teensy3

    #elif defined (__MK20DX256__)  // Teensy3.1

    #elif defined (ARDUINO)        // Fall-through case for basic Arduino support.
    #elif defined (__MANUVR_LINUX) // Linux
      bytes_written = (int) write(_sock, out, out_len);
    #else   // Unsupported.
    #endif

    bytes_sent += bytes_written;
    return true;
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
  read_abort_event.alterScheduleRecurrence(0);
  read_abort_event.alterSchedulePeriod(30);
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
int8_t ManuvrSerial::callback_proc(ManuvrRunnable *event) {
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



int8_t ManuvrSerial::notify(ManuvrRunnable *active_event) {
  int8_t return_value = 0;

  switch (active_event->event_code) {
    case MANUVR_MSG_XPORT_DEBUG:
      printDebug(&local_log);
      return_value++;
      break;

    default:
      return_value += ManuvrXport::notify(active_event);
      break;
  }

  if (local_log.length() > 0) Kernel::log(&local_log);
  return return_value;
}
