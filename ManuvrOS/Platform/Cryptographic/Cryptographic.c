/*
File:   Cryptographic.c
Author: J. Ian Lindsay
Date:   2016.08.13

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

#include "Cryptographic.h"

#if defined(__HAS_CRYPT_WRAPPER)


/*******************************************************************************
* Meta                                                                         *
*******************************************************************************/

/*
* Assumption: DER encoding.
* TODO: These numbers were gathered empirically and fudged upward somewhat.
*         There is likely a scientific means of arriving at the correct size.
* This should only be used to allocate scratch buffers.
*/
bool estimate_pk_size_requirements(CryptoKey k, size_t* pub, size_t* priv, uint16_t* sig) {
  switch (k) {
    #if defined(WRAPPED_ASYM_ECKEY)
      #if defined(WRAPPED_PK_OPT_SECP192R1)
        case CryptoKey::ECC_SECP192R1:
          *pub  = 76;
          *priv = 100;
          *sig  = 56;
          break;
      #endif
      #if defined(WRAPPED_PK_OPT_SECP192K1)
        case CryptoKey::ECC_SECP192K1:
          *pub  = 76;
          *priv = 96;
          *sig  = 56;
          break;
      #endif
      #if defined(WRAPPED_PK_OPT_SECP224R1)
        case CryptoKey::ECC_SECP224R1:
          *pub  = 84;
          *priv = 112;
          *sig  = 64;
          break;
      #endif
      #if defined(WRAPPED_PK_OPT_SECP224K1)
        case CryptoKey::ECC_SECP224K1:
          *pub  = 80;
          *priv = 108;
          *sig  = 64;
          break;
      #endif
      #if defined(WRAPPED_PK_OPT_SECP256R1)
        case CryptoKey::ECC_SECP256R1:
          *pub  = 92;
          *priv = 124;
          *sig  = 72;
          break;
      #endif
      #if defined(WRAPPED_PK_OPT_SECP256K1)
        case CryptoKey::ECC_SECP256K1:
          *pub  = 92;
          *priv = 124;
          *sig  = 72;
          break;
      #endif
      #if defined(WRAPPED_PK_OPT_BP256R1)
        case CryptoKey::ECC_BP256R1:
          *pub  = 96;
          *priv = 128;
          *sig  = 72;
          break;
      #endif
      #if defined(WRAPPED_PK_OPT_SECP384R1)
        case CryptoKey::ECC_SECP384R1:
          *pub  = 124;
          *priv = 172;
          *sig  = 104;
          break;
      #endif
      #if defined(WRAPPED_PK_OPT_BP384R1)
        case CryptoKey::ECC_BP384R1:
          *pub  = 128;
          *priv = 176;
          *sig  = 104;
          break;
      #endif
      #if defined(WRAPPED_PK_OPT_SECP521R1)
        case CryptoKey::ECC_SECP521R1:
          *pub  = 160;
          *priv = 224;
          *sig  = 140;
          break;
      #endif
      #if defined(WRAPPED_PK_OPT_BP512R1)
        case CryptoKey::ECC_BP512R1:
          *pub  = 160;
          *priv = 224;
          *sig  = 140;
          break;
      #endif
      #if defined(WRAPPED_PK_OPT_CURVE25519)
        case CryptoKey::ECC_CURVE25519:
          *pub  = 172;
          *priv = 240;
          *sig  = 180;
          break;
      #endif
    #endif  // WRAPPED_ASYM_ECKEY
    #if defined(WRAPPED_ASYM_RSA)
      case CryptoKey::RSA_1024:
        *pub  = 156;
        *priv = 652;
        *sig  = 128;
        break;
      case CryptoKey::RSA_2048:
        *pub  = 296;
        *priv = 1196;
        *sig  = 256;
        break;
      case CryptoKey::RSA_4096:
        *pub  = 552;
        *priv = 2352;
        *sig  = 512;
        break;
    #endif  // WRAPPED_ASYM_ECKEY
    case CryptoKey::NONE:
    default:
      return false;
  }

  return ((0 < *priv) && (0 < *pub));
}


