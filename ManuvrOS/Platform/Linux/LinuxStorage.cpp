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

#if defined(__MANUVR_LINUX) & defined(MANUVR_STORAGE)
#include <Platform/Linux/LinuxStorage.h>
#include <Platform/Platform.h>
#include <unistd.h>
#include <fcntl.h>

// We want this definition isolated to the compilation unit.
#define STORAGE_PROPS (MANUVR_PL_USES_FILESYSTEM | MANUVR_PL_BLOCK_ACCESS)


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

/**
* Open the file write-only and save the buffer. Clobbering the file's contents.
*/
int _save_file(char* nom, StringBuilder* b) {
  int return_value = -1;  // Fail by default.
  if (nullptr != nom) {
    int fd = open(nom, O_CREAT | O_WRONLY | O_EXCL | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd >= 0) {
      int len = b->length();
      StringBuilder buf(b->string(), len);  // Buffer is now locally-scoped.
      if (len == write(fd, buf.string(), len)) {
        return_value = 0;
      }
      close(fd);
    }
  }
  return return_value;
}


/**
* Open the file read-only and fill the buffer.
*/
int _load_file(char* nom, StringBuilder* buf) {
  if (nullptr != nom) {
    int fd = open(nom, O_RDONLY);
    if (fd >= 0) {
      uint8_t* buffer = (uint8_t*) alloca(128);
      int r_len = read(fd, buffer, 128);
      while (0 < r_len) {
        buf->concat(buffer, r_len);
        r_len = read(fd, buffer, 128);
      }
      close(fd);
      return 0;
    }
  }
  return -1;
}



/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

