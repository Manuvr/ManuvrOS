/*
File:   ManuvrSocket.cpp
Author: J. Ian Lindsay
Date:   2015.09.17

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


This driver is designed to give Manuvr platform-abstracted sockets. By
  this is meant generic asynchronous serial ports. On Arduino, this means
  the Serial (or HardwareSerial) class. On linux, it means /dev/tty<x>.

Platforms that require it should be able to extend this driver for specific
  kinds of hardware support. For an example of this, I would refer you to
  the STM32F4 case-offs I understand that this might seem "upside down"
  WRT how drivers are more typically implemented, and it may change later on.
  But for now, it seems like a good idea.
*/


#if defined(MANUVR_SUPPORT_TCPSOCKET) | defined(MANUVR_SUPPORT_UDP)

#include "ManuvrSocket.h"
#include <CommonConstants.h>

#include <Kernel.h>
#include <Platform/Platform.h>

#if defined (STM32F4XX)        // STM32F4

#elif defined (__MANUVR_LINUX)      // Linux environment
  #include <cstdio>
  #include <stdlib.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <sys/signal.h>
  #include <fstream>
  #include <iostream>
  #include <sys/socket.h>


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
*/
ManuvrSocket::ManuvrSocket(const char* addr, int port, uint32_t opts) : ManuvrXport() {
  setReceiverName("ManuvrSocket");
  _port_number = port;
  _options     = opts;     // These will vary across UDP/WS/TCP.

  if (addr) {
    size_t addr_len = strlen(addr);
    if (0 < addr_len) {
      _addr = (char*) malloc(addr_len+1);
      if (_addr) {
        *(_addr + addr_len) = 0;
        for (size_t i = 0; i < addr_len; i++) {
          *(_addr + i) = *(addr + i);
        }
      }
    }
  }

  #if defined(__MANUVR_LINUX)
    _sock              = 0;
    // Zero the socket parameter structures.
    for (uint16_t i = 0; i < sizeof(_sockaddr);  i++) {
      *((uint8_t *) &_sockaddr + i) = 0;
    }
  #endif

  // Build some pre-formed Events.
  read_abort_event.repurpose(MANUVR_MSG_XPORT_QUEUE_RDY, (EventReceiver*) this);
  read_abort_event.incRefs();
  read_abort_event.specific_target = (EventReceiver*) this;
}


/**
* Destructor
*/
ManuvrSocket::~ManuvrSocket() {
  if (_sock) {
    close(_sock);  // Close the socket.
    _sock = 0;
  }
  if (_addr) {
    free(_addr);
    _addr = nullptr;
  }
}


/**
* On linux, socket cleanup is fairly uniform...
*/
int8_t ManuvrSocket::disconnect() {
  if (listening()) {
    Kernel::log("Socket listener shut down.");
    listening(false);
  }

  if (connected()) {
    Kernel::log("Socket disconnection (client).");
    connected(false);
  }

  if (_sock) {
    close(_sock);  // Close the socket.
    _sock = 0;
  }
  ManuvrXport::disconnect();
  return 0;
}


#else   //Unsupportedness
#endif  // __LINUX


#endif  // Socket support?
