/*
File:   TeensyStorage.cpp
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


Implemented as a CBOR object within EEPROM. This feature therefore
  requires MANUVR_CBOR. In the future, it may be made to operate on some
  other encoding, be run through a cryptographic pipe, etc.

CBOR data begins at offset 4. The first uint32 is broken up this way:
  Offset  |
  --------|----------------
  0       | Magic number (0x7A)
  1       | Magic number (0xB7)
  2       | Bits[7..4] Zero
          | Bits[3..0] MSB of free-space (max, 2044)
  3       | LSB of free-space (max, 2044)
*/

#if defined(MANUVR_STORAGE)
#include <Platform/Teensy3/TeensyStorage.h>
#include <Platform/Platform.h>
#include <EEPROM.h>

// We want this definition isolated to the compilation unit.
#define STORAGE_PROPS (MANUVR_PL_BLOCK_ACCESS | MANUVR_PL_MEDIUM_READABLE | \
                        MANUVR_PL_MEDIUM_WRITABLE)


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

TeensyStorage::TeensyStorage(Argument* opts) : EventReceiver(), Storage() {
  _pl_set_flag(true, STORAGE_PROPS);
  setReceiverName("TeensyStorage");
  if (nullptr != opts) {
  }
}


TeensyStorage::~TeensyStorage() {
}


/*******************************************************************************
*  __  ______   ___   ____   ___    ___   ____
* (( \ | || |  // \\  || \\ // \\  // \\ ||
*  \\    ||   ((   )) ||_// ||=|| (( ___ ||==
* \_))   ||    \\_//  || \\ || ||  \\_|| ||___
* Storage interface.
********************************************************************************/
unsigned long TeensyStorage::freeSpace() {
  return _free_space;
}

int8_t TeensyStorage::wipe() {
  EEPROM.update(0, 0x7A);
  EEPROM.update(1, 0xB7);
  for (int i = 2; i < EEPROM.length(); i++) {
    EEPROM.update(i, 0);
  }
  _free_space = 2044;
  _pl_set_flag(true, MANUVR_PL_MEDIUM_MOUNTED);
  return 0;
}

int8_t TeensyStorage::flush() {
  return 0;
}

int8_t TeensyStorage::persistentWrite(const char* key, uint8_t* value, int len, uint16_t ) {
  return 0;
}

int8_t TeensyStorage::persistentRead(const char* key, uint8_t* value, int len, uint16_t) {
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
int8_t TeensyStorage::bootComplete() {
  EventReceiver::bootComplete();
  if (0x7A == EEPROM.read(0)) {
    if (0xB7 == EEPROM.read(1)) {
      uint8_t len_msb = EEPROM.read(2);
      if (0x0F >= len_msb) {
        _free_space = 2044 - (EEPROM.read(3) + (len_msb * 256));
        _pl_set_flag(true, MANUVR_PL_MEDIUM_MOUNTED);
      }
    }
  }

  if (!_pl_flag(MANUVR_PL_MEDIUM_MOUNTED)) {
    wipe();
  }

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
int8_t TeensyStorage::callback_proc(ManuvrRunnable *event) {
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
void TeensyStorage::printDebug(StringBuilder *output) {
  EventReceiver::printDebug(output);
  Storage::printDebug(output);
}


/*
* This is the override from EventReceiver.
*/
int8_t TeensyStorage::notify(ManuvrRunnable *active_event) {
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
void TeensyStorage::procDirectDebugInstruction(StringBuilder *input) {
  char* str = input->position(0);

  switch (*(str)) {
    case 'w':
      local_log.concatf("Wipe %s.\n", (0 == wipe()) ? "succeeded" : "failed");
      break;

    case 'l':
      local_log.concat("Load failed.\n");
      break;

    case 's':
      local_log.concat("Save failed.\n");
      break;

    #if defined(__MANUVR_DEBUG)
      case 'B':   // Dump binary representation.
        local_log.concat("EEPROM Dump:\n");
        for (uint16_t i = 0; i < EEPROM.length(); i+=32) {
          local_log.concatf("\t0x%04x: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
            i,
            EEPROM.read(i+ 0),
            EEPROM.read(i+ 1),
            EEPROM.read(i+ 2),
            EEPROM.read(i+ 3),
            EEPROM.read(i+ 4),
            EEPROM.read(i+ 5),
            EEPROM.read(i+ 6),
            EEPROM.read(i+ 7),
            EEPROM.read(i+ 8),
            EEPROM.read(i+ 9),
            EEPROM.read(i+10),
            EEPROM.read(i+11),
            EEPROM.read(i+12),
            EEPROM.read(i+13),
            EEPROM.read(i+14),
            EEPROM.read(i+15)
          );
          local_log.concatf(" %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
            EEPROM.read(i+16),
            EEPROM.read(i+17),
            EEPROM.read(i+18),
            EEPROM.read(i+19),
            EEPROM.read(i+20),
            EEPROM.read(i+21),
            EEPROM.read(i+22),
            EEPROM.read(i+23),
            EEPROM.read(i+24),
            EEPROM.read(i+25),
            EEPROM.read(i+26),
            EEPROM.read(i+27),
            EEPROM.read(i+28),
            EEPROM.read(i+29),
            EEPROM.read(i+30),
            EEPROM.read(i+31)
          );
        }
        break;
    #endif

    default:
      EventReceiver::procDirectDebugInstruction(input);
      break;
  }

  if (local_log.length() > 0) {    Kernel::log(&local_log);  }
}
#endif   // __MANUVR_CONSOLE_SUPPORT

#endif   // __MANUVR_LINUX & MANUVR_STORAGE
