/*
File:   ManuvrXport.h
Author: J. Ian Lindsay
Date:   2015.03.17

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
  It might also be a network socket.
  
Platforms that require it should be able to extend this driver for specific 
  kinds of hardware support. For an example of this, I would refer you to
  the RN42HID driver. I understand that this might seem "upside down"
  WRT how drivers are more typically implemented, and it may change later on.
  But for now, it seems like a good idea.  

XenoSessions are optionally established by this class, and broadcast to the
  rest of the system (MANUVR_MSG_SESS_ESTABLISHED). By the time that has
  happened, the session ought to have become sync'd, self-identified, 
  authenticated, etc. Therefore, session configuration must be done on a
  per-transport basis. This means that authentication (for instance) is
  transport-wide.
  
If sessions are enabled for a transport, the highest-level of the protocol
  touched by this class ought to be dealing with sync.
  
For non-session applications of this class, session-creation and management
  can be disabled. This would be appropriate in cases such as GPS, modems,
  and generally, anything that isn't manuvrable.

Note that this class does not require a XenoSession pointer. If applicable
  to a given transport, that should be handled in the class extending this 
  one.

For debuggability, the transport has a special mode for acting as a debug 
  console. 
*/


#ifndef __MANUVR_XPORT_H__
#define __MANUVR_XPORT_H__

#include "../Kernel.h"
#include "StringBuilder/StringBuilder.h"

#define MANUVR_XPORT_FLAG_ALWAYS_CONNECTED 0x00000001
#define MANUVR_XPORT_FLAG_NON_SESSION      0x00000002
#define MANUVR_XPORT_FLAG_DEBUG_CONSOLE    0x00000004
#define MANUVR_XPORT_FLAG_CONNECTED        0x00000008
#define MANUVR_XPORT_FLAG_HAS_SESSION      0x00000010


class XenoSession;

class ManuvrXport : public EventReceiver {
  public:
    ManuvrXport();
    virtual ~ManuvrXport() {};

    ///* Overrides from EventReceiver */
    //virtual int8_t bootComplete()               = 0;
    //virtual const char* getReceiverName()       = 0;
    //virtual void printDebug(StringBuilder *)    = 0;
    //virtual int8_t notify(ManuvrEvent*)         = 0;
    //virtual int8_t callback_proc(ManuvrEvent *) = 0;
    
    
    /*
    * These are overrides that need to be supported by any class extending this one.
    */ 
    virtual int8_t sendBuffer(StringBuilder*) = 0;


    virtual int8_t reapXenoSession(XenoSession*);   // Cleans up XenoSessions that were instantiated by this class.
    

    /*
    * Is this transport connected? 
    */
    inline bool connected() {         return (_xport_flags & (MANUVR_XPORT_FLAG_CONNECTED | MANUVR_XPORT_FLAG_ALWAYS_CONNECTED));  } 
    virtual void connected(bool en);

    /*
    * Is this transport used for non-session purposes? IE, GPS? 
    */
    inline bool hasSession() {         return (_xport_flags & MANUVR_XPORT_FLAG_HAS_SESSION);  } 

    /*
    * Is this transport used for non-session purposes? IE, GPS? 
    */
    inline bool nonSessionUsage() {         return (_xport_flags & MANUVR_XPORT_FLAG_NON_SESSION);  } 
    inline void nonSessionUsage(bool en) {
      _xport_flags = (en) ? (_xport_flags | MANUVR_XPORT_FLAG_NON_SESSION) : (_xport_flags & ~(MANUVR_XPORT_FLAG_NON_SESSION));
    }

    /*
    * Accessors for session behavior regarding connect/disconnect.
    */
    inline bool alwaysConnected() {         return (_xport_flags & MANUVR_XPORT_FLAG_ALWAYS_CONNECTED);  } 
    void alwaysConnected(bool en);
    
    /*
    * Is this transport being used as a debug console? If so, we will hangup a session if
    *   it exists.
    */
    inline bool isDebugConsole() {         return (_xport_flags & MANUVR_XPORT_FLAG_DEBUG_CONSOLE);  } 
    void isDebugConsole(bool en);

    
    static uint16_t TRANSPORT_ID_POOL;

    
    
  protected:
    uint32_t _xport_flags;
    uint32_t _xport_mtu;      // The largest packet size we handle.
    
    virtual int8_t provide_session(XenoSession*) = 0;   // Called whenever we instantiate a session.

    
  private:
    
};


// TODO: Might we need a transport manager of some sort, for cases like TCP socket listenrs, etc...?

#endif   // __MANUVR_XPORT_H__

