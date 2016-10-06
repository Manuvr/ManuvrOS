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


These are cryptographically-backed notions of identity.
*/

#ifndef __MANUVR_IDENTITY_CRYPTO_H__
#define __MANUVR_IDENTITY_CRYPTO_H__

#include <Platform/Cryptographic.h>

/**
* This is the most straight-forward notion of cryptographically-backed identity.
*/
class IdentityPubKey : public Identity {
  public:
    IdentityPubKey(const char* nom, Cipher, CryptoKey);
    IdentityPubKey(const char* nom, Cipher, CryptoKey, Hashes);
    IdentityPubKey(uint8_t* buf, uint16_t len);
    ~IdentityPubKey();

    virtual int8_t sign(uint8_t* in, size_t in_len, uint8_t* out, size_t* out_len);
    virtual int8_t verify(uint8_t* in, size_t in_len, uint8_t* out, size_t* out_len);

    // Helps the caller predict buffer-length. Asymmetric crypto ignores parameter.
    inline size_t sizeOutputBuffer(size_t) {  return _sig_size;  };

    virtual int8_t sanity_check();

    virtual void toString(StringBuilder*);
    virtual int  serialize(uint8_t*, uint16_t);


  protected:
    uint8_t*  _pub         = nullptr;
    uint8_t*  _priv        = nullptr;
    uint16_t  _pub_size    = 0;
    uint16_t  _priv_size   = 0;
    uint16_t  _sig_size    = 0;
    CryptoKey _key_type    = CryptoKey::NONE;
    Cipher    _cipher      = Cipher::NONE;
    Hashes    _digest      = Hashes::NONE;
};


/**
* This is the notion of identity that is common on "large" systems, and
*   is the foundation of most PKI.
* Certificates are revokable/expirable, and fully-specify an identity,
*   put they make an externally-referenced claim of identity, and so
*   treating them properly is more challenging, requires more compute,
*   and (often) network access unless the entire PKI is stored locally.
* You will need this if you want to participate in such PKIs directly.
* Many (D)TLS cipher suites require this.
*/
class IdentityCert : public Identity {
  public:
    IdentityCert(const char* nom);
    IdentityCert(uint8_t* buf, uint16_t len);
    ~IdentityCert();

    int8_t sign();
    int8_t verify();

    int8_t sanity_check();

    void toString(StringBuilder*);
    int  serialize(uint8_t*, uint16_t);


  protected:
    IdentityCert* _issuer  = nullptr;
};


/**
* This is a notion of identity that relies on a pre-shared symmetric key.
* This identity is cryptographically-backed, but it it tantamount to a
*   high-entropy password.
* If you want to use an authenticated cipher, you should use IdentityAuthPSK,
*   as this class will not take advantage of the authentication properties
*   of blockmodes like CCM or GCM.
* For systems that use this notion of identity in a TLS stack, the base-class
*   member "name" should be used as the PSK hint if no other arrangement is made.
*/
class IdentityPSK : public Identity {
  public:
    IdentityPSK(const char* nom, Cipher);
    IdentityPSK(uint8_t* buf, uint16_t len);
    ~IdentityPSK();

    int8_t encrypt(uint8_t* in, size_t in_len, uint8_t* out, size_t* out_len);
    int8_t decrypt(uint8_t* in, size_t in_len, uint8_t* out, size_t* out_len);

    size_t sizeOutputBuffer(size_t in_len);  // Helps the caller predict buffer-length.

    void toString(StringBuilder*);
    int  serialize(uint8_t*, uint16_t);


  protected:
    uint8_t*  _psk         = nullptr;
    uint16_t  _psk_size    = 0;
    Cipher    _cipher      = Cipher::NONE;
};


#endif // __MANUVR_IDENTITY_CRYPTO_H__
