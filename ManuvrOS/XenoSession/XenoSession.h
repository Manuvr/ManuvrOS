/*
File:   XenoSession.h
Author: J. Ian Lindsay
Date:   2014.11.20

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


XenoSession is the class that manages dialog with other systems via some
  transport (IRDa, Bluetooth, USB VCP, etc).
     ---J. Ian Lindsay
*/


#ifndef __XENOSESSION_COMM_LAYER_H__
#define __XENOSESSION_COMM_LAYER_H__

#include "../Kernel.h"
#include "../EnumeratedTypeCodes.h"
#include "../Transports/ManuvrXport.h"

#define CHECKSUM_PRELOAD_BYTE 0x55    // Calculation of new checksums should start with this byte,


/*
* These are the stages that a XenoMessage passes through.
* Each stage is traversed in sequence.
*/
const uint8_t XENO_MSG_PROC_STATE_UNINITIALIZED          = 0x00;    // This message is formless.
const uint8_t XENO_MSG_PROC_STATE_CLAIMED                = 0x01;    // A session has cliamed the message.
const uint8_t XENO_MSG_PROC_STATE_SYNC_PACKET            = 0xEE;    // We parsed the stream and found a sync packet.
const uint8_t XENO_MSG_PROC_STATE_AWAITING_REAP          = 0xFF;    // We should be torn down.

// States that pertain to XenoMessages generated locally (outbound)...
const uint8_t XENO_MSG_PROC_STATE_SERIALIZING            = 0x10;  
const uint8_t XENO_MSG_PROC_STATE_AWAITING_SEND          = 0x11;
const uint8_t XENO_MSG_PROC_STATE_SENDING_COMMAND        = 0x12;
const uint8_t XENO_MSG_PROC_STATE_AWAITING_REPLY         = 0x13;
const uint8_t XENO_MSG_PROC_STATE_RECEIVING_REPLY        = 0x14;
const uint8_t XENO_MSG_PROC_STATE_REPLY_RECEIVED         = 0x15;

// States that pertain to XenoMessages generated remotely (inbound)...
const uint8_t XENO_MSG_PROC_STATE_RECEIVING              = 0x20;
const uint8_t XENO_MSG_PROC_STATE_AWAITING_UNSERIALIZE   = 0x21;
const uint8_t XENO_MSG_PROC_STATE_AWAITING_PROC          = 0x22;
const uint8_t XENO_MSG_PROC_STATE_PROCESSING_COMMAND     = 0x23;
const uint8_t XENO_MSG_PROC_STATE_AWAITING_WRITE         = 0x24;
const uint8_t XENO_MSG_PROC_STATE_WRITING_REPLY          = 0x25;


// Comment the define below to enable ALL messages to be exchanged via the XenoSession. The only possible
// reason for this is debug.
#define XENO_SESSION_IGNORE_NON_EXPORTABLES 1

#define XENO_SESSION_MAX_QUEUE_PRINT        3    // This is only relevant for debug.
#define XENOMESSAGE_PREALLOCATE_COUNT       4    // How many XenoMessages should be preallocated?

/*
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
* Sync packets are the way in which we establish initial order for our dialog. We need to sync
*   initially on connection and if something goes sideways in the course of a normal dialog.
*
* Receiving an unsolicited sync packet should be an indication that the counterparty either
*   can't understand us, or the transport dropped a message and they timed it out. When we see
*   a sync packet on the wire, we should allow the currently-active transaction to complete and
*   then send back a sync stream of our own. When the counterparty notices this, communication
*   can resume.
*/



/**
* This class is a special extension of ManuvrRunnable that is intended for communication with
*   outside devices. The idea is that the outside device ought to be able to build an event that
*   is, in essence, a copy of ours.
*/
class XenoMessage {
  public:

    ManuvrRunnable* event;         // Associates this XenoMessage to an event.
    
    XenoMessage();                  // Typical use: building an inbound XemoMessage. 
    XenoMessage(ManuvrRunnable*);   // Create a new XenoMessage with the given event as source data.

    ~XenoMessage();
    
