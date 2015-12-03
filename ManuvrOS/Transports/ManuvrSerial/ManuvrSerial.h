/*
File:   ManuvrSerial.h
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


#ifndef __MANUVR_COM_PORT_H__
#define __MANUVR_COM_PORT_H__

#include "../ManuvrXport.h"


#if defined (STM32F4XX)        // STM32F4


#elif defined (__MK20DX128__)  // Teensy3


#elif defined (__MK20DX256__)  // Teensy3.1


#elif defined (ARDUINO)        // Fall-through case for basic Arduino support.


#else
  //Assuming a linux environment. Cross your fingers....
  #include <fcntl.h>
  #include <termios.h>
  #include <sys/signal.h>
#endif


class ManuvrSerial : public ManuvrXport {
  public:
    ManuvrSerial(char* tty_nom, int b_rate);
    ManuvrSerial(char* tty_nom, int b_rate, uint32_t opts);
    ~ManuvrSerial();

    /* Overrides from EventReceiver */
    int8_t bootComplete();
    const char* getReceiverName();
    void printDebug(StringBuilder *);
    int8_t notify(ManuvrEvent*);
    int8_t callback_proc(ManuvrEvent *);

    
    int8_t read_port();
    int8_t reset();

    virtual int8_t sendBuffer(StringBuilder*);
    bool write_port(unsigned char* out, int out_len);


  protected:
    void __class_initializer();

    /* Members that deal with sessions. */
    int8_t provide_session(XenoSession*);   // Called whenever we instantiate a session.


  private:
    char*  _addr;
    int    _sock;
    uint32_t _options;

    int baud_rate;

    bool read_timeout_defer;       // Used to timeout a read operation.
    uint32_t pid_read_abort;       // Used to timeout a read operation.
    ManuvrEvent read_abort_event;  // Used to timeout a read operation.
    

};

#endif   // __MANUVR_COM_PORT_H__

