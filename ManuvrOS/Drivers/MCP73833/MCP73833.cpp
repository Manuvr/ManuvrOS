/*
File:   MCP73833.cpp
Author: J. Ian Lindsay
Date:   2015.11.25

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


This class is just a way of tracking a battery charge-manager IC. Doesn't
  do much more than monitor the state of a few GPIO pins, some timers, and
  emits events when the PMIC reports changes in state. Battery charged, charge
  fault, source connection, etc...

This class was coded with the assumption that there would only be one such
  chip in a given system.
*/

#include "MCP73833.h"
#include <DataStructures/StringBuilder.h>
#include <Platform/Platform.h>


volatile MCP73833* MCP73833::INSTANCE = NULL;


/*
Things we can know for certain...
Any other statX combinations are meaningless because it could mean many things.

stat1 stat2   Condition
---------------------------------
  L     H     Charge-in-progress
  H     L     Charge complete
  L     L     System test mode
*/

/*
* This is an ISR.
*/
void mcp73833_stat1_isr() {
  MCP73833::INSTANCE->_stat1_change_time = millis();
  Kernel::log(readPin(MCP73833::INSTANCE->_stat1_pin) ? "stat1 is now high" : "stat1 is now low");
}

/*
* This is an ISR.
*/
void mcp73833_stat2_isr() {
  MCP73833::INSTANCE->_stat2_change_time = millis();
  Kernel::log(readPin(MCP73833::INSTANCE->_stat2_pin) ? "stat2 is now high" : "stat2 is now low");
}



MCP73833::MCP73833(int stat1_pin, int stat2_pin) {
  _stat1_pin  = stat1_pin;
  _stat2_pin  = stat2_pin;

  _stat1_change_time = 0;
  _stat2_change_time = 0;

  // TODO: Formallize this for build targets other than Arduino. Use the abstracted Manuvr
  //       GPIO class instead.
  gpioDefine(_stat1_pin, INPUT_PULLUP);
  gpioDefine(_stat2_pin, INPUT_PULLUP);

  setPinFxn(_stat1_pin, CHANGE, mcp73833_stat1_isr);
  setPinFxn(_stat2_pin, CHANGE, mcp73833_stat2_isr);

  INSTANCE = this;
}


MCP73833::~MCP73833() {
  unsetPinIRQ(_stat1_pin);
  unsetPinIRQ(_stat2_pin);
}
