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

Try to resist re-coding structs and such that the back-ends have already
  provided. Ultimately, many of the wrapped algos will have parameter-accessors
  that are strictly inlines. Some faculty like this must exist, however for
  providing wrappers around (possibly) concurrent software and hardware support.
See CryptOptUnifier.h for more information.
*/

#ifndef __MANUVR_CRYPTO_ABSTRACTION_H__
#define __MANUVR_CRYPTO_ABSTRACTION_H__

#include <inttypes.h>

// Try to contain wrapped header concerns in here, pl0x...
#include "Cryptographic/CryptOptUnifier.h"

#include <DataStructures/StringBuilder.h>


#define MANUVR_ENCRYPT 0x00000001
#define MANUVR_DECRYPT 0x00000002



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
  NONE                    =  WRAPPED_NONE,
  ASYM_RSA                =  WRAPPED_ASYM_RSA,
  ASYM_ECKEY              =  WRAPPED_ASYM_ECKEY,
  ASYM_ECKEY_DH           =  WRAPPED_ASYM_ECKEY_DH,
  ASYM_ECDSA              =  WRAPPED_ASYM_ECDSA,
  ASYM_RSA_ALT            =  WRAPPED_ASYM_RSA_ALT,
  ASYM_RSASSA_PSS         =  WRAPPED_ASYM_RSASSA_PSS,
  ASYM_NONE               =  WRAPPED_ASYM_NONE,
  SYM_NULL                =  WRAPPED_SYM_NULL,
  SYM_AES_128_ECB         =  WRAPPED_SYM_AES_128_ECB,
  SYM_AES_192_ECB         =  WRAPPED_SYM_AES_192_ECB,
  SYM_AES_256_ECB         =  WRAPPED_SYM_AES_256_ECB,
  SYM_AES_128_CBC         =  WRAPPED_SYM_AES_128_CBC,
  SYM_AES_192_CBC         =  WRAPPED_SYM_AES_192_CBC,
  SYM_AES_256_CBC         =  WRAPPED_SYM_AES_256_CBC,
  SYM_AES_128_CFB128      =  WRAPPED_SYM_AES_128_CFB128,
  SYM_AES_192_CFB128      =  WRAPPED_SYM_AES_192_CFB128,
  SYM_AES_256_CFB128      =  WRAPPED_SYM_AES_256_CFB128,
  SYM_AES_128_CTR         =  WRAPPED_SYM_AES_128_CTR,
  SYM_AES_192_CTR         =  WRAPPED_SYM_AES_192_CTR,
  SYM_AES_256_CTR         =  WRAPPED_SYM_AES_256_CTR,
  SYM_AES_128_GCM         =  WRAPPED_SYM_AES_128_GCM,
  SYM_AES_192_GCM         =  WRAPPED_SYM_AES_192_GCM,
  SYM_AES_256_GCM         =  WRAPPED_SYM_AES_256_GCM,
  SYM_CAMELLIA_128_ECB    =  WRAPPED_SYM_CAMELLIA_128_ECB,
  SYM_CAMELLIA_192_ECB    =  WRAPPED_SYM_CAMELLIA_192_ECB,
  SYM_CAMELLIA_256_ECB    =  WRAPPED_SYM_CAMELLIA_256_ECB,
  SYM_CAMELLIA_128_CBC    =  WRAPPED_SYM_CAMELLIA_128_CBC,
  SYM_CAMELLIA_192_CBC    =  WRAPPED_SYM_CAMELLIA_192_CBC,
  SYM_CAMELLIA_256_CBC    =  WRAPPED_SYM_CAMELLIA_256_CBC,
  SYM_CAMELLIA_128_CFB128 =  WRAPPED_SYM_CAMELLIA_128_CFB128,
  SYM_CAMELLIA_192_CFB128 =  WRAPPED_SYM_CAMELLIA_192_CFB128,
  SYM_CAMELLIA_256_CFB128 =  WRAPPED_SYM_CAMELLIA_256_CFB128,
  SYM_CAMELLIA_128_CTR    =  WRAPPED_SYM_CAMELLIA_128_CTR,
  SYM_CAMELLIA_192_CTR    =  WRAPPED_SYM_CAMELLIA_192_CTR,
  SYM_CAMELLIA_256_CTR    =  WRAPPED_SYM_CAMELLIA_256_CTR,
  SYM_CAMELLIA_128_GCM    =  WRAPPED_SYM_CAMELLIA_128_GCM,
  SYM_CAMELLIA_192_GCM    =  WRAPPED_SYM_CAMELLIA_192_GCM,
  SYM_CAMELLIA_256_GCM    =  WRAPPED_SYM_CAMELLIA_256_GCM,
  SYM_DES_ECB             =  WRAPPED_SYM_DES_ECB,
  SYM_DES_CBC             =  WRAPPED_SYM_DES_CBC,
  SYM_DES_EDE_ECB         =  WRAPPED_SYM_DES_EDE_ECB,
  SYM_DES_EDE_CBC         =  WRAPPED_SYM_DES_EDE_CBC,
  SYM_DES_EDE3_ECB        =  WRAPPED_SYM_DES_EDE3_ECB,
  SYM_DES_EDE3_CBC        =  WRAPPED_SYM_DES_EDE3_CBC,
  SYM_BLOWFISH_ECB        =  WRAPPED_SYM_BLOWFISH_ECB,
  SYM_BLOWFISH_CBC        =  WRAPPED_SYM_BLOWFISH_CBC,
  SYM_BLOWFISH_CFB64      =  WRAPPED_SYM_BLOWFISH_CFB64,
  SYM_BLOWFISH_CTR        =  WRAPPED_SYM_BLOWFISH_CTR,
  SYM_ARC4_128            =  WRAPPED_SYM_ARC4_128,
  SYM_AES_128_CCM         =  WRAPPED_SYM_AES_128_CCM,
  SYM_AES_192_CCM         =  WRAPPED_SYM_AES_192_CCM,
  SYM_AES_256_CCM         =  WRAPPED_SYM_AES_256_CCM,
  SYM_CAMELLIA_128_CCM    =  WRAPPED_SYM_CAMELLIA_128_CCM,
  SYM_CAMELLIA_192_CCM    =  WRAPPED_SYM_CAMELLIA_192_CCM,
  SYM_CAMELLIA_256_CCM    =  WRAPPED_SYM_CAMELLIA_256_CCM,
  SYM_NONE                =  WRAPPED_SYM_NONE,
};




