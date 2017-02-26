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

#include "../Cryptographic.h"

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
#if defined(__BUILD_HAS_SYMMETRIC) || defined(__BUILD_HAS_ASYMMETRIC)
bool _is_cipher_symmetric(Cipher ci) {
  switch (ci) {
    #if defined(WRAPPED_SYM_NULL)
      case Cipher::SYM_NULL:             return true;
    #endif
    #if defined(WRAPPED_SYM_AES_128_GCM)
      case Cipher::SYM_AES_128_GCM:      return true;
    #endif
    #if defined(WRAPPED_SYM_AES_192_GCM)
      case Cipher::SYM_AES_192_GCM:      return true;
    #endif
    #if defined(WRAPPED_SYM_AES_256_GCM)
      case Cipher::SYM_AES_256_GCM:      return true;
    #endif
    #if defined(WRAPPED_SYM_AES_128_CCM)
      case Cipher::SYM_AES_128_CCM:      return true;
    #endif
    #if defined(WRAPPED_SYM_AES_192_CCM)
      case Cipher::SYM_AES_192_CCM:      return true;
    #endif
    #if defined(WRAPPED_SYM_AES_256_CCM)
      case Cipher::SYM_AES_256_CCM:      return true;
    #endif
    #if defined(WRAPPED_SYM_AES_128_ECB)
      case Cipher::SYM_AES_128_ECB:      return true;
    #endif
    #if defined(WRAPPED_SYM_AES_192_ECB)
      case Cipher::SYM_AES_192_ECB:      return true;
    #endif
    #if defined(WRAPPED_SYM_AES_256_ECB)
      case Cipher::SYM_AES_256_ECB:      return true;
    #endif
    #if defined(WRAPPED_SYM_AES_128_CBC)
      case Cipher::SYM_AES_128_CBC:      return true;
    #endif
    #if defined(WRAPPED_SYM_AES_192_CBC)
      case Cipher::SYM_AES_192_CBC:      return true;
    #endif
    #if defined(WRAPPED_SYM_AES_256_CBC)
      case Cipher::SYM_AES_256_CBC:      return true;
    #endif
    #if defined(WRAPPED_SYM_AES_128_CFB128)
      case Cipher::SYM_AES_128_CFB128:   return true;
    #endif
    #if defined(WRAPPED_SYM_AES_192_CFB128)
      case Cipher::SYM_AES_192_CFB128:   return true;
    #endif
    #if defined(WRAPPED_SYM_AES_256_CFB128)
      case Cipher::SYM_AES_256_CFB128:   return true;
    #endif
    #if defined(WRAPPED_SYM_AES_128_CTR)
      case Cipher::SYM_AES_128_CTR:      return true;
    #endif
    #if defined(WRAPPED_SYM_AES_192_CTR)
      case Cipher::SYM_AES_192_CTR:      return true;
    #endif
    #if defined(WRAPPED_SYM_AES_256_CTR)
      case Cipher::SYM_AES_256_CTR:      return true;
    #endif
    #if defined(WRAPPED_SYM_CAMELLIA_128_GCM)
      case Cipher::SYM_CAMELLIA_128_GCM: return true;
    #endif
    #if defined(WRAPPED_SYM_CAMELLIA_192_GCM)
      case Cipher::SYM_CAMELLIA_192_GCM: return true;
    #endif
    #if defined(WRAPPED_SYM_CAMELLIA_256_GCM)
      case Cipher::SYM_CAMELLIA_256_GCM: return true;
    #endif
    #if defined(WRAPPED_SYM_CAMELLIA_128_CCM)
      case Cipher::SYM_CAMELLIA_128_CCM: return true;
    #endif
    #if defined(WRAPPED_SYM_CAMELLIA_192_CCM)
      case Cipher::SYM_CAMELLIA_192_CCM: return true;
    #endif
    #if defined(WRAPPED_SYM_CAMELLIA_256_CCM)
      case Cipher::SYM_CAMELLIA_256_CCM: return true;
    #endif
    #if defined(WRAPPED_SYM_CAMELLIA_128_ECB)
      case Cipher::SYM_CAMELLIA_128_ECB: return true;
    #endif
    #if defined(WRAPPED_SYM_CAMELLIA_192_ECB)
      case Cipher::SYM_CAMELLIA_192_ECB: return true;
    #endif
    #if defined(WRAPPED_SYM_CAMELLIA_256_ECB)
      case Cipher::SYM_CAMELLIA_256_ECB: return true;
    #endif
    #if defined(WRAPPED_SYM_CAMELLIA_128_CBC)
      case Cipher::SYM_CAMELLIA_128_CBC: return true;
    #endif
    #if defined(WRAPPED_SYM_CAMELLIA_192_CBC)
      case Cipher::SYM_CAMELLIA_192_CBC: return true;
    #endif
    #if defined(WRAPPED_SYM_CAMELLIA_256_CBC)
      case Cipher::SYM_CAMELLIA_256_CBC: return true;
    #endif
    #if defined(WRAPPED_SYM_CAMELLIA_128_CFB128)
      case Cipher::SYM_CAMELLIA_128_CFB128:      return true;
    #endif
    #if defined(WRAPPED_SYM_CAMELLIA_192_CFB128)
      case Cipher::SYM_CAMELLIA_192_CFB128:      return true;
    #endif
    #if defined(WRAPPED_SYM_CAMELLIA_256_CFB128)
      case Cipher::SYM_CAMELLIA_256_CFB128:      return true;
    #endif
    #if defined(WRAPPED_SYM_CAMELLIA_128_CTR)
      case Cipher::SYM_CAMELLIA_128_CTR:      return true;
    #endif
    #if defined(WRAPPED_SYM_CAMELLIA_192_CTR)
      case Cipher::SYM_CAMELLIA_192_CTR:      return true;
    #endif
    #if defined(WRAPPED_SYM_CAMELLIA_256_CTR)
      case Cipher::SYM_CAMELLIA_256_CTR:      return true;
    #endif
    #if defined(WRAPPED_SYM_DES_ECB)
      case Cipher::SYM_DES_ECB:          return true;
    #endif
    #if defined(WRAPPED_SYM_DES_CBC)
      case Cipher::SYM_DES_CBC:          return true;
    #endif
    #if defined(WRAPPED_SYM_DES_EDE_ECB)
      case Cipher::SYM_DES_EDE_ECB:      return true;
    #endif
    #if defined(WRAPPED_SYM_DES_EDE_CBC)
      case Cipher::SYM_DES_EDE_CBC:      return true;
    #endif
    #if defined(WRAPPED_SYM_DES_EDE3_ECB)
      case Cipher::SYM_DES_EDE3_ECB:     return true;
    #endif
    #if defined(WRAPPED_SYM_DES_EDE3_CBC)
      case Cipher::SYM_DES_EDE3_CBC:     return true;
    #endif
    #if defined(WRAPPED_SYM_BLOWFISH_ECB)
      case Cipher::SYM_BLOWFISH_ECB:     return true;
    #endif
    #if defined(WRAPPED_SYM_BLOWFISH_CBC)
      case Cipher::SYM_BLOWFISH_CBC:     return true;
    #endif
    #if defined(WRAPPED_SYM_BLOWFISH_CFB64)
      case Cipher::SYM_BLOWFISH_CFB64:   return true;
    #endif
    #if defined(WRAPPED_SYM_BLOWFISH_CTR)
      case Cipher::SYM_BLOWFISH_CTR:     return true;
    #endif
    #if defined(WRAPPED_SYM_ARC4_128)
      case Cipher::SYM_ARC4_128:         return true;
    #endif
    #if defined(WRAPPED_SYM_NONE)
      case Cipher::SYM_NONE:             return true;
    #endif
    default:  return false;
  }
}
#endif