/*******************************************************************************
* Parameter compatibility checking matricies...                                *
*******************************************************************************/
/* Privately scoped. */
const bool _is_cipher_symmetric(Cipher ci) {
  switch (ci) {
    #if defined(WRAPPED_SYM_NULL)
      case Cipher::SYM_NULL:
    #endif
    case Cipher::SYM_AES_128_ECB:
    case Cipher::SYM_AES_192_ECB:
    case Cipher::SYM_AES_256_ECB:
    case Cipher::SYM_AES_128_CBC:
    case Cipher::SYM_AES_192_CBC:
    case Cipher::SYM_AES_256_CBC:
    case Cipher::SYM_AES_128_CFB128:
    case Cipher::SYM_AES_192_CFB128:
    case Cipher::SYM_AES_256_CFB128:
    case Cipher::SYM_AES_128_CTR:
    case Cipher::SYM_AES_192_CTR:
    case Cipher::SYM_AES_256_CTR:
    case Cipher::SYM_AES_128_GCM:
    case Cipher::SYM_AES_192_GCM:
    case Cipher::SYM_AES_256_GCM:
    case Cipher::SYM_AES_128_CCM:
    case Cipher::SYM_AES_192_CCM:
    case Cipher::SYM_AES_256_CCM:
    case Cipher::SYM_CAMELLIA_128_ECB:
    case Cipher::SYM_CAMELLIA_192_ECB:
    case Cipher::SYM_CAMELLIA_256_ECB:
    case Cipher::SYM_CAMELLIA_128_CBC:
    case Cipher::SYM_CAMELLIA_192_CBC:
    case Cipher::SYM_CAMELLIA_256_CBC:
    case Cipher::SYM_CAMELLIA_128_CFB128:
    case Cipher::SYM_CAMELLIA_192_CFB128:
    case Cipher::SYM_CAMELLIA_256_CFB128:
    case Cipher::SYM_CAMELLIA_128_CTR:
    case Cipher::SYM_CAMELLIA_192_CTR:
    case Cipher::SYM_CAMELLIA_256_CTR:
    case Cipher::SYM_CAMELLIA_128_GCM:
    case Cipher::SYM_CAMELLIA_192_GCM:
    case Cipher::SYM_CAMELLIA_256_GCM:
    case Cipher::SYM_DES_ECB:
    case Cipher::SYM_DES_CBC:
    case Cipher::SYM_DES_EDE_ECB:
    case Cipher::SYM_DES_EDE_CBC:
    case Cipher::SYM_DES_EDE3_ECB:
    case Cipher::SYM_DES_EDE3_CBC:
    case Cipher::SYM_BLOWFISH_ECB:
    case Cipher::SYM_BLOWFISH_CBC:
    case Cipher::SYM_BLOWFISH_CFB64:
    case Cipher::SYM_BLOWFISH_CTR:
    case Cipher::SYM_ARC4_128:
    case Cipher::SYM_CAMELLIA_128_CCM:
    case Cipher::SYM_CAMELLIA_192_CCM:
    case Cipher::SYM_CAMELLIA_256_CCM:
    case Cipher::SYM_NONE:
      return true;
    default:
      return false;
  }
}

/* Privately scoped. */
const bool _is_cipher_authenticated(Cipher ci) {
  switch (ci) {
    #if defined(WRAPPED_SYM_AES_128_GCM)
      case Cipher::SYM_AES_128_GCM:           return true;
    #endif
    #if defined(WRAPPED_SYM_AES_192_GCM)
      case Cipher::SYM_AES_192_GCM:           return true;
    #endif
    #if defined(WRAPPED_SYM_AES_256_GCM)
      case Cipher::SYM_AES_256_GCM:           return true;
    #endif
    #if defined(WRAPPED_SYM_AES_128_CCM)
      case Cipher::SYM_AES_128_CCM:           return true;
    #endif
    #if defined(WRAPPED_SYM_AES_192_CCM)
      case Cipher::SYM_AES_192_CCM:           return true;
    #endif
    #if defined(WRAPPED_SYM_AES_256_CCM)
      case Cipher::SYM_AES_256_CCM:           return true;
    #endif

    #if defined(WRAPPED_SYM_CAMELLIA_128_GCM)
      case Cipher::SYM_CAMELLIA_128_GCM:      return true;
    #endif
    #if defined(WRAPPED_SYM_CAMELLIA_192_GCM)
      case Cipher::SYM_CAMELLIA_192_GCM:      return true;
    #endif
    #if defined(WRAPPED_SYM_CAMELLIA_256_GCM)
      case Cipher::SYM_CAMELLIA_256_GCM:      return true;
    #endif
    #if defined(WRAPPED_SYM_CAMELLIA_128_CCM)
      case Cipher::SYM_CAMELLIA_128_CCM:      return true;
    #endif
    #if defined(WRAPPED_SYM_CAMELLIA_192_CCM)
      case Cipher::SYM_CAMELLIA_192_CCM:      return true;
    #endif
    #if defined(WRAPPED_SYM_CAMELLIA_256_CCM)
      case Cipher::SYM_CAMELLIA_256_CCM:      return true;
    #endif

    default:
      return false;
  }
}

