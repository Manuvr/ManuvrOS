/*
File:   IdentityCrypto.h
Author: J. Ian Lindsay
Date:   2016.10.04

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

#ifndef __MANUVR_IDENTITY_CRYPTO_H__
#define __MANUVR_IDENTITY_CRYPTO_H__

#include <Platform/Cryptographic.h>

class IdentityCert : public Identity {
  public:
    IdentityCert(const char* nom);
    IdentityCert(uint8_t* buf, uint16_t len);
    ~IdentityCert();

    int8_t sign();
    int8_t validate();

    void toString(StringBuilder*);
    int  serialize(uint8_t*, uint16_t);


  protected:
    IdentityCert* _issuer  = nullptr;
};


class IdentityPubKey : public Identity {
  public:
    IdentityPubKey(const char* nom, Cipher, CryptoKey);
    IdentityPubKey(uint8_t* buf, uint16_t len);
    ~IdentityPubKey();

    int8_t sign();
    int8_t validate();

    void toString(StringBuilder*);
    int  serialize(uint8_t*, uint16_t);


  protected:
    uint8_t*  _pub         = nullptr;
    uint8_t*  _priv        = nullptr;
    uint16_t  _pub_size    = 0;
    uint16_t  _priv_size   = 0;
    CryptoKey _key_type    = CryptoKey::NONE;
    Cipher    _cipher      = Cipher::NONE;
};


class IdentityPSK : public Identity {
  public:
    IdentityPSK(const char* nom, Cipher);
    IdentityPSK(uint8_t* buf, uint16_t len);
    ~IdentityPSK();

    int8_t sign();
    int8_t validate();

    void toString(StringBuilder*);
    int  serialize(uint8_t*, uint16_t);


  protected:
    Cipher ci;
};


#endif // __MANUVR_IDENTITY_CRYPTO_H__
