/*
File:   Identity.cpp
Author: J. Ian Lindsay
Date:   2016.08.28

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


Data-persistence layer for linux.
Implemented as a JSON object within a single file. This feature therefore
  requires MANUVR_JSON. In the future, it may be made to operate on some
  other encoding, be run through a cryptographic pipe, etc.
*/

#include "Identity.h"
#include <alloca.h>



IdentityUUID::IdentityUUID(const char* nom) {
  format = IdentFormat::UUID;
  _ident_len = sizeof(UUID);
  _handle = (char*) nom;   // TODO: Copy?
  uuid_gen(&uuid);
  _ident_set_flag(true, MANUVR_IDENT_FLAG_DIRTY);
}

IdentityUUID::IdentityUUID(const char* nom, char* uuid_str) {
  format = IdentFormat::UUID;
  _ident_len = sizeof(UUID);
  _handle = (char*) nom;   // TODO: Copy?
  uuid_from_str(uuid_str, &uuid);
}


IdentityUUID::~IdentityUUID() {
}


void IdentityUUID::toString(StringBuilder* output) {
  char* uuid_str = (char*) alloca(40);
  bzero(uuid_str, 40);
  uuid_to_str(&uuid, uuid_str, 40);
  output->concatf(uuid_str);
}


int IdentityUUID::toBuffer(uint8_t* buf) {
  for (int i = 0; i < 16; i++) *(buf + i) = *(((uint8_t*)&uuid.id) + i);
  return 16;
}
