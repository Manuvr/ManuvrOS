/*
File:   IdentityUUID.cpp
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


Simple form of identity. Implementable of devices that cannot handle
  a cryptogrphy stack.
*/

#include <Platform/Identity.h>
#include <alloca.h>



IdentityUUID::IdentityUUID(const char* nom) : Identity(nom, IdentFormat::UUID) {
  _ident_len += sizeof(UUID);
  uuid_gen(&uuid);
  _ident_set_flag(true, MANUVR_IDENT_FLAG_DIRTY | MANUVR_IDENT_FLAG_ORIG_GEN);
}

IdentityUUID::IdentityUUID(const char* nom, char* uuid_str) : Identity(nom, IdentFormat::UUID) {
  _ident_len += sizeof(UUID);
  uuid_from_str(uuid_str, &uuid);
}


IdentityUUID::IdentityUUID(uint8_t* buf, uint16_t len) : Identity((const char*) buf, IdentFormat::UUID) {
  const int offset = _ident_len - IDENTITY_BASE_PERSIST_LENGTH + 1;
  if (len >= 17) {
    // If there are at least 16 non-string bytes left in the buffer...
    for (int i = 0; i < 16; i++) *(((uint8_t*)&uuid.id) + i) = *(buf + i + offset);
  }
  _ident_len += 16;
}


IdentityUUID::~IdentityUUID() {
}


void IdentityUUID::toString(StringBuilder* output) {
  char* uuid_str = (char*) alloca(40);
  bzero(uuid_str, 40);
  uuid_to_str(&uuid, uuid_str, 40);
  output->concatf(uuid_str);
}

/*
* Only the persistable particulars of this instance. All the base class and
*   error-checking are done upstream.
*/
int IdentityUUID::serialize(uint8_t* buf, uint16_t len) {
  int offset = Identity::_serialize(buf, len);
  memcpy((buf + offset), (uint8_t*)&uuid.id, 16);
  return offset + 16;
}
