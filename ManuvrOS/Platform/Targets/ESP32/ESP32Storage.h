/*
File:   ESP32Storage.h
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

#ifndef __MANUVR_ESP32_STORAGE_H__
#define __MANUVR_ESP32_STORAGE_H__

#include <EventReceiver.h>
#include <Storage.h>
#include "nvs_flash.h"
#include "nvs.h"

#if defined(CONFIG_MANUVR_STORAGE) && !defined(MANUVR_CBOR)
  #error The ESP32Storage class requires MANUVR_CBOR be enabled.
#endif

class ESP32Storage : public EventReceiver, public Storage {
  public:
    ESP32Storage(Argument*);
    ~ESP32Storage();

    /* Overrides from Storage. */
    uint64_t freeSpace();  // How many bytes are availible for use?
    StorageErr wipe();          // Call to wipe the data store.
    StorageErr flush();         // Blocks until commit completes.
    StorageErr persistentWrite(const char*, uint8_t*, unsigned int, uint16_t);
    StorageErr persistentRead(const char*, uint8_t*, unsigned int*, uint16_t);
    StorageErr persistentWrite(const char*, StringBuilder*, uint16_t);
    StorageErr persistentRead(const char*, StringBuilder*, uint16_t);

    /* Overrides from EventReceiver */
    void printDebug(StringBuilder*);
    int8_t notify(ManuvrMsg*);
    int8_t callback_proc(ManuvrMsg*);


  protected:
    int8_t attached();


  private:
    nvs_handle store_handle;

    int8_t close();             // Blocks until commit completes.
};

#endif // __MANUVR_ESP32_STORAGE_H__
