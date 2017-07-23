/*
File:   OSCSession.h
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

This class originated from IBM's C-client demo code, but was re-written into
  this C++ wrapper.
*/

#ifndef __XENOSESSION_OSC_H__
#define __XENOSESSION_OSC_H__

#include "../XenoSession.h"

#define MAX_PACKET_ID 65535

/*
* These state flags are hosted by the EventReceiver. This may change in the future.
* Might be too much convention surrounding their assignment across inherritence.
*/
#define OSC_SESS_FLAG_PING_WAIT  0x01    // Are we waiting on a ping reply?



class OSCMessage : public XenoMessage {
  public:
    QoS  qos;
    char retained;
    char dup;
    uint16_t unique_id;
    char* topic;
    void *payload;

    OSCMessage();
    ~OSCMessage();

    virtual void printDebug(StringBuilder*);

    // Called to accumulate data into the class.
    // Returns -1 on failure, or the number of bytes consumed on success.
    int serialize(StringBuilder*);       // Returns the number of bytes resulting.
    int accumulate(unsigned char*, int);

    int decompose_publish();

    inline uint16_t packetType() {  return _header.bits.type; };
    inline bool parseComplete() {   return (_parse_stage > 2); };


  private:
    OSCHeader _header;
    int _rem_len;
    uint8_t _parse_stage;
    int _multiplier;
};


struct OSCOpts {
	char* clientid;
	int nodelimiter;
	char* delimiter;
	enum QoS qos;
	char* username;
	char* password;
	int showtopics;
};


class OSCSession : public XenoSession {
  public:
    OSCSession(ManuvrXport*);
    virtual ~OSCSession();

    int8_t sendEvent(ManuvrMsg*);

    /* Management of subscriptions... */
    int8_t subscribe(const char*, ManuvrMsg*);  // Start getting broadcasts about a given message type.
    int8_t unsubscribe(const char*);                 // Stop getting broadcasts about a given message type.
    int8_t resubscribeAll();
    int8_t unsubscribeAll();

    /* Override from BufferPipe. */
    virtual int8_t fromCounterparty(StringBuilder* buf, int8_t mm);

    int8_t connection_callback(bool connected);

    /* Overrides from EventReceiver */
    void procDirectDebugInstruction(StringBuilder*);
    void printDebug(StringBuilder*);
    int8_t notify(ManuvrMsg*);
    int8_t callback_proc(ManuvrMsg*);


  protected:
    int8_t attached();


  private:
    OSCMessage* working;

    std::map<const char*, ManuvrMsg*> _subscriptions;   // Topics we are subscribed to, and the events they trigger.
    PriorityQueue<OSCMessage*> _pending_mqtt_messages;      // Valid OSC messages that have arrived.
    ManuvrMsg _ping_timer;    // Periodic KA ping.

    unsigned int _next_packetid;
    unsigned int command_timeout_ms;

    int8_t bin_stream_rx(unsigned char* buf, int len);            // Used to feed data to the session.

    inline bool _ping_outstanding() {        return (_er_flag(OSC_SESS_FLAG_PING_WAIT));         };
    inline void _ping_outstanding(bool nu) { return (_er_set_flag(OSC_SESS_FLAG_PING_WAIT, nu)); };

    inline bool sendPacket(uint8_t* buf, int len) {
      return (MEM_MGMT_RESPONSIBLE_BEARER == BufferPipe::toCounterparty(buf, len, MEM_MGMT_RESPONSIBLE_BEARER));
    };
    inline int getNextPacketId() {
      return _next_packetid = (_next_packetid == MAX_PACKET_ID) ? 1 : _next_packetid + 1;
    };

    bool sendSub(const char*, enum QoS);
    bool sendUnsub(const char*);
    bool sendKeepAlive();
    bool sendConnectPacket();
    bool sendDisconnectPacket();
    bool sendPublish(ManuvrMsg*);

    int proc_publish(OSCMessage*);
    int process_inbound();

};

#endif //__XENOSESSION_OSC_H__
