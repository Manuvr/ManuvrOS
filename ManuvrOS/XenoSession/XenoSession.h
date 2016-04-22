/*
File:   XenoMessage.h
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
     ---J. Ian Lindsay
*/


#ifndef __XENOSESSION_COMM_LAYER_H__
#define __XENOSESSION_COMM_LAYER_H__

#include "../Kernel.h"
#include "../EnumeratedTypeCodes.h"
#include "../Transports/ManuvrXport.h"

#include <map>

/*
* This is the XenoMessage lifecycle. Each stage is traversed in sequence.
*/
#define XENO_MSG_PROC_STATE_UNINITIALIZED        0x00    // This message is formless.

// States that pertain to XenoMessages generated remotely (inbound)...
#define XENO_MSG_PROC_STATE_RECEIVING            0x01
#define XENO_MSG_PROC_STATE_AWAITING_PROC        0x02
#define XENO_MSG_PROC_STATE_PROCESSING_RUNNABLE  0x04

// States that pertain to XenoMessages generated locally (outbound)...
#define XENO_MSG_PROC_STATE_AWAITING_SEND        0x08
#define XENO_MSG_PROC_STATE_AWAITING_REPLY       0x10

// Terminal states.
#define XENO_MSG_PROC_STATE_SYNC_PACKET          0x20    // We parsed the stream and found a sync packet.
#define XENO_MSG_PROC_STATE_ERROR                0x40    // The error bit.
#define XENO_MSG_PROC_STATE_AWAITING_REAP        0x80    // We should be torn down.


#define XENO_SESSION_IGNORE_NON_EXPORTABLES 1  // Comment to expose the entire internal-messaging system to counterparties.
#define XENO_SESSION_MAX_QUEUE_PRINT        3  // This is only relevant for debug.


/**
* These are the enumerations of the protocols we intend to support.
*/
enum Protos {
  RAW       = 0,   // Raw has no format and no session except that which the transport imposes (if any).
  MANUVR    = 1,   // Manuvr's protocol.
  MQTT      = 2,   // MQTT
  COAP      = 3,   // CoAP
  OSC       = 4,   // OSC
  CONSOLE   = 0xFF // A user with a text console and keyboard.
};


/**
* This class is a special extension of ManuvrRunnable that is intended for communication with
*   outside devices. This is the abstraction between our internal Runnables and the messaging
*   system of our counterparty.
*/
class XenoMessage {
  public:
    ManuvrRunnable* event;          // Associates this XenoMessage to an event.

    XenoMessage();                  // Typical use: building an inbound XemoMessage.
    XenoMessage(ManuvrRunnable*);   // Create a new XenoMessage with the given event as source data.

    void wipe();                    // Call this to put this object into a fresh state (avoid a free/malloc).

    /* Message flow control. */
    int8_t ack();      // Ack this message.
    int8_t retry();    // Asks the counterparty for a retransmission of this packet. Assumes good unique-id.
    int8_t fail();     // Informs the counterparty that the indicated message failed a high-level validity check.

    virtual int feedBuffer(StringBuilder*) =0;  // This is used to build an event from data that arrives in chunks.
    virtual int serialize(StringBuilder*)  =0;  // Returns the number of bytes resulting.

    void provideEvent(ManuvrRunnable*, uint16_t);  // Call to make this XenoMessage outbound.
    inline void provideEvent(ManuvrRunnable* runnable) {    // Override to support laziness.
      provideEvent(runnable, (uint16_t) randomInt());
    };

    void printDebug(StringBuilder*);

    inline uint8_t getState() {  return proc_state; };
    inline uint16_t uniqueId() { return unique_id;  };

    inline int bytesRemaining() {    return (bytes_total - bytes_received);  };

    /*
    * Functions used for manipulating this message's state-machine...
    */
    void claim(XenoSession*);

    static const char* getMessageStateString(uint8_t);
    static int contains_sync_pattern(uint8_t* buf, int len);
    static int locate_sync_break(uint8_t* buf, int len);


  private:
    XenoSession*    session;   // A reference to the session that we are associated with.
    uint32_t  time_created;    // Optional: What time did this message come into existance?
    uint32_t  millis_at_begin; // This is the milliseconds reading when we sent.

    uint32_t  bytes_received;  // How many bytes of this command have we received? Meaningless for the sender.
    uint32_t  bytes_total;     // How many bytes does this message occupy on the wire?

    uint16_t  unique_id;       // An identifier for this message.
    uint16_t  message_code;    // The integer code for this message class.
    uint8_t   retries;         // How many times have we retried this packet?
    uint8_t   proc_state;      // Where are we in the flow of this message? See XENO_MSG_PROC_STATES

    ManuvrRunnable _timeout;   // Occasionally, we must let a defunct message die on the wire...
};





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

    int8_t sendEvent(ManuvrRunnable*);

    /* Returns and isolates the lifecycle phase bits. */
    inline uint8_t getPhase() {      return (session_state & 0xFFF0);    };

    /* Returns the answer to: "Is this session established?" */
    inline bool isEstablished() {    return (0 != (XENOSESSION_STATE_ESTABLISHED  & getPhase()));   }
    inline bool isConnected() {      return (0 == (XENOSESSION_STATE_PENDING_CONN & getPhase()));   }

    /* Overrides from EventReceiver */
    virtual void procDirectDebugInstruction(StringBuilder*);
    virtual const char* getReceiverName() =0;
    virtual void printDebug(StringBuilder*);
    virtual int8_t notify(ManuvrRunnable*);
    virtual int8_t callback_proc(ManuvrRunnable *);


    static const char* sessionPhaseString(uint16_t state_code);


  protected:
    ManuvrXport* owner;           // A reference to the transport that owns this session.
    XenoMessage* working;         // If we are in the middle of receiving a message.

    virtual int8_t bootComplete();
    virtual int8_t bin_stream_rx(unsigned char* buf, int len) =0;            // Used to feed data to the session.

    /**
    * Mark the session with the given status.
    *
    * @param   uint8_t The code representing the desired new state of the session.
    */
    inline void mark_session_state(uint8_t nu_state) {
      session_last_state = session_state;
      session_state      = (session_state & 0x000F) | nu_state;
    }

    int8_t sendPacket(unsigned char *buf, int len);
    int purgeInbound();
    int purgeOutbound();


  private: // TODO: Migrate members here as session mitosis completes telophase.

    LinkedList<XenoMessage*> _outbound_messages;   // Messages that are bound for the counterparty.
    LinkedList<XenoMessage*> _inbound_messages;    // Messages that came from the counterparty.

    std::map<uint16_t, MessageTypeDef*> _relay_list;     // Which message codes will we relay to the counterparty?
    std::map<uint16_t, XenoMessage*>    _pending_exec;   // Messages pending execution (waiting on us).
    std::map<uint16_t, XenoMessage*>    _pending_reply;  // Messages pending reply (waiting on counterparty).

    uint16_t session_state;       // What state is this session in?
    uint16_t session_last_state;  // The prior state of the sesssion.
};


#endif