typedef int8_t (*wrapped_sym_operation)(
  uint8_t* in,
  int in_len,
  uint8_t* out,
  int out_len,
  uint8_t* key,
  int key_len,
  uint8_t* iv,
  Cipher ci,
  uint32_t opts
);

typedef int8_t (*wrapped_sauth_operation)(
  uint8_t* in,
  int in_len,
  uint8_t* out,
  int out_len,
  uint8_t* key,
  int key_len,
  uint8_t* iv,
  Cipher ci,
  uint32_t opts
);

typedef int8_t (*wrapped_asym_operation)(
  uint8_t* in,
  int in_len,
  uint8_t* out,
  int out_len,
  Hashes h,
  Cipher ci,
  CryptoKey key,
  uint32_t opts
);

typedef int8_t (*wrapped_hash_operation)(
  uint8_t* in,
  int in_len,
  uint8_t* out,
  Hashes h
);


/* This stuff needs to be reachable via C-linkage. That means ugly names. :-) */
extern "C" {

/*******************************************************************************
* Message digest (Hashing)
*******************************************************************************/
const int get_digest_output_length(Hashes);
const char* get_digest_label(Hashes);
int8_t manuvr_hash(uint8_t* in, int in_len, uint8_t* out, Hashes h);


/*******************************************************************************
* Cipher/decipher
*******************************************************************************/
const int get_cipher_block_size(Cipher);
const int get_cipher_key_length(Cipher);
int get_cipher_aligned_size(Cipher, int len);
const char* get_cipher_label(Cipher);
int8_t manuvr_sym_cipher(uint8_t* in, int in_len, uint8_t* out, int out_len, uint8_t* key, int key_len, uint8_t* iv, Cipher, uint32_t opts);

int8_t manuvr_asym_keygen(Cipher, int key_len, uint8_t* pub, int pub_len, uint8_t* priv, int priv_len);
int8_t manuvr_asym_cipher(uint8_t* in, int in_len, uint8_t* out, int* out_len, Hashes, Cipher, CryptoKey private_key, uint32_t opts);



/*******************************************************************************
* Randomness                                                                   *
*******************************************************************************/
int8_t manuvr_random_fill(uint8_t* buf, int len);


/*******************************************************************************
* Meta                                                                         *
*******************************************************************************/
void printCryptoOverview(StringBuilder*);

// Is the algorithm implemented in hardware?
bool digest_hardware_backed(Hashes);
bool cipher_hardware_backed(Cipher);

// Is the algorithm provided by the default implementation?
bool digest_deferred_handling(Hashes);
bool cipher_deferred_handling(Cipher);

// Over-ride or provide implementations on an algo-by-algo basis.
bool provide_digest_handler(Hashes);
bool provide_cipher_handler(Cipher);

} // extern "C"

const int _cipher_opcode(uint32_t opts);

#endif // __MANUVR_CRYPTO_ABSTRACTION_H__
