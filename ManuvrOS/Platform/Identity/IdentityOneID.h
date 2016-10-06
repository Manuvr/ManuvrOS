/*
File:   IdentityOneID.h
Author: J. Ian Lindsay
Date:   2016.09.07

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


This is an initial CPP implementation of OneID's asymmetric auth strategy.

This represents tentative support. This should ultimately be distilled-down
  into a portable C library, and then statically linked, as any other library.
  But it will be prototyped here.
*/

#ifndef __MANUVR_IDENTITY_ONEID_H__
#define __MANUVR_IDENTITY_ONEID_H__



class IdentityOneID : public Identity {
  public:
    IdentityOneID(const char* nom);
    IdentityOneID(uint8_t* buf, uint16_t len);
    ~IdentityOneID();

    int8_t sign();
    int8_t verify();

    int8_t sanity_check();

    void toString(StringBuilder*);
    int  serialize(uint8_t*, uint16_t);


  protected:
    uint8_t*  _pub         = nullptr;
    uint8_t*  _priv        = nullptr;
    uint16_t  _pub_size    = 0;
    uint16_t  _priv_size   = 0;
    CryptoKey _key_type    = CryptoKey::ECC_SECP256R1;
    Cipher    _cipher      = Cipher::ASYM_ECDSA;
};


#endif // __MANUVR_IDENTITY_ONEID_H__
