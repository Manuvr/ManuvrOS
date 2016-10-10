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

// TODO: I don't like using std::map. Still need to decide on a replacement.
*/

#ifndef __CRYPTO_WRAPPER_H__
#define __CRYPTO_WRAPPER_H__

// Try to contain wrapped header concerns in here, pl0x...
#include <Rationalizer.h>
#include "Cryptographic/CryptOptUnifier.h"

#if defined(__HAS_CRYPT_WRAPPER)

#include <inttypes.h>
#include <map>   // TODO: Remove dependency.

class StringBuilder;

#define OP_DECRYPT 0x00000000
#define OP_ENCRYPT 0x00000001
#define OP_VERIFY  OP_DECRYPT
#define OP_SIGN    OP_ENCRYPT

/* This stuff needs to be reachable via C-linkage. That means ugly names. :-) */
#ifdef __cplusplus
extern "C" {
#endif


/*******************************************************************************
* Message digest (Hashing)
*******************************************************************************/
#if defined(__BUILD_HAS_DIGEST)
enum class Hashes {
  #if defined(WRAPPED_HASH_MD5)
    MD5 = WRAPPED_HASH_MD5,
  #endif
  #if defined(WRAPPED_HASH_SHA1)
    SHA1 = WRAPPED_HASH_SHA1,
  #endif
  #if defined(WRAPPED_HASH_SHA224)
    SHA224 = WRAPPED_HASH_SHA224,
  #endif
  #if defined(WRAPPED_HASH_SHA256)
    SHA256 = WRAPPED_HASH_SHA256,
  #endif
  #if defined(WRAPPED_HASH_SHA384)
    SHA384 = WRAPPED_HASH_SHA384,
  #endif
  #if defined(WRAPPED_HASH_SHA512)
    SHA512 = WRAPPED_HASH_SHA512,
  #endif
  #if defined(WRAPPED_HASH_RIPEMD160)
    RIPEMD160 = WRAPPED_HASH_RIPEMD160,
  #endif
  NONE = WRAPPED_HASH_NONE
};

typedef int (*wrapped_hash_operation)(
  uint8_t* in,
  size_t in_len,
  uint8_t* out,
  Hashes h
);

const int get_digest_output_length(Hashes);
const char* get_digest_label(Hashes);
int8_t wrapped_hash(uint8_t* in, size_t in_len, uint8_t* out, Hashes h);

// Now some inline definitions to mask the back-end API where it can be done
//   transparently...
inline Hashes* list_supported_digests() {
  return ((Hashes*) mbedtls_md_list());
};

#endif  //__BUILD_HAS_DIGEST


/*
*
* This is obnoxious to read, but it is essential for mating the library-support
*   options to the semantic checks of the compiler, and then THAT to run-time
*   behavior.
*/
#if defined(__BUILD_HAS_SYMMETRIC) || defined(__BUILD_HAS_ASYMMETRIC)
enum class Cipher {
  #if defined(WRAPPED_ASYM_RSA)
    ASYM_RSA                =  WRAPPED_ASYM_RSA,
  #endif
  #if defined(WRAPPED_ASYM_ECKEY)
    ASYM_ECKEY              =  WRAPPED_ASYM_ECKEY,
  #endif
  #if defined(WRAPPED_ASYM_ECKEY_DH)
    ASYM_ECKEY_DH           =  WRAPPED_ASYM_ECKEY_DH,
  #endif
  #if defined(WRAPPED_ASYM_ECDSA)
    ASYM_ECDSA              =  WRAPPED_ASYM_ECDSA,
  #endif
  #if defined(WRAPPED_ASYM_RSA_ALT)
    ASYM_RSA_ALT            =  WRAPPED_ASYM_RSA_ALT,
  #endif
  #if defined(WRAPPED_ASYM_RSASSA_PSS)
    ASYM_RSASSA_PSS         =  WRAPPED_ASYM_RSASSA_PSS,
  #endif
  #if defined(WRAPPED_ASYM_NONE)
    ASYM_NONE               =  WRAPPED_ASYM_NONE,
  #endif
  #if defined(WRAPPED_SYM_NULL)
    SYM_NULL                =  WRAPPED_SYM_NULL,
  #endif
  #if defined(WRAPPED_SYM_AES_128_ECB)
    SYM_AES_128_ECB         =  WRAPPED_SYM_AES_128_ECB,
  #endif
  #if defined(WRAPPED_SYM_AES_192_ECB)
    SYM_AES_192_ECB         =  WRAPPED_SYM_AES_192_ECB,
  #endif
  #if defined(WRAPPED_SYM_AES_256_ECB)
    SYM_AES_256_ECB         =  WRAPPED_SYM_AES_256_ECB,
  #endif
  #if defined(WRAPPED_SYM_AES_128_CBC)
    SYM_AES_128_CBC         =  WRAPPED_SYM_AES_128_CBC,
  #endif
  #if defined(WRAPPED_SYM_AES_192_CBC)
    SYM_AES_192_CBC         =  WRAPPED_SYM_AES_192_CBC,
  #endif
  #if defined(WRAPPED_SYM_AES_256_CBC)
    SYM_AES_256_CBC         =  WRAPPED_SYM_AES_256_CBC,
  #endif
  #if defined(WRAPPED_SYM_AES_128_CFB128)
    SYM_AES_128_CFB128      =  WRAPPED_SYM_AES_128_CFB128,
  #endif
  #if defined(WRAPPED_SYM_AES_192_CFB128)
    SYM_AES_192_CFB128      =  WRAPPED_SYM_AES_192_CFB128,
  #endif
  #if defined(WRAPPED_SYM_AES_256_CFB128)
    SYM_AES_256_CFB128      =  WRAPPED_SYM_AES_256_CFB128,
  #endif
  #if defined(WRAPPED_SYM_AES_128_CTR)
    SYM_AES_128_CTR         =  WRAPPED_SYM_AES_128_CTR,
  #endif
  #if defined(WRAPPED_SYM_AES_192_CTR)
    SYM_AES_192_CTR         =  WRAPPED_SYM_AES_192_CTR,
  #endif
  #if defined(WRAPPED_SYM_AES_256_CTR)
    SYM_AES_256_CTR         =  WRAPPED_SYM_AES_256_CTR,
  #endif
  #if defined(WRAPPED_SYM_AES_128_GCM)
    SYM_AES_128_GCM         =  WRAPPED_SYM_AES_128_GCM,
  #endif
  #if defined(WRAPPED_SYM_AES_192_GCM)
    SYM_AES_192_GCM         =  WRAPPED_SYM_AES_192_GCM,
  #endif
  #if defined(WRAPPED_SYM_AES_256_GCM)
    SYM_AES_256_GCM         =  WRAPPED_SYM_AES_256_GCM,
  #endif
  #if defined(WRAPPED_SYM_CAMELLIA_128_ECB)
    SYM_CAMELLIA_128_ECB    =  WRAPPED_SYM_CAMELLIA_128_ECB,
  #endif
  #if defined(WRAPPED_SYM_CAMELLIA_192_ECB)
    SYM_CAMELLIA_192_ECB    =  WRAPPED_SYM_CAMELLIA_192_ECB,
  #endif
  #if defined(WRAPPED_SYM_CAMELLIA_256_ECB)
    SYM_CAMELLIA_256_ECB    =  WRAPPED_SYM_CAMELLIA_256_ECB,
  #endif
  #if defined(WRAPPED_SYM_CAMELLIA_128_CBC)
    SYM_CAMELLIA_128_CBC    =  WRAPPED_SYM_CAMELLIA_128_CBC,
  #endif
  #if defined(WRAPPED_SYM_CAMELLIA_192_CBC)
    SYM_CAMELLIA_192_CBC    =  WRAPPED_SYM_CAMELLIA_192_CBC,
  #endif
  #if defined(WRAPPED_SYM_CAMELLIA_256_CBC)
    SYM_CAMELLIA_256_CBC    =  WRAPPED_SYM_CAMELLIA_256_CBC,
  #endif
  #if defined(WRAPPED_SYM_CAMELLIA_128_CFB128)
    SYM_CAMELLIA_128_CFB128 =  WRAPPED_SYM_CAMELLIA_128_CFB128,
  #endif
  #if defined(WRAPPED_SYM_CAMELLIA_192_CFB128)
    SYM_CAMELLIA_192_CFB128 =  WRAPPED_SYM_CAMELLIA_192_CFB128,
  #endif
  #if defined(WRAPPED_SYM_CAMELLIA_256_CFB128)
    SYM_CAMELLIA_256_CFB128 =  WRAPPED_SYM_CAMELLIA_256_CFB128,
  #endif
  #if defined(WRAPPED_SYM_CAMELLIA_128_CTR)
    SYM_CAMELLIA_128_CTR    =  WRAPPED_SYM_CAMELLIA_128_CTR,
  #endif
  #if defined(WRAPPED_SYM_CAMELLIA_192_CTR)
    SYM_CAMELLIA_192_CTR    =  WRAPPED_SYM_CAMELLIA_192_CTR,
  #endif
  #if defined(WRAPPED_SYM_CAMELLIA_256_CTR)
    SYM_CAMELLIA_256_CTR    =  WRAPPED_SYM_CAMELLIA_256_CTR,
  #endif
  #if defined(WRAPPED_SYM_CAMELLIA_128_GCM)
    SYM_CAMELLIA_128_GCM    =  WRAPPED_SYM_CAMELLIA_128_GCM,
  #endif
  #if defined(WRAPPED_SYM_CAMELLIA_192_GCM)
    SYM_CAMELLIA_192_GCM    =  WRAPPED_SYM_CAMELLIA_192_GCM,
  #endif
  #if defined(WRAPPED_SYM_CAMELLIA_256_GCM)
    SYM_CAMELLIA_256_GCM    =  WRAPPED_SYM_CAMELLIA_256_GCM,
  #endif
  #if defined(WRAPPED_SYM_DES_ECB)
    SYM_DES_ECB             =  WRAPPED_SYM_DES_ECB,
  #endif
  #if defined(WRAPPED_SYM_DES_CBC)
    SYM_DES_CBC             =  WRAPPED_SYM_DES_CBC,
  #endif
  #if defined(WRAPPED_SYM_DES_EDE_ECB)
    SYM_DES_EDE_ECB         =  WRAPPED_SYM_DES_EDE_ECB,
  #endif
  #if defined(WRAPPED_SYM_DES_EDE_CBC)
    SYM_DES_EDE_CBC         =  WRAPPED_SYM_DES_EDE_CBC,
  #endif
  #if defined(WRAPPED_SYM_DES_EDE3_ECB)
    SYM_DES_EDE3_ECB        =  WRAPPED_SYM_DES_EDE3_ECB,
  #endif
  #if defined(WRAPPED_SYM_DES_EDE3_CBC)
    SYM_DES_EDE3_CBC        =  WRAPPED_SYM_DES_EDE3_CBC,
  #endif
  #if defined(WRAPPED_SYM_BLOWFISH_ECB)
    SYM_BLOWFISH_ECB        =  WRAPPED_SYM_BLOWFISH_ECB,
  #endif
  #if defined(WRAPPED_SYM_BLOWFISH_CBC)
    SYM_BLOWFISH_CBC        =  WRAPPED_SYM_BLOWFISH_CBC,
  #endif
  #if defined(WRAPPED_SYM_BLOWFISH_CFB64)
    SYM_BLOWFISH_CFB64      =  WRAPPED_SYM_BLOWFISH_CFB64,
  #endif
  #if defined(WRAPPED_SYM_BLOWFISH_CTR)
    SYM_BLOWFISH_CTR        =  WRAPPED_SYM_BLOWFISH_CTR,
  #endif
  #if defined(WRAPPED_SYM_ARC4_128)
    SYM_ARC4_128            =  WRAPPED_SYM_ARC4_128,
  #endif
  #if defined(WRAPPED_SYM_AES_128_CCM)
    SYM_AES_128_CCM         =  WRAPPED_SYM_AES_128_CCM,
  #endif
  #if defined(WRAPPED_SYM_AES_192_CCM)
    SYM_AES_192_CCM         =  WRAPPED_SYM_AES_192_CCM,
  #endif
  #if defined(WRAPPED_SYM_AES_256_CCM)
    SYM_AES_256_CCM         =  WRAPPED_SYM_AES_256_CCM,
  #endif
  #if defined(WRAPPED_SYM_CAMELLIA_128_CCM)
    SYM_CAMELLIA_128_CCM    =  WRAPPED_SYM_CAMELLIA_128_CCM,
  #endif
  #if defined(WRAPPED_SYM_CAMELLIA_192_CCM)
    SYM_CAMELLIA_192_CCM    =  WRAPPED_SYM_CAMELLIA_192_CCM,
  #endif
  #if defined(WRAPPED_SYM_CAMELLIA_256_CCM)
    SYM_CAMELLIA_256_CCM    =  WRAPPED_SYM_CAMELLIA_256_CCM,
  #endif
  #if defined(WRAPPED_SYM_NONE)
    SYM_NONE                =  WRAPPED_SYM_NONE,
  #endif
  NONE                    =  WRAPPED_NONE
};

// If we have Cipher support at all, we need these...
const bool _is_cipher_symmetric(Cipher);
const bool _is_cipher_authenticated(Cipher);
const bool _is_cipher_asymmetric(Cipher);
const bool _valid_cipher_params(Cipher);
#endif  //__BUILD_HAS_SYMMETRIC

#if defined(__BUILD_HAS_SYMMETRIC)
typedef int (*wrapped_sym_operation)(
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

typedef int (*wrapped_sauth_operation)(
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
#endif  //__BUILD_HAS_SYMMETRIC


#if defined(__BUILD_HAS_ASYMMETRIC)
enum class CryptoKey {
  #if defined(WRAPPED_ASYM_RSA)
    RSA_1024    = 1024,
    RSA_2048    = 2048,
    RSA_4096    = 4096,
  #endif
  #if defined(WRAPPED_PK_OPT_SECP192R1)
    ECC_SECP192R1   = WRAPPED_PK_OPT_SECP192R1,
  #endif
  #if defined(WRAPPED_PK_OPT_SECP224R1)
    ECC_SECP224R1   = WRAPPED_PK_OPT_SECP224R1,
  #endif
  #if defined(WRAPPED_PK_OPT_SECP256R1)
    ECC_SECP256R1   = WRAPPED_PK_OPT_SECP256R1,
  #endif
  #if defined(WRAPPED_PK_OPT_SECP384R1)
    ECC_SECP384R1   = WRAPPED_PK_OPT_SECP384R1,
  #endif
  #if defined(WRAPPED_PK_OPT_SECP521R1)
    ECC_SECP521R1   = WRAPPED_PK_OPT_SECP521R1,
  #endif
  #if defined(WRAPPED_PK_OPT_SECP192K1)
    ECC_SECP192K1   = WRAPPED_PK_OPT_SECP192K1,
  #endif
  #if defined(WRAPPED_PK_OPT_SECP224K1)
    ECC_SECP224K1   = WRAPPED_PK_OPT_SECP224K1,
  #endif
  #if defined(WRAPPED_PK_OPT_SECP256K1)
    ECC_SECP256K1   = WRAPPED_PK_OPT_SECP256K1,
  #endif
  #if defined(WRAPPED_PK_OPT_BP256R1)
    ECC_BP256R1     = WRAPPED_PK_OPT_BP256R1,
  #endif
  #if defined(WRAPPED_PK_OPT_BP384R1)
    ECC_BP384R1     = WRAPPED_PK_OPT_BP384R1,
  #endif
  #if defined(WRAPPED_PK_OPT_BP512R1)
    ECC_BP512R1     = WRAPPED_PK_OPT_BP512R1,
  #endif
  #if defined(WRAPPED_PK_OPT_CURVE25519)
    ECC_CURVE25519  = WRAPPED_PK_OPT_CURVE25519,
  #endif
  NONE = 0
};

typedef int (*wrapped_sv_operation)(
  Cipher,           // Algorithm class
  CryptoKey,        // Key parameters
  Hashes,           // Digest alg to use.
  uint8_t* msg,     // Buffer to be signed/verified...
  int msg_len,      // ...and its length.
  uint8_t* sig,     // Buffer to hold signature...
  size_t* sig_len,  // ...and its length.
  uint8_t* key,     // Buffer holding the key...
  int key_len,      // ...and its length.
  uint32_t opts     // Options to the operations.
);

typedef int (*wrapped_keygen_operation)(
  Cipher,           // Algorithm class
  CryptoKey,        // Key parameters
  uint8_t* pub,     // Buffer to hold public key.
  size_t* pub_len,  // Length of buffer. Modified to reflect written length.
  uint8_t* priv,    // Buffer to hold private key.
  size_t* priv_len  // Length of buffer. Modified to reflect written length.
);


const char* get_pk_label(CryptoKey);
bool estimate_pk_size_requirements(CryptoKey, size_t* pub, size_t* priv, uint16_t* sig);

#endif  //__BUILD_HAS_ASYMMETRIC



/*******************************************************************************
* Asynchronous operations
*******************************************************************************/
/*
* Hardware involvement in the cryptographic process demands asynchronicity.
* This is one of the rare case when hardware support is typically slower than
*   a software-equivalent. We can't afford to stall.
*
* Some tasks will involve chains of operations in sequence, and because we are
*   potentially dealing with large amounts of memory, we need the ability to
*   inform the caller of completion-with-error states. And in the "happy-path",
*   we still need to dynamically chain operations to minimize IPC traffic.
*
* One final note for now: The processing-end of this operation might be running
*   in a trusted execution environment, or hardware (separate threads). So this
*   structure will ultimately be the focal-point of concurrency-control measures
*   and operation status.
*
* TODO: This is another platform anchor.
* TODO: Define callback structure and state-machine.
*/
#define CRYPT_ASYNC_OP_DIGEST        0x80
#define CRYPT_ASYNC_OP_CIPHER        0x40
#define CRYPT_ASYNC_OP_SIGN_VER      0x20
#define CRYPT_ASYNC_OP_AUTH_CIPHER   0x10
#define CRYPT_ASYNC_OP_KEYGEN        0x08

typedef int (*crypt_op_callback)(
  Cipher,           // Algorithm class
  CryptoKey,        // Key parameters
  uint8_t* pub,     // Buffer to hold public key.
  size_t* pub_len,  // Length of buffer. Modified to reflect written length.
  uint8_t* priv,    // Buffer to hold private key.
  size_t* priv_len  // Length of buffer. Modified to reflect written length.
);

typedef struct _async_crypt_op {
} AsyncCryptOpt;




/*******************************************************************************
* Cipher/decipher
*******************************************************************************/
const int get_cipher_block_size(Cipher);
const int get_cipher_key_length(Cipher);
int get_cipher_aligned_size(Cipher, int len);
const char* get_cipher_label(Cipher);
int wrapped_sym_cipher(uint8_t* in, int in_len, uint8_t* out, int out_len, uint8_t* key, int key_len, uint8_t* iv, Cipher, uint32_t opts);
int wrapped_asym_keygen(Cipher c, CryptoKey, uint8_t* pub, size_t* pub_len, uint8_t* priv, size_t* priv_len);

// Now some inline definitions to mask the back-end API where it can be done
//   transparently...
inline Cipher* list_supported_ciphers() {
  return ((Cipher*) mbedtls_cipher_list());
};

inline CryptoKey* list_supported_curves() {
  return ((CryptoKey*) mbedtls_ecp_grp_id_list());
};


int wrapped_sign_verify(Cipher, CryptoKey, Hashes, uint8_t* msg, int msg_len, uint8_t* sig, size_t* sig_len, uint8_t* key, int key_len, uint32_t opts);


/*******************************************************************************
* Randomness                                                                   *
*******************************************************************************/
int cryptographic_rng_init();
int8_t wrapped_random_fill(uint8_t* buf, int len);


/*******************************************************************************
* Meta                                                                         *
*******************************************************************************/
bool hardware_backed_rng();         // TODO: Only the platform can answer this.

// Is the algorithm provided by the default implementation?
// TODO: Might simply handle all operation calls from a map-like structure and clobber fxn ptrs.
// TODO: Make this a struct for the sake of tracking providers, (hard|soft)ware, etc...
bool digest_deferred_handling(Hashes);
bool cipher_deferred_handling(Cipher);
bool sign_verify_deferred_handling(CryptoKey);
bool keygen_deferred_handling(CryptoKey);

// Over-ride or provide implementations on an algo-by-algo basis.
bool provide_digest_handler(Hashes, wrapped_hash_operation);
bool provide_cipher_handler(Cipher, wrapped_sym_operation);
bool provide_sign_verify_handler(CryptoKey, wrapped_sv_operation);
bool provide_keygen_handler(CryptoKey, wrapped_keygen_operation);

void crypt_error_string(int errnum, char *buffer, size_t buflen);


/*******************************************************************************
* These things are privately-scoped, and are intended for internal use only.   *
*******************************************************************************/

static std::map<Cipher, wrapped_sym_operation>        _sym_overrides;
static std::map<Cipher, wrapped_sauth_operation>      _sauth_overrides;
static std::map<Hashes, wrapped_hash_operation>       _hash_overrides;
static std::map<CryptoKey, wrapped_sv_operation>      _s_v_overrides;
static std::map<CryptoKey, wrapped_keygen_operation>  _keygen_overrides;

#if defined(__BUILD_HAS_SYMMETRIC) || defined(__BUILD_HAS_ASYMMETRIC)
  // If we have Cipher support at all, we need these...
  const bool _is_cipher_symmetric(Cipher);
  const bool _is_cipher_authenticated(Cipher);
  const bool _is_cipher_asymmetric(Cipher);
  const bool _valid_cipher_params(Cipher);
#endif

#ifdef __cplusplus
}
#endif

int randomArt(uint8_t* dgst_raw, unsigned int dgst_raw_len, const char* key_type, StringBuilder* output);

#endif // __HAS_CRYPT_WRAPPER
#endif // __CRYPTO_WRAPPER_H__
