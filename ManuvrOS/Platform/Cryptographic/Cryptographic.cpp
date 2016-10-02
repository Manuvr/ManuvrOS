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


#if defined(__MANUVR_HAS_CRYPTO)

/* Privately scoped prototypes. */
const bool manuvr_is_cipher_symmetric(Cipher);
const bool manuvr_is_cipher_authenticated(Cipher);
const bool manuvr_is_cipher_asymmetric(Cipher);


/*******************************************************************************
* Meta                                                                         *
*******************************************************************************/

/**
* Given the identifier for the hash algorithm return the output size.
*
* @param Hashes The hash algorithm in question.
* @return The size of the buffer (in bytes) required to hold the digest output.
*/
const int get_digest_output_length(Hashes h) {
  const mbedtls_md_info_t* info = mbedtls_md_info_from_type((mbedtls_md_type_t)h);
  if (info) {
    return info->size;
  }
  return 0;
}

/**
* Given the identifier for the cipher algorithm return the key size.
*
* @param Cipher The cipher algorithm in question.
* @return The size of the buffer (in bytes) required to hold the cipher key.
*/
const int get_cipher_key_length(Cipher c) {
  if (manuvr_is_cipher_symmetric(c)) {
    const mbedtls_cipher_info_t* info = mbedtls_cipher_info_from_type((mbedtls_cipher_type_t)c);
    if (info) {
      return info->key_bitlen;
    }
  }
  else {
    const mbedtls_pk_info_t* info = mbedtls_pk_info_from_type((mbedtls_pk_type_t)c);
    if (info) {
      //return info->key_bitlen;
    }
    //mbedtls_pk_get_bitlen
  }
  return 0;
}


/**
* Given the identifier for the cipher algorithm return the block size.
*
* @param Cipher The cipher algorithm in question.
* @return The modulus of the buffer (in bytes) required for this cipher.
*/
const int get_cipher_block_size(Cipher c) {
  if (manuvr_is_cipher_symmetric(c)) {
    const mbedtls_cipher_info_t* info = mbedtls_cipher_info_from_type((mbedtls_cipher_type_t)c);
    if (info) {
      return info->block_size;
    }
  }
  else {
  }
  return 0;
}


/**
* Given the identifier for the cipher algorithm return the block size.
*
* @param Cipher The cipher algorithm in question.
* @param int The starting length of the input data.
* @return The modulus of the buffer (in bytes) required for this cipher.
*/
int get_cipher_aligned_size(Cipher c, int base_len) {
  if (manuvr_is_cipher_symmetric(c)) {
    const mbedtls_cipher_info_t* info = mbedtls_cipher_info_from_type((mbedtls_cipher_type_t)c);
    if (info) {
      int len = base_len;
      if (0 != len % info->block_size) {
        len += (info->block_size - (len % info->block_size));
      }
      return len;
    }
  }
  else {
  }
  return base_len;
}




/*******************************************************************************
* String lookup and debug...                                                   *
*******************************************************************************/

/**
* Prints details about the cryptographic situation on the platform.
*
* @param  StringBuilder* The buffer to output into.
*/
void printCryptoOverview(StringBuilder* out) {
  #if defined(__MANUVR_MBEDTLS)
    out->concat("-- Cryptographic support via mbedtls.\n");
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
  #endif  // __MANUVR_MBEDTLS
}


/**
* Given the indirected identifier for the hash algorithm return its label.
*
* @param Hashes The hash algorithm in question.
* @return The size of the buffer (in bytes) required to hold the digest output.
*/
const char* get_digest_label(Hashes h) {
  const mbedtls_md_info_t* info = mbedtls_md_info_from_type((mbedtls_md_type_t)h);
  if (info) {
    return info->name;
  }
  return "<UNKNOWN>";
}


/**
* Given the indirected identifier for the hash algorithm return its label.
*
* @param Hashes The hash algorithm in question.
* @return The size of the buffer (in bytes) required to hold the digest output.
*/
const char* get_cipher_label(Cipher c) {
  if (manuvr_is_cipher_symmetric(c)) {
    const mbedtls_cipher_info_t* info = mbedtls_cipher_info_from_type((mbedtls_cipher_type_t)c);
    if (info) {
      return info->name;
    }
  }
  else {
    const mbedtls_pk_info_t* info = mbedtls_pk_info_from_type((mbedtls_pk_type_t)c);
    if (info) {
      return info->name;
    }
  }
  return "<UNKNOWN>";
}




