/*
File:   CryptoTest.cpp
Author: J. Ian Lindsay
Date:   2016.09.25

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


This tests the cryptographic system under whatever build options
  were provided.
*/

#include <cstdio>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include <fstream>
#include <iostream>

#include <DataStructures/StringBuilder.h>

#include <Platform/Platform.h>

/*
* Tests to ensure we have support for what we are about to test.
*/
int CRYPTO_TEST_INIT() {
  printf("===< CRYPTO_TEST_INIT >==========================================\n");
  if (platform.hasCryptography()) {
    // This is a good start....
    return 0;
  }
  return -1;
}

/*
* Digest algortithm tests.
* Because these are not reversible, this will take the form of
*   a known-answer test.
*/
int CRYPTO_TEST_HASHES() {
  printf("===< CRYPTO_TEST_HASHES >========================================\n");

  const char* hash_in0  = "";
  uint8_t* hash_in1  = (uint8_t*) alloca(1524);
  random_fill(hash_in1, 1524);

  Hashes algs_to_test[] = {
    Hashes::MD5,
    Hashes::SHA1,
    Hashes::RIPEMD160,
    Hashes::SHA256,
    Hashes::SHA512,
    Hashes::NONE
  };

  int idx = 0;
  while (Hashes::NONE != algs_to_test[idx]) {
    int o_len = get_digest_output_length(algs_to_test[idx]);
    printf("Testing %-14s  %3d-byte block\n--------------------------------------\n",
        get_digest_label(algs_to_test[idx]),
        o_len);

    uint8_t* hash_out0 = (uint8_t*) alloca(o_len);
    uint8_t* hash_out1 = (uint8_t*) alloca(o_len);

    if (manuvr_hash((uint8_t*) hash_in0, strlen(hash_in0), hash_out0, o_len, algs_to_test[idx])) {
      printf("Failed to hash.\n");
      return -1;
    }
    if (manuvr_hash((uint8_t*) hash_in1, 1524, hash_out1, o_len, algs_to_test[idx])) {
      printf("Failed to hash.\n");
      return -1;
    }
    printf("hash_out0:  ");
    for (uint8_t i = 0; i < o_len; i++) printf("%02x", *(hash_out0 + i));
    printf("\n");
    printf("hash_out1:  ");
    for (uint8_t i = 0; i < o_len; i++) printf("%02x", *(hash_out1 + i));
    printf("\n");
    idx++;
  }
  return 0;
}


/*
* Symmetric algorthms.
*/
int CRYPTO_TEST_SYMMETRIC() {
  printf("===< CRYPTO_TEST_SYMMETRIC >=====================================\n");
  int i_len = 30;

  Cipher algs_to_test[] = {
    Cipher::SYM_AES_128_CBC,
    Cipher::SYM_AES_192_CBC,
    Cipher::SYM_AES_256_CBC,
    Cipher::SYM_BLOWFISH_CBC,
    Cipher::SYM_NULL,
    Cipher::NONE
  };

  int ret = 0;
  int idx = 0;
  while (Cipher::NONE != algs_to_test[idx]) {
    const int block_size = get_cipher_block_size(algs_to_test[idx]);
    const int key_size   = get_cipher_key_length(algs_to_test[idx]);
    int o_len = get_cipher_aligned_size(algs_to_test[idx], i_len);
    uint8_t* plaintext_in  = (uint8_t*) alloca(o_len);
    random_fill(plaintext_in, o_len);

    printf("Testing %-14s  %3d-byte block, %4d-bit key\n----------------------------------------------------\n",
        get_cipher_label(algs_to_test[idx]),
        block_size,
        key_size);

    if (0 != o_len % block_size) {
      o_len += (block_size - (o_len % block_size));
    }

    uint8_t* ciphertext    = (uint8_t*) alloca(o_len);
    uint8_t* plaintext_out = (uint8_t*) alloca(o_len);
    uint8_t iv[16];
    uint8_t key[(key_size>>3)];
    bzero(iv, 16);
    bzero(ciphertext, o_len);
    bzero(plaintext_out, o_len);
    random_fill(key, (key_size>>3));

    printf("Key:           ");
    for (uint8_t i = 0; i < (key_size>>3); i++) printf("%02x", *(key + i));
    printf("\n");

    printf("Plaintext in:  ");
    for (uint8_t i = 0; i < i_len; i++) printf("%02x", *(plaintext_in + i));
    printf("\t(%d bytes)\n", i_len);

    ret = manuvr_sym_encrypt((uint8_t*) plaintext_in, o_len, ciphertext, o_len, key, key_size, iv, algs_to_test[idx]);
    if (ret) {
      printf("Failed to encrypt. Error %d\n", ret);
      return -1;
    }

    printf("Ciphertext:    ");
    for (uint8_t i = 0; i < o_len; i++) printf("%02x", *(ciphertext + i));
    printf("\t(%d bytes)\n", o_len);

    bzero(iv, 16);
    ret = manuvr_sym_decrypt(ciphertext, o_len, plaintext_out, o_len, key, key_size, iv, algs_to_test[idx]);
    if (ret) {
      printf("Failed to decrypt. Error %d\n", ret);
      return -1;
    }

    printf("Plaintext out: ");
    for (uint8_t i = 0; i < o_len; i++) printf("%02x", *(plaintext_out + i));
    printf("\t(%d bytes)\n", o_len);

    // Now check that the plaintext versions match...
    for (uint8_t i = 0; i < i_len; i++) {
      if (*(plaintext_in + i) != *(plaintext_out + i)) {
        printf("Plaintext mismatch. Test fails.\n");
        return -1;
      }
    }
    printf("\n");
    idx++;
  }
  return 0;
}


/*
* Asymmetric algorthms.
*/
int CRYPTO_TEST_ASYMMETRIC() {
  printf("===< CRYPTO_TEST_ASYMMETRIC >====================================\n");
  return -1;
}



void printTestFailure(const char* test) {
  printf("\n");
  printf("*********************************************\n");
  printf("* %s FAILED tests.\n", test);
  printf("*********************************************\n");
}

/****************************************************************************************************
* The main function.                                                                                *
****************************************************************************************************/
int main(int argc, char *argv[]) {
  int exit_value = 1;   // Failure is the default result.

  platform.platformPreInit();   // Our test fixture needs random numbers.

  if (0 == CRYPTO_TEST_INIT()) {
    if (0 == CRYPTO_TEST_HASHES()) {
      if (0 == CRYPTO_TEST_SYMMETRIC()) {
        if (0 == CRYPTO_TEST_ASYMMETRIC()) {
          printf("**********************************\n");
          printf("*  Cryptography tests all pass   *\n");
          printf("**********************************\n");
          exit_value = 0;
        }
        else printTestFailure("CRYPTO_TEST_ASYMMETRIC");
      }
      else printTestFailure("CRYPTO_TEST_SYMMETRIC");
    }
    else printTestFailure("CRYPTO_TEST_HASHES");
  }
  else printTestFailure("CRYPTO_TEST_INIT");

  exit(exit_value);
}
