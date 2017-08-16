/*
   Copyright 2014-2015 Stanislav Ovsyannikov

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

#ifndef CBOR_CPP_LISTENER_H
#define CBOR_CPP_LISTENER_H

#include <stdint.h>


namespace cbor {
    class listener {
    public:
        virtual void on_integer(int8_t   value) = 0;
        virtual void on_integer(int16_t  value) = 0;
        virtual void on_integer(int32_t  value) = 0;
        virtual void on_integer(uint8_t  value) = 0;
        virtual void on_integer(uint16_t value) = 0;
        virtual void on_integer(uint32_t value) = 0;

        virtual void on_float32(float value) = 0;

        virtual void on_bytes(unsigned char *data, int size) = 0;

        virtual void on_string(char* str) = 0;

        virtual void on_array(int size) = 0;

        virtual void on_map(int size) = 0;

        virtual void on_tag(unsigned int tag) = 0;

        virtual void on_special(unsigned int code) = 0;

        virtual void on_error(const char *error) = 0;

        virtual void on_extra_integer(unsigned long long value, int sign) {
        }

        virtual void on_extra_tag(unsigned long long tag) {
        }

        virtual void on_extra_special(unsigned long long tag) {
        }
    };
}


#endif // CBOR_CPP_LISTENER_H
