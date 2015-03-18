/*
File:   ManuvrXport.h
Author: J. Ian Lindsay
Date:   2015.03.17

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


This driver is designed to give Manuvr platform-abstracted COM ports. By
  this is meant generic asynchronous serial ports. On Arduino, this means
  the Serial (or HardwareSerial) class. On linux, it means /dev/tty<x>.

Platforms that require it should be able to extend this driver for specific 
  kinds of hardware support. For an example of this, I would refer you to
  the STM32F4 case-offs I understand that this might seem "upside down"
  WRT how drivers are more typically implemented, and it may change later on.
  But for now, it seems like a good idea.  
*/


#ifndef __MANUVR_XPORT_H__
#define __MANUVR_XPORT_H__

#include "../EventManager.h"


class ManuvrXport : public EventReceiver {
  public:
    //ManuvrXport();
    //virtual ~ManuvrXport() {};
    

    ///* Overrides from EventReceiver */
    //virtual int8_t bootComplete()               = 0;
    //virtual const char* getReceiverName()       = 0;
    //virtual void printDebug(StringBuilder *)    = 0;
    //virtual int8_t notify(ManuvrEvent*)         = 0;
    //virtual int8_t callback_proc(ManuvrEvent *) = 0;

    
  protected:

  private:
    
    
};



//class ManuvrXportManager : public EventReceiver {
//  public:
//    ManuvrXportManager();
//    ~ManuvrXportManager();
//    
//    /* Overrides from EventReceiver */
//    const char* getReceiverName();
//    void printDebug(StringBuilder *);
//    int8_t notify(ManuvrEvent*);
//    int8_t callback_proc(ManuvrEvent *);
//
//    
//  protected:
//    
//
//  private:
//    void __class_initializer();
//    
//    
//};

#endif   // __MANUVR_XPORT_H__

