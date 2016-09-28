/*
File:   Cryptographic.h
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


This file is meant to contain a set of common functions for cryptographic
  operations. This is a tricky area. Security is full of landmines and subtle
  value-assumptions. So this code is likely to be in-flux until a generalization
  strategy is decided.
Baseline support will be via mbedtls, but certain platforms will have hardware
  support for many of these operations, or will wish to handle them in their
  own way for some other reason. For that reason, we will make all such
  function definitions weak.
If you wish to use another crypto library (OpenSSL? MatrixSSL? uECC?) then
  the weak-reference functions in question should be extended with preprocessor
  logic to support them.
*/

#ifndef __MANUVR_CRYPTO_ABSTRACTION_H__
#define __MANUVR_CRYPTO_ABSTRACTION_H__

#include <inttypes.h>

#include <DataStructures/StringBuilder.h>

#if defined(__MANUVR_MBEDTLS)
  #include "mbedtls/ssl.h"
  #include "mbedtls/md.h"
  #include "mbedtls/md_internal.h"
  #include "mbedtls/base64.h"
  #include "mbedtls/aes.h"
  #include "mbedtls/blowfish.h"
#endif

typedef struct {
} CryptOpt;


enum class Hashes {
  #if defined(__MANUVR_MBEDTLS)
    #if defined(MBEDTLS_MD5_C)
      MD5 = MBEDTLS_MD_MD5,
    #endif
    #if defined(MBEDTLS_SHA1_C)
      SHA1 = MBEDTLS_MD_SHA1,
    #endif
    #if defined(MBEDTLS_SHA224_C)
      SHA224 = MBEDTLS_MD_SHA224,
    #endif
    #if defined(MBEDTLS_SHA256_C)
      SHA256 = MBEDTLS_MD_SHA256,
    #endif
    #if defined(MBEDTLS_SHA384_C)
      SHA384 = MBEDTLS_MD_SHA384,
    #endif
    #if defined(MBEDTLS_SHA512_C)
      SHA512 = MBEDTLS_MD_SHA512,
    #endif
    #if defined(MBEDTLS_RIPEMD160_C)
      RIPEMD160 = MBEDTLS_MD_RIPEMD160,
    #endif
    NONE = MBEDTLS_MD_NONE
  #else // No MBED support.
    NONE = 0
  #endif
};

enum class CryptoKey {
};

/* Some ciphers have nuances like block-mode (AES), */
enum class CipherMode {
};



enum class Cipher {
  NONE            = 0,
  ASYM_RSA        = MBEDTLS_PK_RSA,
  ASYM_ECKEY      = MBEDTLS_PK_ECKEY,
  ASYM_ECKEY_DH   = MBEDTLS_PK_ECKEY_DH,
  ASYM_ECDSA      = MBEDTLS_PK_ECDSA,
  ASYM_RSA_ALT    = MBEDTLS_PK_RSA_ALT,
  ASYM_RSASSA_PSS = MBEDTLS_PK_RSASSA_PSS,
  SYM_BF_CBC      = MBEDTLS_CIPHER_ID_BLOWFISH,
  ASYM_NONE       = MBEDTLS_PK_NONE,

