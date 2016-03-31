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

#include "Transports/ManuvrXport.h"

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
#else
  // No supportage.
#endif



// TODO: Might generalize UDP and websocket support into this. For now, we only deal in TCP.
// If generalization takes place, we should probably have a pure interface class "ManuvrSocket"
//   that handles all the common-gound.
class ManuvrTCP : public ManuvrXport {
  public:
    ManuvrTCP(const char* addr, int port);
    ManuvrTCP(const char* addr, int port, uint32_t opts);
    ManuvrTCP(ManuvrTCP* listening_instance, int nu_sock, struct sockaddr_in* nu_sockaddr);
    ~ManuvrTCP();

    /* Overrides from EventReceiver */
    int8_t bootComplete();
    const char* getReceiverName();
    void printDebug(StringBuilder *);
    int8_t notify(ManuvrRunnable*);
    int8_t callback_proc(ManuvrRunnable *);


    int8_t connect();
    int8_t listen();
    int8_t reset();

    bool write_port(unsigned char* out, int out_len);
    bool write_port(int sock, unsigned char* out, int out_len);
    int8_t read_port();

    inline int getSockID() {  return _sock; };

  protected:
    void __class_initializer();


  private:
    const char* _addr;
    int         _sock;
    uint32_t    _options;

    int         _port_number;

    // Related to threading and pipes. This is linux-specific.
    StringBuilder __io_buffer;
    int __parent_pid;
    int __blocking_pid;

    #if defined(__MANUVR_LINUX)
      struct sockaddr_in _sockaddr;
    #endif

    LinkedList<ManuvrTCP*> _connections;   // A list of client connections.
};

#endif   // __MANUVR_SOCKET_H__
