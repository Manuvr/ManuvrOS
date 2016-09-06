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


This file is meant to contain a set of common functions that are
  typically platform-dependent. The goal is to make a class instance
  that is pre-processor-selectable to reflect the platform with an API
  that is consistent, thereby giving the kernel the ability to...
    * Access the realtime clock (if applicatble)
    * Get definitions for GPIO pins.
    * Access a true RNG (if it exists)
    * Persist and retrieve data across runtimes.
*/


#ifndef __PLATFORM_TEENSY3_H__
#define __PLATFORM_TEENSY3_H__

class Teensy3 : public ManuvrPlatform {
  public:
    virtual int8_t platformPreInit();
    virtual int8_t platformPostInit();

    /* Platform state-reset functions. */
    void seppuku();           // Simple process termination. Reboots if not implemented.
    void reboot();
    void hardwareShutdown();
    void jumpToBootloader();

    virtual void printDebug(StringBuilder* out);


  protected:
};

#endif  // __PLATFORM_VANILLA_LINUX_H__
