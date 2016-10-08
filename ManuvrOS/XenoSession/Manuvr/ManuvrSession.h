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

#include "../XenoSession.h"


#define CHECKSUM_PRELOAD_BYTE          0x55  // Calculation of new checksums should start with this byte,
#define XENOSESSION_INITIAL_SYNC_COUNT   24  // How many sync packets to send before giving up.

#define XENOMSG_M_PREALLOC_COUNT          8  // How many XenoMessages should be preallocated?

#define MANUVR_MAX_PARSE_FAILURES         3  // How many failures-to-parse should we tolerate before SYNCing?
#define MANUVR_MAX_ACK_FAILURES           3  // How many failures-to-ACK should we tolerate before SYNCing?


/*
* These are bitflags in the same space as the above-def'd constants. They all pertain to
* the sync state of the session.
*/
#define XENOSESSION_STATE_SYNC_INITIATED  0x0080  // The counterparty noticed a problem.
#define XENOSESSION_STATE_SYNC_INITIATOR  0x0040  // We noticed a problem.
#define XENOSESSION_STATE_SYNC_PEND_EXIT  0x0020  // We think we have just recovered from a sync.
#define XENOSESSION_STATE_SYNC_CASTING    0x0010  // If set, we are broadcasting sync packets.
#define XENOSESSION_STATE_SYNC_SYNCD      0x0000  // Pedantry... Just helps document.


/**
* This class is a special extension of ManuvrRunnable that is intended for communication with
*   outside devices. This is the abstraction between our internal Runnables and the messaging
*   system of our counterparty.
*/
class XenoManuvrMessage : public XenoMessage {
  public:
    XenoManuvrMessage();                  // Typical use: building an inbound XemoMessage.
    XenoManuvrMessage(ManuvrRunnable*);   // Create a new XenoMessage with the given event as source data.

    ~XenoManuvrMessage() {};

    void wipe();                    // Call this to put this object into a fresh state (avoid a free/malloc).

    /* Message flow control. */
    int8_t ack();      // Ack this message.
    int8_t retry();    // Asks the counterparty for a retransmission of this packet. Assumes good unique-id.
    int8_t fail();     // Informs the counterparty that the indicated message failed a high-level validity check.

    int feedBuffer(StringBuilder*);  // This is used to build an event from data that arrives in chunks.

    int serialize(StringBuilder*);       // Returns the number of bytes resulting.
    int accumulate(unsigned char*, int); // Returns the number of bytes consumed.

    void provideEvent(ManuvrRunnable*, uint16_t);  // Call to make this XenoMessage outbound.
    inline void provideEvent(ManuvrRunnable* runnable) {    // Override to support laziness.
      provideEvent(runnable, (uint16_t) randomInt());
    };

    bool isReply();      // Returns true if this message is a reply to another message.
    bool expectsACK();   // Returns true if this message demands an ACK.

    void printDebug(StringBuilder*);

    inline uint16_t uniqueId() { return unique_id;  };
    inline bool rxComplete() {
      return ((bytes_received == bytes_total) && (0 != message_code) && (checksum_c == checksum_i));
    };

    /*
    * Functions used for manipulating this message's state-machine...
    */
    void claim(XenoSession*);

    const char* getMessageStateString();

    /* Preallocation machinary. */
    static uint32_t _heap_instantiations;   // Prealloc starvation counter...
    static uint32_t _heap_freeds;           // Prealloc starvation counter...
    static XenoMessage* fetchPreallocation(XenoSession*);
    static void reclaimPreallocation(XenoMessage*);
    static const uint8_t SYNC_PACKET_BYTES[4];    // Plase note the subtle abuse of type....


  private:
    XenoSession*    session;   // A reference to the session that we are associated with.
    uint32_t  time_created;    // Optional: What time did this message come into existance?
    uint32_t  millis_at_begin; // This is the milliseconds reading when we sent.

    uint16_t  unique_id;       // An identifier for this message.
    uint16_t  message_code;    // The integer code for this message class.

    uint8_t   retries;         // How many times have we retried this packet?

    uint8_t   arg_count;

    uint8_t   checksum_i;      // The checksum of the data that we receive.
    uint8_t   checksum_c;      // The checksum of the data that we calculate.

    static XenoManuvrMessage __prealloc_pool[XENOMSG_M_PREALLOC_COUNT];

    static int contains_sync_pattern(uint8_t* buf, int len);
    static int locate_sync_break(uint8_t* buf, int len);
};



class ManuvrSession : public XenoSession {
  public:
    ManuvrSession(ManuvrXport*);
    virtual ~ManuvrSession();

    /* Override from BufferPipe. */
    virtual int8_t fromCounterparty(StringBuilder* buf, int8_t mm);

    /* Overrides from EventReceiver */
    void printDebug(StringBuilder*);
    int8_t notify(ManuvrRunnable*);
    int8_t callback_proc(ManuvrRunnable *);
    #if defined(MANUVR_CONSOLE_SUPPORT)
      void procDirectDebugInstruction(StringBuilder*);
    #endif  //MANUVR_CONSOLE_SUPPORT


  protected:
    XenoManuvrMessage* working;         // If we are in the middle of receiving a message,

    int8_t attached();


  private:
    ManuvrRunnable sync_event;

    /*
    * A buffer for holding inbound stream until enough has arrived to parse. This eliminates
    *   the need for the transport to care about how much data we consumed versus left in its buffer.
    */
    StringBuilder session_buffer;

    /* These variables track failure cases to inform sync-initiation. */
    uint8_t _seq_parse_failures;  // How many parse attempts have failed in-a-row?
    uint8_t _seq_ack_failures;    // How many of our outbound packets have failed to ACK?
    uint8_t _stacked_sync_state;  // Is our stream sync'd?
    uint8_t _sync_state;          // Is our stream sync'd?

    int8_t bin_stream_rx(unsigned char* buf, int len);            // Used to feed data to the session.

    /* Returns the answer to: "Is this session in sync?"   */
    inline bool syncd() {     return (_sync_state == XENOSESSION_STATE_SYNC_SYNCD);   }

    int8_t sendKeepAlive();
    int8_t sendSyncPacket();
    int8_t scan_buffer_for_sync();
    void   mark_session_desync(uint8_t desync_source);
    void   mark_session_sync(bool pending);

    const char* getSessionSyncString();
};


#endif //__MANUVRSESSION_MANUVR_H__
