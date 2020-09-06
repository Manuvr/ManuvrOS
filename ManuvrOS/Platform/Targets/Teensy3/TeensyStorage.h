/*
File:   TeensyStorage.h
Author: J. Ian Lindsay
Date:   2016.09.05

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


Data-persistence layer for Teensy.
*/

#ifndef __MANUVR_TEENSY_STORAGE_H__
#define __MANUVR_TEENSY_STORAGE_H__

#include <EventReceiver.h>
#include <Storage.h>

#if defined(CONFIG_MANUVR_STORAGE) && !defined(MANUVR_CBOR)
  #error The TeensyStorage class requires MANUVR_CBOR be enabled.
#endif

class TeensyStorage : public EventReceiver, public Storage {
  public:
    TeensyStorage(Argument*);
    ~TeensyStorage();

    /* Overrides from Storage. */
    unsigned long freeSpace();  // How many bytes are availible for use?
    int8_t wipe();              // Call to wipe the data store.
    int8_t flush();             // Blocks until commit completes.

    int persistentWrite(const char*, uint8_t*, unsigned int, uint16_t);
    int persistentRead(const char*, uint8_t*, unsigned int, uint16_t);
    int persistentRead(const char*, StringBuilder*);

    /* Overrides from EventReceiver */
    void printDebug(StringBuilder*);
    int8_t notify(ManuvrMsg*);
    int8_t callback_proc(ManuvrMsg*);


  protected:
    int8_t attached();


  private:
    uint16_t _free_space = 0;
};

#endif // __MANUVR_TEENSY_STORAGE_H__
