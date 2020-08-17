/*
File:   LinuxStorage.cpp
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
Implemented as a JSON object within a single file. This feature therefore
  requires MANUVR_JSON. In the future, it may be made to operate on some
  other encoding, be run through a cryptographic pipe, etc.
*/

#include "LinuxStorage.h"

#if defined(__MANUVR_LINUX) && defined(CONFIG_MANUVR_STORAGE)
#include <Platform/Platform.h>
#include <unistd.h>
#include <fcntl.h>

// We want this definition isolated to the compilation unit.
#define STORAGE_PROPS (PL_FLAG_USES_FILESYSTEM | PL_FLAG_BLOCK_ACCESS)


/*******************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here.
*******************************************************************************/


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

LinuxStorage::LinuxStorage(Argument* opts) : EventReceiver("LinuxStorage"), Storage() {
  _pl_set_flag(true, STORAGE_PROPS);
  if (nullptr != opts) {
    char* str = nullptr;
    if (0 == opts->getValueAs("store_path", &str)) {
      // If we have a filename option, we can open it.
      int len = strlen(str) + 1;  // Because: NULL-terminator.
      _filename = (char*) malloc(len);
      for (int i = 0; i < len; i++) {
        *(_filename+i) = *(str+i);
      }
    }
  }
}


LinuxStorage::~LinuxStorage() {
  if (nullptr != _filename) {
    free(_filename);
    _filename = nullptr;
  }
}


/*******************************************************************************
*  __  ______   ___   ____   ___    ___   ____
* (( \ | || |  // \\  || \\ // \\  // \\ ||
*  \\    ||   ((   )) ||_// ||=|| (( ___ ||==
* \_))   ||    \\_//  || \\ || ||  \\_|| ||___
* Storage interface.
********************************************************************************/
uint64_t LinuxStorage::freeSpace() {
  // TODO: Need a sophisticated apparatus to find this accurately.
  // What partition is _filename on?
  // How much space free on it?
  // Cap to arbitrary maximum value for now.
  return ((2 << 16) - _disk_buffer.length());
}

StorageErr LinuxStorage::wipe() {
  _disk_buffer.clear();
  _disk_buffer.concat('\0');
  return _save_file(&_disk_buffer);
}

//StorageErr LinuxStorage::flush() {
//  if (_filename) {
//    if (0 < _disk_buffer.length()) {
//      return _save_file(&_disk_buffer);
//    }
//  }
//  return 0;
//}


StorageErr LinuxStorage::persistentWrite(const char* key, uint8_t* buf, unsigned int len, uint16_t flags) {
  if (isMounted()) {
    _disk_buffer.clear();
    _disk_buffer.concat(buf, len);
    return _save_file(&_disk_buffer);
  }
  return StorageErr::NOT_MOUNTED;
}


StorageErr LinuxStorage::persistentRead(const char* key, uint8_t* buf, uint* len, uint16_t flags) {
  if (isMounted()) {
    uint max_l = _disk_buffer.length();
    uint r_len = (max_l > *len) ? *len : max_l;
    uint8_t* d_buf = _disk_buffer.string();
    for (uint i = 0; i < r_len; i++) {
      *(buf+i) = *(d_buf+i);
    }
    for (uint i = r_len; i < *len; i++) {
      *(buf+i) = 0;  // Zero the  unused buffer, for safety.
    }
    *len = r_len;
    return StorageErr::NONE;
  }
  return StorageErr::NOT_MOUNTED;
}


StorageErr LinuxStorage::persistentRead(const char* key, StringBuilder* out, uint16_t flags) {
  if (isMounted()) {
    out->concat(&_disk_buffer);
    //return _disk_buffer.length();
    return StorageErr::NONE;
  }
  return StorageErr::NOT_MOUNTED;
}


/**
* Open the file write-only and save the buffer. Clobbering the file's contents.
*/
StorageErr LinuxStorage::_save_file(StringBuilder* b) {
  StorageErr return_value = StorageErr::BAD_PARAM;  // Fail by default.
  if (_filename) {
    return_value = StorageErr::NOT_READABLE;
    int fd = open(_filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd >= 0) {
      _pl_set_flag(true, PL_FLAG_BUSY_WRITE);
      int len = b->length();
      StringBuilder buf(b->string(), len);  // Buffer is now locally-scoped.
      int q = write(fd, buf.string(), len);
      if (len == q) {
        return_value = StorageErr::NONE;
      }
      close(fd);
      _pl_set_flag(false, PL_FLAG_BUSY_WRITE);
    }
  }
  return return_value;
}


