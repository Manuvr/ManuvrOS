/*
File:   IdentityOIC.cpp
Author: J. Ian Lindsay
Date:   2016.09.10

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


OIC credentials.
*/


#if defined (MANUVR_OPENINTERCONNECT)

#include <Platform/Identity.h>
#include <alloca.h>

#include "oc_api.h"



IdentityOIC::IdentityOIC(const char* nom) : Identity(nom, IdentFormat::OIC_CRED) {
  _ident_len += sizeof(_priv) + sizeof(_subject) + 1;
  for (int i = 0; i < sizeof(_priv); i++) _priv[i] = 0;   // Zero private data.
  _ident_set_flag(true, MANUVR_IDENT_FLAG_DIRTY | MANUVR_IDENT_FLAG_ORIG_GEN);
}

IdentityOIC::IdentityOIC(const char* nom, char* subj) : IdentityOIC(nom) {
  uuid_from_str(uuid_str, &_subject);
}


IdentityOIC::IdentityOIC(uint8_t* buf, uint16_t len) : IdentityOIC((const char*) buf) {
}


IdentityOIC::~IdentityOIC() {
}


void IdentityOIC::toString(StringBuilder* output) {
  char* uuid_str = (char*) alloca(40);
  bzero(uuid_str, 40);
  uuid_to_str(&uuid, uuid_str, 40);
  output->concatf(uuid_str);
}

/*
* Only the persistable particulars of this instance. All the base class and
*   error-checking are done upstream.
*/
int IdentityOIC::toBuffer(uint8_t* buf, uint16_t len) {
  int offset = Identity::toBuffer(buf, len);
  memcpy((buf + offset), (uint8_t*)&uuid.id, 16);
  return 16;
}


void IdentityOIC::setCredType(OICCredType t) {
  if (OICCredType::UNDEF == _credtype) {
    // Can only set this once.
    _credtype = t;
  }
}


#endif // MANUVR_OPENINTERCONNECT
