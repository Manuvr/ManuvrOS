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

#include "encoder.h"

using namespace cbor;

#include <string.h>

encoder::encoder(output &out) {
  _out = &out;
}

encoder::~encoder() {}


void encoder::write_type_value(int major_type, unsigned int value) {
  major_type <<= 5;
  if (value < 24) {
    _out->put_byte((unsigned char) (major_type | value));
  }
  else if(value < 256) {
    _out->put_byte((unsigned char) (major_type | 24));
    _out->put_byte((unsigned char) value);
  }
  else if(value < 65536) {
    _out->put_byte((unsigned char) (major_type | 25));
    _out->put_byte((unsigned char) (value >> 8));
    _out->put_byte((unsigned char) value);
  }
  else {
    _out->put_byte((unsigned char) (major_type | 26));
    _out->put_byte((unsigned char) (value >> 24));
    _out->put_byte((unsigned char) (value >> 16));
    _out->put_byte((unsigned char) (value >> 8));
    _out->put_byte((unsigned char) value);
  }
}

void encoder::write_type_value(int major_type, unsigned long long value) {
  major_type <<= 5;
  if (value < 24ULL) {
    _out->put_byte((unsigned char) (major_type | value));
  }
  else if(value < 256ULL) {
    _out->put_byte((unsigned char) (major_type | 24));
    _out->put_byte((unsigned char) value);
  }
  else if(value < 65536ULL) {
    _out->put_byte((unsigned char) (major_type | 25));
    _out->put_byte((unsigned char) (value >> 8));
  }
  else if(value < 4294967296ULL) {
    _out->put_byte((unsigned char) (major_type | 26));
    _out->put_byte((unsigned char) (value >> 24));
    _out->put_byte((unsigned char) (value >> 16));
    _out->put_byte((unsigned char) (value >> 8));
    _out->put_byte((unsigned char) value);
  }
  else {
    _out->put_byte((unsigned char) (major_type | 27));
    _out->put_byte((unsigned char) (value >> 56));
    _out->put_byte((unsigned char) (value >> 48));
    _out->put_byte((unsigned char) (value >> 40));
    _out->put_byte((unsigned char) (value >> 32));
    _out->put_byte((unsigned char) (value >> 24));
    _out->put_byte((unsigned char) (value >> 16));
    _out->put_byte((unsigned char) (value >> 8));
    _out->put_byte((unsigned char) value);
  }
}


void encoder::write_int(long long value) {
  if(value < 0) {
    write_type_value(1, (unsigned long long) -(value+1));
  }
  else {
    write_type_value(0, (unsigned long long) value);
  }
}

void encoder::write_int(int value) {
  if(value < 0) {
    write_type_value(1, (unsigned int) -(value+1));
  }
  else {
    write_type_value(0, (unsigned int) value);
  }
}

void encoder::write_float(float value) {
  void* punny = &value;
  _out->put_byte((unsigned char) (7<<5) | 26);
  _out->put_byte(*((uint8_t*) punny+3));
  _out->put_byte(*((uint8_t*) punny+2));
  _out->put_byte(*((uint8_t*) punny+1));
  _out->put_byte(*((uint8_t*) punny+0));
}

void encoder::write_bytes(const unsigned char *data, unsigned int size) {
  write_type_value(2, size);
  _out->put_bytes(data, size);
}

void encoder::write_string(const char *data, unsigned int size) {
  write_type_value(3, size);
  _out->put_bytes((const unsigned char *) data, size);
}

void encoder::write_string(const char* str) {
  int len = strlen(str);
  write_type_value(3, (unsigned int) len);
  _out->put_bytes((const unsigned char *) str, len);
}
