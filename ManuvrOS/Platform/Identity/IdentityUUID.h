/*
File:   IdentityUUID.h
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



*/

#ifndef __MANUVR_IDENTITY_UUID_H__
#define __MANUVR_IDENTITY_UUID_H__

#include <StringBuilder.h>
#include <uuid.h>


class IdentityUUID : public Identity {
  public:
    IdentityUUID(const char* nom);
    IdentityUUID(const char* nom, char* uuid_str);
    IdentityUUID(uint8_t* buf, uint16_t len);
    ~IdentityUUID();

    void toString(StringBuilder*);
    int  serialize(uint8_t*, uint16_t);

    /*
    * Convenience fxn to write the raw value of the UUID into the
    *   provided buffer, which is assumed to be 16-bytes or larger.
    */
    inline void copyRaw(uint8_t* output) {
      memcpy(output, (uint8_t*) &uuid.id, 16);
    }


  private:
    UUID uuid;
};


#endif // __MANUVR_IDENTITY_UUID_H__
