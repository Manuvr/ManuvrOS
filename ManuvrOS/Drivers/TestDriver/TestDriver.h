/*
File:   TestDriver.h
Author: J. Ian Lindsay
Date:   2016.09.19

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


This module is built and loaded by Platform.cpp if the MANUVR_TESTS flag
  is set by the build system. This ER ought to perform various tests that
  only make sense at runtime. Security and abuse-hardening ought to be
  done this way.
*/


#ifndef __MANUVR_TESTING_DRIVER_H__
#define __MANUVR_TESTING_DRIVER_H__

#include <Platform/Platform.h>


class TestDriver : public EventReceiver {
  public:
    TestDriver();
    ~TestDriver();

    /* Overrides from EventReceiver */
    int8_t notify(ManuvrRunnable*);
    int8_t callback_proc(ManuvrRunnable *);
    void printDebug(StringBuilder*);
    #if defined(MANUVR_CONSOLE_SUPPORT)
      void procDirectDebugInstruction(StringBuilder*);
    #endif  //MANUVR_CONSOLE_SUPPORT


  protected:
    int8_t attached();


  private:
};


#endif  // __MANUVR_TESTING_DRIVER_H__
