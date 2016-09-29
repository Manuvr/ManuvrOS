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
#include <map>

/* Privately prototypes. */
const bool manuvr_is_cipher_symmetric(Cipher);
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
    else if (Cipher::SYM_NULL == c) {
      return 1;
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
    const int* list = mbedtls_ssl_list_ciphersuites();
    while (0 != *(list)) {
      if ((0 == idx++ % 2) && (0 != *(list))) out->concat("\n--\t");
      out->concatf("\t%-40s", mbedtls_ssl_get_ciphersuite_name(*(list++)));
    }
    out->concat("\n-- Supported ciphers:");
    idx = 0;
    list = mbedtls_cipher_list();
    while (0 != *(list)) {
      if ((0 == idx++ % 4) && (0 != *(list))) out->concat("\n--\t");
      out->concatf("\t%-18s", get_cipher_label((Cipher) *(list++)));
    }
    out->concat("\n-- Supported digests:");
    idx = 0;
    list = mbedtls_md_list();
    while (0 != *(list)) {
      if ((0 == idx++ % 6) && (0 != *(list))) out->concat("\n--\t");
      out->concatf("\t%-10s", get_digest_label((Hashes) *(list++)));
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
    else if (Cipher::SYM_NULL == c) {
      return "SYM-NULL";
    }
  }
  else {
  }
  return "<UNKNOWN>";
}




/*******************************************************************************
* Parameter compatibility checking matricies...                                *
*******************************************************************************/
/* Privately scoped. */
const bool manuvr_is_cipher_symmetric(Cipher ci) {
  switch (ci) {
    case Cipher::SYM_NULL:
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
};





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
// TODO: I don't like using std::map. Still need to decide on a replacement.
std::map<Cipher, wrapped_sym_operation>  _sym_overrides;   // Symmetric runtime overrides.
std::map<Cipher, wrapped_asym_operation> _asym_overrides;  // Asymmetric runtime overrides.
std::map<Hashes, wrapped_hash_operation> _hash_overrides;  // Digest runtime overrides.


/**
* Tests for an implementation-specific deferral for the given cipher.
*
* @param Cipher The cipher to test for deferral.
* @return true if the root function ought to defer.
*/
bool cipher_deferred_handling(Cipher ci) {
  switch (ci) {
    case Cipher::SYM_AES_256_CBC:
    case Cipher::SYM_AES_192_CBC:
    case Cipher::SYM_AES_128_CBC:
      if (_sym_overrides[ci]) {
        return true;
      }
    default:
      return false;
  }
}

/**
* Tests for an implementation-specific deferral for the given hash.
*
* @param Hashes The hash to test for deferral.
* @return true if the root function ought to defer.
*/
bool digest_deferred_handling(Hashes h) {
  switch (h) {
    default:
      return false;
  }
}


/*******************************************************************************
* Symmetric ciphers                                                            *
*******************************************************************************/
/**
* Block symmetric ciphers.
* Please note that linker-override is possible, but dynamic override is generally
*   preferable to avoid clobbering all symmetric support.
*
* @param uint8_t* Buffer containing plaintext.
* @param int      Length of plaintext.
* @param uint8_t* Target buffer for ciphertext.
* @param int      Length of output.
* @param uint8_t* Buffer containing the symmetric key.
* @param int      Length of the key, in bits.
* @param uint8_t* IV. Caller's responsibility to use correct size.
* @param Cipher   The cipher by which to encrypt.
* @param uint32_t Options to the optionation.
* @return true if the root function ought to defer.
*/
int8_t __attribute__((weak)) manuvr_sym_cipher(uint8_t* in, int in_len, uint8_t* out, int out_len, uint8_t* key, int key_len, uint8_t* iv, Cipher ci, uint32_t opts) {
  if (cipher_deferred_handling(ci)) {
    // If overriden by user implementation.
    return _sym_overrides[ci](in, in_len, out, out_len, key, key_len, iv, ci, opts);
  }
  int8_t ret = -1;
  switch (ci) {
    case Cipher::SYM_AES_256_CBC:
    case Cipher::SYM_AES_192_CBC:
    case Cipher::SYM_AES_128_CBC:
      {
        mbedtls_aes_context ctx;
        if (opts & MANUVR_ENCRYPT) {
          mbedtls_aes_setkey_enc(&ctx, key, key_len);
        }
        else {
          mbedtls_aes_setkey_dec(&ctx, key, key_len);
        }
        ret = mbedtls_aes_crypt_cbc(&ctx, _cipher_opcode(ci, opts), in_len, iv, in, out);
        mbedtls_aes_free(&ctx);
      }
      break;
    case Cipher::SYM_BLOWFISH_CBC:
      {
        mbedtls_blowfish_context ctx;
        mbedtls_blowfish_setkey(&ctx, key, key_len);
        ret = mbedtls_blowfish_crypt_cbc(&ctx, _cipher_opcode(ci, opts), in_len, iv, in, out);
        mbedtls_blowfish_free(&ctx);
      }
      break;
    case Cipher::SYM_NULL:
      memcpy(out, in, in_len);
      ret = 0;
      break;
    default:
      break;
  }
  return ret;
}



/*******************************************************************************
* Asymmetric ciphers                                                           *
*******************************************************************************/
int8_t __attribute__((weak)) manuvr_asym_keygen(Cipher, int key_len, uint8_t* pub, int pub_len, uint8_t* priv, int priv_len) {
  return -1;
}


int8_t __attribute__((weak)) manuvr_asym_cipher(uint8_t* in, int in_len, uint8_t* sig, int* out_len, Hashes h, Cipher ci, CryptoKey private_key, uint32_t opts) {
  return -1;
}
