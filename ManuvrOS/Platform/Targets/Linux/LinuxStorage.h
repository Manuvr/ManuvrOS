/*
File:   LinuxStorage.h
Author: J. Ian Lindsay
Date:   2016.08.28

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


Data-persistence layer for linux.
Implemented as a CBOR object within a single file. This feature therefore
  requires MANUVR_CBOR. In the future, it may be made to operate on some
  other encoding, be run through a cryptographic pipe, etc.
*/

#ifndef __MANUVR_LINUX_STORAGE_H__
#define __MANUVR_LINUX_STORAGE_H__

#include <EventReceiver.h>
#include <Storage.h>

#ifndef MANUVR_CBOR
  #error The LinuxStorage class requires MANUVR_CBOR be enabled.
#endif

class LinuxStorage : public EventReceiver, public Storage {
  public:
    LinuxStorage(Argument*);
    ~LinuxStorage();

    /* Overrides from Storage. */
    uint64_t   freeSpace();     // How many bytes are availible for use?
    StorageErr wipe();          // Call to wipe the data store.
    //StorageErr flush();             // Blocks until commit completes.

    /* Raw buffer API. Might have more overhead on some platforms. */
    StorageErr persistentWrite(const char*, uint8_t*, unsigned int, uint16_t);
    StorageErr persistentRead(const char*, uint8_t*, unsigned int*, uint16_t);

    /* StringBuilder API to avoid pedantic copying. */
    StorageErr persistentWrite(const char*, StringBuilder*, uint16_t);
    StorageErr persistentRead(const char*, StringBuilder*, uint16_t);


    /* Overrides from EventReceiver */
    void printDebug(StringBuilder*);
    int8_t notify(ManuvrMsg*);
    int8_t callback_proc(ManuvrMsg*);


  protected:
    int8_t attached();


  private:
    char*          _filename   = nullptr;
    StringBuilder  _disk_buffer;

    StorageErr _save_file(StringBuilder* b);
    StorageErr _load_file(StringBuilder* b);
};

#endif // __MANUVR_LINUX_STORAGE_H__
