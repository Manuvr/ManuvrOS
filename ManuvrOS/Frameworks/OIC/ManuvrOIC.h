/*
File:   ManuvrOIC.cpp
Author: J. Ian Lindsay
Date:   2016.08.04

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


This is the root of Manuvr's OIC implementation. If this feature is desired,
  it will be compiled as any other EventReceiver. Resources (other EventReceivers)
  can then be fed to it to expose their capabilities to any connected counterparties.

Target is OIC v1.1.

*/

#ifndef __MANUVR_OIC_FRAMEWORK_H__
#define __MANUVR_OIC_FRAMEWORK_H__

#include <EventReceiver.h>


class ManuvrOIC : public EventReceiver {
  public:
    ManuvrOIC();
    virtual ~ManuvrOIC();

    /* Overrides from EventReceiver */
    void procDirectDebugInstruction(StringBuilder*);
    void printDebug(StringBuilder*);
    int8_t notify(ManuvrRunnable*);
    int8_t callback_proc(ManuvrRunnable *);


  protected:
    int8_t bootComplete();


  private:
};

#endif //__MANUVR_OIC_FRAMEWORK_H__
