/*
File:   MQTTSession.h
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

#ifndef __XENOSESSION_MQTT_H__
#define __XENOSESSION_MQTT_H__

#include "../XenoSession.h"

#include <paho.mqtt.embedded-c/MQTTPacket.h>



#define MAX_PACKET_ID 65535
#define MAX_MESSAGE_HANDLERS 5

enum QoS { QOS0, QOS1, QOS2 };

struct MQTTMessage {
    enum QoS qos;
    char retained;
    char dup;
    unsigned short id;
    void *payload;
    size_t payloadlen;
};

struct MessageData {
    MQTTMessage* message;
    MQTTString* topicName;
};

struct MessageHandlers {
    const char* topicFilter;
    void (*fp) (MessageData*);
} messageHandlers[MAX_MESSAGE_HANDLERS];      // Message handlers are indexed by subscription topic


class MQTTSession : public XenoSession {
  public:
    MQTTSession(ManuvrXport*);
    ~MQTTSession();

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
    unsigned int _next_packetid;
    unsigned int command_timeout_ms;
    size_t buf_size, readbuf_size;
    unsigned char *buf;
    unsigned char *readbuf;
    unsigned int keepAliveInterval;
    char ping_outstanding;
    int isconnected;

    void (*defaultMessageHandler) (MessageData*);

    // TODO: Implement as Runnable
    //Timer ping_timer;

    inline int getNextPacketId() {
      return _next_packetid = (_next_packetid == MAX_PACKET_ID) ? 1 : _next_packetid + 1;
    };

};

#endif //__XENOSESSION_MQTT_H__