  SYM_NULL                  = MBEDTLS_CIPHER_NULL,
  SYM_AES_128_ECB           = MBEDTLS_CIPHER_AES_128_ECB,
  SYM_AES_192_ECB           = MBEDTLS_CIPHER_AES_192_ECB,
  SYM_AES_256_ECB           = MBEDTLS_CIPHER_AES_256_ECB,
  SYM_AES_128_CBC           = MBEDTLS_CIPHER_AES_128_CBC,
  SYM_AES_192_CBC           = MBEDTLS_CIPHER_AES_192_CBC,
  SYM_AES_256_CBC           = MBEDTLS_CIPHER_AES_256_CBC,
  SYM_AES_128_CFB128        = MBEDTLS_CIPHER_AES_128_CFB128,
  SYM_AES_192_CFB128        = MBEDTLS_CIPHER_AES_192_CFB128,
  SYM_AES_256_CFB128        = MBEDTLS_CIPHER_AES_256_CFB128,
  SYM_AES_128_CTR           = MBEDTLS_CIPHER_AES_128_CTR,
  SYM_AES_192_CTR           = MBEDTLS_CIPHER_AES_192_CTR,
  SYM_AES_256_CTR           = MBEDTLS_CIPHER_AES_256_CTR,
  SYM_AES_128_GCM           = MBEDTLS_CIPHER_AES_128_GCM,
  SYM_AES_192_GCM           = MBEDTLS_CIPHER_AES_192_GCM,
  SYM_AES_256_GCM           = MBEDTLS_CIPHER_AES_256_GCM,
  SYM_CAMELLIA_128_ECB      = MBEDTLS_CIPHER_CAMELLIA_128_ECB,
  SYM_CAMELLIA_192_ECB      = MBEDTLS_CIPHER_CAMELLIA_192_ECB,
  SYM_CAMELLIA_256_ECB      = MBEDTLS_CIPHER_CAMELLIA_256_ECB,
  SYM_CAMELLIA_128_CBC      = MBEDTLS_CIPHER_CAMELLIA_128_CBC,
  SYM_CAMELLIA_192_CBC      = MBEDTLS_CIPHER_CAMELLIA_192_CBC,
  SYM_CAMELLIA_256_CBC      = MBEDTLS_CIPHER_CAMELLIA_256_CBC,
  SYM_CAMELLIA_128_CFB128   = MBEDTLS_CIPHER_CAMELLIA_128_CFB128,
  SYM_CAMELLIA_192_CFB128   = MBEDTLS_CIPHER_CAMELLIA_192_CFB128,
  SYM_CAMELLIA_256_CFB128   = MBEDTLS_CIPHER_CAMELLIA_256_CFB128,
  SYM_CAMELLIA_128_CTR      = MBEDTLS_CIPHER_CAMELLIA_128_CTR,
  SYM_CAMELLIA_192_CTR      = MBEDTLS_CIPHER_CAMELLIA_192_CTR,
  SYM_CAMELLIA_256_CTR      = MBEDTLS_CIPHER_CAMELLIA_256_CTR,
  SYM_CAMELLIA_128_GCM      = MBEDTLS_CIPHER_CAMELLIA_128_GCM,
  SYM_CAMELLIA_192_GCM      = MBEDTLS_CIPHER_CAMELLIA_192_GCM,
  SYM_CAMELLIA_256_GCM      = MBEDTLS_CIPHER_CAMELLIA_256_GCM,
  SYM_DES_ECB               = MBEDTLS_CIPHER_DES_ECB,
  SYM_DES_CBC               = MBEDTLS_CIPHER_DES_CBC,
  SYM_DES_EDE_ECB           = MBEDTLS_CIPHER_DES_EDE_ECB,
  SYM_DES_EDE_CBC           = MBEDTLS_CIPHER_DES_EDE_CBC,
  SYM_DES_EDE3_ECB          = MBEDTLS_CIPHER_DES_EDE3_ECB,
  SYM_DES_EDE3_CBC          = MBEDTLS_CIPHER_DES_EDE3_CBC,
  SYM_BLOWFISH_ECB          = MBEDTLS_CIPHER_BLOWFISH_ECB,
  SYM_BLOWFISH_CBC          = MBEDTLS_CIPHER_BLOWFISH_CBC,
  SYM_BLOWFISH_CFB64        = MBEDTLS_CIPHER_BLOWFISH_CFB64,
  SYM_BLOWFISH_CTR          = MBEDTLS_CIPHER_BLOWFISH_CTR,
  SYM_ARC4_128              = MBEDTLS_CIPHER_ARC4_128,
  SYM_AES_128_CCM           = MBEDTLS_CIPHER_AES_128_CCM,
  SYM_AES_192_CCM           = MBEDTLS_CIPHER_AES_192_CCM,
  SYM_AES_256_CCM           = MBEDTLS_CIPHER_AES_256_CCM,
  SYM_CAMELLIA_128_CCM      = MBEDTLS_CIPHER_CAMELLIA_128_CCM,
  SYM_CAMELLIA_192_CCM      = MBEDTLS_CIPHER_CAMELLIA_192_CCM,
  SYM_CAMELLIA_256_CCM      = MBEDTLS_CIPHER_CAMELLIA_256_CCM,
  SYM_NONE                  = MBEDTLS_CIPHER_NONE
};




typedef int8_t (*wrapped_sym_operation)(
  uint8_t* in,
  int in_len,
  uint8_t* out,
  int out_len,
  uint8_t* key,
  int key_len,
  uint8_t* iv,
  Cipher ci
);

typedef int8_t (*wrapped_asym_operation)(
  uint8_t* in,
  int in_len,
  uint8_t* out,
  int out_len,
  Hashes h,
  Cipher ci,
  CryptoKey key
);


/* This stuff needs to be reachable via C-linkage. That means ugly names. :-) */
extern "C" {

/*******************************************************************************
* Message digest (Hashing)
*******************************************************************************/
const int get_digest_output_length(Hashes);
const char* get_digest_label(Hashes);
int8_t manuvr_hash(uint8_t* in, int in_len, uint8_t* out, int out_len, Hashes h);


/*******************************************************************************
* Cipher/decipher
*******************************************************************************/
const int get_cipher_block_size(Cipher);
const int get_cipher_key_length(Cipher);
int get_cipher_aligned_size(Cipher, int len);
const char* get_cipher_label(Cipher);
int8_t manuvr_sym_encrypt(uint8_t* in, int in_len, uint8_t* out, int out_len, uint8_t* key, int key_len, uint8_t* iv, Cipher);
int8_t manuvr_sym_decrypt(uint8_t* in, int in_len, uint8_t* out, int out_len, uint8_t* key, int key_len, uint8_t* iv, Cipher);

int8_t manuvr_asym_keygen(Cipher, int key_len, uint8_t* pub, int pub_len, uint8_t* priv, int priv_len);
int8_t manuvr_asym_encrypt(uint8_t* in, int in_len, uint8_t* out, int* out_len, Hashes, Cipher, CryptoKey private_key);
int8_t manuvr_asym_decrypt(uint8_t* in, int in_len, uint8_t* out, int* out_len, Hashes, Cipher, CryptoKey public_key);



/*******************************************************************************
* Randomness                                                                   *
*******************************************************************************/
int8_t manuvr_random_fill(uint8_t* buf, int len);


/*******************************************************************************
* Meta                                                                         *
*******************************************************************************/
void printCryptoOverview(StringBuilder*);



} // extern "C"

#endif // __MANUVR_CRYPTO_ABSTRACTION_H__
