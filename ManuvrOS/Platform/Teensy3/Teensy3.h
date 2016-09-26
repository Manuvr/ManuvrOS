/*
File:   Linux.h
Author: J. Ian Lindsay
Date:   2016.08.31

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


Despite being an Arduino-ish environment, Teensyduino is largely an
  API relationship with no phylogenic relationship to Arduino's
  hardware support.
Moreover, Teensyduino's code base is well-maintained and full-featured.
  So we will use it outright.

Many of the libraries that ultimately became ManuvrOS were developed
  on a Teensy3.0, and this platform represents a "hard-floor" for
  general support.
*/


#ifndef __PLATFORM_TEENSY3_H__
#define __PLATFORM_TEENSY3_H__

class Teensy3 : public ManuvrPlatform {
  public:
    inline  int8_t platformPreInit() {   return platformPreInit(nullptr); };
    virtual int8_t platformPreInit(Argument*);
    virtual void   printDebug(StringBuilder* out);

    /* Platform state-reset functions. */
    void seppuku();           // Simple process termination. Reboots if not implemented.
    void reboot();
    void hardwareShutdown();
    void jumpToBootloader();



  protected:
    virtual int8_t platformPostInit();
    #if defined(MANUVR_STORAGE)
      // Called during boot to load configuration.
      int8_t _load_config();
    #endif
};

#endif  // __PLATFORM_TEENSY3_H__
