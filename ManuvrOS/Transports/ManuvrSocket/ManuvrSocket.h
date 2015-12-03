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


This driver is designed to give Manuvr platform-abstracted COM ports. By
  this is meant generic asynchronous serial ports. On Arduino, this means
  the Serial (or HardwareSerial) class. On linux, it means /dev/tty<x>.

Platforms that require it should be able to extend this driver for specific 
  kinds of hardware support. For an example of this, I would refer you to
  the STM32F4 case-offs I understand that this might seem "upside down"
  WRT how drivers are more typically implemented, and it may change later on.
  But for now, it seems like a good idea.  
*/


#ifndef __MANUVR_SOCKET_H__
#define __MANUVR_SOCKET_H__

#include "../ManuvrXport.h"


#if defined (MANUVR_SUPPORT_TCPSOCKET)
  //Assuming a linux environment. Cross your fingers....
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
#endif


// TODO: Might generalize UDP and websocket support into this. For now, we only deal in TCP.
// If generalization takes place, we should probably have a pure interface class "ManuvrSocket"
//   that handles all the common-gound.
class ManuvrTCP : public ManuvrXport {
  public:
    ManuvrTCP(char* addr, int port);
    ManuvrTCP(char* addr, int port, uint32_t opts);
    ~ManuvrTCP();
    
    /* Overrides from EventReceiver */
    int8_t bootComplete();
    const char* getReceiverName();
    void printDebug(StringBuilder *);
    int8_t notify(ManuvrEvent*);
    int8_t callback_proc(ManuvrEvent *);


    // TODO: Migrate to Xport class.
    inline bool initialized() { return (xport_state & MANUVR_XPORT_STATE_INITIALIZED);  }

    XenoSession* getSession();

    int8_t read_port();
    int8_t reset();
    
    int8_t listen(bool);

    virtual int8_t sendBuffer(StringBuilder*);
    bool write_port(unsigned char* out, int out_len);


  protected:
    /*
    * The session member is not part of the Xport class because not all Xports have sessions,
    *   and some that DO might have many sessions at once.
    */
    XenoSession *session;
    uint16_t xport_id;
    uint8_t  xport_state;
    
    uint32_t bytes_sent;
    uint32_t bytes_received;
    
    void __class_initializer();

    /* Members that deal with sessions. */
    int8_t provide_session(XenoSession*);   // Called whenever we instantiate a session.


  private:
    char*    _addr;
    int      _sock;
    uint32_t _options;

    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;

    int      _port_number;
};

#endif   // __MANUVR_SOCKET_H__