/**
* Open the file read-only and fill the buffer.
*/
StorageErr LinuxStorage::_load_file(StringBuilder* buf) {
  if (_filename) {
    int fd = open(_filename, O_RDONLY);
    if (fd >= 0) {
      int total_read = 0;
      _pl_set_flag(true, PL_FLAG_BUSY_READ);
      uint8_t* buffer = (uint8_t*) alloca(2048);
      int r_len = read(fd, buffer, 2048);
      while (0 < r_len) {
        buf->concat(buffer, r_len);
        total_read += r_len;
        r_len = read(fd, buffer, 2048);
      }
      close(fd);
      _pl_set_flag(false, PL_FLAG_BUSY_READ);
      _pl_set_flag(true, PL_FLAG_MEDIUM_MOUNTED | PL_FLAG_MEDIUM_READABLE | PL_FLAG_MEDIUM_WRITABLE);
      return StorageErr::NONE;
    }
  }
  return StorageErr::BAD_PARAM;
}



/*******************************************************************************
* ######## ##     ## ######## ##    ## ########  ######
* ##       ##     ## ##       ###   ##    ##    ##    ##
* ##       ##     ## ##       ####  ##    ##    ##
* ######   ##     ## ######   ## ## ##    ##     ######
* ##        ##   ##  ##       ##  ####    ##          ##
* ##         ## ##   ##       ##   ###    ##    ##    ##
* ########    ###    ######## ##    ##    ##     ######
*
* These are overrides from EventReceiver interface...
*******************************************************************************/

/**
* This is called when the kernel attaches the module.
* This is the first time the class can be expected to have kernel access.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t LinuxStorage::attached() {
  if (EventReceiver::attached()) {
    if (0 <= (int8_t) _load_file(&_disk_buffer)) {
    }
    else {
      // Attempt to create the file.
      wipe();
      if (StorageErr::NONE == wipe()) _load_file(&_disk_buffer);
    }
    return 1;
  }
  return 0;
}


/**
* If we find ourselves in this fxn, it means an event that this class built (the argument)
*   has been serviced and we are now getting the chance to see the results. The argument
*   to this fxn will never be NULL.
*
* Depending on class implementations, we might choose to handle the completed Event differently. We
*   might add values to event's Argument chain and return RECYCLE. We may also free() the event
*   ourselves and return DROP. By default, we will return REAP to instruct the Kernel
*   to either free() the event or return it to it's preallocate queue, as appropriate. If the event
*   was crafted to not be in the heap in its own allocation, we will return DROP instead.
*
* @param  event  The event for which service has been completed.
* @return A callback return code.
*/
int8_t LinuxStorage::callback_proc(ManuvrMsg* event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = (0 == event->refCount()) ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

  /* Some class-specific set of conditionals below this line. */
  switch (event->eventCode()) {
    default:
      break;
  }

  return return_value;
}


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
void LinuxStorage::printDebug(StringBuilder *output) {
  EventReceiver::printDebug(output);
  Storage::printStorage(output);
  output->concatf("-- _filename:           %s\n", (nullptr == _filename ? "<unset>" : _filename));
}


/*
* This is the override from EventReceiver.
*/
int8_t LinuxStorage::notify(ManuvrMsg* active_event) {
  int8_t return_value = 0;

  switch (active_event->eventCode()) {
    /* Things that only this class is likely to care about. */
    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }

  flushLocalLog();
  return return_value;
}


//void LinuxStorage::consoleCmdProc(StringBuilder* input) {
//  char* str = input->position(0);
//
//  switch (*(str)) {
//    case 'w':
//      if (_filename) {
//        local_log.concatf("Wipe of %s %s.\n", _filename, (0 == wipe()) ? "succeeded" : "failed");
//      }
//      else {
//        local_log.concat("Filename not set.\n");
//      }
//      break;
//
//    case 'l':
//      if (0 == _load_file(&_disk_buffer)) {
//        local_log.concatf("Loaded %u bytes from %s.\n", _disk_buffer.length(), _filename);
//      }
//      else {
//        local_log.concat("Load failed.\n");
//      }
//      break;
//
//    case 's':
//      if (isMounted()) {
//        Argument a(randomUInt32());
//        a.setKey("random_number");
//        a.append((int8_t) 64)->setKey("number_64");
//        StringBuilder shuttle;
//        if (0 <= Argument::encodeToCBOR(&a, &shuttle)) {
//          if (0 < persistentWrite(nullptr, shuttle.string(), shuttle.length(), 0)) {
//            local_log.concat("Save succeeded.\n");
//          }
//        }
//      }
//      break;
//
//    #if defined(MANUVR_DEBUG)
//      case 'B':   // Dump binary representation.
//        local_log.concat("Dump:\n");
//        _disk_buffer.printDebug(&local_log);
//        break;
//    #endif
//
//    default:
//      break;
//  }
//
//  flushLocalLog();
//}
#endif   // __MANUVR_LINUX & CONFIG_MANUVR_STORAGE
