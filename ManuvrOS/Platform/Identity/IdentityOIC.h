/*
File:   IdentityOIC.h
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



*/

#ifndef __MANUVR_IDENTITY_OIC_H__
#define __MANUVR_IDENTITY_OIC_H__

#include <StringBuilder.h>
#include <uuid.h>


enum class OICCredType : uint8_t {
  /*
  * These defs were pulled from the OIC1.1 draft.
  */
  UNDEF = 0x00,    // Nothing here.
  PAIRWISE_SYM,    // Pairwise symmetric
  GROUP_SYM,       // Group symmetric
  ASYM_AUTH,       // Asymmetric authenticated
  CERT,            // Certificate
  PASSWD           // Pin/Password
};


class IdentityOIC : public Identity {
  public:
    IdentityOIC(const char* nom);
    IdentityOIC(const char* nom, char* uuid_str);
    IdentityOIC(uint8_t* buf, uint16_t len);
    ~IdentityOIC();

    void toString(StringBuilder*);
    int  serialize(uint8_t*, uint16_t);

    void setCredType(OICCredType);
    OICCredType getCredType() {    return _credtype;   };


  private:
    int         _credid   = 0;
    OICCredType _credtype = OICCredType::UNDEF;
    UUID        _subject;
    uint8_t     _priv[32];
};


#endif // __MANUVR_IDENTITY_OIC_H__
