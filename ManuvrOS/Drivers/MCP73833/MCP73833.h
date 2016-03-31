/*
File:   MCP73833.h
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

#ifndef __MCP73833_H__
#define __MCP73833_H__

#include <inttypes.h>
#include <stdint.h>
#include <Kernel.h>

#if defined(_BOARD_FUBARINO_MINI_)
  #define BOARD_IRQS_AND_PINS_DISTINCT 1
#endif


class StringBuilder;

class MCP73833 {
  public:
    // TODO: Everything hanging out in the open until basic operation is more complete.
    uint32_t _stat1_change_time;
    uint32_t _stat2_change_time;
    uint8_t  _stat1_pin;
    uint8_t  _stat2_pin;

    MCP73833(int stat1_pin, int stat2_pin);
    ~MCP73833();

    volatile static MCP73833* INSTANCE;


  protected:


  private:
};




#endif  // __MCP73833_H__
