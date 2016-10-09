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

// TODO: These preprocessor breakouts are not granular enough.
/*
* Given in orderr of preference.
*/
const IdentFormat Identity::supported_notions[] = {
  #if defined(__HAS_CRYPT_WRAPPER)
    IdentFormat::CERT_FORMAT_DER,
    IdentFormat::PK,
    #if defined(WRAPPED_PK_OPT_SECP256R1) && defined(WRAPPED_ASYM_ECDSA)
      IdentFormat::ONE_ID,
    #endif
  #endif
  #if defined(MANUVR_OPENINTERCONNECT)
    IdentFormat::OIC_CRED,
  #endif
  IdentFormat::UUID,
  IdentFormat::SERIAL_NUM,
  IdentFormat::UNDETERMINED   // 0. Null-terminated.
};


const IdentFormat* Identity::supportedNotions() {
  return supported_notions;
}

const char* Identity::identityTypeString(IdentFormat fmt) {
  switch (fmt) {
    #if defined(__HAS_CRYPT_WRAPPER)
    case IdentFormat::CERT_FORMAT_DER:  return "CERT";
    case IdentFormat::PK:               return "ASYM";
    case IdentFormat::PSK_SYM:          return "PSK";
    #if defined(WRAPPED_PK_OPT_SECP256R1) && defined(WRAPPED_ASYM_ECDSA)
      case IdentFormat::ONE_ID:         return "oneID";
    #endif
    #endif
    #if defined(MANUVR_OPENINTERCONNECT)
    case IdentFormat::OIC_CRED:         return "OIC_CRED";
    #endif
    case IdentFormat::UUID:             return "UUID";
    case IdentFormat::SERIAL_NUM:       return "SERIAL_NUM";
    case IdentFormat::UNDETERMINED:     return "UNDETERMINED";
  }
  return "UNDEF";
}

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
  Identity* return_value = nullptr;
  if (len > IDENTITY_BASE_PERSIST_LENGTH) {
    uint16_t ident_len = (((uint16_t) *(buf+0)) << 8) + *(buf+1);
    uint16_t ident_flg = (((uint16_t) *(buf+2)) << 8) + *(buf+3);
    IdentFormat fmt = (IdentFormat) *(buf+4);

    if (ident_len <= len) {
      len -= (IDENTITY_BASE_PERSIST_LENGTH - 1);  // Incur deficit for null-term.
      buf += (IDENTITY_BASE_PERSIST_LENGTH - 1);  // Incur deficit for null-term.

      switch (fmt) {
        case IdentFormat::UUID:
          return_value = (Identity*) new IdentityUUID(buf, (uint16_t) len);
          break;
        case IdentFormat::SERIAL_NUM:
          // TODO: Ill-conceived? Why persist a hardware serial number???
          break;
        #if defined(__HAS_CRYPT_WRAPPER)
          case IdentFormat::CERT_FORMAT_DER:
            break;
          case IdentFormat::PSK_SYM:
            break;
          case IdentFormat::PK:
            return_value = (Identity*) new IdentityPubKey(buf, (uint16_t) len);
            break;

          #if defined(WRAPPED_PK_OPT_SECP256R1) && defined(WRAPPED_ASYM_ECDSA)
            case IdentFormat::ONE_ID:
              return_value = (Identity*) new IdentityOneID(buf, (uint16_t) len);
              break;
          #endif
        #endif

        #if defined (MANUVR_OPENINTERCONNECT)
          case IdentFormat::OIC_CRED:
            break;
        #endif

        case IdentFormat::UNDETERMINED:
        default:
          break;
      }
    }
    if (return_value) return_value->_flags = ident_flg;
  }
  return return_value;
}


