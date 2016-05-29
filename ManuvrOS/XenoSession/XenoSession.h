/*
File:   XenoSession.h
Author: J. Ian Lindsay
Date:   2014.11.20

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


XenoSession is the class that manages dialog with other systems via some
  transport (IRDa, Bluetooth, USB VCP, etc).
*/


#ifndef __XENOSESSION_COMM_LAYER_H__
#define __XENOSESSION_COMM_LAYER_H__

#include "../Kernel.h"
#include "../EnumeratedTypeCodes.h"
#include "../Transports/ManuvrXport.h"
#include "XenoMessage.h"

#include <map>

#define XENO_SESSION_IGNORE_NON_EXPORTABLES 1  // Comment to expose the entire internal-messaging system to counterparties.
#define XENO_SESSION_MAX_QUEUE_PRINT        3  // This is only relevant for debug.

/*
* These are defines for the low-4 bits of the session_state. They confine the space of our
*   possible dialog, and bias the conversation in a given direction.
*/
#define XENOSESSION_STATE_UNINITIALIZED   0x0000  // Nothing has happened. Freshly-instantiated session.
#define XENOSESSION_STATE_PENDING_CONN    0x0001  // We are not connected.
#define XENOSESSION_STATE_PENDING_SETUP   0x0002  // We are in the setup phase of the session.
#define XENOSESSION_STATE_PENDING_AUTH    0x0004  // Waiting on authentication.
#define XENOSESSION_STATE_ESTABLISHED     0x0008  // Session is in the nominal state.
#define XENOSESSION_STATE_PENDING_HANGUP  0x0010  // Session hangup is imminent.
#define XENOSESSION_STATE_HUNGUP          0x0020  // Session is hungup.
#define XENOSESSION_STATE_DISCONNECTED    0x0040  // Transport informs us that our session is pointless.

/*
* These are bitflags in the same space as the above-def'd constants.
*/
// TODO: Cut the mutual-usage of the same flags variable.
#define XENOSESSION_STATE_AUTH_REQUIRED   0x0100  // Set if this session requires authentication.
#define XENOSESSION_STATE_AUTHD           0x0200  // Set if this session has been authenticated.
#define XENOSESSION_STATE_OVERFLOW_GUARD  0x0400  // Set to protect the session buffer from overflow.



/**
* This class represents an open comm session with a foreign device. That comm session might
*   be happening over USB, BlueTooth, WiFi, IRDa, etc. All we care about is the byte stream.
* A transport class instantiates us, and maintains a pointer to us.
*/
class XenoSession : public EventReceiver {
  public:
    XenoSession(ManuvrXport*);
    ~XenoSession();

    /* Functions that are indirectly called by counterparty requests for subscription. */
    int8_t tapMessageType(uint16_t code);     // Start getting broadcasts about a given message type.
    int8_t untapMessageType(uint16_t code);   // Stop getting broadcasts about a given message type.
    int8_t untapAll();

    virtual int8_t sendEvent(ManuvrRunnable*);

    /* Returns and isolates the lifecycle phase bits. */
    inline uint8_t getPhase() {      return (session_state & 0x00FF);    };

    /* Returns the answer to: "Is this session established?" */
    inline bool isEstablished() {    return (0 != (XENOSESSION_STATE_ESTABLISHED  & getPhase()));   }
    inline bool isConnected() {      return (0 == (XENOSESSION_STATE_PENDING_CONN & getPhase()));   }

    virtual int8_t connection_callback(bool connected);
    virtual int8_t bin_stream_rx(unsigned char* buf, int len) =0;            // Used to feed data to the session.

    /* Overrides from EventReceiver */
    virtual const char* getReceiverName() =0;
    virtual void printDebug(StringBuilder*);
    virtual int8_t notify(ManuvrRunnable*);
    virtual int8_t callback_proc(ManuvrRunnable *);
    #if defined(__MANUVR_CONSOLE_SUPPORT)
      virtual void procDirectDebugInstruction(StringBuilder *);
    #endif


    static const char* sessionPhaseString(uint16_t state_code);


  protected:
    ManuvrXport* owner;           // A reference to the transport that owns this session.
    XenoMessage* working;         // If we are in the middle of receiving a message,

    virtual int8_t bootComplete();

    /**
    * Mark the session with the given status.
    *
    * @param   uint8_t The code representing the desired new state of the session.
    */
    inline void mark_session_state(uint16_t nu_state) {
      session_last_state = session_state;
      session_state      = (session_state & 0xFF00) | (nu_state & 0x00FF);
    };

    int8_t sendPacket(unsigned char *buf, int len);
    inline void requestService() {   raiseEvent(&_session_service);  };
    int purgeInbound();
    int purgeOutbound();




  private: // TODO: Migrate members here as session mitosis completes telophase.
    ManuvrRunnable _session_service;
    LinkedList<XenoMessage*> _outbound_messages;   // Messages that are bound for the counterparty.
    LinkedList<XenoMessage*> _inbound_messages;    // Messages that came from the counterparty.

    std::map<uint16_t, MessageTypeDef*> _relay_list;     // Which message codes will we relay to the counterparty?
    std::map<uint16_t, XenoMessage*>    _pending_exec;   // Messages pending execution (waiting on us).
    std::map<uint16_t, XenoMessage*>    _pending_reply;  // Messages pending reply (waiting on counterparty).

    uint16_t session_state;       // What state is this session in?
    uint16_t session_last_state;  // The prior state of the sesssion.
};


#endif
