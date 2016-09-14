/*
File:   Identity.cpp
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


Basic machinery of Identity objects.
*/

#include "Identity.h"
#include <alloca.h>
#include <Platform/Identity/IdentityUUID.h>
#include <stdlib.h>

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
* This is an abstract factory function for re-constituting identities from
*   storage. It sets flags to reflect origins, and handles casting and so-forth.
* Buffer is formatted like so...
*   /--------------------------------------------------------------------------\
*   | Len MSB | Len LSB | Flgs MSB | Flgs LSB | Format | null-term str | extra |
*   \--------------------------------------------------------------------------/
*
* The "extra" field contains the class-specific data remaining in the buffer.
*
* @param buf
* @param len
* @return An identity chain on success, or nullptr on failure.
*/
Identity* Identity::fromBuffer(uint8_t* buf, int len) {
  if (len > IDENTITY_BASE_PERSIST_LENGTH) {
    uint16_t ident_len = (((uint16_t) *(buf+0)) << 8) + *(buf+1);
    uint16_t ident_flg = (((uint16_t) *(buf+2)) << 8) + *(buf+3);
    IdentFormat fmt = (IdentFormat) *(buf+4);
    len -= 5;
    buf += 5;

    switch (fmt) {
      case IdentFormat::UUID:
        if (ident_len >= len) {
          IdentityUUID* ret = new IdentityUUID(buf, (uint16_t) len);
          ret->_ident_flags = ident_flg;
          return (Identity*)ret;
        }
      case IdentFormat::SERIAL_NUM:
        // TODO: Ill-conceived? Why persist a hardware serial number???
        break;
      case IdentFormat::HASH:
        break;
      case IdentFormat::CERT_FORMAT_DER:
        break;
      case IdentFormat::PSK_ASYM:
        break;
      case IdentFormat::PSK_SYM:
        break;
      #if defined (MANUVR_OPENINTERCONNECT)
        case IdentFormat::OIC_CRED:
          break;
      #endif
      #if defined (MANUVR_ONEID)
        case IdentFormat::ONE_ID:
          break;
      #endif
      case IdentFormat::UNDETERMINED:
      default:
        break;
    }
  }
  return nullptr;
}



/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

Identity::Identity(const char* nom, IdentFormat _f) {
  format = _f;
  _ident_len = strlen(nom);   // TODO: Scary....
  _handle = (char*) malloc(_ident_len + 1);
  // Also copies the required null-terminator.
  for (int i = 0; i < _ident_len+1; i++) *(_handle + i) = *(nom+i);
  _ident_len += IDENTITY_BASE_PERSIST_LENGTH;
}


Identity::~Identity() {
  if (_handle) {
    free(_handle);
    _handle = nullptr;
  }
}


/*
* Only the persistable particulars of this instance. All the base class and
*   error-checking are done upstream.
*/
int Identity::_serialize(uint8_t* buf, uint16_t len) {
  if (len > IDENTITY_BASE_PERSIST_LENGTH) {
    *(buf+0) = (uint8_t) (_ident_len >> 8) & 0xFF;
    *(buf+1) = (uint8_t) _ident_len & 0xFF;
    *(buf+2) = (uint8_t) (_ident_flags >> 8) & 0xFF;
    *(buf+3) = (uint8_t) _ident_flags & 0xFF;
    *(buf+4) = (uint8_t) format;
    len -= 5;
    buf += 5;

    int str_bytes = 0;
    if (_handle) {
      str_bytes = strlen((const char*) _handle);
      if (str_bytes < len) {
        memcpy(buf, _handle, str_bytes + 1);
      }
    }
    else {
      *(buf+5) = '\0';
    }

    return str_bytes + IDENTITY_BASE_PERSIST_LENGTH;
  }
  return 0;
}
