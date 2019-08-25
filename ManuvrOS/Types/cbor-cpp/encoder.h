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


#ifndef __CborEncoder_H_
#define __CborEncoder_H_

#include "output.h"
#include <string>

namespace cbor {
  class encoder {
    private:
      output *_out;

      void write_type_value(int major_type, unsigned int value);
      void write_type_value(int major_type, unsigned long long value);


    public:
      encoder(output &out);
      ~encoder();

      void write_int(int value);
      void write_int(long long value);
      inline void write_int(unsigned int v) {            write_type_value(0, v);     };
      inline void write_int(unsigned long long v) {      write_type_value(0, v);     };
      inline void write_tag(const unsigned int tag) {    write_type_value(6, tag);   };
      inline void write_array(int size) {  write_type_value(4, (unsigned int) size); };
      inline void write_map(int size) {    write_type_value(5, (unsigned int) size); };
      inline void write_special(int v) {   write_type_value(7, (unsigned int) v);    };

      void write_float(float value);
      void write_double(double value);
      void write_bytes(const unsigned char *data, unsigned int size);
      void write_string(const char* data, unsigned int size);
      void write_string(const char* str);
  };
}

#endif //__CborEncoder_H_
