/*
File:   Storage.h
Author: J. Ian Lindsay
Date:   2016.08.15

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


This file represents the platform-specific interface to a persistent
  data storage mechanism. Firmware that wants to use local data that
  is not compiled-in as constants will need at least one class that
  extends this.
*/


#ifndef __MANUVR_PERSIST_LAYER_H__
#define __MANUVR_PERSIST_LAYER_H__

#define MANUVR_PL_USES_FILESYSTEM      0x0001  // Medium contains a filesystem.
#define MANUVR_PL_BLOCK_ACCESS         0x0002  // I/O operations must occur blockwise.
#define MANUVR_PL_ENCRYPTED            0x0004  // Data stored in this medium is encrypted at rest.
#define MANUVR_PL_REMOVABLE            0x0008  // Media are removable.
#define MANUVR_PL_BATTERY_DEPENDENT    0x0010  // Data-retention is contingent on constant current.
#define MANUVR_PL_MEDIUM_MOUNTED       0x0020  // Medium is ready for use.
#define MANUVR_PL_BUSY_READ            0x4000  // There is a read operation pending completion.
#define MANUVR_PL_BUSY_WRITE           0x8000  // There is a write operation pending completion.


/**
* This class is a gateway to I/O. It will almost certainly need to have
*   some of it's operations run asynchronously or threaded. We leave those
*   concerns for any implementing class.
*/
class Storage {
  public:
    virtual long   freeSpace() =0;  // How many bytes are availible for use?
    virtual int8_t wipe()      =0;  // Call to wipe the data store.

    virtual int8_t persistentWrite(const char*, uint8_t*, int, uint16_t) =0;
    virtual int8_t persistentRead(const char*, uint8_t*, int, uint16_t)  =0;


  protected:
    Storage() {};  // Protected constructor.

    inline bool _pl_flag(uint16_t _flag) {        return (_pl_flags & _flag);  };
    inline void _pl_set_flag(bool nu, uint16_t _flag) {
      _pl_flags = (nu) ? (_pl_flags | _flag) : (_pl_flags & ~_flag);
    };


  private:
    uint16_t      _pl_flags = 0;
};

#endif // __MANUVR_PERSIST_LAYER_H__
