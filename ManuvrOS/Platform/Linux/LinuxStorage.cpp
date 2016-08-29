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

// We want this definition isolated to the compilation unit.
#define STORAGE_PROPS (MANUVR_PL_USES_FILESYSTEM)


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
    if (0 == opts->getValueAs("filename", &_filename)) {
      // If we have a filename option, we can open it.
    }
  }
}


LinuxStorage::~LinuxStorage() {
}


/*******************************************************************************
*  __  ______   ___   ____   ___    ___   ____
* (( \ | || |  // \\  || \\ // \\  // \\ ||
*  \\    ||   ((   )) ||_// ||=|| (( ___ ||==
* \_))   ||    \\_//  || \\ || ||  \\_|| ||___
* Storage interface.
********************************************************************************/
long LinuxStorage::freeSpace() {
  return 0;
}

int8_t LinuxStorage::wipe() {
  return 0;
}

int8_t LinuxStorage::persistentWrite(const char*, uint8_t*, int, uint16_t) {
  return 0;
}

int8_t LinuxStorage::persistentRead(const char*, uint8_t*, int, uint16_t) {
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
  output->concatf("-- _filename:                %s\n", (nullptr == _filename ? "<unset>" : _filename));
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
  EventReceiver::procDirectDebugInstruction(input);
  if (local_log.length() > 0) {    Kernel::log(&local_log);  }
}
#endif   // __MANUVR_CONSOLE_SUPPORT

#endif   // __MANUVR_LINUX & MANUVR_STORAGE
