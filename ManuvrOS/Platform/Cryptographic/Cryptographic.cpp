/*
File:   Cryptographic.cpp
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
#include <Kernel.h>


#if defined(__HAS_CRYPT_WRAPPER)


/*******************************************************************************
* Meta                                                                         *
*******************************************************************************/



/*******************************************************************************
* String lookup and debug...                                                   *
*******************************************************************************/

/**
* Prints details about the cryptographic situation on the platform.
*
* @param  StringBuilder* The buffer to output into.
*/
void printCryptoOverview(StringBuilder* out) {
  #if defined(WITH_MBEDTLS)
    out->concatf("-- Cryptographic support via %s.\n", __CRYPTO_BACKEND);
    out->concat("-- Supported TLS ciphersuites:");
    int idx = 0;
    const int* cs_list = mbedtls_ssl_list_ciphersuites();
    while (0 != *(cs_list)) {
      if (0 == idx++ % 2) out->concat("\n--\t");
      out->concatf("\t%-40s", mbedtls_ssl_get_ciphersuite_name(*(cs_list++)));
    }
    out->concat("\n-- Supported ciphers:");
    idx = 0;
    Cipher* list = list_supported_ciphers();
    while (Cipher::NONE != *(list)) {
      if (0 == idx++ % 4) out->concat("\n--\t");
      out->concatf("\t%-20s", get_cipher_label((Cipher) *(list++)));
    }

    out->concat("\n-- Supported ECC curves:");
    const mbedtls_ecp_curve_info* c_list = mbedtls_ecp_curve_list();
    idx = 0;
    while (c_list[idx].name) {
      if (0 == idx % 4) out->concat("\n--\t");
      out->concatf("\t%-20s", c_list[idx++].name);
    }

    out->concat("\n-- Supported digests:");
    idx = 0;
    Hashes* h_list = list_supported_digests();
    while (Hashes::NONE != *(h_list)) {
      if (0 == idx++ % 6) out->concat("\n--\t");
      out->concatf("\t%-10s", get_digest_label((Hashes) *(h_list++)));
    }
  #else
  out->concat("No cryptographic support.\n");
  #endif  // WITH_MBEDTLS
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

const int _cipher_opcode(Cipher ci, uint32_t opts) {
  switch (ci) {
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
      return (opts & OP_ENCRYPT) ? MBEDTLS_AES_ENCRYPT : MBEDTLS_AES_DECRYPT;
    case Cipher::SYM_BLOWFISH_ECB:
    case Cipher::SYM_BLOWFISH_CBC:
    case Cipher::SYM_BLOWFISH_CFB64:
    case Cipher::SYM_BLOWFISH_CTR:
      return (opts & OP_ENCRYPT) ? MBEDTLS_BLOWFISH_ENCRYPT : MBEDTLS_BLOWFISH_DECRYPT;
    default:
      return 0;  // TODO: Sketchy....
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
  return (_sym_overrides[ci] || _sauth_overrides[ci] || _asym_overrides[ci]);
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



#endif  // __HAS_CRYPT_WRAPPER
