/*
File:   ManuvrSession.h
Author: J. Ian Lindsay
Date:   2016.04.09

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


*/


/**
* Manuvr's protocol is formatted this way:
*
* All multibyte values are stored "little-endian".
* Checksum = 0x55 (Preload) + Every other byte after it. Then truncate to uint8.
*
* |<-------------------HEADER------------------------>|
* |--------------------------------------------------|  /---------------\
* | Total Length | Checksum | Unique ID | Message ID |  | Optional data |
* |--------------------------------------------------|  \---------------/
*       3             1           2           2            <---- Bytes
*
* Minimum packet size is the sync packet (4 bytes). Sync packet is
*   always the same (for a given CHECKSUM_PRELOAD):
*        0x04 0x00 0x00 0x55
*                        |----- Checksum preload
*
* Sync packets are the way in which we establish initial offset for a dialog over a stream.
* We need to sync initially on connection and if something goes sideways in the course
*   of a normal dialog.
*
* Receiving an unsolicited sync packet should be an indication that the counterparty either
*   can't understand us, or the transport dropped a message and they timed it out. When we see
*   a sync packet on the wire, we should allow the currently-active transaction to complete and
*   then send back a sync stream of our own. When the counterparty notices this, communication
*   can resume.
*/

#ifndef __MANUVRSESSION_MANUVR_H__
#define __MANUVRSESSION_MANUVR_H__


#define CHECKSUM_PRELOAD_BYTE 0x55    // Calculation of new checksums should start with this byte,
#define XENOSESSION_INITIAL_SYNC_COUNT    24      // How many sync packets to send before giving up.

#define XENOMSG_M_PREALLOC_COUNT       8    // How many XenoMessages should be preallocated?


#include "../XenoSession.h"


/**
* This class is a special extension of ManuvrRunnable that is intended for communication with
*   outside devices. This is the abstraction between our internal Runnables and the messaging
*   system of our counterparty.
*/
class XenoManuvrMessage : public XenoMessage {
  public:
    ManuvrRunnable* event;          // Associates this XenoMessage to an event.

    XenoManuvrMessage();                  // Typical use: building an inbound XemoMessage.
    XenoManuvrMessage(ManuvrRunnable*);   // Create a new XenoMessage with the given event as source data.

    ~XenoManuvrMessage();

    void wipe();                    // Call this to put this object into a fresh state (avoid a free/malloc).

    /* Message flow control. */
    int8_t ack();      // Ack this message.
    int8_t retry();    // Asks the counterparty for a retransmission of this packet. Assumes good unique-id.
    int8_t fail();     // Informs the counterparty that the indicated message failed a high-level validity check.

    int feedBuffer(StringBuilder*);  // This is used to build an event from data that arrives in chunks.
    int serialize(StringBuilder*);   // Returns the number of bytes resulting.

    void provideEvent(ManuvrRunnable*, uint16_t);  // Call to make this XenoMessage outbound.
    inline void provideEvent(ManuvrRunnable* runnable) {    // Override to support laziness.
      provideEvent(runnable, (uint16_t) randomInt());
    };

    bool isReply();      // Returns true if this message is a reply to another message.
    bool expectsACK();   // Returns true if this message demands an ACK.

    void printDebug(StringBuilder*);

    inline uint8_t getState() {  return proc_state; };
    inline uint16_t uniqueId() { return unique_id;  };
    inline bool rxComplete() {
      return ((bytes_received == bytes_total) && (0 != message_code) && (checksum_c == checksum_i));
    };

    inline int bytesRemaining() {    return (bytes_total - bytes_received);  };

    /*
    * Functions used for manipulating this message's state-machine...
    */
    void claim(XenoSession*);

    const char* getMessageStateString();


    static const uint8_t SYNC_PACKET_BYTES[4];    // Plase note the subtle abuse of type....

    static int contains_sync_pattern(uint8_t* buf, int len);
    static int locate_sync_break(uint8_t* buf, int len);

    /* Preallocation machinary. */
    static uint32_t _heap_instantiations;   // Prealloc starvation counter...
    static uint32_t _heap_freeds;           // Prealloc starvation counter...
    static XenoMessage* fetchPreallocation(XenoSession*);
    static void reclaimPreallocation(XenoMessage*);


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
    uint8_t   arg_count;

    uint8_t   checksum_i;      // The checksum of the data that we receive.
    uint8_t   checksum_c;      // The checksum of the data that we calculate.

    ManuvrRunnable _timeout;   // Occasionally, we must let a defunct message die on the wire...

    static XenoManuvrMessage __prealloc_pool[XENOMSG_M_PREALLOC_COUNT];
};



class ManuvrSession : public XenoSession {
  public:
    ManuvrSession(ManuvrXport*);
    ~ManuvrSession();

    int8_t sendSyncPacket();

    // Returns and isolates the sync bits.
    inline uint8_t getSync() {
      return (session_state & 0x00F0);
    };

    // Returns the answer to: "Is this session in sync?"
    inline bool syncd() {
      return (0 == ((session_state & 0x00F0) | XENOSESSION_STATE_SYNC_SYNCD));
    }

    /* Overrides from EventReceiver */
    void procDirectDebugInstruction(StringBuilder*);
    const char* getReceiverName();
    void printDebug(StringBuilder*);
    int8_t notify(ManuvrRunnable*);
    int8_t callback_proc(ManuvrRunnable *);


  protected:
    int8_t bootComplete();
    int8_t bin_stream_rx(unsigned char* buf, int len);            // Used to feed data to the session.

  private:
    ManuvrRunnable sync_event;

    int8_t scan_buffer_for_sync();
    void   mark_session_desync(uint8_t desync_source);
    void   mark_session_sync(bool pending);

    const char* getSessionSyncString();
};


#endif //__MANUVRSESSION_MANUVR_H__