    int serialize();      // Returns the number of bytes resulting.
    int8_t dispatch();    // If incoming, sends event. If outgoing, sends bitstream to transport.
    void wipe();          // Call this to put this object into a fresh state (avoid a free/malloc).
    int8_t inflateArgs(); // 
    
    /* Message flow control. */
    int8_t ack();      // Ack this message.
    int8_t retry();    // Asks the counterparty for a retransmission of this packet. Assumes good unique-id.
    int8_t fail();     // Informs the counterparty that the indicated message failed a high-level validity check.
    
    int feedBuffer(StringBuilder *buf);  // This is used to build an event from data that arrives in chunks.
    void provideEvent(ManuvrRunnable*);  // Call to make this XenoMessage outbound.
    void provide_event(ManuvrRunnable*, uint16_t);  // Call to make this XenoMessage outbound.
    
    bool isReply();      // Returns true if this message is a reply to another message.
    bool expectsACK();   // Returns true if this message demands an ACK.

    void printDebug(StringBuilder*);
    
    inline int8_t getBuffer(StringBuilder** _buf) {
      *(_buf) = &buffer;
      return 0;
    };

    inline uint8_t getState() { return proc_state; };
    inline uint8_t uniqueId() { return unique_id;  };
    inline bool rxComplete() { 
      return ((bytes_received == bytes_total) && (0 != message_code) && (checksum_c == checksum_i));
    };

    /*
    * Functions used for manipulating this message's state-machine...
    */
    void claim(XenoSession*);


    static const char* getMessageStateString(uint8_t code);


  private:
    XenoSession*    session;   // A reference to the session that we are associated with.
    StringBuilder   buffer;    // Holds the intermediary form of the message that traverses the transport.
    StringBuilder   argbuf;    // Holds the bytes-in-excess-of packet-minimum until they can be parsed.
    uint32_t  time_created;    // Optional: What time did this message come into existance?
    uint32_t  millis_at_begin; // This is the milliseconds reading when we sent.
    uint8_t   retries;         // How many times have we retried this packet?

    uint8_t         proc_state;    // Where are we in the flow of this message? See XENO_MSG_PROC_STATES
    uint32_t  bytes_received;      // How many bytes of this command have we received? Meaningless for the sender.

    uint8_t   arg_count;

    uint32_t  bytes_total;     // How many bytes does this message occupy on the wire?

    uint8_t   checksum_i;      // The checksum of the data that we receive.
    uint8_t   checksum_c;      // The checksum of the data that we calculate.

    uint16_t  unique_id;       // An identifier for this message.
    uint16_t  message_code;    // The integer code for this message class.

    void __class_initializer();
};



#define XENOSESSION_INITIAL_SYNC_COUNT    24      // How many sync packets to send before giving up.


/*
* These are defines for the low-4 bits of the session_state. They confine the space of our
*   possible dialog, and bias the conversation in a given direction.
*/
#define XENOSESSION_STATE_UNINITIALIZED   0x0000  // Nothing has happened. Freshly-instantiated session.
#define XENOSESSION_STATE_PENDING_SETUP   0x0004  // We are in the setup phase of the session.
#define XENOSESSION_STATE_PENDING_AUTH    0x0008  // Waiting on authentication.
#define XENOSESSION_STATE_ESTABLISHED     0x000A  // Session is in the nominal state.
#define XENOSESSION_STATE_PENDING_HANGUP  0x000C  // Session hangup is imminent.
#define XENOSESSION_STATE_HUNGUP          0x000E  // Session is hungup.
#define XENOSESSION_STATE_DISCONNECTED    0x000F  // Transport informs us that our session is pointless.

/*
* These are bitflags in the same space as the above-def'd constants. They all pertain to
* the sync state of the session.
*/
#define XENOSESSION_STATE_SYNC_INITIATED  0x0080  // The counterparty noticed a problem.
#define XENOSESSION_STATE_SYNC_INITIATOR  0x0040  // We noticed a problem.
#define XENOSESSION_STATE_SYNC_PEND_EXIT  0x0020  // We think we have just recovered from a sync.
#define XENOSESSION_STATE_SYNC_CASTING    0x0010  // If set, we are broadcasting sync packets.
#define XENOSESSION_STATE_SYNC_SYNCD      0x0000  // Pedantry... Just helps document.

