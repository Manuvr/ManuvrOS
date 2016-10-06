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

// OneID's chosen curve is this big.
#define ONEID_PUB_KEY_MAX_LEN     92


class IdentityOneID : public IdentityPubKey {
  public:
    IdentityOneID(const char* nom);
    IdentityOneID(uint8_t* buf, uint16_t len);
    ~IdentityOneID();

    void toString(StringBuilder*);
    int  serialize(uint8_t*, uint16_t);


  private:
    uint8_t _oneid_project_key[ONEID_PUB_KEY_MAX_LEN];
    uint8_t _oneid_server_key[ONEID_PUB_KEY_MAX_LEN];
    uint8_t _oneid_reset_key[ONEID_PUB_KEY_MAX_LEN];
};


#endif // __MANUVR_IDENTITY_ONEID_H__
