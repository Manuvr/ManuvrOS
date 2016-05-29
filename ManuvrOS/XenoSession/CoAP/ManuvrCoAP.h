/*
File:   ManuvrCoAP.h
Author: J. Ian Lindsay
Date:   2016.03.31

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


This class implements CoAP over UDP.

*/


#ifndef __MANUVR_COAP_H__
#define __MANUVR_COAP_H__

#if defined(__MANUVR_COAP)

#include "Kernel.h"

#if defined(__MANUVR_LINUX)
  #include <inttypes.h>
  #include <cstdio>
  #include <stdlib.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <termios.h>
  #include <sys/signal.h>
  #include <fstream>
  #include <iostream>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>

  #include <string.h>
  #include <ctype.h>
  #include <sys/select.h>
  #include <sys/types.h>
  #include <netdb.h>
  #include <sys/stat.h>
  #include <dirent.h>
  #include <errno.h>
  #include <signal.h>

#else
  // No supportage.
#endif



class ManuvrCoAP : public EventReceiver {
  public:
    ManuvrCoAP(const char* addr, const char* port);
    ~ManuvrCoAP();

    /* Overrides from EventReceiver */
    int8_t notify(ManuvrRunnable*);
    int8_t callback_proc(ManuvrRunnable *);
    const char* getReceiverName();
    void printDebug(StringBuilder*);
    #if defined(__MANUVR_CONSOLE_SUPPORT)
      void procDirectDebugInstruction(StringBuilder*);
    #endif  //__MANUVR_CONSOLE_SUPPORT


  protected:
    void __class_initializer();
    int8_t bootComplete();


  private:
    void* _ctx;
    ManuvrRunnable read_abort_event;  // Used to timeout a read operation.

    const char* _addr;
    const char* _port_number;
    int         _sock;
    uint32_t    _options;

    // Related to threading and pipes. This is linux-specific.
    StringBuilder __io_buffer;
    int __parent_pid;
    int __blocking_pid;

    static bool __msgs_registered;


    #if defined(__MANUVR_LINUX) | defined(__MANUVR_FREERTOS)
      // Threaded platforms have a concept of threads...
      unsigned long _thread_id;
    #endif

    int8_t init_resources();
};

#endif // __MANUVR_COAP
#endif // __MANUVR_COAP_H__
