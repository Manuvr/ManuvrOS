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

/* This is the fixed size of this class of identity. */
const size_t IdentityPubKey::_SERIALIZED_LEN = sizeof(CryptoKey) + sizeof(Cipher) + sizeof(Hashes) + 4;


/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

IdentityPubKey::IdentityPubKey(const char* nom, Cipher c, CryptoKey k) : IdentityPubKey(nom, c, k, Hashes::NONE) {
}


IdentityPubKey::IdentityPubKey(const char* nom, Cipher c, CryptoKey k, Hashes h) : Identity(nom, IdentFormat::PK) {
  _digest = (Hashes::NONE != h) ? h : *(list_supported_digests());
  _ident_set_flag(true, MANUVR_IDENT_FLAG_DIRTY | MANUVR_IDENT_FLAG_ORIG_GEN);
  size_t tmp_public_len  = 0;
  size_t tmp_privat_len  = 0;
  _key_type = k;
  if (estimate_pk_size_requirements(k, &tmp_public_len, &tmp_privat_len, &_sig_size)) {
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
        _ident_len += _SERIALIZED_LEN + _pub_size + _priv_size;
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


IdentityPubKey::IdentityPubKey(uint8_t* buf, uint16_t len) : Identity((const char*) buf, IdentFormat::PK) {
  int offset = _ident_len-5;
  if ((len - offset) >= _SERIALIZED_LEN) {
    memcpy((uint8_t*) &_pub_size, (buf + offset), sizeof(_pub_size));
    offset += sizeof(_pub_size);
    memcpy((uint8_t*) &_priv_size, (buf + offset), sizeof(_priv_size));
    offset += sizeof(_priv_size);
    memcpy((uint8_t*) &_key_type, (buf + offset), sizeof(_key_type));
    offset += sizeof(_key_type);
    memcpy((uint8_t*) &_cipher, (buf + offset), sizeof(_cipher));
    offset += sizeof(_cipher);
    memcpy((uint8_t*) &_digest, (buf + offset), sizeof(_digest));
    offset += sizeof(_digest);

    if (_pub_size) {
      _pub  = (uint8_t*) malloc(_pub_size);
      if (_pub) {
        memcpy(_pub, (buf + offset), _pub_size);
        offset += _pub_size;
      }
    }
    if (_priv_size) {
      _priv = (uint8_t*) malloc(_priv_size);
      if (_priv) {
        memcpy(_priv, (buf + offset), _priv_size);
        offset += _priv_size;
      }
    }
    _ident_len += _SERIALIZED_LEN + _pub_size + _priv_size;
  }
}


IdentityPubKey::~IdentityPubKey() {
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
void IdentityPubKey::toString(StringBuilder* output) {
  size_t hash_len = get_digest_output_length(_digest);
  uint8_t* hash = (uint8_t*) alloca(hash_len);

  if (_pub_size) {
    if (0 == wrapped_hash((uint8_t*) _pub, _pub_size, hash, _digest)) {
      randomArt(hash, hash_len, get_pk_label(_key_type), output);
    }
  }
}


/*
* Only the persistable particulars of this instance. All the base class and
*   error-checking are done upstream.
*/
int IdentityPubKey::serialize(uint8_t* buf, uint16_t len) {
  int offset = Identity::_serialize(buf, len);
  if ((len - offset) >= (_SERIALIZED_LEN + _pub_size + _priv_size)) {
    memcpy((buf + offset), (uint8_t*) &_pub_size, sizeof(_pub_size));
    offset += sizeof(_pub_size);
    memcpy((buf + offset), (uint8_t*) &_priv_size, sizeof(_priv_size));
    offset += sizeof(_priv_size);
    memcpy((buf + offset), (uint8_t*) &_key_type, sizeof(_key_type));
    offset += sizeof(_key_type);
    memcpy((buf + offset), (uint8_t*) &_cipher, sizeof(_cipher));
    offset += sizeof(_cipher);
    memcpy((buf + offset), (uint8_t*) &_digest, sizeof(_digest));
    offset += sizeof(_digest);

    if (_pub_size) {
      memcpy((buf + offset), _pub, _pub_size);
      offset += _pub_size;
    }
    if (_priv_size) {
      memcpy((buf + offset), _priv, _priv_size);
      offset += _priv_size;
    }
    return offset;
  }
  return 0;
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
int8_t IdentityPubKey::sanity_check() {
  if (_ident_flag(MANUVR_IDENT_FLAG_VALID)) {
    // TODO: Actually test signing? Check for HSM ownership?
    return 0;
  }
  return -1;
}


int8_t IdentityPubKey::sign(uint8_t* in, size_t in_len, uint8_t* out, size_t* out_len) {
  return -1;
}


int8_t IdentityPubKey::verify(uint8_t* in, size_t in_len, uint8_t* out, size_t* out_len) {
  return -1;
}




#endif  // __HAS_CRYPT_WRAPPER