void Identity::staticToString(Identity* ident, StringBuilder* output) {
  output->concatf(
    "++ Identity: %s %14s  (%s) %s\n++ Acceptable for %s %s\n",
    ident->getHandle(),
    Identity::identityTypeString(ident->_format),
    (ident->isDirty() ? "Dirty" : "Persisted"),
    (ident->_ident_flag(MANUVR_IDENT_FLAG_REVOKABLE) ? "(Revokable)" : ""),
    (ident->_ident_flag(MANUVR_IDENT_FLAG_NET_ACCEPT) ? "Network" : ""),
    (ident->_ident_flag(MANUVR_IDENT_FLAG_APP_ACCEPT) ? "Policy" : "")
  );
  output->concatf("++ Validity checks pass:         %s\n", (ident->_ident_flag(MANUVR_IDENT_FLAG_VALID) ? "YES":"NO"));
  if (ident->_ident_flag(MANUVR_IDENT_FLAG_REVOKED | MANUVR_IDENT_FLAG_REVOKABLE)) {
    output->concat("++ REVOKED\n");
  }

  const char* o_str = "someone else.";
  if (ident->_ident_flag(MANUVR_IDENT_FLAG_OUR_OWN)) {
    o_str = "us.";
  }
  else if (ident->_ident_flag(MANUVR_IDENT_FLAG_LOCAL_CHAIN)) {
    o_str = "our alibi.";
  }
  if (ident->_ident_flag(MANUVR_IDENT_FLAG_3RD_PARTY_CA)) {
    o_str = "a CA.";
  }
  output->concatf("++ Belongs to %s\n", o_str);

  output->concatf(
    "++ Origin flags:    %s %s %s\n",
    (ident->_ident_flag(MANUVR_IDENT_FLAG_ORIG_PERSIST) ? "(Loaded from storage) " : ""),
    (ident->_ident_flag(MANUVR_IDENT_FLAG_ORIG_EXTER) ? "(Came from outside) " : ""),
    (ident->_ident_flag(MANUVR_IDENT_FLAG_ORIG_GEN) ? "(Generated locally) " : ""),
    (ident->_ident_flag(MANUVR_IDENT_FLAG_ORIG_PKI) ? "(Imparted by a PKI) " : ""),
    (ident->_ident_flag(MANUVR_IDENT_FLAG_ORIG_HSM) ? "(Shadowed in an HSM) " : "")
  );

  ident->toString(output);
  if (ident->_next) {
    Identity::staticToString(ident, output);
  }
  output->concat("\n");
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
  _format = _f;
  _ident_len = strlen(nom);   // TODO: Scary....
  _handle = (char*) malloc(_ident_len + 1);
  if (_handle) {
    // Also copies the required null-terminator.
    for (int i = 0; i < _ident_len+1; i++) *(_handle + i) = *(nom+i);
  }
  _ident_len += IDENTITY_BASE_PERSIST_LENGTH;
}


Identity::~Identity() {
  if (_handle) {
    free(_handle);
    _handle = nullptr;
  }
}


/*******************************************************************************
* _________ ______   _______  _       ___________________________
* \__   __/(  __  \ (  ____ \( (    /|\__   __/\__   __/\__   __/|\     /|
*    ) (   | (  \  )| (    \/|  \  ( |   ) (      ) (      ) (   ( \   / )
*    | |   | |   ) || (__    |   \ | |   | |      | |      | |    \ (_) /
*    | |   | |   | ||  __)   | (\ \) |   | |      | |      | |     \   /
*    | |   | |   ) || (      | | \   |   | |      | |      | |      ) (
* ___) (___| (__/  )| (____/\| )  \  |   | |   ___) (___   | |      | |
* \_______/(______/ (_______/|/    )_)   )_(   \_______/   )_(      \_/
* Functions to support the concept of identity.
*******************************************************************************/

/*
* Only the persistable particulars of this instance. All the base class and
*   error-checking are done upstream.
*/
int Identity::_serialize(uint8_t* buf, uint16_t len) {
  uint16_t i_flags = _flags & ~(MANUVR_IDENT_FLAG_PERSIST_MASK);
  if (len > IDENTITY_BASE_PERSIST_LENGTH) {
    *(buf+0) = (uint8_t) (_ident_len >> 8) & 0xFF;
    *(buf+1) = (uint8_t) _ident_len & 0xFF;
    *(buf+2) = (uint8_t) (i_flags >> 8) & 0xFF;
    *(buf+3) = (uint8_t) i_flags & 0xFF;
    *(buf+4) = (uint8_t) _format;
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



/*******************************************************************************
* Linked-list fxns
*******************************************************************************/
/**
* Call to get a specific identity of a given type.
* ONLY searches this chain.
*
* @param  fmt The notion of identity sought by the caller.
* @return     An instance of Identity that is of the given format.
*/
Identity* Identity::getIdentity(IdentFormat fmt) {
  if (fmt == _format) {
    return this;
  }
  else if (_next) {
    return _next->getIdentity(fmt);
  }
  else {
    return nullptr;
  }
}

/**
* Call to get a specific identity by its name.
* ONLY searches this chain.
*
* @param  fmt The notion of identity sought by the caller.
* @return     An instance of Identity that is of the given format.
*/
Identity* Identity::getIdentity(const char* nom) {
  if (nom == _handle) {  // TODO: Direct comparison won't work here.
    return this;
  }
  else if (_next) {
    return _next->getIdentity(nom);
  }
  else {
    return nullptr;
  }
}
