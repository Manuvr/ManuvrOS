/*
File:   ManuvrSerial.h
Author: J. Ian Lindsay
Date:   2015.03.17

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


This driver is designed to give Manuvr platform-abstracted COM ports. By
  this is meant generic asynchronous serial ports. On Arduino, this means
  the Serial (or HardwareSerial) class. On linux, it means /dev/tty<x>.

Platforms that require it should be able to extend this driver for specific
  kinds of hardware support. For an example of this, I would refer you to
  the STM32F4 case-offs I understand that this might seem "upside down"
  WRT how drivers are more typically implemented, and it may change later on.
  But for now, it seems like a good idea.
*/


#ifndef __MANUVR_SERIAL_PORT_H__
#define __MANUVR_SERIAL_PORT_H__

#include "../ManuvrXport.h"


#if defined (STM32F7XX)        // STM32F7

#elif defined (STM32F4XX)      // STM32F4

#elif defined(__MK20DX256__) | defined(__MK20DX128__)  // Teensy3.0/3.1

#elif defined (ARDUINO)        // Fall-through case for basic Arduino support.

#elif defined(__MANUVR_LINUX)
  #include <fcntl.h>
  #include <sys/signal.h>
  #include <fstream>
  #include <iostream>
  #include <termios.h>
#else
  // Unsupported platform.
#endif


class ManuvrSerial : public ManuvrXport {
  public:
    ManuvrSerial(const char* tty_path, int b_rate);
    ManuvrSerial(const char* tty_path, int b_rate, uint32_t opts);
    ~ManuvrSerial();

    /* Override from BufferPipe. */
    virtual int8_t toCounterparty(uint8_t* buf, unsigned int len, int8_t mm);
    virtual int8_t fromCounterparty(uint8_t* buf, unsigned int len, int8_t mm);

    /* Overrides from EventReceiver */
    int8_t bootComplete();
    const char* getReceiverName();
    void printDebug(StringBuilder *);
    int8_t notify(ManuvrRunnable*);
    int8_t callback_proc(ManuvrRunnable *);


    int8_t connect();
    int8_t disconnect();
    int8_t listen();
    int8_t reset();

    int8_t read_port();
    bool   write_port(unsigned char* out, int out_len);



  protected:
    void __class_initializer();


  private:
    const char* _addr;
    int         _sock;
    uint32_t    _options;

    int _baud_rate;

    #if defined(__MANUVR_LINUX)
      struct termios termAttr;
    #endif

    int8_t init();
};

#endif   // __MANUVR_SERIAL_PORT_H__
