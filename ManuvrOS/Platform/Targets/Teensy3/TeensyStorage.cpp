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
          | Bits[3..0] LSB of free-space (max, 2044)
  3       | LSB of free-space (max, 2044)
*/

#include <Platform/Platform.h>
#include <Platform/Targets/Teensy3/TeensyStorage.h>

#if defined(CONFIG_MANUVR_STORAGE)
#include <EEPROM.h>

// We want this definition isolated to the compilation unit.
#define STORAGE_PROPS (PL_FLAG_BLOCK_ACCESS | PL_FLAG_MEDIUM_READABLE | \
                        PL_FLAG_MEDIUM_WRITABLE)


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

TeensyStorage::TeensyStorage(Argument* opts) : EventReceiver("TeensyStorage"), Storage() {
  _pl_set_flag(true, STORAGE_PROPS);
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
  _pl_set_flag(true, PL_FLAG_MEDIUM_MOUNTED);
  return 0;
}

int8_t TeensyStorage::flush() {
  return 0;
}

int TeensyStorage::persistentWrite(const char* key, uint8_t* buf, unsigned int len, uint16_t ) {
  if (isMounted() && len < 2044) {
    int written = 0;
    for (unsigned int i = 0; i < len; i++) {
      EEPROM.update(i+4, *(buf+i));  // NOTE: +4 because of magic number.
      written++;
    }
    _free_space = 2044 - len;
    EEPROM.update(2, (uint8_t) (0x0F & (len >> 8)));
    EEPROM.update(3, (uint8_t) (0xFF & len));
    return written;
  }
  return -1;
}

int TeensyStorage::persistentRead(const char* key, uint8_t* buf, unsigned int len, uint16_t) {
  if (len <= 2044) {
    unsigned int max_l = (2044 - _free_space);
    unsigned int r_len = (max_l > len) ? len : max_l;
    for (unsigned int i = 0; i < r_len; i++) {
      *(buf+i) = EEPROM.read(i+4);  // NOTE: +4 because of magic number.
    }
    for (unsigned int i = r_len; i < len; i++) {
      *(buf+i) = 0;  // Zero the  unused buffer, for safety.
    }
    return r_len;
  }
  return -1;
}


int TeensyStorage::persistentRead(const char*, StringBuilder* out) {
  if (isMounted()) {
    int r_len = (2044 - _free_space);
    uint8_t buf[r_len];
    for (int i = 0; i < r_len; i++) {
      *(buf+i) = EEPROM.read(i+4);  // NOTE: +4 because of magic number.
    }
    out->concat(buf, r_len);
    return r_len;
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
int8_t TeensyStorage::attached() {
  if (EventReceiver::attached()) {
    if (0x7A == EEPROM.read(0)) {
      if (0xB7 == EEPROM.read(1)) {
        uint8_t len_msb = EEPROM.read(2);
        uint8_t len_lsb = EEPROM.read(3);
        if (0x0F >= len_msb) {
          _free_space = 2044 - (len_lsb + (len_msb * 256));
          _pl_set_flag(true, PL_FLAG_MEDIUM_MOUNTED);
        }
      }
    }
    if (!_pl_flag(PL_FLAG_MEDIUM_MOUNTED)) {
      // Try to wipe and set status.
      _pl_set_flag((0 == wipe()), PL_FLAG_MEDIUM_MOUNTED);
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
int8_t TeensyStorage::callback_proc(ManuvrMsg* event) {
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
void TeensyStorage::printDebug(StringBuilder *output) {
  EventReceiver::printDebug(output);
  Storage::printDebug(output);
}


/*
* This is the override from EventReceiver.
*/
int8_t TeensyStorage::notify(ManuvrMsg* active_event) {
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


//#if defined(MANUVR_CONSOLE_SUPPORT)
//void TeensyStorage::procDirectDebugInstruction(StringBuilder *input) {
//  char* str = input->position(0);
//
//  switch (*(str)) {
//    case 'w':
//      local_log.concatf("Wipe %s.\n", (0 == wipe()) ? "succeeded" : "failed");
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
//        local_log.concat("EEPROM Dump:\n");
//        for (uint16_t i = 0; i < EEPROM.length(); i+=32) {
//          local_log.concatf("\t0x%04x: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
//            i,
//            EEPROM.read(i+ 0),
//            EEPROM.read(i+ 1),
//            EEPROM.read(i+ 2),
//            EEPROM.read(i+ 3),
//            EEPROM.read(i+ 4),
//            EEPROM.read(i+ 5),
//            EEPROM.read(i+ 6),
//            EEPROM.read(i+ 7),
//            EEPROM.read(i+ 8),
//            EEPROM.read(i+ 9),
//            EEPROM.read(i+10),
//            EEPROM.read(i+11),
//            EEPROM.read(i+12),
//            EEPROM.read(i+13),
//            EEPROM.read(i+14),
//            EEPROM.read(i+15)
//          );
//          local_log.concatf(" %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
//            EEPROM.read(i+16),
//            EEPROM.read(i+17),
//            EEPROM.read(i+18),
//            EEPROM.read(i+19),
//            EEPROM.read(i+20),
//            EEPROM.read(i+21),
//            EEPROM.read(i+22),
//            EEPROM.read(i+23),
//            EEPROM.read(i+24),
//            EEPROM.read(i+25),
//            EEPROM.read(i+26),
//            EEPROM.read(i+27),
//            EEPROM.read(i+28),
//            EEPROM.read(i+29),
//            EEPROM.read(i+30),
//            EEPROM.read(i+31)
//          );
//        }
//        break;
//    #endif
//
//    default:
//      break;
//  }
//  flushLocalLog();
//}
//#endif   // MANUVR_CONSOLE_SUPPORT
#endif   // __MANUVR_LINUX & CONFIG_MANUVR_STORAGE
