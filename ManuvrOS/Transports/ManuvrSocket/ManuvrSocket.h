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
#else
  // No supportage.
#endif


#if defined(MANUVR_SUPPORT_TCPSOCKET) | defined(MANUVR_SUPPORT_UDP)

/*
* This is a wrapper around sockets as they exist in a linux system.
* TODO: This might be an appropriate place for lwip?
*/
class ManuvrSocket : public ManuvrXport {
  public:
    ManuvrSocket(const char* addr, int port, uint32_t opts);
    ~ManuvrSocket();

    inline int getSockID() {  return _sock; };

    virtual int8_t disconnect();


  protected:
    const char* _addr;
    int         _port_number;
    uint32_t    _options;
    int         _sock;

    #if defined(__MANUVR_LINUX)
      struct sockaddr_in _sockaddr;
    #endif

  private:
    void __class_initializer();
};



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


  protected:
    int8_t bootComplete();


  private:
    LinkedList<ManuvrTCP*> _connections;   // A list of client connections.
};

#endif  // MANUVR_SUPPORT_TCPSOCKET


#if defined(MANUVR_SUPPORT_UDP)
/*
* UDP presents a clash with Manuvr's xport<--->session relationships.
* The fundamental problem is: If we want bi-directional communication, we can
*   only respond to individual packets we receive. The is no "connection", and
*   no session. So this class _must_ have some notion of a session that only
*   lasts for the duration of a request if we are to avoid passing return-path
*   data around with the buffers (see UDPPipe below).
*
*
* Furthermore, Manuvr's xport classes were not designed with multicast in mind.
*   The turmoil will continue until these issues have been properly solved.
*
*/

/*
* To faciliate the usage of this transport in a manner consistent with a
*   connectionless, multicast protocol, we use the messaging system as an
*   additional means of transacting with this transport.
* If the class is listening, and a packet arrives without an attached BufferPipe
*   into which to direct the packet, the class will emit it into the message
*   bus.
*/
#define MANUVR_MSG_UDP_RX  0xF544
#define MANUVR_MSG_UDP_TX  0xF545

class ManuvrUDP;

/*
* We use this to track packets and replies so that addressing information does
*   not need to leave the class, thereby damaging the abstraction.
*/
class UDPPipe : public BufferPipe {
  public:
    UDPPipe();
    UDPPipe(ManuvrUDP*, uint32_t, uint16_t);
    UDPPipe(ManuvrUDP*, BufferPipe*);
    ~UDPPipe();

    /* Override from BufferPipe. */
    virtual int8_t toCounterparty(uint8_t* buf, unsigned int len, int8_t mm);
    virtual int8_t fromCounterparty(uint8_t* buf, unsigned int len, int8_t mm);
    inline int8_t fromCounterparty(uint8_t* buf, unsigned int len) {
      return fromCounterparty(buf, len, _far_mm_default);
    };

    void printDebug(StringBuilder*);
    int takeAccumulator(StringBuilder*);
    bool persistAfterReply();


  private:
    uint16_t      _port;
    uint16_t      _flags;
    uint32_t      _ip;
    ManuvrUDP*    _udp;
    StringBuilder _accumulator;   // Holds an incoming packet prior to setFar().
};



class ManuvrUDP : public ManuvrSocket, BufferPipe {
  public:
    ManuvrUDP(const char* addr, int port);
    ManuvrUDP(const char* addr, int port, uint32_t opts);
    ~ManuvrUDP();

    /* Override from BufferPipe. */
    virtual int8_t toCounterparty(uint8_t* buf, unsigned int len, int8_t mm);
    virtual int8_t fromCounterparty(uint8_t* buf, unsigned int len, int8_t mm);

    int8_t connect();
    int8_t listen();
    int8_t reset();
    int8_t read_port();
    bool write_port(unsigned char* out, int out_len);

    bool write_datagram(unsigned char* out, int out_len, uint32_t addr, int port, uint32_t opts);
    bool write_datagram(unsigned char* out, int out_len, const char* addr, int port, uint32_t opts);
    inline bool write_datagram(unsigned char* out, int out_len, const char* addr, int port) {
      return write_datagram(out, out_len, addr, port, 0);
    };

    /* Overrides from EventReceiver */
    int8_t notify(ManuvrRunnable*);
    int8_t callback_proc(ManuvrRunnable *);
    const char* getReceiverName();
    void printDebug(StringBuilder*);
    #if defined(__MANUVR_CONSOLE_SUPPORT)
      void procDirectDebugInstruction(StringBuilder*);
    #endif  //__MANUVR_CONSOLE_SUPPORT


    // UDP is connectionless. We should only have a single instance of this class.
    volatile static ManuvrUDP* INSTANCE;


  protected:
    int8_t bootComplete();


  private:
    // We index our spawned UDPPipes by counterparty port number.
    std::map<uint16_t, UDPPipe*> _open_replies;
    int         _client_sock;
};


#endif  // MANUVR_SUPPORT_UDP
#endif  // General socket support
#endif  // Header guard __MANUVR_SOCKET_H__
