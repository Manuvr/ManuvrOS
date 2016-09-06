/*
File:   Identity.h
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


This might be better-viewed as a data structure. Notions of identity should
  be encapsulated here.
*/

#ifndef __MANUVR_IDENTITY_H__
#define __MANUVR_IDENTITY_H__

#include <DataStructures/StringBuilder.h>
#include <DataStructures/uuid.h>


#define MANUVR_IDENT_FLAG_DIRTY           0x8000  // This ID will be lost if not persisted.


enum class IdentFormat {
  /*
  * TODO: Since we wrote this interface against mbedtls, we will use the preprocessor
  *   defines from that library for normalizing purposes.
  */
  UNDETERMINED = 0x00,  // Nothing here.
  SERIAL_NUM,           // Nearly universal.
  UUID,                 // Low-grade. Easy.
  HASH,                 // Open-ended.
  CERT_FORMAT_DER,      // Certificate in DER format.
  PSK                   // Pre-shared symmetric key.
};


enum class OCFCredType {
  /*
  * TODO: Since we wrote this interface against mbedtls, we will use the preprocessor
  *   defines from that library for normalizing purposes.
  */
  UNDEF = 0x00,    // Nothing here.
  PAIRWISE_SYM,    // Pairwise symmetric
  GROUP_SYM,       // Group symmetric
  ASYM_AUTH,       // Asymmetric authenticated
  CERT,            // Certificate
  PASSWD,          // Pin/Password
};


/*
* This is our manner of grappling with the concept of "identity". The issue is
*   one of the core-problems of knowledge, and defining it will take effort and
*   refinement.
* This is a base virtual for the common ground...
*/
class Identity {
  public:
    IdentFormat format = IdentFormat::UNDETERMINED;   // What is the nature of this identity?

    /* Provides concise representations... */
    virtual void toString(StringBuilder*) =0;   // For readability.
    virtual int  toBuffer(uint8_t*)       =0;   // For an algorithm.

    // TODO: int8_t serialize();
    inline int   length() {     return _ident_len;  };
    inline char* getHandle() {  return (nullptr == _handle ? (char*) "unnamed":_handle);  };

    inline bool isDirty() {     return _ident_flag(MANUVR_IDENT_FLAG_DIRTY);  };



  protected:
    char*      _handle     = nullptr;  // Human-readable name of this identity.
    int        _ident_len  = 0;        // How many bytes does the identity occupy?

    inline bool _ident_flag(uint16_t f) {   return ((_ident_flags & f) == f);  };
    inline void _ident_set_flag(bool nu, uint16_t _flag) {
      _ident_flags = (nu) ? (_ident_flags | _flag) : (_ident_flags & ~_flag);
    };


  private:
    //Identity* _next = nullptr;  // TODO: Chaining is probably better than flatness in this case.
    // TODO: Expiration.
    uint16_t _ident_flags = 0;
};


class IdentityUUID : public Identity {
  public:
    IdentityUUID(const char* nom);
    IdentityUUID(const char* nom, char* uuid_str);
    ~IdentityUUID();

    void toString(StringBuilder*);
    int  toBuffer(uint8_t*);


  private:
    UUID uuid;
};


#if defined (MANUVR_OIC)
class IdentityOCFCred : public Identity {
  UUID subject_uuid;

};
#endif // MANUVR_OIC


class IdentityHash : public Identity {
  public:
    IdentityHash();
    ~IdentityHash();

  protected:

};

class IdentityCert : public Identity {
  public:
    IdentityCert();
    ~IdentityCert();

    virtual int8_t sign() =0;
    virtual int8_t validate() =0;


  protected:
    IdentityCert* _issuer  = nullptr;

};

#endif // __MANUVR_IDENTITY_H__
