/*
File:   ManuvrTCP.h
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


This driver is designed to give Manuvr platform-abstracted socket connection.
This is basically only for linux until it is needed in a smaller space.

*/


#ifndef __MANUVR_SOCKET_H__
#define __MANUVR_SOCKET_H__

#include <Transports/ManuvrXport.h>
#include <XenoSession/XenoSession.h>
#include <DataStructures/BufferPipe.h>

#if defined(__MANUVR_LINUX)
  #include <cstdio>
  #include <stdlib.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <termios.h>
  #include <sys/signal.h>
  #include <fstream>
  #include <iostream>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
#elif defined(__MANUVR_ESP32)
  #include "lwip/err.h"
  #include "lwip/sockets.h"
  #include "lwip/sys.h"
#else
  // No supportage.
#endif


class SocketOpts {
  public:
  private:
    uint32_t _flags = 0;
};

/*
* This is a wrapper around sockets as they exist in a linux system.
* TODO: This might be an appropriate place for lwip?
*/
class ManuvrSocket : public ManuvrXport {
  public:
    virtual ~ManuvrSocket();

    inline int getSockID() {  return _sock; };
    virtual int8_t disconnect();


  protected:
    SocketOpts* _opts        = 0;
    char*       _addr        = nullptr;
    int         _port_number = 0;
    int         _sock        = 0;
    struct sockaddr_in _sockaddr;

    ManuvrSocket(const char* nom, const char* addr, int port, SocketOpts* opts);


  private:
};

#endif  // Header guard __MANUVR_SOCKET_H__
