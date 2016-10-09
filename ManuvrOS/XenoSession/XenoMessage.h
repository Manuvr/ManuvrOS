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

#include <CommonConstants.h>
#include <EnumeratedTypeCodes.h>
#include <ManuvrMsg/ManuvrMsg.h>

class XenoSession;

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
  #if defined(MANUVR_OVER_THE_WIRE)
  PROTO_MANUVR    = 1,   // Manuvr's protocol.
  #endif
  #if defined(MANUVR_SUPPORT_MQTT)
  PROTO_MQTT      = 2,   // MQTT
  #endif
  #if defined(MANUVR_SUPPORT_COAP)
  PROTO_COAP      = 3,   // CoAP
  #endif
  #if defined(MANUVR_SUPPORT_OSC)
  PROTO_OSC       = 4,   // OSC
  #endif
  #if defined(MANUVR_CONSOLE_SUPPORT)
  PROTO_CONSOLE   = 0xFF, // TODO: A user with a text console and keyboard.
  #endif
  PROTO_RAW       = 0    // Raw has no format and no session except that which the transport imposes (if any).
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
* This class is a special extension of ManuvrMsg that is intended for communication with
*   outside devices. This is the abstraction between our internal Msgs and the messaging
*   system of our counterparty.
*/
class XenoMessage {
  public:
    XenoMessage();                  // Typical use: building an inbound XemoMessage.
    XenoMessage(ManuvrMsg*);   // Create a new XenoMessage with the given event as source data.

    virtual ~XenoMessage() {};

    virtual void wipe();            // Call this to put this object into a fresh state (avoid a free/malloc).
    virtual void printDebug(StringBuilder*);
    virtual void provideEvent(ManuvrMsg*);

    // Mandatory overrides
    virtual int serialize(StringBuilder*) =0;       // Returns the number of bytes resulting.
    virtual int accumulate(unsigned char*, int) =0; // Returns the number of bytes consumed.

    inline uint8_t getState() {      return proc_state; };
    inline int argumentBytes() {     return (bytes_total);                   };
    inline int bytesRemaining() {    return (bytes_total - bytes_received);  };

    /* Any required setup finished without problems? */
    inline bool awaitingSend() { return (proc_state & XENO_MSG_PROC_STATE_AWAITING_SEND); };
    inline void awaitingSend(bool en) {
      proc_state = (en) ? (proc_state | XENO_MSG_PROC_STATE_AWAITING_SEND) : (proc_state & ~(XENO_MSG_PROC_STATE_AWAITING_SEND));
    };


    static const char* getMessageStateString(uint8_t);


  protected:
    uint32_t  bytes_received;  // How many bytes of this command have we received? Meaningless for the sender.
    uint32_t  bytes_total;     // How many bytes does this message occupy on the wire?
    ManuvrMsg* event;     // Associates this XenoMessage to an event.
    ManuvrMsg  _timeout;    // Occasionally, we must let a defunct message die on the wire...
    uint8_t         proc_state;  // Where are we in the flow of this message? See XENO_MSG_PROC_STATES


  private:
};


// TODO: These are the members that may yet wind up in the base-class.
class XenoMessage_old {
  public:
    void claim(XenoSession*);

  private:
    XenoSession*    session;   // A reference to the session that we are associated with.
    uint32_t  time_created;    // Optional: What time did this message come into existance?
    uint32_t  millis_at_begin; // This is the milliseconds reading when we sent.
    uint8_t   retries;         // How many times have we retried this packet?
};

#endif   //__XENOMESSAGE_H__
