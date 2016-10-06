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


IdentityOneID::IdentityOneID(const char* nom) : Identity(nom, IdentFormat::ONE_ID) {
  _ident_set_flag(true, MANUVR_IDENT_FLAG_DIRTY | MANUVR_IDENT_FLAG_ORIG_GEN);
  size_t tmp_public_len  = 0;
  size_t tmp_privat_len  = 0;
  size_t tmp_sig_len     = 0;
  if (estimate_pk_size_requirements(_key_type, &tmp_public_len, &tmp_privat_len, &tmp_sig_len)) {
    uint8_t* tmp_pub = (uint8_t*) alloca(tmp_public_len);
    uint8_t* tmp_prv = (uint8_t*) alloca(tmp_privat_len);

    int ret = wrapped_asym_keygen(_cipher, _key_type, tmp_pub, &tmp_public_len, tmp_prv, &tmp_privat_len);
    if (0 == ret) {
      _pub  = (uint8_t*) malloc(tmp_public_len);
      _priv = (uint8_t*) malloc(tmp_privat_len);
      if ((nullptr != _priv) & (nullptr != _pub)) {
        _pub_size  = (uint16_t) (0xFFFF & tmp_public_len);
        _priv_size = (uint16_t) (0xFFFF & tmp_privat_len);
        memcpy(_pub,  tmp_pub, _pub_size);
        memcpy(_priv, tmp_prv, _priv_size);
        _ident_len += sizeof(CryptoKey) + sizeof(Cipher) + _pub_size + _priv_size;
        _ident_set_flag(true, MANUVR_IDENT_FLAG_VALID);
      }
    }
    else {
      // TODO: Need logging for failures?
      //char* err_buf[128] = {0};
      //crypt_error_string(ret, (char*)&err_buf, 128);
      //StringBuilder log("Failure to generate: ");
      //log.concat((char*)&err_buf);
      //printf((const char*) log.string());
    }
  }
}


IdentityOneID::IdentityOneID(uint8_t* buf, uint16_t len) : Identity((const char*) buf, IdentFormat::ONE_ID) {
  const int offset = _ident_len - IDENTITY_BASE_PERSIST_LENGTH + 1;
  if (len >= 17) {
    // If there are at least 16 non-string bytes left in the buffer...
    //for (int i = 0; i < 16; i++) *(((uint8_t*)&uuid.id) + i) = *(buf + i + offset);
  }
  _ident_len += 16;
}


IdentityOneID::~IdentityOneID() {
}


/*
* We shouldn't trust that keypairs loaded from any given source are *actually pairs*.
*   This function will test them against one another to ensure that they are truely paired.
* This would be the proper place to do things like key-pinning.
* Note that this is not a "go/no-go" situation. We might see keys that the platform owns,
*  and therefore only have the public component (despite it possibly being a self-identity).
*  Communication with the cryptographic and platform layers may be required to give a
*  sensible answer.
*
* @return 0 if the intentity sanity-checks, negative on error, and positive if trusted.
*/
int8_t IdentityOneID::sanity_check() {
  if (_ident_flag(MANUVR_IDENT_FLAG_VALID)) {
    // TODO: Actually test signing? Check for HSM ownership?
    return 0;
  }
  return -1;
}


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
