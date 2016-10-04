/*
File:   CryptOptUnifier.h
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


This header is meant to be the layer that unifies types and preprocessor
  definitions for different cryptographic backends. It should not be included
  from any file other than Cryptographic.h.

Note that this represents a maximum scope-of-support regarding cryptographic
  algorithms.

// TODO: Current software support is confined to mbedTLS. Should
//         be extended to other libraries (OpenSSL, uECC, etc...).
*/

#ifndef __CRYPTO_ABSTRACTION_H__
#define __CRYPTO_ABSTRACTION_H__

#if defined(WITH_MBEDTLS)
/* MbedTLS support assumes that we have a local copy of the mbedTLS source tree
     at <build-root>/lib/mbedtls. See the downloadDeps.sh script. */
  #define __HAS_CRYPT_WRAPPER   1
  #define __CRYPTO_BACKEND      "mbedTLS"
  #include "mbedtls/error.h"
  #include "mbedtls/ssl.h"
  #include "mbedtls/entropy.h"
  #include "mbedtls/ctr_drbg.h"
  #include "mbedtls/md.h"
  #include "mbedtls/md_internal.h"
  #include "mbedtls/base64.h"
  #if defined(MBEDTLS_PK_C)
    #include "mbedtls/pk.h"
    #include "mbedtls/pk_internal.h"
  #endif
  #if defined(MBEDTLS_AES_C)
    #include "mbedtls/aes.h"
  #endif
  #if defined(MBEDTLS_BLOWFISH_C)
    #include "mbedtls/blowfish.h"
  #endif
  #if defined(MBEDTLS_RSA_C)
    #include "mbedtls/rsa.h"
  #endif
  #if defined(MBEDTLS_ECP_C)
    #include "mbedtls/ecp.h"
  #endif

  // Now we are going to re-assign some defines to make our base-wrapper. All
  //   code everywhere in this framework ought to code against these defs.
  // We rely on comparable checks by mbedTLS on it's config.h file. We are using
  //   the end-results of that output, so we can be sure it is valid.
  #define WRAPPED_NONE                      0
  #if defined(MBEDTLS_PK_C)
    #define WRAPPED_ASYM_NONE                 MBEDTLS_PK_NONE
  #endif
  #if defined(MBEDTLS_CIPHER_C)
    #define WRAPPED_SYM_NONE                  MBEDTLS_CIPHER_NONE
  #endif
  #if defined(MBEDTLS_CIPHER_NULL_CIPHER)
    #define WRAPPED_SYM_NULL                  MBEDTLS_CIPHER_NULL
  #endif

  #if defined(MBEDTLS_PK_RSA_ALT_SUPPORT)
    #define WRAPPED_ASYM_RSA_ALT              MBEDTLS_PK_RSA_ALT
  #endif
  #if defined(MBEDTLS_RSA_C)
    #if defined(MBEDTLS_X509_RSASSA_PSS_SUPPORT)
      #define WRAPPED_ASYM_RSASSA_PSS           MBEDTLS_PK_RSASSA_PSS
    #endif
    #define WRAPPED_ASYM_RSA                  MBEDTLS_PK_RSA
  #endif

  #if defined(MBEDTLS_ECP_C)
    #if defined(MBEDTLS_ECDSA_C)
      #define WRAPPED_ASYM_ECDSA                MBEDTLS_PK_ECDSA
    #endif
    #if defined(MBEDTLS_ECDH_C)
      #define WRAPPED_ASYM_ECKEY_DH             MBEDTLS_PK_ECKEY_DH
    #endif
    #define WRAPPED_ASYM_ECKEY                MBEDTLS_PK_ECKEY
  #endif

  #if defined(MBEDTLS_AES_C)
    #define WRAPPED_SYM_AES_128_ECB           MBEDTLS_CIPHER_AES_128_ECB
    #define WRAPPED_SYM_AES_192_ECB           MBEDTLS_CIPHER_AES_192_ECB
    #define WRAPPED_SYM_AES_256_ECB           MBEDTLS_CIPHER_AES_256_ECB
  #endif

  #if defined(MBEDTLS_CAMELLIA_C)
    #define WRAPPED_SYM_CAMELLIA_128_ECB      MBEDTLS_CIPHER_CAMELLIA_128_ECB
    #define WRAPPED_SYM_CAMELLIA_192_ECB      MBEDTLS_CIPHER_CAMELLIA_192_ECB
    #define WRAPPED_SYM_CAMELLIA_256_ECB      MBEDTLS_CIPHER_CAMELLIA_256_ECB
  #endif

  #if defined(MBEDTLS_CIPHER_MODE_CBC)
    #if defined(MBEDTLS_AES_C)
      #define WRAPPED_SYM_AES_128_CBC           MBEDTLS_CIPHER_AES_128_CBC
      #define WRAPPED_SYM_AES_192_CBC           MBEDTLS_CIPHER_AES_192_CBC
      #define WRAPPED_SYM_AES_256_CBC           MBEDTLS_CIPHER_AES_256_CBC
    #endif
    #if defined(MBEDTLS_CAMELLIA_C)
      #define WRAPPED_SYM_CAMELLIA_128_CBC      MBEDTLS_CIPHER_CAMELLIA_128_CBC
      #define WRAPPED_SYM_CAMELLIA_192_CBC      MBEDTLS_CIPHER_CAMELLIA_192_CBC
      #define WRAPPED_SYM_CAMELLIA_256_CBC      MBEDTLS_CIPHER_CAMELLIA_256_CBC
    #endif
    #if defined(MBEDTLS_DES_C)
      #define WRAPPED_SYM_DES_CBC               MBEDTLS_CIPHER_DES_CBC
      #define WRAPPED_SYM_DES_ECB               MBEDTLS_CIPHER_DES_ECB
      #define WRAPPED_SYM_DES_EDE_ECB           MBEDTLS_CIPHER_DES_EDE_ECB
      #define WRAPPED_SYM_DES_EDE_CBC           MBEDTLS_CIPHER_DES_EDE_CBC
      #define WRAPPED_SYM_DES_EDE3_ECB          MBEDTLS_CIPHER_DES_EDE3_ECB
      #define WRAPPED_SYM_DES_EDE3_CBC          MBEDTLS_CIPHER_DES_EDE3_CBC
    #endif
  #endif

  #if defined(MBEDTLS_GCM_C)
    #if defined(MBEDTLS_AES_C)
      #define WRAPPED_SYM_AES_128_GCM           MBEDTLS_CIPHER_AES_128_GCM
      #define WRAPPED_SYM_AES_192_GCM           MBEDTLS_CIPHER_AES_192_GCM
      #define WRAPPED_SYM_AES_256_GCM           MBEDTLS_CIPHER_AES_256_GCM
    #endif
    #if defined(MBEDTLS_CAMELLIA_C)
      #define WRAPPED_SYM_CAMELLIA_128_GCM      MBEDTLS_CIPHER_CAMELLIA_128_GCM
      #define WRAPPED_SYM_CAMELLIA_192_GCM      MBEDTLS_CIPHER_CAMELLIA_192_GCM
      #define WRAPPED_SYM_CAMELLIA_256_GCM      MBEDTLS_CIPHER_CAMELLIA_256_GCM
    #endif
  #endif

  #if defined(MBEDTLS_CIPHER_MODE_CFB)
    #if defined(MBEDTLS_AES_C)
      #define WRAPPED_SYM_AES_128_CFB128        MBEDTLS_CIPHER_AES_128_CFB128
      #define WRAPPED_SYM_AES_192_CFB128        MBEDTLS_CIPHER_AES_192_CFB128
      #define WRAPPED_SYM_AES_256_CFB128        MBEDTLS_CIPHER_AES_256_CFB128
    #endif
    #if defined(MBEDTLS_CAMELLIA_C)
      #define WRAPPED_SYM_CAMELLIA_128_CFB128   MBEDTLS_CIPHER_CAMELLIA_128_CFB128
      #define WRAPPED_SYM_CAMELLIA_192_CFB128   MBEDTLS_CIPHER_CAMELLIA_192_CFB128
      #define WRAPPED_SYM_CAMELLIA_256_CFB128   MBEDTLS_CIPHER_CAMELLIA_256_CFB128
    #endif
  #endif

  #if defined(MBEDTLS_CIPHER_MODE_CTR)
    #if defined(MBEDTLS_AES_C)
      #define WRAPPED_SYM_AES_128_CTR           MBEDTLS_CIPHER_AES_128_CTR
      #define WRAPPED_SYM_AES_192_CTR           MBEDTLS_CIPHER_AES_192_CTR
      #define WRAPPED_SYM_AES_256_CTR           MBEDTLS_CIPHER_AES_256_CTR
    #endif
    #if defined(MBEDTLS_CAMELLIA_C)
      #define WRAPPED_SYM_CAMELLIA_128_CTR      MBEDTLS_CIPHER_CAMELLIA_128_CTR
      #define WRAPPED_SYM_CAMELLIA_192_CTR      MBEDTLS_CIPHER_CAMELLIA_192_CTR
      #define WRAPPED_SYM_CAMELLIA_256_CTR      MBEDTLS_CIPHER_CAMELLIA_256_CTR
    #endif
  #endif

  #if defined(MBEDTLS_BLOWFISH_C)
    #define WRAPPED_SYM_BLOWFISH_ECB          MBEDTLS_CIPHER_BLOWFISH_ECB
    #define WRAPPED_SYM_BLOWFISH_CBC          MBEDTLS_CIPHER_BLOWFISH_CBC
    #define WRAPPED_SYM_BLOWFISH_CFB64        MBEDTLS_CIPHER_BLOWFISH_CFB64
    #define WRAPPED_SYM_BLOWFISH_CTR          MBEDTLS_CIPHER_BLOWFISH_CTR
  #endif

  #if defined(MBEDTLS_ARC4_C)
    #define WRAPPED_SYM_ARC4_128              MBEDTLS_CIPHER_ARC4_128
  #endif

  #if defined(MBEDTLS_CCM_C)
    #if defined(MBEDTLS_AES_C)
      #define WRAPPED_SYM_AES_128_CCM           MBEDTLS_CIPHER_AES_128_CCM
      #define WRAPPED_SYM_AES_192_CCM           MBEDTLS_CIPHER_AES_192_CCM
      #define WRAPPED_SYM_AES_256_CCM           MBEDTLS_CIPHER_AES_256_CCM
    #endif
    #if defined(MBEDTLS_CAMELLIA_C)
      #define WRAPPED_SYM_CAMELLIA_128_CCM      MBEDTLS_CIPHER_CAMELLIA_128_CCM
      #define WRAPPED_SYM_CAMELLIA_192_CCM      MBEDTLS_CIPHER_CAMELLIA_192_CCM
      #define WRAPPED_SYM_CAMELLIA_256_CCM      MBEDTLS_CIPHER_CAMELLIA_256_CCM
    #endif
  #endif


  // Discover and wrap hash support.
  #define WRAPPED_HASH_NONE       MBEDTLS_MD_NONE
  #if defined(MBEDTLS_MD5_C)
    #define WRAPPED_HASH_MD5        MBEDTLS_MD_MD5
  #endif
  #if defined(MBEDTLS_SHA1_C)
    #define WRAPPED_HASH_SHA1       MBEDTLS_MD_SHA1
  #endif
  #if defined(MBEDTLS_SHA224_C)
    #define WRAPPED_HASH_SHA224     MBEDTLS_MD_SHA224
  #endif
  #if defined(MBEDTLS_SHA256_C)
    #define WRAPPED_HASH_SHA256     MBEDTLS_MD_SHA256
  #endif
  #if defined(MBEDTLS_SHA384_C)
    #define WRAPPED_HASH_SHA384     MBEDTLS_MD_SHA384
  #endif
  #if defined(MBEDTLS_SHA512_C)
    #define WRAPPED_HASH_SHA512     MBEDTLS_MD_SHA512
  #endif
  #if defined(MBEDTLS_RIPEMD160_C)
    #define WRAPPED_HASH_RIPEMD160  MBEDTLS_MD_RIPEMD160
  #endif


  // Domain parameters and key-sizes for the PK layer.
  #if defined(MBEDTLS_ECP_DP_SECP192R1_ENABLED)
    #define WRAPPED_PK_OPT_SECP192R1  MBEDTLS_ECP_DP_SECP192R1
  #endif
  #if defined(MBEDTLS_ECP_DP_SECP224R1_ENABLED)
    #define WRAPPED_PK_OPT_SECP224R1  MBEDTLS_ECP_DP_SECP224R1
  #endif
  #if defined(MBEDTLS_ECP_DP_SECP256R1_ENABLED)
    #define WRAPPED_PK_OPT_SECP256R1  MBEDTLS_ECP_DP_SECP256R1
  #endif
  #if defined(MBEDTLS_ECP_DP_SECP384R1_ENABLED)
    #define WRAPPED_PK_OPT_SECP384R1  MBEDTLS_ECP_DP_SECP384R1
  #endif
  #if defined(MBEDTLS_ECP_DP_SECP521R1_ENABLED)
    #define WRAPPED_PK_OPT_SECP521R1  MBEDTLS_ECP_DP_SECP521R1
  #endif
  #if defined(MBEDTLS_ECP_DP_SECP192K1_ENABLED)
    #define WRAPPED_PK_OPT_SECP192K1  MBEDTLS_ECP_DP_SECP192K1
  #endif
  #if defined(MBEDTLS_ECP_DP_SECP224K1_ENABLED)
    #define WRAPPED_PK_OPT_SECP224K1  MBEDTLS_ECP_DP_SECP224K1
  #endif
  #if defined(MBEDTLS_ECP_DP_SECP256K1_ENABLED)
    #define WRAPPED_PK_OPT_SECP256K1  MBEDTLS_ECP_DP_SECP256K1
  #endif
  #if defined(MBEDTLS_ECP_DP_BP256R1_ENABLED)
    #define WRAPPED_PK_OPT_BP256R1  MBEDTLS_ECP_DP_BP256R1
  #endif
  #if defined(MBEDTLS_ECP_DP_BP384R1_ENABLED)
    #define WRAPPED_PK_OPT_BP384R1  MBEDTLS_ECP_DP_BP384R1
  #endif
  #if defined(MBEDTLS_ECP_DP_BP512R1_ENABLED)
    #define WRAPPED_PK_OPT_BP512R1  MBEDTLS_ECP_DP_BP512R1
  #endif
  #if defined(MBEDTLS_ECP_DP_CURVE25519_ENABLED)
    #define WRAPPED_PK_OPT_CURVE25519  MBEDTLS_ECP_DP_CURVE25519
  #endif




