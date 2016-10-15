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


#include <CommonConstants.h>
#include "ManuvrSerial.h"

#include <Kernel.h>
#include <Platform/Platform.h>
#include <XenoSession/XenoSession.h>


#if defined (STM32F4XX)        // STM32F4

#elif defined(__MK20DX256__) | defined(__MK20DX128__)  // Teensy3.0/3.1

#elif defined (ARDUINO)        // Fall-through case for basic Arduino support.

#elif defined(__MANUVR_LINUX)
  // Linux requires these libraries for serial port.
  #include <cstdio>
  #include <stdlib.h>
  #include <unistd.h>
#else
  // No special globals needed for this platform.
#endif


/*******************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here.
*******************************************************************************/


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/
/**
* Constructor.
*
* @param  tty_path  Path to the TTY.
* @param  b_rate    The port baud rate.
* @param  opts      Options to the underlying implementation.
*/
ManuvrSerial::ManuvrSerial(const char* tty_path, int b_rate, uint32_t opts) : ManuvrXport() {
  setReceiverName("ManuvrSerial");
  set_xport_state(MANUVR_XPORT_FLAG_STREAM_ORIENTED);
  _addr     = tty_path;
  _baud_rate = b_rate;
  _options  = opts;
}

/**
* Constructor.
*
* @param  tty_path  Path to the TTY.
* @param  b_rate    The port baud rate.
*/
ManuvrSerial::ManuvrSerial(const char* tty_path, int b_rate) : ManuvrSerial(tty_path, b_rate, 0) {
}

/**
* Destructor
*/
ManuvrSerial::~ManuvrSerial() {
}


/*******************************************************************************
*  _       _   _        _
* |_)    _|_ _|_ _  ._ |_) o ._   _
* |_) |_| |   | (/_ |  |   | |_) (/_
*                            |
* Overrides and addendums to BufferPipe.
*******************************************************************************/
/**
* Inward toward the transport.
*
* @param  buf    A pointer to the buffer.
* @param  mm     A declaration of memory-management responsibility.
* @return A declaration of memory-management responsibility.
*/
int8_t ManuvrSerial::toCounterparty(StringBuilder* buf, int8_t mm) {
  uint8_t* ptr = buf->string();
  int      len = buf->length();
  bool     res = write_port(ptr, len);

  if (res) {
    buf->clear();
  }
  return (res ? MEM_MGMT_RESPONSIBLE_BEARER : MEM_MGMT_RESPONSIBLE_CALLER);
}



/*******************************************************************************
* ___________                                                  __
* \__    ___/___________    ____   ____________   ____________/  |_
*   |    |  \_  __ \__  \  /    \ /  ___/\____ \ /  _ \_  __ \   __\
*   |    |   |  | \// __ \|   |  \\___ \ |  |_> >  <_> )  | \/|  |
*   |____|   |__|  (____  /___|  /____  >|   __/ \____/|__|   |__|
*                       \/     \/     \/ |__|
* These members are particular to the transport driver and any implicit
*   protocol it might contain.
*******************************************************************************/

int8_t ManuvrSerial::init() {
  uint32_t xport_state_modifier = MANUVR_XPORT_FLAG_CONNECTED | MANUVR_XPORT_FLAG_LISTENING | MANUVR_XPORT_FLAG_INITIALIZED;
  #ifdef __MANUVR_DEBUG
    if (getVerbosity() > 4) local_log.concatf("Resetting port %s...\n", _addr);
  #endif
  bytes_sent         = 0;
  bytes_received     = 0;

  #if defined (STM32F4XX)        // STM32F4

  #elif defined (__MK20DX128__) || defined (__MK20DX256__) || defined (ARDUINO)
    // Arduino, Teensyduino, chipKIT, etc
    if (nullptr == _addr) {
      if (getVerbosity() > 1) {
        local_log.concatf("No serial port ID supplied.\n", *_addr);
        flushLocalLog();
      }
      return -1;
    }
    switch (*_addr) {
      case 'U':   // This is the USB driver.
        Serial.begin(_baud_rate);
        break;
      #if defined (__MK20DX256__)  // Teensy3.1 has more options.
      #endif
      default:
        if (getVerbosity() > 1) {
          local_log.concatf("Unsupported serial port: %c...\n", *_addr);
          flushLocalLog();
        }
        return -1;
    }
    initialized(true);
    connected(true);
    listening(true);

  #elif defined (__MANUVR_LINUX) // Linux environment
    if (_sock) {
      close(_sock);
    }

    _sock = open(_addr, _options);
    if (_sock == -1) {
      #ifdef __MANUVR_DEBUG
        if (getVerbosity() > 1) local_log.concatf("Unable to open port: (%s)\n", _addr);
        flushLocalLog();
      #endif
      unset_xport_state(xport_state_modifier);
      return -1;
    }
    #ifdef __MANUVR_DEBUG
      if (getVerbosity() > 4) local_log.concatf("Opened port (%s) at %d\n", _addr, _baud_rate);
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
        if (getVerbosity() > 6) local_log.concatf("Port opened, and handler bound.\n");
      #endif //__MANUVR_DEBUG
    }
    else {
      unset_xport_state(xport_state_modifier);
      initialized(false);
      #ifdef __MANUVR_DEBUG
        if (getVerbosity() > 1) local_log.concatf("Failed to tcsetattr...\n");
      #endif
    }
  #endif //LINUX

  flushLocalLog();
  return 0;
}



