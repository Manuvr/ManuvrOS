/*
File:   ManuvrConsole.h
Author: J. Ian Lindsay
Date:   2016.05.28

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


This class represents a console session with the running Kernel. This simple
  role is done with this amount of complexity to avoid the necessity of
  designating a transport or a serial port specifically for this task.

If you want this feature, you must define MANUVR_CONSOLE_SESSION as well as
  __MANUVR_CONSOLE_SUPPORT in the firmware defs file, or pass it into the build.
*/

#ifndef __MANUVR_CONSOLE_SESS_H__
#define __MANUVR_CONSOLE_SESS_H__

#include "../XenoSession.h"

class ManuvrConsole : public XenoSession {
  public:
    ManuvrConsole(ManuvrXport*);
    ~ManuvrConsole();

    /* Overrides from EventReceiver */
    void procDirectDebugInstruction(StringBuilder*);
    const char* getReceiverName();
    void printDebug(StringBuilder*);
    int8_t notify(ManuvrRunnable*);
    int8_t callback_proc(ManuvrRunnable *);


  protected:
    XenoManuvrMessage* working;         // If we are in the middle of receiving a message,

    int8_t bootComplete();
    int8_t bin_stream_rx(unsigned char* buf, int len);            // Used to feed data to the session.


  private:
    /*
    * A buffer for holding inbound stream until enough has arrived to parse. This eliminates
    *   the need for the transport to care about how much data we consumed versus left in its buffer.
    */
    StringBuilder session_buffer;
};


#endif  // __MANUVR_CONSOLE_SESS_H__