#elif defined(WITH_OPENSSL)
/* OpenSSL support will use the library on the host building the binary. If the
     host cannot supply a static version of OpenSSL, this will fail to build. */
  #define __HAS_CRYPT_WRAPPER   1
  #define __CRYPTO_BACKEND      "OpenSSL"
  #include <openssl/evp.h>
  #include <openssl/aes.h>
  #include <openssl/bn.h>
  #include <openssl/ec.h>
  #include <openssl/ecdsa.h>
  #include <openssl/obj_mac.h>
  #include <openssl/hmac.h>
  #include <openssl/sha.h>
  #include <openssl/err.h>
  #include <openssl/rand.h>
  #include <openssl/x509v3.h>

  // Now we are going to re-assign some defines to make our base-wrapper. All
  //   code everywhere in this framework ought to code against these defs.
  #define WRAPPED_NONE                      0
  #define WRAPPED_ASYM_NONE                 MBEDTLS_PK_NONE
  #define WRAPPED_ASYM_RSA                  MBEDTLS_PK_RSA
  #define WRAPPED_ASYM_ECKEY                MBEDTLS_PK_ECKEY
  #define WRAPPED_ASYM_ECKEY_DH             MBEDTLS_PK_ECKEY_DH
  #define WRAPPED_ASYM_ECDSA                MBEDTLS_PK_ECDSA
  #define WRAPPED_ASYM_RSA_ALT              MBEDTLS_PK_RSA_ALT
  #define WRAPPED_ASYM_RSASSA_PSS           MBEDTLS_PK_RSASSA_PSS
  #define WRAPPED_SYM_NONE                  MBEDTLS_CIPHER_NONE
  #define WRAPPED_SYM_NULL                  MBEDTLS_CIPHER_NULL
  #define WRAPPED_SYM_AES_128_ECB           MBEDTLS_CIPHER_AES_128_ECB
  #define WRAPPED_SYM_AES_192_ECB           MBEDTLS_CIPHER_AES_192_ECB
  #define WRAPPED_SYM_AES_256_ECB           MBEDTLS_CIPHER_AES_256_ECB
  #define WRAPPED_SYM_AES_128_CBC           MBEDTLS_CIPHER_AES_128_CBC
  #define WRAPPED_SYM_AES_192_CBC           MBEDTLS_CIPHER_AES_192_CBC
  #define WRAPPED_SYM_AES_256_CBC           MBEDTLS_CIPHER_AES_256_CBC
  #define WRAPPED_SYM_AES_128_CFB128        MBEDTLS_CIPHER_AES_128_CFB128
  #define WRAPPED_SYM_AES_192_CFB128        MBEDTLS_CIPHER_AES_192_CFB128
  #define WRAPPED_SYM_AES_256_CFB128        MBEDTLS_CIPHER_AES_256_CFB128
  #define WRAPPED_SYM_AES_128_CTR           MBEDTLS_CIPHER_AES_128_CTR
  #define WRAPPED_SYM_AES_192_CTR           MBEDTLS_CIPHER_AES_192_CTR
  #define WRAPPED_SYM_AES_256_CTR           MBEDTLS_CIPHER_AES_256_CTR
  #define WRAPPED_SYM_AES_128_GCM           MBEDTLS_CIPHER_AES_128_GCM
  #define WRAPPED_SYM_AES_192_GCM           MBEDTLS_CIPHER_AES_192_GCM
  #define WRAPPED_SYM_AES_256_GCM           MBEDTLS_CIPHER_AES_256_GCM
  #define WRAPPED_SYM_CAMELLIA_128_ECB      MBEDTLS_CIPHER_CAMELLIA_128_ECB
  #define WRAPPED_SYM_CAMELLIA_192_ECB      MBEDTLS_CIPHER_CAMELLIA_192_ECB
  #define WRAPPED_SYM_CAMELLIA_256_ECB      MBEDTLS_CIPHER_CAMELLIA_256_ECB
  #define WRAPPED_SYM_CAMELLIA_128_CBC      MBEDTLS_CIPHER_CAMELLIA_128_CBC
  #define WRAPPED_SYM_CAMELLIA_192_CBC      MBEDTLS_CIPHER_CAMELLIA_192_CBC
  #define WRAPPED_SYM_CAMELLIA_256_CBC      MBEDTLS_CIPHER_CAMELLIA_256_CBC
  #define WRAPPED_SYM_CAMELLIA_128_CFB128   MBEDTLS_CIPHER_CAMELLIA_128_CFB128
  #define WRAPPED_SYM_CAMELLIA_192_CFB128   MBEDTLS_CIPHER_CAMELLIA_192_CFB128
  #define WRAPPED_SYM_CAMELLIA_256_CFB128   MBEDTLS_CIPHER_CAMELLIA_256_CFB128
  #define WRAPPED_SYM_CAMELLIA_128_CTR      MBEDTLS_CIPHER_CAMELLIA_128_CTR
  #define WRAPPED_SYM_CAMELLIA_192_CTR      MBEDTLS_CIPHER_CAMELLIA_192_CTR
  #define WRAPPED_SYM_CAMELLIA_256_CTR      MBEDTLS_CIPHER_CAMELLIA_256_CTR
  #define WRAPPED_SYM_CAMELLIA_128_GCM      MBEDTLS_CIPHER_CAMELLIA_128_GCM
  #define WRAPPED_SYM_CAMELLIA_192_GCM      MBEDTLS_CIPHER_CAMELLIA_192_GCM
  #define WRAPPED_SYM_CAMELLIA_256_GCM      MBEDTLS_CIPHER_CAMELLIA_256_GCM
  #define WRAPPED_SYM_DES_ECB               MBEDTLS_CIPHER_DES_ECB
  #define WRAPPED_SYM_DES_CBC               MBEDTLS_CIPHER_DES_CBC
  #define WRAPPED_SYM_DES_EDE_ECB           MBEDTLS_CIPHER_DES_EDE_ECB
  #define WRAPPED_SYM_DES_EDE_CBC           MBEDTLS_CIPHER_DES_EDE_CBC
  #define WRAPPED_SYM_DES_EDE3_ECB          MBEDTLS_CIPHER_DES_EDE3_ECB
  #define WRAPPED_SYM_DES_EDE3_CBC          MBEDTLS_CIPHER_DES_EDE3_CBC
  #define WRAPPED_SYM_BLOWFISH_ECB          MBEDTLS_CIPHER_BLOWFISH_ECB
  #define WRAPPED_SYM_BLOWFISH_CBC          MBEDTLS_CIPHER_BLOWFISH_CBC
  #define WRAPPED_SYM_BLOWFISH_CFB64        MBEDTLS_CIPHER_BLOWFISH_CFB64
  #define WRAPPED_SYM_BLOWFISH_CTR          MBEDTLS_CIPHER_BLOWFISH_CTR
  #define WRAPPED_SYM_ARC4_128              MBEDTLS_CIPHER_ARC4_128
  #define WRAPPED_SYM_AES_128_CCM           MBEDTLS_CIPHER_AES_128_CCM
  #define WRAPPED_SYM_AES_192_CCM           MBEDTLS_CIPHER_AES_192_CCM
  #define WRAPPED_SYM_AES_256_CCM           MBEDTLS_CIPHER_AES_256_CCM
  #define WRAPPED_SYM_CAMELLIA_128_CCM      MBEDTLS_CIPHER_CAMELLIA_128_CCM
  #define WRAPPED_SYM_CAMELLIA_192_CCM      MBEDTLS_CIPHER_CAMELLIA_192_CCM
  #define WRAPPED_SYM_CAMELLIA_256_CCM      MBEDTLS_CIPHER_CAMELLIA_256_CCM



