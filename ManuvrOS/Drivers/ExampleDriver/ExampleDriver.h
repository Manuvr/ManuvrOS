/*
File:   ExampleDriver.h
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

*/


#ifndef __MANUVR_EXAMPLE_DRIVER_H__
#define __MANUVR_EXAMPLE_DRIVER_H__

#include "Kernel.h"

#if defined(__MANUVR_LINUX)
#else
  // No supportage.
#endif



class ExampleDriver : public EventReceiver {
  public:
    ExampleDriver();
    ~ExampleDriver();

    /* Overrides from EventReceiver */
    int8_t notify(ManuvrRunnable*);
    int8_t callback_proc(ManuvrRunnable *);
    const char* getReceiverName();
    void printDebug(StringBuilder*);
    #if defined(__MANUVR_CONSOLE_SUPPORT)
      void procDirectDebugInstruction(StringBuilder*);
    #endif  //__MANUVR_CONSOLE_SUPPORT

    volatile static ExampleDriver* INSTANCE;


  protected:
    int8_t bootComplete();


  private:
};


#endif  // __MANUVR_EXAMPLE_DRIVER_H__
