/*
File:   CoAP.h
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

The message class was derived from cantcoap. Pieces of the original header
  files are here until they are fully-digested and the useless WIN32 case-offs
  are removed.
*/

#ifndef SYSDEP_H
#define SYSDEP_H

#include <stdint.h>

#include <arpa/inet.h>  /* __BYTE_ORDER */

#if !defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    #define __LITTLE_ENDIAN__
  #elif __BYTE_ORDER == __BIG_ENDIAN
    #define __BIG_ENDIAN__
  #endif
#endif

#ifdef __LITTLE_ENDIAN__

#define endian_be16(x) ntohs(x)
#define endian_be32(x) ntohl(x)

#if defined(_byteswap_uint64) || (defined(_MSC_VER) && _MSC_VER >= 1400)
#  define endian_be64(x) (_byteswap_uint64(x))
#elif defined(bswap_64)
#  define endian_be64(x) bswap_64(x)
#elif defined(__DARWIN_OSSwapInt64)
#  define endian_be64(x) __DARWIN_OSSwapInt64(x)
#else
#define endian_be64(x) \
    ( ((((uint64_t)x) << 56)                         ) | \
      ((((uint64_t)x) << 40) & 0x00ff000000000000ULL ) | \
      ((((uint64_t)x) << 24) & 0x0000ff0000000000ULL ) | \
      ((((uint64_t)x) <<  8) & 0x000000ff00000000ULL ) | \
      ((((uint64_t)x) >>  8) & 0x00000000ff000000ULL ) | \
      ((((uint64_t)x) >> 24) & 0x0000000000ff0000ULL ) | \
      ((((uint64_t)x) >> 40) & 0x000000000000ff00ULL ) | \
      ((((uint64_t)x) >> 56)                         ) )
#endif

#define endian_load16(cast, from) ((cast)( \
        (((uint16_t)((uint8_t*)(from))[0]) << 8) | \
        (((uint16_t)((uint8_t*)(from))[1])     ) ))

#define endian_load32(cast, from) ((cast)( \
        (((uint32_t)((uint8_t*)(from))[0]) << 24) | \
        (((uint32_t)((uint8_t*)(from))[1]) << 16) | \
        (((uint32_t)((uint8_t*)(from))[2]) <<  8) | \
        (((uint32_t)((uint8_t*)(from))[3])      ) ))

#define endian_load64(cast, from) ((cast)( \
        (((uint64_t)((uint8_t*)(from))[0]) << 56) | \
        (((uint64_t)((uint8_t*)(from))[1]) << 48) | \
        (((uint64_t)((uint8_t*)(from))[2]) << 40) | \
        (((uint64_t)((uint8_t*)(from))[3]) << 32) | \
        (((uint64_t)((uint8_t*)(from))[4]) << 24) | \
        (((uint64_t)((uint8_t*)(from))[5]) << 16) | \
        (((uint64_t)((uint8_t*)(from))[6]) << 8)  | \
        (((uint64_t)((uint8_t*)(from))[7])     )  ))

#else // __LITTLE_ENDIAN__

#define endian_be16(x) (x)
#define endian_be32(x) (x)
#define endian_be64(x) (x)

#define endian_load16(cast, from) ((cast)( \
        (((uint16_t)((uint8_t*)(from))[0]) << 8) | \
        (((uint16_t)((uint8_t*)(from))[1])     ) ))

#define endian_load32(cast, from) ((cast)( \
        (((uint32_t)((uint8_t*)(from))[0]) << 24) | \
        (((uint32_t)((uint8_t*)(from))[1]) << 16) | \
        (((uint32_t)((uint8_t*)(from))[2]) <<  8) | \
        (((uint32_t)((uint8_t*)(from))[3])      ) ))

#define endian_load64(cast, from) ((cast)( \
        (((uint64_t)((uint8_t*)(from))[0]) << 56) | \
        (((uint64_t)((uint8_t*)(from))[1]) << 48) | \
        (((uint64_t)((uint8_t*)(from))[2]) << 40) | \
        (((uint64_t)((uint8_t*)(from))[3]) << 32) | \
        (((uint64_t)((uint8_t*)(from))[4]) << 24) | \
        (((uint64_t)((uint8_t*)(from))[5]) << 16) | \
        (((uint64_t)((uint8_t*)(from))[6]) << 8)  | \
        (((uint64_t)((uint8_t*)(from))[7])     )  ))
#endif

#define endian_store16(to, num) \
    do { uint16_t val = endian_be16(num); memcpy(to, &val, 2); } while(0)
#define endian_store32(to, num) \
    do { uint32_t val = endian_be32(num); memcpy(to, &val, 4); } while(0)
#define endian_store64(to, num) \
    do { uint64_t val = endian_be64(num); memcpy(to, &val, 8); } while(0)

#endif // SYSDEP_H



#ifndef __XENOSESSION_CoAP_H__
#define __XENOSESSION_CoAP_H__

#include "../XenoSession.h"
#include "cantcoap/cantcoap.h"

#define MAX_PACKET_ID 65535

/*
* These state flags are hosted by the EventReceiver. This may change in the future.
* Might be too much convention surrounding their assignment across inherritence.
*/
#define CoAP_SESS_FLAG_PING_WAIT  0x01    // Are we waiting on a ping reply?



class CoAPMessage : public XenoMessage {
  public:
    char retained;
    char dup;
    uint16_t unique_id;
    char* topic;
    void *payload;

    CoAPMessage();
    ~CoAPMessage();

    virtual void printDebug(StringBuilder*);

    // Called to accumulate data into the class.
    // Returns -1 on failure, or the number of bytes consumed on success.
    int serialize(StringBuilder*);       // Returns the number of bytes resulting.
    int accumulate(unsigned char*, int);

    int decompose_publish();


  private:
};


struct CoAPOpts {
	char* clientid;
	int nodelimiter;
	char* delimiter;
	char* username;
	char* password;
	int showtopics;
};


class CoAPSession : public XenoSession {
  public:
    CoAPSession(BufferPipe*);
    ~CoAPSession();

    int8_t sendEvent(ManuvrRunnable*);

    /* Management of subscriptions... */
    int8_t subscribe(const char*, ManuvrRunnable*);  // Start getting broadcasts about a given message type.
    int8_t unsubscribe(const char*);                 // Stop getting broadcasts about a given message type.
    int8_t resubscribeAll();
    int8_t unsubscribeAll();

    int8_t connection_callback(bool connected);

    /* Override from BufferPipe. */
    virtual int8_t toCounterparty(uint8_t* buf, unsigned int len, int8_t mm);
    virtual int8_t fromCounterparty(uint8_t* buf, unsigned int len, int8_t mm);

    /* Overrides from EventReceiver */
    void procDirectDebugInstruction(StringBuilder*);
    const char* getReceiverName();
    void printDebug(StringBuilder*);
    int8_t notify(ManuvrRunnable*);
    int8_t callback_proc(ManuvrRunnable *);


  protected:
    int8_t bootComplete();


  private:
    CoAPMessage* working;

    PriorityQueue<CoAPMessage*> _pending_coap_messages;      // Valid CoAP messages that have arrived.
    ManuvrRunnable _ping_timer;    // Periodic KA ping.

    unsigned int _next_packetid;

    int8_t bin_stream_rx(unsigned char* buf, int len);            // Used to feed data to the session.

    inline int getNextPacketId() {
      return _next_packetid = (_next_packetid == MAX_PACKET_ID) ? 1 : _next_packetid + 1;
    };
};

#endif //__XENOSESSION_CoAP_H__
