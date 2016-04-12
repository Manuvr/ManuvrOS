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


*/

#ifndef __XENOSESSION_MQTT_H__
#define __XENOSESSION_MQTT_H__

#include "../XenoSession.h"

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
    int8_t scan_buffer_for_sync();
};

#endif //__XENOSESSION_MQTT_H__
