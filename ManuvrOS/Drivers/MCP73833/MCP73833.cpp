/*
File:   MCP73833.cpp
Author: J. Ian Lindsay
Date:   2015.11.25


Copyright (C) 2014 J. Ian Lindsay
All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


This class is just a way of tracking a battery charge-manager IC. Doesn't
  do much more than monitor the state of a few GPIO pins, some timers, and
  emits events when the PMIC reports changes in state. Battery charged, charge
  fault, source connection, etc...
  
This class was coded with the assumption that there would only be one such
  chip in a given system.
*/

#include "StaticHub/StaticHub.h"
#include "MCP73833.h"
#include <StringBuilder/StringBuilder.h>


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
  StaticHub::log(digitalRead(MCP73833::INSTANCE->_stat1_pin) ? "stat1 is now high" : "stat1 is now low");
}

/*
* This is an ISR.
*/
void mcp73833_stat2_isr() {
  MCP73833::INSTANCE->_stat2_change_time = millis();
  StaticHub::log(digitalRead(MCP73833::INSTANCE->_stat2_pin) ? "stat2 is now high" : "stat2 is now low");
}



MCP73833::MCP73833(int stat1_pin, int stat2_pin) {
  _stat1_pin  = stat1_pin;
  _stat2_pin  = stat2_pin;

  _stat1_change_time = 0;
  _stat2_change_time = 0;
  
  // TODO: Formallize this for build targets other than Arduino. Use the abstracted Manuvr
  //       GPIO class instead.
  pinMode(_stat1_pin, INPUT_PULLUP);
  pinMode(_stat2_pin, INPUT_PULLUP);
  
  attachInterrupt(_stat1_pin, mcp73833_stat1_isr, CHANGE);
  attachInterrupt(_stat2_pin, mcp73833_stat2_isr, CHANGE);
  
  INSTANCE = this;
}


MCP73833::~MCP73833() {
  detachInterrupt(_stat1_pin);
  detachInterrupt(_stat2_pin);
}

