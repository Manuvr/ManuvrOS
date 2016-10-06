/*
File:   IdentityOneID.cpp
Author: J. Ian Lindsay
Date:   2016.10.04

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


*/

#include <Platform/Cryptographic.h>

#if defined(WRAPPED_PK_OPT_SECP256R1) && defined(WRAPPED_ASYM_ECDSA)
// OneID depends on this curve and this cipher.

#include <Platform/Identity.h>

//private_log_shunt

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
}


IdentityOneID::IdentityOneID(uint8_t* buf, uint16_t len) : IdentityPubKey(buf, len) {
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
  Hashes* biggest_hash = list_supported_digests();
  size_t hash_len = get_digest_output_length(*biggest_hash);
  uint8_t* hash = (uint8_t*) alloca(hash_len);

  if (_pub_size) {
    if (0 == wrapped_hash((uint8_t*) _pub, _pub_size, hash, *biggest_hash)) {
      randomArt(hash, hash_len, get_pk_label(_key_type), output);
    }
  }
}


/*
* Only the persistable particulars of this instance. All the base class and
*   error-checking are done upstream.
*/
int IdentityOneID::serialize(uint8_t* buf, uint16_t len) {
  int offset = Identity::_serialize(buf, len);
  memcpy((buf + offset), (uint8_t*) &_pub_size, sizeof(_pub_size));
  offset += sizeof(_pub_size);
  memcpy((buf + offset), (uint8_t*) &_priv_size, sizeof(_priv_size));
  offset += sizeof(_priv_size);
  memcpy((buf + offset), (uint8_t*) &_key_type, sizeof(_key_type));
  offset += sizeof(_key_type);
  memcpy((buf + offset), (uint8_t*) &_cipher, sizeof(_cipher));
  offset += sizeof(_cipher);

  if (_pub_size) {
    memcpy((buf + offset), _pub, _pub_size);
    offset += _pub_size;
  }
  if (_priv_size) {
    memcpy((buf + offset), _priv, _priv_size);
    offset += _priv_size;
  }
  return offset + 16;
}


#endif  // WRAPPED_PK_OPT_SECP256R1 && WRAPPED_ASYM_ECDSA
