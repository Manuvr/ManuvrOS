/*
File:   ManuvrXport.h
Author: J. Ian Lindsay
Date:   2015.03.17

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


This driver is designed to give Manuvr platform-abstracted transports.
*/


#ifndef __MANUVR_XPORT_H__
#define __MANUVR_XPORT_H__

#include <Kernel.h>
#include <DataStructures/StringBuilder.h>
#include <DataStructures/BufferPipe.h>

#include <Platform/Platform.h>


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
#define MANUVR_XPORT_FLAG_STREAM_ORIENTED  0x10000000  // See note below.
#define MANUVR_XPORT_FLAG_LISTENING        0x08000000  // We are listening for connections.
#define MANUVR_XPORT_FLAG_RESERVED_1       0x04000000  //
#define MANUVR_XPORT_FLAG_RESERVED_2       0x02000000  //
#define MANUVR_XPORT_FLAG_RESERVED_0       0x01000000  //
#define MANUVR_XPORT_FLAG_ALWAYS_CONNECTED 0x00800000  // Serial ports.
#define MANUVR_XPORT_FLAG_CONNECTIONLESS   0x00400000  // This transport is "connectionless". See Note0 below.
#define MANUVR_XPORT_FLAG_HAS_MULTICAST    0x00200000  // This transport supports multicast.
#define MANUVR_XPORT_FLAG_AUTO_CONNECT     0x00100000  // This transport should periodically attempt to connect.
#define MANUVR_XPORT_FLAG_REAP_SESSION     0x00080000  // This transport is responsible for managing sesion allocation.

/**
* Note0:
* Until recently, Manuvr only supported stream-oriented transports. This was a conscious
*   design choice. More elaboration on this is beyond the scope of code commentary, but
*   suffice it to say that there will likely be some upheaval in the transport abstraction
*   as it is generallized to encompass datagrams, small MTU radio links, and so-forth.
* More explanation to come once code is committed.
*      ---J. Ian Lindsay 2016.04.08
*/


/*
* Note about MANUVR_XPORT_FLAG_STREAM_ORIENTED:
* This might get cut.
*
* The behavior of this class, (and classes that extend it) ought to be as follows:
*   1) If a session is not present, the port simply moves data via the event system, hoping
*        that something else in the system cares.
*   2) If a session IS attached, the session should control all i/o on this port, as it holds
*        the protocol spec. So outside requests for data to be sent should be given to the session,
*        if not rejected entirely.
*/

#define XPORT_DEFAULT_AUTOCONNECT_PERIOD 15000  // In ms. Unless otherwise specified...

class XenoSession;


class ManuvrXport : public EventReceiver, public BufferPipe {
  public:
    virtual ~ManuvrXport();

    /* Override from BufferPipe. */
    virtual int8_t toCounterparty(ManuvrPipeSignal, void*);
    virtual inline int8_t toCounterparty(StringBuilder* buf, int8_t mm) {
      return BufferPipe::toCounterparty(buf, mm);
    };
    virtual inline int8_t fromCounterparty(StringBuilder* buf, int8_t mm) {
      return BufferPipe::fromCounterparty(buf, mm);
    };

    /*
    * State imperatives.
    */
    virtual int8_t connect()    = 0;
    virtual int8_t disconnect();
    virtual int8_t listen()     = 0;
    virtual int8_t reset()      = 0;

    // TODO: This is going to be cut in favor of BufferPipe's API.
    // Mandatory override.
    virtual int8_t read_port() = 0;

    /*
    * State accessors.
    */
    inline uint32_t getMTU() {   return _xport_mtu;  };

    /* Connection/Listen states */
    inline bool connected() {   return (_xport_flags & (MANUVR_XPORT_FLAG_CONNECTED | MANUVR_XPORT_FLAG_ALWAYS_CONNECTED));  }
    inline bool listening() {   return (_xport_flags & MANUVR_XPORT_FLAG_LISTENING);   };

    /* Can the transport be relied upon to provide connection status? */
    inline bool alwaysConnected() {         return (_xport_flags & MANUVR_XPORT_FLAG_ALWAYS_CONNECTED);  }
    void alwaysConnected(bool en);

    /* Is this transport set to autoconnect? Also returns true if alwaysConnected(). */
    inline bool autoConnect() {   return (_xport_flags & (MANUVR_XPORT_FLAG_ALWAYS_CONNECTED | MANUVR_XPORT_FLAG_AUTO_CONNECT));  }
    inline void autoConnect(bool en) {   autoConnect(en, XPORT_DEFAULT_AUTOCONNECT_PERIOD);  };
    void autoConnect(bool en, uint32_t _ac_period);

    /* Members that deal with sessions. */
    inline bool streamOriented() {          return (_xport_flags & MANUVR_XPORT_FLAG_STREAM_ORIENTED);  };

    /* Any required setup finished without problems? */
    inline bool initialized() { return (_xport_flags & MANUVR_XPORT_FLAG_INITIALIZED); };
    inline void initialized(bool en) {
      _xport_flags = (en) ? (_xport_flags | MANUVR_XPORT_FLAG_INITIALIZED) : (_xport_flags & ~(MANUVR_XPORT_FLAG_INITIALIZED));
    };

    /* We will override these functions in EventReceiver. */
    // TODO: I'm not sure I've evaluated the full impact of this sort of
    //    choice.  Calltimes? vtable size? alignment? Fragility? Dig.
    virtual void   printDebug(StringBuilder *);
    virtual int8_t notify(ManuvrRunnable*);
    #if defined(MANUVR_CONSOLE_SUPPORT)
      virtual void   procDirectDebugInstruction(StringBuilder*);
    #endif  //MANUVR_CONSOLE_SUPPORT

    // We can have up-to 65535 transport instances concurrently. This well-exceeds
    //   the configured limits of most linux installations, so it should be enough.
    static uint16_t TRANSPORT_ID_POOL;



  protected:
    ManuvrRunnable* _autoconnect_schedule;

    // Can also be used to poll the other side. Implementation is completely at the discretion
    //   any extending class. But generally, this feature is necessary.
    ManuvrRunnable read_abort_event;  // Used to timeout a read operation.

    uint16_t xport_id;
    uint32_t _xport_mtu;      // The largest packet size we handle.
    uint32_t bytes_sent;
    uint32_t bytes_received;

    ManuvrXport();

    virtual int8_t attached() =0;
    const char* pipeName();

    // TODO: Should be private. provide_session() / reset() are the blockers.
    inline void set_xport_state(uint32_t bitmask) {    _xport_flags = (bitmask  | _xport_flags);   }
    inline void unset_xport_state(uint32_t bitmask) {  _xport_flags = (~bitmask & _xport_flags);   }

    void connected(bool);
    void listening(bool);



  private:
    uint32_t _xport_flags = 0;;

    /* Connection/Listen states */
    inline void mark_connected(bool en) {
      _xport_flags = (en) ? (_xport_flags | MANUVR_XPORT_FLAG_CONNECTED) : (_xport_flags & ~(MANUVR_XPORT_FLAG_CONNECTED));
    };
};

#endif   // __MANUVR_XPORT_H__
