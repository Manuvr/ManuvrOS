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

*/

#if defined(__MANUVR_MBEDTLS)

#include "Cryptographic.h"
#include <Kernel.h>


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
    #if defined(MBEDTLS_AES_C)
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
int8_t __attribute__((weak)) manuvr_asym_keygen(Cipher, int key_len, uint8_t* pub, int pub_len, uint8_t* priv, int priv_len) {
  return -1;
}


int8_t __attribute__((weak)) manuvr_asym_cipher(uint8_t* in, int in_len, uint8_t* sig, int* out_len, Hashes h, Cipher ci, CryptoKey private_key, uint32_t opts) {
  return -1;
}


#endif   // __MANUVR_MBEDTLS
