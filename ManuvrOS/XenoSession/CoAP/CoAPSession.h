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


*/

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
    CoAPSession(ManuvrXport*);
    ~CoAPSession();

    int8_t sendEvent(ManuvrRunnable*);

    /* Management of subscriptions... */
    int8_t subscribe(const char*, ManuvrRunnable*);  // Start getting broadcasts about a given message type.
    int8_t unsubscribe(const char*);                 // Stop getting broadcasts about a given message type.
    int8_t resubscribeAll();
    int8_t unsubscribeAll();

    int8_t connection_callback(bool connected);
    int8_t bin_stream_rx(unsigned char* buf, int len);            // Used to feed data to the session.

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

    inline int getNextPacketId() {
      return _next_packetid = (_next_packetid == MAX_PACKET_ID) ? 1 : _next_packetid + 1;
    };
};

#endif //__XENOSESSION_CoAP_H__
