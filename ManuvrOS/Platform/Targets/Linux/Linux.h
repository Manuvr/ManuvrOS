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


This file forms the catch-all for linux platforms that have no support.
*/


#ifndef __PLATFORM_VANILLA_LINUX_H__
#define __PLATFORM_VANILLA_LINUX_H__
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>

#if defined(MANUVR_STORAGE)
#include "LinuxStorage.h"
#endif

#if defined(__MACH__) && defined(__APPLE__)
typedef unsigned long pthread_t;
#endif


/* Used to build an Argument chain from parameters passed to main(). */
Argument* parseFromArgCV(int argc, const char* argv[]);


class LinuxPlatform : public ManuvrPlatform {
  public:
    ~LinuxPlatform();

    inline  int8_t platformPreInit() {   return platformPreInit(nullptr); };
    virtual int8_t platformPreInit(Argument*);
    virtual void   printDebug(StringBuilder* out);

    /* Platform state-reset functions. */
    virtual void seppuku();           // Simple process termination. Reboots if not implemented.
    virtual void reboot();
    virtual void hardwareShutdown();
    virtual void jumpToBootloader();


  protected:
    const char* _board_name = "Generic";
    virtual int8_t platformPostInit();

    #if defined(MANUVR_STORAGE)
      // Called during boot to load configuration.
      int8_t _load_config();
    #endif


  private:
    #if defined(__HAS_CRYPT_WRAPPER)
      uint8_t _binary_hash[32];
      int8_t internal_integrity_check(uint8_t* test_buf, int test_len);
      int8_t hash_self();
    #endif
    void   init_rng();
    void _close_open_threads();
};

#endif  // __PLATFORM_VANILLA_LINUX_H__
