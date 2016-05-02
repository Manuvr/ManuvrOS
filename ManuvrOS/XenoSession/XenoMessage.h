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


XenoMessage is the class that unifies our counterparty's message format
  into a single internal type.
*/


#ifndef __XENOMESSAGE_H__
#define __XENOMESSAGE_H__

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


/**
* These are the enumerations of the protocols we intend to support.
*/
enum Protos {
  PROTO_RAW       = 0,   // Raw has no format and no session except that which the transport imposes (if any).
  PROTO_MANUVR    = 1,   // Manuvr's protocol.
  PROTO_MQTT      = 2,   // MQTT
  PROTO_COAP      = 3,   // TODO: CoAP
  PROTO_OSC       = 4,   // TODO: OSC
  PROTO_CONSOLE   = 0xFF // TODO: A user with a text console and keyboard.
};

/**
* These are the enumerations of the data encodings we intend to support. Sometimes
*   this choice is independent of the protocol.
*/
enum TypeFormats {
  TYPES_NONE      = 0,   // No type format.
  TYPES_MANUVR    = 1,   // Manuvr's native types.
  TYPES_CBOR      = 2,   // TODO: CBOR
  TYPES_OSC       = 3,   // TODO: OSC's native types.
  TYPES_CONSOLE   = 0xFF // Debug consoles interpret all I/O as C-style strings.
};


/**
* This class is a special extension of ManuvrRunnable that is intended for communication with
*   outside devices. This is the abstraction between our internal Runnables and the messaging
*   system of our counterparty.
*/
class XenoMessage {
  public:
    XenoMessage();                  // Typical use: building an inbound XemoMessage.
    XenoMessage(ManuvrRunnable*);   // Create a new XenoMessage with the given event as source data.

    virtual ~XenoMessage() {};


  private:
    ManuvrRunnable _timeout;   // Occasionally, we must let a defunct message die on the wire...
};






class XenoMessage_old {
  public:
    ManuvrRunnable* event;          // Associates this XenoMessage to an event.

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

#endif   //__XENOMESSAGE_H__
