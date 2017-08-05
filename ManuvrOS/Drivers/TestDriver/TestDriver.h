/*
File:   TestDriver.h
Author: J. Ian Lindsay
Date:   2016.09.19

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


This module is built and loaded by Platform.cpp if the CONFIG_MANUVR_BENCHMARKS flag
  is set by the build system. This ER ought to perform various tests that
  only make sense at runtime. Security and abuse-hardening ought to be
  done this way.
*/


#include <Platform/Platform.h>

#ifndef __MANUVR_TESTING_DRIVER_H__
#define __MANUVR_TESTING_DRIVER_H__

#define MANUVR_MSG_BNCHMRK_ACRYPT       0xF575   //
#define MANUVR_MSG_BNCHMRK_SCRYPT       0xF576   //
#define MANUVR_MSG_BNCHMRK_RNG          0xF577   //
#define MANUVR_MSG_BNCHMRK_MSG_LOAD     0xF578   //
#define MANUVR_MSG_BNCHMRK_FLOAT        0xF579   //
#define MANUVR_MSG_BNCHMRK_HASH         0xF57A   //


class TestDriver : public EventReceiver {
  public:
    TestDriver();
    ~TestDriver();

    /* Overrides from EventReceiver */
    int8_t notify(ManuvrMsg*);
    int8_t callback_proc(ManuvrMsg*);
    void printDebug(StringBuilder*);
    #if defined(MANUVR_CONSOLE_SUPPORT)
      void procDirectDebugInstruction(StringBuilder*);
    #endif  //MANUVR_CONSOLE_SUPPORT

    void printBenchmarkResults(StringBuilder*);


  protected:
    int8_t attached();


  private:
    unsigned long _msg_t0     = 0;
    unsigned int  _msg_passes = 0;

    #if defined(__BUILD_HAS_DIGEST)
      int CRYPTO_TEST_HASHES();
    #endif

    #if defined(__BUILD_HAS_SYMMETRIC)
      int CRYPTO_TEST_SYMMETRIC();
    #endif

    #if defined(__BUILD_HAS_ASYMMETRIC)
      int CRYPTO_TEST_ASYMMETRIC_SET(Cipher, CryptoKey*);
      int CRYPTO_TEST_ASYMMETRIC();
    #endif
};


#endif  // __MANUVR_TESTING_DRIVER_H__
