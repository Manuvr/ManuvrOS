/*
File:   Raspi.h
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

#ifndef __PLATFORM_RASPI_H__
#define __PLATFORM_RASPI_H__
#include <Platform/Targets/Linux/Linux.h>


class Raspi : public LinuxPlatform {
  public:
    Raspi();

    int8_t platformPreInit() {   return platformPreInit(nullptr); };
    int8_t platformPreInit(Argument*);
    void printDebug(StringBuilder* out);


  protected:
    virtual int8_t platformPostInit();
};

#endif  // __PLATFORM_RASPI_H__