/* Privately scoped. */
bool _is_cipher_authenticated(Cipher ci) {
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
bool _is_cipher_asymmetric(Cipher ci) {
  switch (ci) {
    #if defined(WRAPPED_ASYM_ECKEY)
      case Cipher::ASYM_ECKEY:        return true;
    #endif
    #if defined(WRAPPED_ASYM_ECKEY_DH)
      case Cipher::ASYM_ECKEY_DH:     return true;
    #endif
    #if defined(WRAPPED_ASYM_ECDSA)
      case Cipher::ASYM_ECDSA:        return true;
    #endif
    #if defined(WRAPPED_ASYM_RSA)
      case Cipher::ASYM_RSA:          return true;
    #endif
    #if defined(WRAPPED_ASYM_RSA_ALT)
      case Cipher::ASYM_RSA_ALT:      return true;
    #endif
    #if defined(WRAPPED_ASYM_RSASSA_PSS)
      case Cipher::ASYM_RSASSA_PSS:   return true;
    #endif
    default:                          return false;
  }
}

/* Privately scoped. */
bool _valid_cipher_params(Cipher ci) {
  switch (ci) {
    #if defined(WRAPPED_ASYM_ECKEY)
      case Cipher::ASYM_ECKEY:        return true;
    #endif
    #if defined(WRAPPED_ASYM_ECKEY_DH)
      case Cipher::ASYM_ECKEY_DH:     return true;
    #endif
    #if defined(WRAPPED_ASYM_ECDSA)
      case Cipher::ASYM_ECDSA:        return true;
    #endif
    #if defined(WRAPPED_ASYM_RSA)
      case Cipher::ASYM_RSA:          return true;
    #endif
    #if defined(WRAPPED_ASYM_RSA_ALT)
      case Cipher::ASYM_RSA_ALT:      return true;
    #endif
    #if defined(WRAPPED_ASYM_RSASSA_PSS)
      case Cipher::ASYM_RSASSA_PSS:   return true;
    #endif
    default:                          return false;
  }
}


/*******************************************************************************
* Pluggable crypto modules...                                                  *
*******************************************************************************/

/**
* Tests for an implementation-specific deferral for the given cipher.
*
* @param enum Cipher The cipher to test for deferral.
* @return true if the root function ought to defer.
*/
bool cipher_deferred_handling(enum Cipher ci) {
  #if defined(WITH_BLIND_CRYPTO)
  // TODO: Slow. Ugly.
  return (_sym_overrides[ci] || _sauth_overrides[ci] || _asym_overrides[ci]);
  #else
  return false;
  #endif
}

/**
* Tests for an implementation-specific deferral for the given hash.
*
* @param enum Hashes The hash to test for deferral.
* @return true if the root function ought to defer.
*/
bool digest_deferred_handling(enum Hashes h) {
  #if defined(WITH_BLIND_CRYPTO)
  return ((bool)(_hash_overrides[h]));
  #else
  return false;
  #endif
}


/**
* Tests for an implementation-specific deferral for the given sign/verify algorithm.
*
* @param enum CryptoKey The key type to test for deferral.
* @return true if the root function ought to defer.
*/
bool sign_verify_deferred_handling(enum CryptoKey k) {
  #if defined(WITH_BLIND_CRYPTO)
  return ((bool)(_s_v_overrides[k]));
  #else
  return false;
  #endif
}


/**
* Tests for an implementation-specific deferral for key generation using the given algorithm.
*
* @param enum CryptoKey The key type to test for deferral.
* @return true if the root function ought to defer.
*/
bool keygen_deferred_handling(enum CryptoKey k) {
  #if defined(WITH_BLIND_CRYPTO)
  return ((bool)(_keygen_overrides[k]));
  #else
  return false;
  #endif
}



bool provide_cipher_handler(enum Cipher c, wrapped_sym_operation fxn) {
  #if defined(WITH_BLIND_CRYPTO)
  if (!cipher_deferred_handling(c)) {
    _sym_overrides[c] = fxn;
    return true;
  }
  #endif
  return false;
}


bool provide_digest_handler(enum Hashes h, wrapped_hash_operation fxn) {
  #if defined(WITH_BLIND_CRYPTO)
  if (!digest_deferred_handling(h)) {
    _hash_overrides[h] = fxn;
    return true;
  }
  #endif
  return false;
}


bool provide_sign_verify_handler(enum CryptoKey k, wrapped_sv_operation fxn) {
  #if defined(WITH_BLIND_CRYPTO)
  if (!sign_verify_deferred_handling(k)) {
    _s_v_overrides[k] = fxn;
    return true;
  }
  #endif
  return false;
}


bool provide_keygen_handler(enum CryptoKey k, wrapped_keygen_operation fxn) {
  #if defined(WITH_BLIND_CRYPTO)
  if (!keygen_deferred_handling(k)) {
    _keygen_overrides[k] = fxn;
    return true;
  }
  #endif
  return false;
}

#endif  // __HAS_CRYPT_WRAPPER