/* Privately scoped. */
const bool _is_cipher_asymmetric(Cipher ci) {
  switch (ci) {
    case Cipher::ASYM_ECKEY:
    case Cipher::ASYM_ECKEY_DH:
    case Cipher::ASYM_ECDSA:
    case Cipher::ASYM_RSA:
    case Cipher::ASYM_RSA_ALT:
    case Cipher::ASYM_RSASSA_PSS:
      return true;
    default:
      return false;
  }
}

/* Privately scoped. */
const bool _valid_cipher_params(Cipher ci) {
  switch (ci) {
    case Cipher::ASYM_ECKEY:
    case Cipher::ASYM_ECKEY_DH:
    case Cipher::ASYM_ECDSA:
    case Cipher::ASYM_RSA:
    case Cipher::ASYM_RSA_ALT:
    case Cipher::ASYM_RSASSA_PSS:
      return true;
    default:
      return false;
  }
}


/*******************************************************************************
* Pluggable crypto modules...                                                  *
*******************************************************************************/

/**
* Tests for an implementation-specific deferral for the given cipher.
*
* @param Cipher The cipher to test for deferral.
* @return true if the root function ought to defer.
*/
bool cipher_deferred_handling(Cipher ci) {
  // TODO: Slow. Ugly.
  return (_sym_overrides[ci] || _sauth_overrides[ci]);
}

/**
* Tests for an implementation-specific deferral for the given hash.
*
* @param Hashes The hash to test for deferral.
* @return true if the root function ought to defer.
*/
bool digest_deferred_handling(Hashes h) {
  return ((bool)(_hash_overrides[h]));
}


/**
* Tests for an implementation-specific deferral for the given sign/verify algorithm.
*
* @param CryptoKey The key type to test for deferral.
* @return true if the root function ought to defer.
*/
bool sign_verify_deferred_handling(CryptoKey k) {
  return ((bool)(_s_v_overrides[k]));
}


/**
* Tests for an implementation-specific deferral for key generation using the given algorithm.
*
* @param CryptoKey The key type to test for deferral.
* @return true if the root function ought to defer.
*/
bool keygen_deferred_handling(CryptoKey k) {
  return ((bool)(_keygen_overrides[k]));
}



bool provide_cipher_handler(Cipher c, wrapped_sym_operation fxn) {
  if (!cipher_deferred_handling(c)) {
    _sym_overrides[c] = fxn;
    return true;
  }
  return false;
}


bool provide_digest_handler(Hashes h, wrapped_hash_operation fxn) {
  if (!digest_deferred_handling(h)) {
    _hash_overrides[h] = fxn;
    return true;
  }
  return false;
}


bool provide_sign_verify_handler(CryptoKey k, wrapped_sv_operation fxn) {
  if (!sign_verify_deferred_handling(k)) {
    _s_v_overrides[k] = fxn;
    return true;
  }
  return false;
}


bool provide_keygen_handler(CryptoKey k, wrapped_keygen_operation fxn) {
  if (!keygen_deferred_handling(k)) {
    _keygen_overrides[k] = fxn;
    return true;
  }
  return false;
}



#endif  // __HAS_CRYPT_WRAPPER
