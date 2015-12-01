/*
File:   ManuvrableGPIO.h
Author: J. Ian Lindsay
Date:   2015.09.21


Copyright (C) 2015 Manuvr
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

*/


#ifndef MANUVRABLE_GPIO_H
  #define MANUVRABLE_GPIO_H

  #include "ManuvrOS/Kernel.h"


  class ManuvrableGPIO : public EventReceiver {
  
    public:
      ManuvrableGPIO();
      ~ManuvrableGPIO();


      /* Overrides from EventReceiver */
      void printDebug(StringBuilder*);
      const char* getReceiverName();
      int8_t notify(ManuvrEvent*);
      int8_t callback_proc(ManuvrEvent *);
  
  
    protected:
      int8_t bootComplete();
  
  
    private:
  };

#endif // MANUVRABLE_GPIO_H
