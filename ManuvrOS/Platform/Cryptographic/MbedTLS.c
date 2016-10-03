/*
File:   MbedTLS.c
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


Implements cryptography via mbedTLS.

MbedTLS support assumes that we have a local copy of the mbedTLS source tree
  at <build-root>/lib/mbedtls. See the downloadDeps.sh script. 
*/

#if defined(WITH_MBEDTLS)


#include "Cryptographic.h"
#include <Platform/Platform.h>


const int _cipher_opcode(Cipher ci, uint32_t opts);


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
  if (_is_cipher_symmetric(c)) {
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
  if (_is_cipher_symmetric(c)) {
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
  if (_is_cipher_symmetric(c)) {
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
  if (_is_cipher_symmetric(c)) {
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
* Randomness                                                                   *
*******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif
mbedtls_entropy_context entropy;


int mbedtls_hardware_poll(void* data, unsigned char* buf, size_t len, size_t *olen) {
  random_fill((uint8_t*) buf, len);
  *olen = len;
  return 0;
}


#ifdef __cplusplus
}
#endif


int cryptographic_rng_init() {
  mbedtls_entropy_init(&entropy);
  int ret = mbedtls_entropy_add_source(
    &entropy,
    mbedtls_hardware_poll,
    NULL,
    PLATFORM_RNG_CARRY_CAPACITY * sizeof(uint32_t),
    MBEDTLS_ENTROPY_SOURCE_STRONG
  );
  return ret;
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
//  if (0 == wrapped_hash((uint8_t*) hash_in, strlen(hash_in), hash_out, 32, Hashes::MBEDTLS_MD_SHA256)) {
//    printf("Hash value:  ");
//    for (uint8_t i = 0; i < 32; i++) printf("0x%02x ", *(hash_out + i));
//    printf("\n");
//  }
//  else {
//    printf("Failed to hash.\n");
//  }
int8_t __attribute__((weak)) wrapped_hash(uint8_t* in, int in_len, uint8_t* out, Hashes h) {
  int8_t return_value = -1;
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
  return return_value;
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
int8_t __attribute__((weak)) wrapped_sym_cipher(uint8_t* in, int in_len, uint8_t* out, int out_len, uint8_t* key, int key_len, uint8_t* iv, Cipher ci, uint32_t opts) {
  if (cipher_deferred_handling(ci)) {
    // If overriden by user implementation.
    return _sym_overrides[ci](in, in_len, out, out_len, key, key_len, iv, ci, opts);
  }
  int8_t ret = -1;
  switch (ci) {
    #if defined(MBEDTLS_AES_C)
      case Cipher::SYM_AES_256_CBC:
      case Cipher::SYM_AES_192_CBC:
      case Cipher::SYM_AES_128_CBC:
        {
          mbedtls_aes_context ctx;
          if (opts & OP_ENCRYPT) {
            mbedtls_aes_setkey_enc(&ctx, key, key_len);
          }
          else {
            mbedtls_aes_setkey_dec(&ctx, key, key_len);
          }
          ret = mbedtls_aes_crypt_cbc(&ctx, _cipher_opcode(ci, opts), in_len, iv, in, out);
          mbedtls_aes_free(&ctx);
        }
        break;
    #endif

    #if defined(MBEDTLS_RSA_C)
      case Cipher::ASYM_RSA:
        {
          mbedtls_ctr_drbg_context ctr_drbg;
          mbedtls_ctr_drbg_init(&ctr_drbg);
          size_t olen = 0;
          mbedtls_pk_context ctx;
          mbedtls_pk_init(&ctx);
          if (opts & OP_ENCRYPT) {
            ret = mbedtls_pk_encrypt(&ctx, in, in_len, out, &olen, out_len, mbedtls_ctr_drbg_random, &ctr_drbg);
          }
          else {
            ret = mbedtls_pk_decrypt(&ctx, in, in_len, out, &olen, out_len, mbedtls_ctr_drbg_random, &ctr_drbg);
          }
          mbedtls_pk_free(&ctx);
        }
        break;
    #endif

    #if defined(MBEDTLS_BLOWFISH_C)
      case Cipher::SYM_BLOWFISH_CBC:
        {
          mbedtls_blowfish_context ctx;
          mbedtls_blowfish_setkey(&ctx, key, key_len);
          ret = mbedtls_blowfish_crypt_cbc(&ctx, _cipher_opcode(ci, opts), in_len, iv, in, out);
          mbedtls_blowfish_free(&ctx);
        }
        break;
    #endif

    #if defined(WRAPPED_SYM_NULL)
      case Cipher::SYM_NULL:
        memcpy(out, in, in_len);
        ret = 0;
        break;
    #endif

    default:
      break;
  }
  return ret;
}



/*******************************************************************************
* Asymmetric ciphers                                                           *
*******************************************************************************/
int __attribute__((weak)) wrapped_asym_keygen(Cipher c, CryptoKey key_type, uint8_t* pub, int* pub_len, uint8_t* priv, int* priv_len) {
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_ctr_drbg_init(&ctr_drbg);

  mbedtls_pk_context key;
  mbedtls_pk_init(&key);

  uint32_t pers = randomInt();

  int ret = mbedtls_ctr_drbg_seed(
    &ctr_drbg, mbedtls_entropy_func, &entropy,
    (const uint8_t*) &pers, 4
  );
  if (0 == ret) {
    switch (c) {
      #if defined(WRAPPED_ASYM_RSA)
        case Cipher::ASYM_RSA:
          {
            ret = mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
            if (0 == ret) {
              mbedtls_rsa_context* rsa = mbedtls_pk_rsa(key);
              ret = mbedtls_rsa_gen_key(
                rsa,
                mbedtls_ctr_drbg_random, &ctr_drbg,
                (int) key_type, 65537
              );
              if (0 == ret) {
                ret--;
                memset(pub,  0, *pub_len);
                memset(priv, 0, *priv_len);
                int written = mbedtls_pk_write_pubkey_der(&key, pub, *pub_len);
                if (0 < written) {
                  *pub_len = written;
                  written = mbedtls_pk_write_key_der(&key, priv, *priv_len);
                  if (0 < written) {
                    *priv_len = written;
                    ret = 0;
                  }
                }
              }
            }
          }
          break;
      #endif
      #if defined(MBEDTLS_ECP_C)
        case Cipher::ASYM_ECKEY:
          {
            ret = mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
            if (0 == ret) {
              mbedtls_ecp_keypair* ec_kp = mbedtls_pk_ec(key);
              ret = mbedtls_ecp_gen_key(
                (mbedtls_ecp_group_id) key_type,
                ec_kp,
                mbedtls_ctr_drbg_random, &ctr_drbg
              );
              if (0 == ret) {
                ret--;
                memset(pub,  0, *pub_len);
                memset(priv, 0, *priv_len);
                int written = mbedtls_pk_write_pubkey_der(&key, pub, *pub_len);
                if (0 < written) {
                  *pub_len = written;
                  written = mbedtls_pk_write_key_der(&key, priv, *priv_len);
                  if (0 < written) {
                    *priv_len = written;
                    ret = 0;
                  }
                }
              }
            }
          }
          break;
      #endif
      default:
        break;
    }
  }
  mbedtls_pk_free(&key);
  mbedtls_ctr_drbg_free(&ctr_drbg);
  return ret;
}


int __attribute__((weak)) wrapped_asym_cipher(uint8_t* in, int in_len, uint8_t* sig, int* out_len, Hashes h, Cipher ci, CryptoKey private_key, uint32_t opts) {
  return -1;
}


#endif   // WITH_MBEDTLS