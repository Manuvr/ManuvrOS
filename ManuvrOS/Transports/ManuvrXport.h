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


This driver is designed to give Manuvr platform-abstracted transports.

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

For debuggability, the transport has a special mode for acting as a debug 
  console. 
*/


#ifndef __MANUVR_XPORT_H__
#define __MANUVR_XPORT_H__

#include "../Kernel.h"
#include "DataStructures/StringBuilder.h"

#include <ManuvrOS/Platform/Platform.h>

/*
* Notes about how transport flags are organized:
* This class is virtual, but contains the private member _xport_flags. All extending transport
*   classes are expected to store their own custom flags in this member via protected accessor.
*   The high 16-bits of the flags member is used for states and options common to all transports.
*/

/* State and option flags that apply to any transport...  */
#define MANUVR_XPORT_FLAG_INITIALIZED      0x80000000  // The xport was present and init'd corrently.
#define MANUVR_XPORT_FLAG_CONNECTED        0x40000000  // The xport is active and able to move data.
#define MANUVR_XPORT_FLAG_BUSY             0x20000000  // The xport is moving something.
#define MANUVR_XPORT_FLAG_HAS_SESSION      0x10000000  // See note below. 
#define MANUVR_XPORT_FLAG_LISTENING        0x08000000  // We are listening for connections.
#define MANUVR_XPORT_FLAG_IS_BRIDGED       0x04000000  // This transport instance is bridged to another.
#define MANUVR_XPORT_FLAG_NON_SESSION      0x02000000
#define MANUVR_XPORT_FLAG_DEBUG_CONSOLE    0x01000000
#define MANUVR_XPORT_FLAG_ALWAYS_CONNECTED 0x00800000

/*
* Note about MANUVR_XPORT_FLAG_HAS_SESSION:
* This might get cut. it ought to be sufficient to check if the session member is NULL.
*
* The behavior of this class, (and classes that extend it) ought to be as follows:
*   1) If a session is not present, the port simply moves data via the event system, hoping
*        that something else in the system cares.
*   2) If a session IS attached, the session should control all i/o on this port, as it holds
*        the protocol spec. So outside requests for data to be sent should be given to the session,
*        if not rejected entirely.
*/


class XenoSession;



class ManuvrXport : public EventReceiver {
  public:
    ManuvrXport();
    virtual ~ManuvrXport() {};

    /* Members that deal with sessions. */
    // TODO: Is this transport used for non-session purposes? IE, GPS? 
    inline bool hasSession() {           return (_xport_flags & MANUVR_XPORT_FLAG_HAS_SESSION);  };
    inline EventReceiver* getSession() { return session;  };

    /* Transport bridging... */
    inline bool isBridge() {           return (_xport_flags & MANUVR_XPORT_FLAG_IS_BRIDGED);  };
    int8_t bridge(ManuvrXport*);


    /*
    * Accessors for session behavior regarding connect/disconnect.
    */
    inline bool alwaysConnected() {         return (_xport_flags & MANUVR_XPORT_FLAG_ALWAYS_CONNECTED);  } 
    void alwaysConnected(bool en);


    /*
    * High-level data functions.
    */
    int8_t sendBuffer(StringBuilder* buf);


    /*
    * State imperatives.
    */
    virtual int8_t connect() = 0;
    virtual int8_t listen()  = 0;
    virtual int8_t reset()   = 0;


    /*
    * State accessors.
    */
    /* Connection/Listen states */
    inline bool connected() {   return (_xport_flags & (MANUVR_XPORT_FLAG_CONNECTED | MANUVR_XPORT_FLAG_ALWAYS_CONNECTED));  }
    void connected(bool);
    inline bool listening() {   return (_xport_flags & MANUVR_XPORT_FLAG_LISTENING);   };
    void listening(bool);
    
    /* Any required setup finished without problems? */
    inline bool initialized() { return (_xport_flags & MANUVR_XPORT_FLAG_INITIALIZED); };
    void initialized(bool en);

    /* Is this transport used for non-session purposes? IE, GPS? */
    inline bool nonSessionUsage() {         return (_xport_flags & MANUVR_XPORT_FLAG_NON_SESSION);  };
    inline void nonSessionUsage(bool en) {
      _xport_flags = (en) ? (_xport_flags | MANUVR_XPORT_FLAG_NON_SESSION) : (_xport_flags & ~(MANUVR_XPORT_FLAG_NON_SESSION));
    };

    /* Is this transport being used as a debug console? If so, we will hangup a session if it exists.    */
    inline bool isDebugConsole() {         return (_xport_flags & MANUVR_XPORT_FLAG_DEBUG_CONSOLE);  };
    void isDebugConsole(bool en);

    // Mandatory override.
    virtual bool   write_port(unsigned char* out, int out_len) = 0;
    virtual int8_t read_port() = 0;

    
    /* We will override these functions in EventReceiver. */
    // TODO: I'm not sure I've evaluated the full impact of this sort of 
    //    choice.  Calltimes? vtable size? alignment? Fragility? Dig.
    virtual void   printDebug(StringBuilder *);
    //virtual int8_t bootComplete();
    virtual int8_t notify(ManuvrRunnable*);
    //virtual int8_t callback_proc(ManuvrRunnable *);

    // We can have up-to 65535 transport instances concurrently. This well-exceeds
    //   the configured limits of most linux installations, so it should be enough. 
    static uint16_t TRANSPORT_ID_POOL;




  protected:
    EventReceiver *session;
    
    // Can also be used to poll the other side. Implementation is completely at the discretion
    //   any extending class. But generally, this feature is necessary.
    ManuvrRunnable read_abort_event;  // Used to timeout a read operation.
    //uint32_t pid_read_abort;       // Used to timeout a read operation.
    bool read_timeout_defer;       // Used to timeout a read operation.

    uint32_t _xport_mtu;      // The largest packet size we handle.
    
    uint32_t bytes_sent;
    uint32_t bytes_received;
    
    uint16_t xport_id;

    #if defined(__MANUVR_LINUX) | defined(__MANUVR_FREERTOS)
      // Threaded platforms have a concept of threads...
      unsigned long _thread_id;
    #endif

    // TODO: This is preventing us from encapsulating more deeply.
    //   The reason it isn't done is because there are instance-specific nuances in behavior that have
    //   to be delt with. A GPS device driver might be confused if we load a session. How about session hand-off
    //   to other transport instances? Until this behavior can be generalized, we rely on the extending class to
    //   handle undefined combinations as it chooses.
    //       ---J. Ian Lindsay   Thu Dec 03 03:37:41 MST 2015
    virtual int8_t reapXenoSession(XenoSession*);   // Cleans up XenoSessions that were instantiated by this class.
    virtual int8_t provide_session(XenoSession*);   // Called whenever we instantiate a session.

    // TODO: Should be private. provide_session() / reset() are the blockers.
    inline void set_xport_state(uint32_t bitmask) {    _xport_flags = (bitmask  | _xport_flags);   }
    inline void unset_xport_state(uint32_t bitmask) {  _xport_flags = (~bitmask & _xport_flags);   }



  private:
    uint32_t _xport_flags;
};


// TODO: Might we need a transport manager of some sort, for cases like TCP socket listenrs, etc...?

#endif   // __MANUVR_XPORT_H__