/*******************************************************************************
* Parameter compatibility checking matricies...                                *
*******************************************************************************/
/* Privately scoped. */
const bool manuvr_is_cipher_symmetric(Cipher ci) {
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
const bool manuvr_is_cipher_authenticated(Cipher ci) {
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
const bool manuvr_is_cipher_asymmetric(Cipher ci) {
  switch (ci) {
    case Cipher::ASYM_RSA:
    case Cipher::ASYM_ECKEY:
    case Cipher::ASYM_ECKEY_DH:
    case Cipher::ASYM_ECDSA:
    case Cipher::ASYM_RSA_ALT:
    case Cipher::ASYM_RSASSA_PSS:
      return true;
    default:
      return false;
  }
}

/* Privately scoped. */
const bool manuvr_valid_cipher_params(Cipher ci) {
  switch (ci) {
    case Cipher::ASYM_RSA:
    case Cipher::ASYM_ECKEY:
    case Cipher::ASYM_ECKEY_DH:
    case Cipher::ASYM_ECDSA:
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
      return (opts & MANUVR_ENCRYPT) ? MBEDTLS_AES_ENCRYPT : MBEDTLS_AES_DECRYPT;
    case Cipher::SYM_BLOWFISH_ECB:
    case Cipher::SYM_BLOWFISH_CBC:
    case Cipher::SYM_BLOWFISH_CFB64:
    case Cipher::SYM_BLOWFISH_CTR:
      return (opts & MANUVR_ENCRYPT) ? MBEDTLS_BLOWFISH_ENCRYPT : MBEDTLS_BLOWFISH_DECRYPT;
    default:
      return 0;  // TODO: Sketchy....
  }
}





/*******************************************************************************
* Message digest and Hash                                                      *
*******************************************************************************/

/**
* General interface to message digest functions. Isolates caller from knowledge
*   of hashing context. Blocks thread until complete.
* NOTE: We assume that the caller has the foresight to allocate a large-enough output buffer.
*
* @return 0 on success. Non-zero otherwise.
*/
// Usage example:
//  char* hash_in  = "Uniform input text";
//  uint8_t* hash_out = (uint8_t*) alloca(64);
//  if (0 == manuvr_hash((uint8_t*) hash_in, strlen(hash_in), hash_out, 32, Hashes::MBEDTLS_MD_SHA256)) {
//    printf("Hash value:  ");
//    for (uint8_t i = 0; i < 32; i++) printf("0x%02x ", *(hash_out + i));
//    printf("\n");
//  }
//  else {
//    printf("Failed to hash.\n");
//  }
int8_t __attribute__((weak)) manuvr_hash(uint8_t* in, int in_len, uint8_t* out, Hashes h) {
  int8_t return_value = -1;
  #if defined(__MANUVR_MBEDTLS)
  const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type((mbedtls_md_type_t)h);

  if (NULL != md_info) {
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    switch (mbedtls_md_setup(&ctx, md_info, 0)) {
      case 0:
        if (0 == mbedtls_md_starts(&ctx)) {
          // Start feeding data...
          if (0 == mbedtls_md_update(&ctx, in, in_len)) {
            if (0 == mbedtls_md_finish(&ctx, out)) {
              return_value = 0;
            }
            else {
              Kernel::log("hash(): Failed during finish.\n");
            }
          }
          else {
            Kernel::log("hash(): Failed during digest.\n");
          }
        }
        else {
          Kernel::log("hash(): Bad input data.\n");
        }
        break;
      case MBEDTLS_ERR_MD_BAD_INPUT_DATA:
        Kernel::log("hash(): Bad parameters.\n");
        break;
      case MBEDTLS_ERR_MD_ALLOC_FAILED:
        Kernel::log("hash(): Allocation failure.\n");
        break;
      default:
        break;
    }
    mbedtls_md_free(&ctx);
  }
  #else
  Kernel::log("hash(): No hash implementation.\n");
  #endif  // __MANUVR_MBEDTLS
  return return_value;
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



#endif  // __MANUVR_HAS_CRYPTO
