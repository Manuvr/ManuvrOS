/*
File:   IdentityCrypto.cpp
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


Cryptographically-backed identities.
*/

#include <Platform/Cryptographic.h>

#if defined(__HAS_CRYPT_WRAPPER)
#include <Platform/Identity.h>
#include <alloca.h>


IdentityPubKey::IdentityPubKey(const char* nom, Cipher c, CryptoKey k) : Identity(nom, IdentFormat::PSK_ASYM) {
  size_t tmp_public_len  = 0;
  size_t tmp_privat_len  = 0;
  size_t tmp_sig_len     = 0;
  _key_type = k;
  if (estimate_pk_size_requirements(k, &tmp_public_len, &tmp_privat_len, &tmp_sig_len)) {
    uint8_t* tmp_pub = (uint8_t*) alloca(tmp_public_len);
    uint8_t* tmp_prv = (uint8_t*) alloca(tmp_privat_len);

    int ret = wrapped_asym_keygen(c, k, tmp_pub, &tmp_public_len, tmp_prv, &tmp_privat_len);
    if (0 == ret) {
      _pub  = (uint8_t*) malloc(tmp_public_len);
      _priv = (uint8_t*) malloc(tmp_privat_len);
      if ((nullptr != _priv) & (nullptr != _pub)) {
        _pub_size  = (uint16_t) (0xFFFF & tmp_public_len);
        _priv_size = (uint16_t) (0xFFFF & tmp_privat_len);
        memcpy(_pub,  tmp_pub, _pub_size);
        memcpy(_priv, tmp_prv, _priv_size);
        _ident_set_flag(true, MANUVR_IDENT_FLAG_DIRTY | MANUVR_IDENT_FLAG_ORIG_GEN);
        _ident_len += sizeof(CryptoKey) + sizeof(Cipher) + _pub_size + _priv_size;
      }
    }
  }
}


IdentityPubKey::IdentityPubKey(uint8_t* buf, uint16_t len) : Identity((const char*) buf, IdentFormat::PSK_ASYM) {
  const int offset = _ident_len - IDENTITY_BASE_PERSIST_LENGTH + 1;
  if (len >= 17) {
    // If there are at least 16 non-string bytes left in the buffer...
    //for (int i = 0; i < 16; i++) *(((uint8_t*)&uuid.id) + i) = *(buf + i + offset);
  }
  _ident_len += 16;
}


IdentityPubKey::~IdentityPubKey() {
}


void IdentityPubKey::toString(StringBuilder* output) {
  Hashes* biggest_hash = list_supported_digests();
  size_t hash_len = get_digest_output_length(*biggest_hash);
  uint8_t* hash = (uint8_t*) alloca(hash_len);

  if (0 == wrapped_hash((uint8_t*) _pub, _pub_size, hash, *biggest_hash)) {
    randomArt(hash, hash_len, get_pk_label(_key_type), output);
  }
}


/*
* Only the persistable particulars of this instance. All the base class and
*   error-checking are done upstream.
*/
int IdentityPubKey::serialize(uint8_t* buf, uint16_t len) {
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


#endif  // __HAS_CRYPT_WRAPPER
