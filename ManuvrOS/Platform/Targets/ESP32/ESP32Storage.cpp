/*
File:   ESP32Storage.cpp
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


Implemented as a CBOR object within NVS. This feature therefore
  requires MANUVR_CBOR. In the future, it may be made to operate on some
  other encoding, be run through a cryptographic pipe, etc.

CBOR data begins at offset 4. The first uint32 is broken up this way:
  Offset  |
  --------|----------------
  0       | Magic number (0x7A)
  1       | Magic number (0xB7)
  2       | Bits[7..4] Zero
          | Bits[3..0] LSB of free-space (max, 2044)
  3       | LSB of free-space (max, 2044)
*/

#include "esp_system.h"
#include "ESP32Storage.h"
#include <Platform/Platform.h>

#if defined(MANUVR_STORAGE)

// We want this definition isolated to the compilation unit.
#define STORAGE_PROPS (MANUVR_PL_BLOCK_ACCESS | MANUVR_PL_MEDIUM_READABLE | \
                        MANUVR_PL_MEDIUM_WRITABLE)


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

ESP32Storage::ESP32Storage(Argument* opts) : EventReceiver("ESP32Storage"), Storage() {
  _pl_set_flag(true, STORAGE_PROPS);
}


ESP32Storage::~ESP32Storage() {
  close();
}


/*******************************************************************************
*  __  ______   ___   ____   ___    ___   ____
* (( \ | || |  // \\  || \\ // \\  // \\ ||
*  \\    ||   ((   )) ||_// ||=|| (( ___ ||==
* \_))   ||    \\_//  || \\ || ||  \\_|| ||___
* Storage interface.
********************************************************************************/
unsigned long ESP32Storage::freeSpace() {
  return _free_space;
}

int8_t ESP32Storage::wipe() {
  if (isMounted()) {
    if (ESP_OK == nvs_erase_all(store_handle)) {
      return 0;
    }
  }
  return -1;
}

int8_t ESP32Storage::flush() {
  return (ESP_OK == nvs_commit(store_handle)) ? 0 : -1;
}

int ESP32Storage::persistentWrite(const char* key, uint8_t* buf, unsigned int len, uint16_t opts) {
  if (isMounted()) {
    if (ESP_OK == nvs_set_blob(store_handle, key, (const void*) buf, len)) {
      return len;
    }
  }
  return -1;
}

int ESP32Storage::persistentRead(const char* key, uint8_t* buf, unsigned int len, uint16_t opts) {
  if (isMounted()) {
    size_t nvs_len = 0;
    if (ESP_OK == nvs_get_blob(store_handle, key, (void*) buf, &nvs_len)) {
      if (nvs_len <= len) {
        if (ESP_OK == nvs_get_blob(store_handle, key, (void*) buf, &nvs_len)) {
          return nvs_len;
        }
      }
    }
  }
  return -1;
}


int ESP32Storage::persistentRead(const char* key, StringBuilder* out) {
  if (isMounted()) {
    size_t nvs_len = 0;
    if (ESP_OK == nvs_get_blob(store_handle, key, nullptr, &nvs_len)) {
      uint8_t* buf = (uint8_t*) alloca(nvs_len);
      if (buf) {
        if (ESP_OK == nvs_get_blob(store_handle, key, (void*) buf, &nvs_len)) {
          out->concat(buf, nvs_len);
          return nvs_len;
        }
      }
    }
  }
  return -1;
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
* Boot done finished-up.
* Check to ensure that our storage apparatus is initialized.
*
* @return 0 on no action, 1 on action, -1 on failure.
*/
int8_t ESP32Storage::attached() {
  if (EventReceiver::attached()) {
    // TODO: Init the driver and check for an indication that we've written data.
    if (!isMounted()) {
      // Try to wipe and set status.
      if (ESP_OK == nvs_flash_init()) {
        if (ESP_OK == nvs_open("manuvr", NVS_READWRITE, &store_handle)) {
          _pl_set_flag(true, MANUVR_PL_MEDIUM_MOUNTED);
        }
        else {
          _pl_set_flag((0 == wipe()), MANUVR_PL_MEDIUM_MOUNTED);
        }
      }
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
int8_t ESP32Storage::callback_proc(ManuvrMsg* event) {
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
void ESP32Storage::printDebug(StringBuilder *output) {
  EventReceiver::printDebug(output);
  Storage::printDebug(output);
}


/**
* Debug support method. This fxn is only present in debug builds.
*
* @param   StringBuilder* The buffer into which this fxn should write its output.
*/
int8_t ESP32Storage::close() {
  if (isMounted()) {
    if (0 == flush()) {
      nvs_close(store_handle);
      _pl_set_flag(false, MANUVR_PL_MEDIUM_MOUNTED);
      return 0;
    }
  }
  return -1;
}


/*
* This is the override from EventReceiver.
*/
int8_t ESP32Storage::notify(ManuvrMsg* active_event) {
  int8_t return_value = 0;

  switch (active_event->eventCode()) {
    /* Things that only this class is likely to care about. */
    case MANUVR_MSG_SYS_REBOOT:
    case MANUVR_MSG_SYS_SHUTDOWN:
    case MANUVR_MSG_SYS_BOOTLOADER:
      if (isMounted()) {
        close();
        return_value++;
      }
      break;
    default:
      return_value += EventReceiver::notify(active_event);
      break;
  }

  flushLocalLog();
  return return_value;
}


#if defined(MANUVR_CONSOLE_SUPPORT)
void ESP32Storage::procDirectDebugInstruction(StringBuilder *input) {
  char* str = input->position(0);

  switch (*(str)) {
    case 'w':
      local_log.concatf("Wipe %s.\n", (0 == wipe()) ? "succeeded" : "failed");
      break;

    #if defined(MANUVR_DEBUG)
      case 's':
        if (isMounted()) {
          Argument a(randomInt());
          a.setKey("random_number");
          a.append((int8_t) 64)->setKey("number_64");
          StringBuilder shuttle;
          if (0 <= Argument::encodeToCBOR(&a, &shuttle)) {
            if (0 < persistentWrite("test_key", shuttle.string(), shuttle.length(), 0)) {
              local_log.concat("Save succeeded.\n");
            }
          }
        }
        break;

      case 'B':   // Dump binary representation.
        if (isMounted()) {
          local_log.concat("Key dump:\n");
          StringBuilder shuttle;
          if (0 < persistentRead("test_key", &shuttle)) {
            shuttle.printDebug(&local_log);
          }
        }
        else {
          local_log.concat("Storage not mounted.\n");
        }
        break;
    #endif  // MANUVR_DEBUG

    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }

  flushLocalLog();
}
#endif   // MANUVR_CONSOLE_SUPPORT

#endif   // MANUVR_STORAGE
