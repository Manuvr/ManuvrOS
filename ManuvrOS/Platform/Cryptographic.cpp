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



/**
* General interface to message digest functions. Isolates caller from knowledge
*   of hashing context. Blocks thread until complete.
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
int8_t __attribute__((weak)) manuvr_hash(uint8_t* in, int in_len, uint8_t* out, int out_len, Hashes h) {
  int8_t return_value = -1;
  #if defined(__MANUVR_MBEDTLS)
  const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type((mbedtls_md_type_t)h);

  if (NULL != md_info) {
    if (out_len >= md_info->size) {
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
    else {
      Kernel::log("hash(): Inadequately-sized output buffer.\n");
    }
  }
  #else
  Kernel::log("hash(): No hash implementation.\n");
  #endif
  return return_value;
}


void printCrpytoOverview(StringBuilder* out) {
  #if defined(__MANUVR_MBEDTLS)
    out->concat("-- Cryptographic support via mbedtls.\n");
    out->concat("-- Supported TLS ciphersuites:\n");
    const int* ciphersuites = mbedtls_ssl_list_ciphersuites();
    while (0 != *(ciphersuites)) {
      out->concatf("\t %s\n", mbedtls_ssl_get_ciphersuite_name(*(ciphersuites++)));
    }
  #else
  out->concat("No cryptographic support.\n");
  #endif
}
