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
* Tests to ensure the RNG works.
*/
int CRYPTO_TEST_RNG() {
  printf("===< CRYPTO_TEST_RNG >===========================================\n");
  uint8_t* result  = (uint8_t*) alloca(PLATFORM_RNG_CARRY_CAPACITY*2);
  int idx = 0;

  size_t size_tests[] = {
    (PLATFORM_RNG_CARRY_CAPACITY/2),
    (PLATFORM_RNG_CARRY_CAPACITY),
    (PLATFORM_RNG_CARRY_CAPACITY),
    0
  };

  while (size_tests[idx] != 0) {
    size_t r_len = size_tests[idx];
    printf("Requesting %d random bytes...\n\t", r_len);
    random_fill(result, r_len);
    for (unsigned int i = 0; i < r_len; i++) printf("%02x", *(result + i));
    printf("\n");
    idx++;
  }

  return 0;
}


/*
* Digest algortithm tests.
* Because these are not reversible, this will take the form of
*   a known-answer test.
*/
int CRYPTO_TEST_HASHES() {
  printf("===< CRYPTO_TEST_HASHES >========================================\n");

  const char* hash_in0  = "";
  int i_len = 1524;
  uint8_t* hash_in1  = (uint8_t*) alloca(i_len);
  random_fill(hash_in1, i_len);

  Hashes* algs_to_test = list_supported_digests();

  int idx = 0;
  while (Hashes::NONE != algs_to_test[idx]) {
    int o_len = get_digest_output_length(algs_to_test[idx]);
    printf("Testing %-14s  %3d-byte block\n--------------------------------------\n",
        get_digest_label(algs_to_test[idx]),
        o_len);

    uint8_t* hash_out0 = (uint8_t*) alloca(o_len);
    uint8_t* hash_out1 = (uint8_t*) alloca(o_len);

    if (wrapped_hash((uint8_t*) hash_in0, strlen(hash_in0), hash_out0, algs_to_test[idx])) {
      printf("Failed to hash.\n");
      return -1;
    }
    if (wrapped_hash((uint8_t*) hash_in1, i_len, hash_out1, algs_to_test[idx])) {
      printf("Failed to hash.\n");
      return -1;
    }
    printf("0-length: ");
    for (int i = 0; i < o_len; i++) printf("%02x", *(hash_out0 + i));
    printf("\n");
    printf("hash_out: ");
    for (int i = 0; i < o_len; i++) printf("%02x", *(hash_out1 + i));
    printf("\n\n");
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
    //Cipher::SYM_NULL,
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
    for (int i = 0; i < (key_size>>3); i++) printf("%02x", *(key + i));
    printf("\n");

    printf("Plaintext in:  ");
    for (int i = 0; i < i_len; i++) printf("%02x", *(plaintext_in + i));
    printf("\t(%d bytes)\n", i_len);

    ret = wrapped_sym_cipher((uint8_t*) plaintext_in, o_len, ciphertext, o_len, key, key_size, iv, algs_to_test[idx], OP_ENCRYPT);
    if (ret) {
      printf("Failed to encrypt. Error %d\n", ret);
      return -1;
    }

    printf("Ciphertext:    ");
    for (int i = 0; i < o_len; i++) printf("%02x", *(ciphertext + i));
    printf("\t(%d bytes)\n", o_len);

    bzero(iv, 16);
    ret = wrapped_sym_cipher(ciphertext, o_len, plaintext_out, o_len, key, key_size, iv, algs_to_test[idx], OP_DECRYPT);
    if (ret) {
      printf("Failed to decrypt. Error %d\n", ret);
      return -1;
    }

    printf("Plaintext out: ");
    for (int i = 0; i < o_len; i++) printf("%02x", *(plaintext_out + i));
    printf("\t(%d bytes)\n", o_len);

    // Now check that the plaintext versions match...
    for (int i = 0; i < i_len; i++) {
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
  const int BASE_BUFFER_LEN = 2048;
  uint8_t* rsa_public_buf  = (uint8_t*) alloca(2048);
  uint8_t* rsa_privat_buf  = (uint8_t*) alloca(2048);
  uint8_t* ecc_public_buf  = (uint8_t*) alloca(2048);
  uint8_t* ecc_privat_buf  = (uint8_t*) alloca(2048);

  int rsa_public_len  = 2048;
  int rsa_privat_len  = 2048;
  int ecc_public_len  = 2048;
  int ecc_privat_len  = 2048;


  int ret = wrapped_asym_keygen(Cipher::ASYM_RSA, CryptoKey::RSA_2048, rsa_public_buf, &rsa_public_len, rsa_privat_buf, &rsa_privat_len);
  if (0 == ret) {
    printf("RSA keygen succeeded:\n");
    printf("Public:  ");
    for (int i = BASE_BUFFER_LEN-rsa_public_len; i < BASE_BUFFER_LEN; i++) printf("%02x", *(rsa_public_buf + i));
    printf("\t(%d bytes)\nPrivate: ", rsa_public_len);
    for (int i = BASE_BUFFER_LEN-rsa_privat_len; i < BASE_BUFFER_LEN; i++) printf("%02x", *(rsa_privat_buf + i));
    printf("\t(%d bytes)\n\n", rsa_privat_len);

    ret = wrapped_asym_keygen(Cipher::ASYM_ECKEY, CryptoKey::ECC_SECP384R1, ecc_public_buf, &ecc_public_len, ecc_privat_buf, &ecc_privat_len);
    if (0 == ret) {
      printf("ECC keygen succeeded:\n");
      printf("Public:  ");
      for (int i = BASE_BUFFER_LEN-ecc_public_len; i < BASE_BUFFER_LEN; i++) printf("%02x", *(ecc_public_buf + i));
      printf("\t(%d bytes)\nPrivate: ", ecc_public_len);
      for (int i = BASE_BUFFER_LEN-ecc_privat_len; i < BASE_BUFFER_LEN; i++) printf("%02x", *(ecc_privat_buf + i));
      printf("\t(%d bytes)\n\n", ecc_privat_len);
      return 0;
    }
    else {
      printf("ECC keygen failed with code 0x%04x.\n", ret);
    }
  }
  else {
    printf("RSA keygen failed with code 0x%04x.\n", ret);
  }
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
  platform.bootstrap();

  StringBuilder log;
  printCryptoOverview(&log);
  printf("%s\n", (const char*) log.string());

  if (0 == CRYPTO_TEST_INIT()) {
    if (0 == CRYPTO_TEST_RNG()) {
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
    else printTestFailure("CRYPTO_TEST_RNG");
  }
  else printTestFailure("CRYPTO_TEST_INIT");

  exit(exit_value);
}