LinuxStorage::LinuxStorage(Argument* opts) : EventReceiver(), Storage() {
  _pl_set_flag(true, STORAGE_PROPS);
  setReceiverName("LinuxStorage");
  if (nullptr != opts) {
    char* str = nullptr;
    if (0 == opts->getValueAs("filepath", &str)) {
      // If we have a filename option, we can open it.
      int len = strlen(str) + 1;  // Because: NULL-terminator.
      _filename = (char*) malloc(len);
      for (int i = 0; i < len; i++) {
        *(_filename+i) = *(str+i);
      }
      _pl_set_flag(true, MANUVR_PL_BUSY_READ);
      if (_load_file(_filename, &_disk_buffer)) {
        _pl_set_flag(true, MANUVR_PL_MEDIUM_MOUNTED | MANUVR_PL_MEDIUM_READABLE);
      }
      _pl_set_flag(false, MANUVR_PL_BUSY_READ);
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
unsigned long LinuxStorage::freeSpace() {
  // TODO: Need a sophisticated apparatus to find this accurately.
  // What partition is )_filename on?
  // How much space free on it?
  // Cap to arbitrary maximum value for now.
  return ((2 << 16) - _disk_buffer.length());
}

int8_t LinuxStorage::wipe() {
  if (nullptr != _filename) {
    _disk_buffer.clear();
    _disk_buffer.concat('\0');
    return _save_file(_filename, &_disk_buffer);
  }
  return -1;
}

int8_t LinuxStorage::flush() {
  if (nullptr != _filename) {
    if (0 < _disk_buffer.length()) {
      return _save_file(_filename, &_disk_buffer);
    }
  }
  return 0;
}

int8_t LinuxStorage::persistentWrite(const char* key, uint8_t* value, int len, uint16_t ) {

  return 0;
}

int8_t LinuxStorage::persistentRead(const char* key, uint8_t* value, int len, uint16_t) {
  return 0;
}


int8_t persistentRead(const char*, StringBuilder*) {
  return 0;
}


/****************************************************************************************************
*  ▄▄▄▄▄▄▄▄▄▄▄  ▄               ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄        ▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄
* ▐░░░░░░░░░░░▌▐░▌             ▐░▌▐░░░░░░░░░░░▌▐░░▌      ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
* ▐░█▀▀▀▀▀▀▀▀▀  ▐░▌           ▐░▌ ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌░▌     ▐░▌ ▀▀▀▀█░█▀▀▀▀ ▐░█▀▀▀▀▀▀▀▀▀
* ▐░▌            ▐░▌         ▐░▌  ▐░▌          ▐░▌▐░▌    ▐░▌     ▐░▌     ▐░▌
* ▐░█▄▄▄▄▄▄▄▄▄    ▐░▌       ▐░▌   ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌ ▐░▌   ▐░▌     ▐░▌     ▐░█▄▄▄▄▄▄▄▄▄
* ▐░░░░░░░░░░░▌    ▐░▌     ▐░▌    ▐░░░░░░░░░░░▌▐░▌  ▐░▌  ▐░▌     ▐░▌     ▐░░░░░░░░░░░▌
* ▐░█▀▀▀▀▀▀▀▀▀      ▐░▌   ▐░▌     ▐░█▀▀▀▀▀▀▀▀▀ ▐░▌   ▐░▌ ▐░▌     ▐░▌      ▀▀▀▀▀▀▀▀▀█░▌
* ▐░▌                ▐░▌ ▐░▌      ▐░▌          ▐░▌    ▐░▌▐░▌     ▐░▌               ▐░▌
* ▐░█▄▄▄▄▄▄▄▄▄        ▐░▐░▌       ▐░█▄▄▄▄▄▄▄▄▄ ▐░▌     ▐░▐░▌     ▐░▌      ▄▄▄▄▄▄▄▄▄█░▌
* ▐░░░░░░░░░░░▌        ▐░▌        ▐░░░░░░░░░░░▌▐░▌      ▐░░▌     ▐░▌     ▐░░░░░░░░░░░▌
*  ▀▀▀▀▀▀▀▀▀▀▀          ▀          ▀▀▀▀▀▀▀▀▀▀▀  ▀        ▀▀       ▀       ▀▀▀▀▀▀▀▀▀▀▀
*
* These are overrides from EventReceiver interface...
****************************************************************************************************/
/**
* Boot done finished-up.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t LinuxStorage::bootComplete() {
  EventReceiver::bootComplete();
  return 1;
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
int8_t LinuxStorage::callback_proc(ManuvrRunnable *event) {
  /* Setup the default return code. If the event was marked as mem_managed, we return a DROP code.
     Otherwise, we will return a REAP code. Downstream of this assignment, we might choose differently. */
  int8_t return_value = event->kernelShouldReap() ? EVENT_CALLBACK_RETURN_REAP : EVENT_CALLBACK_RETURN_DROP;

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
  output->concatf("-- _filename:           %s\n", (nullptr == _filename ? "<unset>" : _filename));
  Storage::printDebug(output);
}


/*
* This is the override from EventReceiver.
*/
int8_t LinuxStorage::notify(ManuvrRunnable *active_event) {
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


#if defined(__MANUVR_CONSOLE_SUPPORT)
void LinuxStorage::procDirectDebugInstruction(StringBuilder *input) {
  char* str = input->position(0);

  switch (*(str)) {
    case 'w':
      if (0 == wipe()) {
        local_log.concatf("Wipe of %s succeeded.\n", _filename);
      }
      else {
        local_log.concat("Wipe failed.\n");
      }
      break;

    case 'l':
      if (0 == _load_file(_filename, &_disk_buffer)) {
        local_log.concatf("Loaded %u bytes from %s.\n", _disk_buffer.length(), _filename);
      }
      else {
        local_log.concat("Load failed.\n");
      }
      break;

    case 's':
      if (0 == _save_file(_filename, &_disk_buffer)) {
        local_log.concatf("Saved %s.\n", _filename);
      }
      else {
        local_log.concat("Save failed.\n");
      }
      break;
    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }

  if (local_log.length() > 0) {    Kernel::log(&local_log);  }
}
#endif   // __MANUVR_CONSOLE_SUPPORT

#endif   // __MANUVR_LINUX & MANUVR_STORAGE
