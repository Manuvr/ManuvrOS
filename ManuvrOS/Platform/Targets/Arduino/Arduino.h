/*
File:   Arduino.h
Author: J. Ian Lindsay
Date:   2016.09.25

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


ManuvrOS has an on-again, off-again relationship WRT Arduino support.

At first, ManuvrOS was a library that was loaded into the Arduino
  environment. But this quickly became untenable because of the
  need to include libraries within libraries.

Rather than subject ourselves to the Arduino IDE and its nerfed usage
  of autotools, we will ask the user to either clone Arduino libraries
  into ManuvrOS' build tree, or require that the user provide a path
  to his locally-installed copy.

Other platforms might inherrit from this API, so keep it general.

Specific hardware targets using Arduino's API should probably fork
  this platform, and use direct hardware features where practical.
*/


#ifndef __PLATFORM_ARDUINO_H__
#define __PLATFORM_ARDUINO_H__

#include <Arduino.h>


class ArduinoWrapper : public ManuvrPlatform {
  public:
    Arduino() : ManuvrPlatform("Arduino") {};
    
    inline  int8_t platformPreInit() {   return platformPreInit(nullptr); };
    virtual int8_t platformPreInit(Argument*);
    virtual void   printDebug(StringBuilder* out);

    /* Platform state-reset functions. */
    void seppuku();           // Simple process termination. Reboots if not implemented.
    void reboot();
    void hardwareShutdown();
    void jumpToBootloader();


  protected:
    const char* _board_name = "Generic";
    virtual int8_t platformPostInit();
    #if defined(CONFIG_MANUVR_STORAGE)
      // Called during boot to load configuration.
      int8_t _load_config();
    #endif
};

#endif  // __PLATFORM_ARDUINO_H__
