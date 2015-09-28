/*
File:   ManuvrSocket.h
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


#ifndef __MANUVR_COM_PORT_H__
#define __MANUVR_COM_PORT_H__

#include "../ManuvrXport.h"


#if defined (STM32F4XX)        // STM32F4

#else   //Assuming a linux environment. Cross your fingers....
  #include <fcntl.h>
  #include <termios.h>
  #include <sys/signal.h>
#endif

class XenoSession;


#define MANUVR_XPORT_STATE_UNINITIALIZED  0b00000000  // Not treated as a bit-mask. Just a nice label.
#define MANUVR_XPORT_STATE_INITIALIZED    0b10000000  // The com port was present and init'd corrently.
#define MANUVR_XPORT_STATE_CONNECTED      0b01000000  // The com port is active and able to move data.
#define MANUVR_XPORT_STATE_BUSY           0b00100000  // The com port is moving something.
#define MANUVR_XPORT_STATE_HAS_SESSION    0b00010000  // See note below. 
#define MANUVR_XPORT_STATE_LISTENING      0b00001000  // We are listening for connections.

/*
* Note about MANUVR_XPORT_STATE_HAS_SESSION:
* This might get cut. it ought to be sufficient to check if the session member is NULL.
*
* The behavior of this class, (and classes that extend it) ought to be as follows:
*   1) If a session is not present, the port simply moves data via the event system, hoping
*        that something else in the system cares.
*   2) If a session IS attached, the session should control all i/o on this port, as it holds
*        the protocol spec. So outside requests for data to be sent should be given to the session,
*        if not rejected entirely.
*/

class ManuvrSocket : public ManuvrXport {
  public:
    ManuvrSocket();
    ~ManuvrSocket();
    
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
    const char* tty_name;
    int port_number;
    int baud_rate;
    uint32_t options;
    
    bool read_timeout_defer;       // Used to timeout a read operation.
    uint32_t pid_read_abort;       // Used to timeout a read operation.
    ManuvrEvent read_abort_event;  // Used to timeout a read operation.
    
};

#endif   // __MANUVR_COM_PORT_H__

