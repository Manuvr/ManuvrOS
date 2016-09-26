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
#endif

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

enum class Cipher {
};



/* This stuff needs to be reachable via C-linkage. */
extern "C" {

/*******************************************************************************
* Message digest (Hashing)
*******************************************************************************/
int8_t manuvr_hash(uint8_t* in, int in_len, uint8_t* out, int out_len, Hashes h);


/*******************************************************************************
* Cipher/decipher
*******************************************************************************/
int8_t manuvr_block_encrypt(uint8_t* in, int in_len, uint8_t* out, int out_len, Cipher);
int8_t manuvr_block_decrypt(uint8_t* in, int in_len, uint8_t* out, int out_len, Cipher);

int8_t manuvr_sign(uint8_t* in, int in_len, uint8_t* sig, int* out_len, Hashes, Cipher, CryptoKey private_key);
int8_t manuvr_verify(uint8_t* in, int in_len, uint8_t* sig, int* out_len, Hashes, Cipher, CryptoKey public_key);



/*******************************************************************************
* Randomness
*******************************************************************************/

/*******************************************************************************
* Debug
*******************************************************************************/
void printCryptoOverview(StringBuilder*);


} // extern "C"

#endif // __MANUVR_CRYPTO_ABSTRACTION_H__
