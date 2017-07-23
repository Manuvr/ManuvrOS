/*
File:   IdentityTest.cpp
Author: J. Ian Lindsay
Date:   2016.09.20

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


This program runs tests on Identity elements.
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
#include <Types/cbor-cpp/cbor.h>
#include <Platform/Identity.h>


IdentityUUID* id_uuid = nullptr;
#if defined (MANUVR_OPENINTERCONNECT)
#endif


int UUID_IDENT_TESTS() {
  int return_value = -1;
  StringBuilder log("===< UUID_IDENT_TESTS >=================================\n");
  // Create identities from nothing...
  IdentityUUID id_uuid("testUUID");
  log.concat("\t Creating a new identity...\n");
  Identity::staticToString(&id_uuid, &log);
  log.concat("\n\t Loading from buffer...\n");

  // Create identities from serialized representations.
  uint8_t buf[] = {0, 24, 0, 0, (uint8_t) IdentFormat::UUID, 65, 65, 0,   1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
  IdentityUUID* ident0 = (IdentityUUID*) Identity::fromBuffer(buf, sizeof(buf));
  if (ident0) {
    Identity::staticToString(ident0, &log);
    log.concat("\n");
    // Serialize identity.
    int rep_len = ident0->length();
    if (rep_len == sizeof(buf)) {
      uint8_t* buf0 = (uint8_t*) alloca(rep_len);
      int ser_len = ident0->serialize(buf0, rep_len);
      if (ser_len == rep_len) {
        return_value = 0;
        for (int i = 0; i < ser_len; i++) {
          if (buf[i] != buf0[i]) {
            log.concatf("Index %d mismatch. Found 0x%02x, expected 0x%02x.\n", i, buf0[i], buf[i]);
            return return_value;
          }
        }
        return_value = 0;  // PASS
      }
      else {
        log.concatf("Serialized length is %d bytes. Should be %d bytes.\n", ser_len, rep_len);
      }
    }
    else {
      log.concatf("Reported length is %d bytes. Should be %d bytes.\n", rep_len, sizeof(buf));
    }
  }
  else {
    log.concatf("Failed to deserialize.\n");
  }

  log.concat("\n\n");
  printf((const char*) log.string());
  return return_value;
}


int CRYPTO_IDENT_TESTS() {
  #if defined(__HAS_CRYPT_WRAPPER)
  int return_value = -1;
  StringBuilder log("===< CRYPTO_IDENT_TESTS >===============================\n");
  // Create identities from nothing...

  IdentityPubKey ident_rsa("TesIdentRSA", Cipher::ASYM_RSA,   CryptoKey::RSA_2048);
  IdentityPubKey ident_ecp("TesIdentECP", Cipher::ASYM_ECDSA, CryptoKey::ECC_SECP256R1);

  Identity::staticToString(&ident_rsa, &log);
  Identity::staticToString(&ident_ecp, &log);

  if (ident_ecp.isValid()) {
    uint16_t len = ident_ecp.length();
    uint8_t* ser_buf = (uint8_t*) alloca(len);
    if (ser_buf) {
      log.concatf("\t Serialized successfully. Length is %d\n", len);
      len = ident_ecp.serialize(ser_buf, len);
      if (len) {
        for (int i = 0; i < len; i++) log.concatf("%02x", *(ser_buf + i));
        log.concat("\n");
        IdentityPubKey* ecp_restored = (IdentityPubKey*) Identity::fromBuffer(ser_buf, len);
        if (ecp_restored) {
          log.concatf("\t Unserialized successfully. Length is %d\n", ecp_restored->length());
          Identity::staticToString(ecp_restored, &log);
          return_value = 0;  // PASS
        }
        else {
          log.concatf("Failed to deserialize.\n");
        }
      }
      else {
        log.concatf("Failed to serialize.\n");
      }
    }
    else {
      log.concatf("Failed to allocate for serialization.\n");
    }
  }
  else {
    log.concatf("Failed to create specified identity.\n");
  }

  log.concat("\n\n");
  printf((const char*) log.string());
  return return_value;
  #else
  return 0;
  #endif
}


void printTestFailure(const char* test) {
  printf("\n");
  printf("*********************************************\n");
  printf("* %s FAILED tests.\n", test);
  printf("*********************************************\n");
}

/*******************************************************************************
* The main function.                                                           *
*******************************************************************************/
int main(int argc, char *argv[]) {
  int exit_value = 1;   // Failure is the default result.

  platform.platformPreInit();   // Our test fixture needs random numbers.
  platform.bootstrap();   // Our test fixture needs random numbers.

  if (0 == UUID_IDENT_TESTS()) {
    if (0 == CRYPTO_IDENT_TESTS()) {
      printf("**********************************\n");
      printf("*  Identity tests all pass       *\n");
      printf("**********************************\n");
      exit_value = 0;
    }
    else printTestFailure("CRYPTO_IDENT_TESTS");
  }
  else printTestFailure("UUID_IDENT_TESTS");

  exit(exit_value);
}
