/*
File:   MCP73833.h
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

#ifndef __MCP73833_H__
#define __MCP73833_H__

#include <inttypes.h>
#include <stdint.h>
#include "ManuvrOS/Kernel.h"

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
