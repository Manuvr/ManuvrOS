/*
File:   ManuvrTCP.h
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


This driver is designed to give Manuvr platform-abstracted socket connection.
This is basically only for linux until it is needed in a smaller space.
  
*/


#ifndef __MANUVR_SOCKET_H__
#define __MANUVR_SOCKET_H__

#include "../ManuvrXport.h"

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
    ManuvrTCP(char* addr, int port);
    ManuvrTCP(char* addr, int port, uint32_t opts);
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
    char*    _addr;
    int      _sock;
    uint32_t _options;

    int      _port_number;
    
    // Related to threading and pipes. This is linux-specific. 
    StringBuilder __io_buffer;
    int __parent_pid;
    int __blocking_pid;

    struct sockaddr_in _sockaddr;

    LinkedList<ManuvrTCP*> _connections;   // A list of client connections.
};

#endif   // __MANUVR_SOCKET_H__

