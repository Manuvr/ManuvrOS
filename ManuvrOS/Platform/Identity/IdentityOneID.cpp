/*
File:   IdentityOneID.cpp
Author: J. Ian Lindsay
Date:   2016.10.04

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


https://tools.ietf.org/html/rfc7515
*/

#include <Platform/Cryptographic.h>

#if defined(WRAPPED_PK_OPT_SECP256R1) && defined(WRAPPED_ASYM_ECDSA)
// OneID depends on this curve and this cipher.

#include <Platform/Identity.h>

#define ONEID_CONSTANT_FEATURES MANUVR_IDENT_FLAG_ORIG_PKI | \
                                MANUVR_IDENT_FLAG_REVOKABLE | \
                                MANUVR_IDENT_FLAG_NET_ACCEPT | \
                                MANUVR_IDENT_FLAG_APP_ACCEPT

const size_t IdentityOneID::_SERIALIZED_LEN = ONEID_PUB_KEY_MAX_LEN*3;


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

IdentityOneID::IdentityOneID(const char* nom) : IdentityPubKey(nom, Cipher::ASYM_ECDSA, CryptoKey::ECC_SECP256R1, Hashes::SHA256) {
  _clobber_format(IdentFormat::ONE_ID);
  _ident_len += _SERIALIZED_LEN;
  _ident_set_flag(true, ONEID_CONSTANT_FEATURES | MANUVR_IDENT_FLAG_DIRTY);
}


IdentityOneID::IdentityOneID(const char* nom, ProvBundle* prov) : IdentityPubKey(nom, Cipher::ASYM_ECDSA, CryptoKey::ECC_SECP256R1, Hashes::SHA256) {
  _clobber_format(IdentFormat::ONE_ID);
  _ident_len += _SERIALIZED_LEN;
  _ident_set_flag(true, ONEID_CONSTANT_FEATURES | MANUVR_IDENT_FLAG_DIRTY);
}


IdentityOneID::IdentityOneID(uint8_t* buf, uint16_t len) : IdentityPubKey(buf, len) {
  int offset = _ident_len-5;
  if ((len - offset) >= _SERIALIZED_LEN) {
    memcpy((uint8_t*) &_oneid_project_key, (buf + offset), ONEID_PUB_KEY_MAX_LEN);
    offset += ONEID_PUB_KEY_MAX_LEN;
    memcpy((uint8_t*) &_oneid_server_key, (buf + offset), ONEID_PUB_KEY_MAX_LEN);
    offset += ONEID_PUB_KEY_MAX_LEN;
    memcpy((uint8_t*) &_oneid_reset_key, (buf + offset), ONEID_PUB_KEY_MAX_LEN);
    offset += ONEID_PUB_KEY_MAX_LEN;
  }
  _ident_len += _SERIALIZED_LEN;
}


IdentityOneID::~IdentityOneID() {
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

void IdentityOneID::toString(StringBuilder* output) {
  IdentityPubKey::toString(output);
}


/*
* Only the persistable particulars of this instance. All the base class and
*   error-checking are done upstream.
*/
int IdentityOneID::serialize(uint8_t* buf, uint16_t len) {
  int offset = IdentityPubKey::serialize(buf, len);
  if ((len - offset) >= _SERIALIZED_LEN) {
    memcpy((buf + offset), (uint8_t*) &_oneid_project_key, ONEID_PUB_KEY_MAX_LEN);
    offset += ONEID_PUB_KEY_MAX_LEN;
    memcpy((buf + offset), (uint8_t*) &_oneid_server_key, ONEID_PUB_KEY_MAX_LEN);
    offset += ONEID_PUB_KEY_MAX_LEN;
    memcpy((buf + offset), (uint8_t*) &_oneid_reset_key, ONEID_PUB_KEY_MAX_LEN);
    offset += ONEID_PUB_KEY_MAX_LEN;
    return offset;
  }
  return 0;
}


/*******************************************************************************
* Cryptographic-layer functions...
*******************************************************************************/

int8_t IdentityOneID::verify(uint8_t* in, size_t in_len, uint8_t* out, size_t* out_len) {
  return -1;
}



#endif  // WRAPPED_PK_OPT_SECP256R1 && WRAPPED_ASYM_ECDSA