/*
* These are bitflags in the same space as the above-def'd constants. They all pertain to
* the sync state of the session.
*/
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
    
    /* Functions indended to be called by the transport. */
    int8_t   markMessageComplete(uint16_t id);

    /* Functions that are indirectly called by counterparty requests for subscription. */
    int8_t tapMessageType(uint16_t code);     // Start getting broadcasts about a given message type.
    int8_t untapMessageType(uint16_t code);   // Stop getting broadcasts about a given message type.
    int8_t untapAll();

    int8_t sendEvent(ManuvrRunnable*);
    
    int8_t sendSyncPacket();
    int8_t sendKeepAlive();
    
    /* Overrides from EventReceiver */
    void procDirectDebugInstruction(StringBuilder*);
    const char* getReceiverName();
    void printDebug(StringBuilder*);
    int8_t notify(ManuvrRunnable*);
    int8_t callback_proc(ManuvrRunnable *);

    // Returns and isolates the state bits.
    inline uint8_t getState() {
      return (session_state & 0xFF0F);
    };

    // Returns and isolates the sync bits.
    inline uint8_t getSync() {
      return (session_state & 0x00F0);
    };

    // Returns the answer to: "Is this session in sync?"
    inline bool syncd() {
      return (0 == ((session_state & 0x00F0) | XENOSESSION_STATE_SYNC_SYNCD));
    }

    // Returns the answer to: "Is this session established?"
    inline bool isEstablished() {
      return (XENOSESSION_STATE_ESTABLISHED == (session_state & 0xFF0F));
    }
    
    static int contains_sync_pattern(uint8_t* buf, int len);
    static int locate_sync_break(uint8_t* buf, int len);



  protected:
    int8_t bootComplete();


  private:
    ManuvrXport* owner;           // A reference to the transport that owns this session.
    XenoMessage* working;         // If we are in the middle of receiving a message.

    uint16_t session_state;       // What state is this session in?
    uint16_t session_last_state;  // The prior state of the sesssion.
    
    /*
    * A buffer for holding inbound stream until enough has arrived to parse. This eliminates
    *   the need for the transport to care about how much data we consumed versus left in its buffer. 
    */
    StringBuilder session_buffer;
    ManuvrRunnable sync_event;

    LinkedList<MessageTypeDef*> msg_relay_list;   // Which message codes will we relay to the counterparty?
    LinkedList<XenoMessage*> outbound_messages;   // Messages that are bound for the counterparty.
    LinkedList<XenoMessage*> inbound_messages;    // Messages that came from the counterparty.
    
    /* These variables track failure cases to inform sync-initiation. */
    uint8_t MAX_PARSE_FAILURES;         // How many failures-to-parse should we tolerate before SYNCing?
    uint8_t MAX_ACK_FAILURES;           // How many failures-to-ACK should we tolerate before SYNCing?
    uint8_t sequential_parse_failures;  // How many parse attempts have failed in-a-row?
    uint8_t sequential_ack_failures;    // How many of our outbound packets have failed to ACK?

    int8_t bin_stream_rx(unsigned char* buf, int len);            // Used to feed data to the session.
    int8_t scan_buffer_for_sync();
    void   mark_session_desync(uint8_t desync_source);
    void   mark_session_sync(bool pending);
    
    /**
    * Mark the session with the given status.
    *
    * @param   uint8_t The code representing the desired new state of the session.
    */
    inline void mark_session_state(uint8_t nu_state) {
      session_last_state = session_state;
      session_state      = (session_state & 0x00F0) | nu_state;
    }
    
    int purgeInbound();
    int purgeOutbound();
   
    const char* getSessionStateString();
    const char* getSessionSyncString();

    
    // Plase note the subtle abuse of type....
    static const uint8_t SYNC_PACKET_BYTES[4];
    
    /* Preallocation machinary. */ 
    // Prealloc starvation counters...
    static uint32_t _heap_instantiations;
    static uint32_t _heap_freeds;
    
    static XenoMessage __prealloc_pool[XENOMESSAGE_PREALLOCATE_COUNT];

    static XenoMessage* fetchPreallocation(XenoSession*);
    static void reclaimPreallocation(XenoMessage*);
};


#endif