#elif defined(WITH_BLIND_CRYPTO)
/* "Blind" means we are going to assume we support a whole mess of crpytographic
     algorithms without any specific plan for doing so. It is then the job of
     drivers to clobber our default functions at link-time, or use the more
     granular run-time override.
   Note that platform-level drivers can do this as well to provide hooks into
     such things as TPMs and microcontroller cryptographic peripherals. */
  #define __HAS_CRYPT_WRAPPER   1
  #define __CRYPTO_BACKEND      "Blind"

  // We will unquestioningly provide definition and support for all manner of
  //   elemental algorithm. We have no way of predicting what will be added at
  //   run-time.
  #define WRAPPED_NONE                      0
  #define WRAPPED_ASYM_NONE                 1
  #define WRAPPED_ASYM_RSA                  2
  #define WRAPPED_ASYM_ECKEY                3
  #define WRAPPED_ASYM_ECKEY_DH             4
  #define WRAPPED_ASYM_ECDSA                5
  #define WRAPPED_ASYM_RSA_ALT              6
  #define WRAPPED_ASYM_RSASSA_PSS           7
  #define WRAPPED_SYM_NONE                  8
  #define WRAPPED_SYM_NULL                  9
  #define WRAPPED_SYM_AES_128_ECB           10
  #define WRAPPED_SYM_AES_192_ECB           11
  #define WRAPPED_SYM_AES_256_ECB           12
  #define WRAPPED_SYM_AES_128_CBC           13
  #define WRAPPED_SYM_AES_192_CBC           14
  #define WRAPPED_SYM_AES_256_CBC           15
  #define WRAPPED_SYM_AES_128_CFB128        16
  #define WRAPPED_SYM_AES_192_CFB128        17
  #define WRAPPED_SYM_AES_256_CFB128        18
  #define WRAPPED_SYM_AES_128_CTR           19
  #define WRAPPED_SYM_AES_192_CTR           20
  #define WRAPPED_SYM_AES_256_CTR           21
  #define WRAPPED_SYM_AES_128_GCM           22
  #define WRAPPED_SYM_AES_192_GCM           23
  #define WRAPPED_SYM_AES_256_GCM           24
  #define WRAPPED_SYM_CAMELLIA_128_ECB      25
  #define WRAPPED_SYM_CAMELLIA_192_ECB      26
  #define WRAPPED_SYM_CAMELLIA_256_ECB      27
  #define WRAPPED_SYM_CAMELLIA_128_CBC      28
  #define WRAPPED_SYM_CAMELLIA_192_CBC      29
  #define WRAPPED_SYM_CAMELLIA_256_CBC      30
  #define WRAPPED_SYM_CAMELLIA_128_CFB128   31
  #define WRAPPED_SYM_CAMELLIA_192_CFB128   32
  #define WRAPPED_SYM_CAMELLIA_256_CFB128   33
  #define WRAPPED_SYM_CAMELLIA_128_CTR      34
  #define WRAPPED_SYM_CAMELLIA_192_CTR      35
  #define WRAPPED_SYM_CAMELLIA_256_CTR      36
  #define WRAPPED_SYM_CAMELLIA_128_GCM      37
  #define WRAPPED_SYM_CAMELLIA_192_GCM      38
  #define WRAPPED_SYM_CAMELLIA_256_GCM      39
  #define WRAPPED_SYM_DES_ECB               40
  #define WRAPPED_SYM_DES_CBC               41
  #define WRAPPED_SYM_DES_EDE_ECB           42
  #define WRAPPED_SYM_DES_EDE_CBC           43
  #define WRAPPED_SYM_DES_EDE3_ECB          44
  #define WRAPPED_SYM_DES_EDE3_CBC          45
  #define WRAPPED_SYM_BLOWFISH_ECB          46
  #define WRAPPED_SYM_BLOWFISH_CBC          47
  #define WRAPPED_SYM_BLOWFISH_CFB64        48
  #define WRAPPED_SYM_BLOWFISH_CTR          49
  #define WRAPPED_SYM_ARC4_128              50
  #define WRAPPED_SYM_AES_128_CCM           51
  #define WRAPPED_SYM_AES_192_CCM           52
  #define WRAPPED_SYM_AES_256_CCM           53
  #define WRAPPED_SYM_CAMELLIA_128_CCM      54
  #define WRAPPED_SYM_CAMELLIA_192_CCM      55
  #define WRAPPED_SYM_CAMELLIA_256_CCM      56


#else
  // There is no cryptographic support.
#endif

#endif  // __CRYPTO_ABSTRACTION_H__