int8_t ManuvrSerial::connect() {
  // We're a serial port. If we are initialized, we are always connected.
  return 0;
}

int8_t ManuvrSerial::disconnect() {
  ManuvrXport::disconnect();
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
  int8_t return_value = 0;
  if (connected()) {
    uint8_t* buf;
    int n = 0;
    #if defined (STM32F4XX)        // STM32F4

    #elif defined (__MK20DX128__) || defined (__MK20DX256__) // Teensy3.x
      int available = Serial.available();

      if (available) {
        buf = (uint8_t*) malloc(available + 1);
        if (buf) {
          for (n = 0; n < available; n++) {
            *(buf + n++) = Serial.read();
          }
          bytes_received += n;
        }
        *(buf + n) = '\0';  // NULL-terminate, JIC
        StringBuilder temp;
        temp.concatHandoff(buf, available);
        BufferPipe::fromCounterparty(&temp, MEM_MGMT_RESPONSIBLE_BEARER);
        return_value = 1;
      }
    #elif defined (ARDUINO)        // Fall-through case for basic Arduino support.
        if (Serial.available()) {
          while (Serial.available()) {
            *(buf + n++) = Serial.read();
          }
          bytes_received += n;
          *(buf + n) = '\0';  // NULL-terminate, JIC
          BufferPipe::fromCounterparty(buf, n, MEM_MGMT_RESPONSIBLE_BEARER);
          return_value = 1;
        }

    #elif defined (__MANUVR_LINUX) // Linux with pthreads...
        n = read(_sock, buf, 255);
        if (n > 0) {
          bytes_received += n;
          BufferPipe::fromCounterparty(buf, n, MEM_MGMT_RESPONSIBLE_BEARER);
          return_value = 1;
        }
    #endif
  }
  else {
    #ifdef __MANUVR_DEBUG
    if (getVerbosity() > 5) local_log.concat("Somehow we are trying to read a port that is not marked as open.\n");
    flushLocalLog();
    #endif
  }
  return return_value;
}


/**
* Does what it claims to do on linux.
* Returns false on error and true on success.
*/
bool ManuvrSerial::write_port(unsigned char* out, int out_len) {
  if (connected()) {
    int bytes_written = 0;

    #if defined (STM32F4XX)        // STM32F4

    #elif defined(STM32F7XX) | defined(STM32F746xx)
      bytes_written = out_len;
    #elif defined (__MK20DX128__) || defined (__MK20DX256__) // Teensy3.x
      Serial.print((char*) out);
      bytes_written = out_len;
    #elif defined (ARDUINO)        // Fall-through case for basic Arduino support.

    #elif defined (__MANUVR_LINUX) // Linux
      if (_sock == -1) {
        #ifdef __MANUVR_DEBUG
          if (getVerbosity() > 2) {
            local_log.concatf("Unable to write to transport: %s\n", _addr);
            Kernel::log(&local_log);
          }
        #endif
        return false;
      }
      bytes_written = (int) write(_sock, out, out_len);
    #else   // Unsupported.
    #endif

    bytes_sent += bytes_written;
    return true;
  }
  return false;
}



/*******************************************************************************
* ######## ##     ## ######## ##    ## ########  ######
* ##       ##     ## ##       ###   ##    ##    ##    ##
* ##       ##     ## ##       ####  ##    ##    ##
* ######   ##     ## ######   ## ## ##    ##     ######
* ##        ##   ##  ##       ##  ####    ##          ##
* ##         ## ##   ##       ##   ###    ##    ##    ##
* ########    ###    ######## ##    ##    ##     ######
*
* These are overrides from EventReceiver interface...
*******************************************************************************/

/**
* This is called when the kernel attaches the module.
* This is the first time the class can be expected to have kernel access.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t ManuvrSerial::attached() {
  if (EventReceiver::attached()) {
    read_abort_event.repurpose(MANUVR_MSG_XPORT_QUEUE_RDY, (EventReceiver*) this);
    read_abort_event.isManaged(true);
    read_abort_event.specific_target = (EventReceiver*) this;
    read_abort_event.priority(2);
    // Tolerate 30ms of latency on the line before flushing the buffer.
    read_abort_event.alterSchedulePeriod(30);
    read_abort_event.autoClear(false);
    #if !defined (__BUILD_HAS_THREADS)
      read_abort_event.enableSchedule(true);
      read_abort_event.alterScheduleRecurrence(-1);
      platform.kernel()->addSchedule(&read_abort_event);
    #else
      read_abort_event.alterScheduleRecurrence(0);
    #endif
    reset();
    return 1;
  }
  return 0;
}


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
  #if defined(__MANUVR_LINUX)
    temp->concatf("-- _sock           0x%08x\n", _sock);
  #endif
  temp->concatf("-- Baud            %d\n",     _baud_rate);
  temp->concatf("-- Class size      %d\n",     sizeof(ManuvrSerial));
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
int8_t ManuvrSerial::callback_proc(ManuvrMsg* event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = event->kernelShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->eventCode()) {
    case MANUVR_MSG_XPORT_SEND:
      event->clearArgs();
      break;
    default:
      break;
  }

  return return_value;
}


int8_t ManuvrSerial::notify(ManuvrMsg* active_event) {
  int8_t return_value = 0;

  switch (active_event->eventCode()) {
    case MANUVR_MSG_XPORT_QUEUE_RDY:
      read_port();
      return_value++;
      break;

    default:
      return_value += ManuvrXport::notify(active_event);
      break;
  }

  flushLocalLog();
  return return_value;
}
