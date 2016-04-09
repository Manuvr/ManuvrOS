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
  #include <arpa/inet.h>
#else
  // No supportage.
#endif


#if defined(MANUVR_SUPPORT_TCPSOCKET) | defined(MANUVR_SUPPORT_UDP)

class ManuvrSocket : public ManuvrXport {
  public:
    ManuvrSocket(const char* addr, int port);
    ManuvrSocket(const char* addr, int port, uint32_t opts);
    ~ManuvrSocket();

  protected:
    const char* _addr;
    int         _port_number;
    uint32_t    _options;

  private:
    void __class_initializer();
};

#endif // General socket support



#if defined(MANUVR_SUPPORT_TCPSOCKET)

// TODO: Might generalize UDP and websocket support into this. For now, we only deal in TCP.
// If generalization takes place, we should probably have a pure interface class "ManuvrSocket"
//   that handles all the common-gound.
class ManuvrTCP : public ManuvrSocket {
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


  private:
    int         _sock;

    // Related to threading and pipes. This is linux-specific.
    StringBuilder __io_buffer;
    int __parent_pid;
    int __blocking_pid;

    #if defined(__MANUVR_LINUX)
      struct sockaddr_in _sockaddr;
    #endif

    LinkedList<ManuvrTCP*> _connections;   // A list of client connections.

    void __class_initializer();
};

#endif  // MANUVR_SUPPORT_TCPSOCKET


#if defined(MANUVR_SUPPORT_UDP)

#define MANUVR_MSG_UDP_RX  0xF544
#define MANUVR_MSG_UDP_TX  0xF545

class ManuvrUDP : public EventReceiver {
  public:
    // TODO: Expediency. Triage.... Make private.
    uint32_t bytes_sent;
    uint32_t bytes_received;

    ManuvrUDP(const char* addr, int port);
    ManuvrUDP(const char* addr, int port, uint32_t opts);
    ~ManuvrUDP();

    int8_t listen();
    inline bool listening() {       return (_xport_flags & MANUVR_XPORT_FLAG_LISTENING);   };
    inline void listening(bool) {   _xport_flags |= MANUVR_XPORT_FLAG_LISTENING;   };

    bool write_datagram(unsigned char* out, int out_len, const char* addr, int port, uint32_t opts);
    inline bool write_datagram(unsigned char* out, int out_len, const char* addr, int port) {
      return write_datagram(out, out_len, addr, port, 0);
    };
    int8_t read_port();

    /* Overrides from EventReceiver */
    int8_t notify(ManuvrRunnable*);
    int8_t callback_proc(ManuvrRunnable *);
    void procDirectDebugInstruction(StringBuilder *);
    const char* getReceiverName();
    void printDebug(StringBuilder*);

    inline int getSockID() {  return _sock; };


    // UDP is connectionless. We should only have a single instance of this class.
    volatile static ManuvrUDP* INSTANCE;


  protected:
    void __class_initializer();
    int8_t bootComplete();


  private:
    ManuvrRunnable read_abort_event;  // Used to timeout a read operation.
    uint32_t _xport_flags;

    const char* _addr;
    int         _sock;
    int         _client_sock;
    uint32_t    _options;

    int         _port_number;

    // Related to threading and pipes. This is linux-specific.
    StringBuilder __io_buffer;
    int __parent_pid;
    int __blocking_pid;

    #if defined(__MANUVR_LINUX) | defined(__MANUVR_FREERTOS)
      // Threaded platforms have a concept of threads...
      unsigned long _thread_id;
      #if defined(__MANUVR_LINUX)
        struct sockaddr_in _sockaddr;
      #endif
    #endif
};


#endif  // MANUVR_SUPPORT_UDP

#endif   // __MANUVR_SOCKET_H__
