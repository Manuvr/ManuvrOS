/*
File:   ManuvrableGPIO.h
Author: J. Ian Lindsay
Date:   2015.09.21

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


The idea here is not to provide any manner of abstraction for GPIO. Our
  only goal is to expose GPIO functionality to outside systems.
*/


#ifndef MANUVRABLE_GPIO_H
  #define MANUVRABLE_GPIO_H

  #include <Kernel.h>
  #include <Platform/Platform.h>


  class ManuvrableGPIO : public EventReceiver {

    public:
      ManuvrableGPIO();
      virtual ~ManuvrableGPIO();


      /* Overrides from EventReceiver */
      void printDebug(StringBuilder*);
      int8_t notify(ManuvrMsg*);
      int8_t callback_proc(ManuvrMsg*);
      #if defined(MANUVR_CONSOLE_SUPPORT)
        void procDirectDebugInstruction(StringBuilder*);
      #endif  //MANUVR_CONSOLE_SUPPORT


    protected:
      int8_t attached();


    private:
      ManuvrMsg _gpio_notice;
  };

#endif // MANUVRABLE_GPIO_H
