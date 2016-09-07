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


#define MANUVR_IDENT_FLAG_DIRTY        0x8000  // This ID will be lost if not persisted.
#define MANUVR_IDENT_FLAG_OUR_OWN      0x4000  // This is our own identity.
#define MANUVR_IDENT_FLAG_LOCAL_CHAIN  0x2000  // An identity required to validate our own.
#define MANUVR_IDENT_FLAG_ORIG_PERSIST 0x1000  // Was loaded from persisted data.
#define MANUVR_IDENT_FLAG_ORIG_GEN     0x0800  // Was generated on this system.
#define MANUVR_IDENT_FLAG_ORIG_EXTER   0x0400  // Came from outside. Usually from TLS.
#define MANUVR_IDENT_FLAG_ORIG_HSM     0x0200  // Came from an HSM, local or not.
#define MANUVR_IDENT_FLAG_ORIG_PKI     0x0100  // Imparted by a PKI.
#define MANUVR_IDENT_FLAG_RESERVED_2   0x0080  //
#define MANUVR_IDENT_FLAG_RESERVED_1   0x0040  //
#define MANUVR_IDENT_FLAG_RESERVED_0   0x0020  //
#define MANUVR_IDENT_FLAG_3RD_PARTY_CA 0x0010  // This is an intermediary or CA cert.
#define MANUVR_IDENT_FLAG_REVOKED      0x0008  // This identity is no longer valid.
#define MANUVR_IDENT_FLAG_REVOKABLE    0x0004  // This identity can be revoked.
#define MANUVR_IDENT_FLAG_APP_ACCEPT   0x0002  // Is acceptable for the application layer.
#define MANUVR_IDENT_FLAG_NET_ACCEPT   0x0001  // Is acceptable for the network layer.

/* These flags should not be persisted. */
#define MANUVR_IDENT_FLAG_PERSIST_MASK (MANUVR_IDENT_FLAG_RESERVED_0 | \
                                        MANUVR_IDENT_FLAG_RESERVED_1 | \
                                        MANUVR_IDENT_FLAG_RESERVED_2 | \
                                        MANUVR_IDENT_FLAG_DIRTY)
/*
* Note on NET_ACCEPT / APP_ACCEPT:
* This distinction is needed to isolate policy actions from merely the
*   "authorization to connect". Without this distinction, we have no way
*   of isolating TLS credentials from consideration by any policy layer
*   making choices about the instructions themselves.
* However, failure to draw a distinction is a valid route to support in
*   tight spaces, as there is no need for a separate identity-management
*   apparatus devoted to policy.
* When the non-isolated behavior is desired, the program should set BOTH
*   flags. Certain kinds of identity may not be valid for either, in which
*   case both flags should be cleared (default condition).
* TODO: This is covered by an RFC that I forget the name/number for. Dig.
*/

enum class IdentFormat {
  /*
  * TODO: Since we wrote this interface against mbedtls, we will use the preprocessor
  *   defines from that library for normalizing purposes.
  */
  UNDETERMINED    = 0x00,  // Nothing here.
  SERIAL_NUM      = 0x01,  // Nearly universal.
  UUID            = 0x02,  // Low-grade. Easy.
  HASH            = 0x03,  // Open-ended.
  CERT_FORMAT_DER = 0x04,  // Certificate in DER format.
  PSK_ASYM        = 0x05,  // Pre-shared asymmetric key.
  PSK_SYM         = 0x06   // Pre-shared symmetric key.
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
    uint16_t _ident_len   = 0;        // How many bytes does the identity occupy?

    char*      _handle     = nullptr;  // Human-readable name of this identity.

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